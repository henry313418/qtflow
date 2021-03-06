#!/usr/bin/tclsh
#---------------------------------------------------------------------------
# blifanno.tcl ---
#
# Read a BLIF file and a post-placement DEF file.  The placement stage is
# assumed to have rewired buffer trees for optimal placement, making the
# BLIF file netlist invalid.  The contents of the DEF file are used to
# back-annotate the correct buffer tree connections to the BLIF netlist.
# The output is a corrected BLIF netlist.
#
#---------------------------------------------------------------------------
namespace path {::tcl::mathop ::tcl::mathfunc}

if {$argc < 2} {
   puts stdout "Usage:  blifanno.tcl <blif_file> <def_file> \[<blif_out>\]"
   exit 0
}

puts stdout "Running blifanno.tcl"

# NOTE:  There is no scaling.  GrayWolf values are in centimicrons,
# as are DEF values (UNITS DISTANCE MICRONS 100)

set blifinname [lindex $argv 0]
set defname [lindex $argv 1]

set units 100		;# write centimicron units into the DEF file

#-----------------------------------------------------------------
# Open all files for reading and writing
#-----------------------------------------------------------------

if [catch {open $defname r} fdef] {
   puts stderr "Error: can't open file $defname for input"
   return
}

if [catch {open $blifinname r} fnet] {
   puts stderr "Error: can't open file $blifinname for input"
   return
}

if {$argc == 3} {
   set blifoutname [lindex $argv 2]
   if [catch {open $blifoutname w} fout] {
      puts stderr "Error: can't open file $blifoutname for output"
      return
   }
} else {
   set fout stdout
}

#----------------------------------------------------------------
# Read through a LEF file section that we don't care about.
#----------------------------------------------------------------

proc skip_section {leffile sectionname} {
   while {[gets $leffile line] >= 0} {
      if [regexp {[ \t]*END[ \t]+(.+)[ \t]*$} $line lmatch sectiontest] {
         if {"$sectiontest" != "$sectionname"} {
            puts -nonewline stderr "Unexpected END statement $line "
            puts stderr "while reading section $sectionname"
         }
         break
      }
   }
}

#-----------------------------------------------------------------
# Parse the NETS section of the DEF file
# Assuming this file was generated by place2def, each net
# connection should be on a separate line.
#-----------------------------------------------------------------

proc parse_nets {deffile nets} {
   upvar $nets rdict
   set ignore 0
   while {[gets $deffile line] >= 0} {
      if [regexp {[ \t]*END[ \t]+(.+)[ \t\n]*$} $line lmatch sectiontest] {
         if {"$sectiontest" == "NETS"} {
	    break
	 } else {
            puts -nonewline stderr "Unexpected END statement $line "
            puts stderr "while reading section NETS"
         }
         break
      } elseif [regexp {[ \t]*-[ \t]+([^ \t]+)} $line lmatch netname] {
	 set ignore 0
      } elseif [regexp {[ \t]*\+[ \t]+([^ \t]+)} $line lmatch option] {
	 set ignore 1
      } elseif {$ignore == 0} {
	 if [regexp {[ \t]*\([ \t]*([^ \t]+)[ \t]+([^ \t]+)[ \t]*\)[ \t\n]*$} \
		$line lmatch instname pinname] {
	    dict set rdict ${instname}/${pinname} $netname
	 }
      }
   }
}

#-----------------------------------------------------------------
# Read the DEF file once to get the number of rows and the length
# of each row
#-----------------------------------------------------------------

puts stdout "Reading DEF file ${defname}. . ."
flush stdout

while {[gets $fdef line] >= 0} {
   if [regexp {[ \t]*COMPONENTS[ \t]+([^ \t]+)[ \t]*;} $line lmatch number] {
      skip_section $fdef COMPONENTS
   } elseif [regexp {[ \t]*SPECIALNETS[ \t]+([^ \t]+)} $line lmatch netnums] {
      skip_section $fdef SPECIALNETS
   } elseif [regexp {[ \t]*NETS[ \t]+([^ \t]+)} $line lmatch netnums] {
      set nets [dict create]
      # Parse the "NETS" section
      parse_nets $fdef nets
      # puts stdout "Done with NETS section, dict size is [dict size $nets]"
   } elseif [regexp {[ \t]*PINS[ \t]+([^ \t]+)} $line lmatch pinnum] {
      skip_section $fdef PINS
   } elseif [regexp {[ \t]*VIARULE[ \t]+([^ \t]+)} $line lmatch viarulename] {
      skip_section $fdef $viarulename
   } elseif [regexp {[ \t]*VIA[ \t]+(.+)[ \t]*$} $line lmatch sitename] {
      skip_section $fdef $sitename
   } elseif [regexp {[ \t]*END[ \t]+DESIGN[ \t]*$} $line lmatch] {
      break
   } elseif [regexp {^[ \t]*#} $line lmatch] {
      # Comment line, ignore.
   } elseif ![regexp {^[ \t]*$} $line lmatch] {
      # Other things we don't care about
      set matches 0
      if [regexp {[ \t]*NAMESCASESENSITIVE} $line lmatch] {
         incr matches
      } elseif [regexp {[ \t]*VERSION} $line lmatch] {
         incr matches
      } elseif [regexp {[ \t]*BUSBITCHARS} $line lmatch] {
         incr matches
      } elseif [regexp {[ \t]*DIVIDERCHAR} $line lmatch] {
         incr matches
      } elseif [regexp {[ \t]*USEMINSPACING} $line lmatch] {
         incr matches
      } elseif [regexp {[ \t]*CLEARANCEMEASURE} $line lmatch] {
         incr matches
      } elseif [regexp {[ \t]*MANUFACTURINGGRID} $line lmatch] {
         incr matches
      } elseif [regexp {[ \t]*UNITS} $line lmatch] {
         incr matches
      } elseif [regexp {[ \t]*DESIGN} $line lmatch] {
         incr matches
      } elseif [regexp {[ \t]*DIEAREA} $line lmatch] {
         incr matches
      } elseif [regexp {[ \t]*TRACKS} $line lmatch] {
         incr matches
      } else {
         puts stderr "Unexpected input in DEF file:"
         puts stdout "Line is: $line"
      }
   }
}

close $fdef

#-----------------------------------------------------------------
# Now read the BLIF netlist, and rewrite all net connections from
# the list found in the DEF file
#-----------------------------------------------------------------

set instcount [dict create]

while {[gets $fnet line] >= 0} {
   if [regexp {^[ \t]*\.gate[ \t]+([^ \t]+)[ \t]+(.*)$} \
		$line lmatch macroname rest] {
      if {[dict exists $instcount $macroname]} {
	 set iidx [dict get $instcount $macroname]
	 incr iidx
	 dict set instcount $macroname $iidx
      } else {
	 dict set instcount $macroname 1
	 set iidx 1
      }
      set gateline ".gate $macroname"
      while {[regexp {[ \t]*([^ \t]+)[ \t]*=[ \t]*([^ \t]+)[ \t]*(.*)$} \
		$rest lmatch pinname netname nextconn] > 0} {
	 if {[catch {set newnet [dict get $nets ${macroname}_${iidx}/${pinname}]}]} {
	    # NOTE:  Dangling buffer outputs (for debug) do not show up in
	    # graywolf output.  They cannot be sorted, so just copy them
	    # as they are in the original blif file.
	    set gateline "${gateline} ${pinname}=${netname}"
	 } else {
	    set gateline "${gateline} ${pinname}=${newnet}"
	 }
	 set rest $nextconn
      }
      puts $fout "$gateline"
   } else {
      puts $fout $line
   }
}

#-----------------------------------------------------------------
#-----------------------------------------------------------------

close $fnet
if {$fout != "stdout"} {close $fout}

puts stdout "Done with blifanno.tcl"
