#include "project.h"

void Project::buildPadFrame()
{
	lef::LEFMacro* m;
	lef::LEFPin* p;
	qreal height_corner = 0;
	int mx,my;
	int j;
	qreal mw,mh;
	qreal px,py;
	qreal pw,ph;
	qreal coreleft,coreright,corebottom,coretop; // die dimensions
	qreal padframeleft,padframeright,padframebottom,padframetop; // padframe dimensions
	qreal corew, coreh;
	qreal padframew,padframeh; // pad frame dimensions
	qreal len;
	QStringList sides;
	sides << "T" << "B" << "L" << "R";

	QMap<QString,QString> indexedCellInstance;
	QString pl1path = QDir(getLayoutDir()).filePath(getTopLevel()+".pl1");
	QString pl2path = QDir(getLayoutDir()).filePath(getTopLevel()+".pl2");
	QString pinpath = QDir(getLayoutDir()).filePath(getTopLevel()+".pin");
	QString preroutepath = QDir(getLayoutDir()).filePath(getTopLevel()+".preroute");
	QString padspath = getPadInfoFile();

	QFile pl1file(pl1path);
	QFile pl2file(pl2path);
	QFile pinfile(pinpath);
	QFile preroutefile(preroutepath);

	QString padName;
	QString cellName;
	QPointF pinCenter;
	QString pinSignal;
	QStringList lineList;
	QMap<QString,int> instanceOrientationMapping;
	QMap<QString,QRectF> instanceBox;
	int orient;

	pw = 100;
	ph = 100;

	if(pl1file.open(QIODevice::ReadOnly)) {
		bool firstLine = true;
		int cx1,cy1,cx2,cy2;
		QTextStream pl1stream(&pl1file);
		while (!pl1stream.atEnd()) {
			lineList = pl1stream.readLine().split(QRegExp("[\r\n\t ]+"), QString::SkipEmptyParts);
			if(lineList.count()>4) {
				cx1 = lineList[1].toInt();
				cy1 = lineList[2].toInt();
				cx2 = lineList[3].toInt();
				cy2 = lineList[4].toInt();
				if(firstLine) {
					coreleft=cx1;
					coreright=cx2;
					corebottom=cy1;
					coretop=cy2;
				} else {
					if(coreleft>cx1) coreleft=cx1;
					if(coreright<cx2) coreright=cx2;
					if(corebottom>cy1) corebottom=cy1;
					if(coretop<cy2) coretop=cy2;
				}
			}
		}
		pl1file.close();
	} else {
		qDebug() << pl1path << " can't be opened";
		return;
	}

	corew = coreright-coreleft;
	coreh = coretop-corebottom;

	if(QFileInfo(padspath).exists()) {
		PadInfo padInfo(padspath);

		instanceOrientationMapping["CORNER1"]=0; // left upper corner
		instanceOrientationMapping["CORNER2"]=6; // left lower corner
		instanceOrientationMapping["CORNER3"]=7; // right upper corner
		instanceOrientationMapping["CORNER4"]=3; // right lower corner
		foreach(QString side, sides) {
			for(int i=0;i<padInfo.getSideLength();i++) {
				padName=side+QString::number(i+1);
				if(side=="T") instanceOrientationMapping[padName]=0;
				if(side=="L") instanceOrientationMapping[padName]=6;
				if(side=="R") instanceOrientationMapping[padName]=7;
				if(side=="B") instanceOrientationMapping[padName]=3;
			}
		}

		padframew = 0;
		padframeh = 0;
		foreach(QString side, sides) {
			len = 0;
			for(int i=0;i<padInfo.getSideLength();i++) {
				padName=side+QString::number(i+1);
				cellName = padInfo.getPadCell(padName);
				m = getMacro(cellName);
				mw = ((m)?m->getWidth():100)*100; // next to each other
				len+=mw;
			}
			if(side=="L") {
				m = getMacro(padInfo.getPadCell("CORNER1"));
				if(m) len+=m->getHeight()*100;
				m = getMacro(padInfo.getPadCell("CORNER2"));
				if(m) len+=m->getHeight()*100;
			}
			if(side=="R") {
				m = getMacro(padInfo.getPadCell("CORNER3"));
				if(m) len+=m->getHeight()*100;
				m = getMacro(padInfo.getPadCell("CORNER4"));
				if(m) len+=m->getHeight()*100;
			}
			if(side=="T") {
				m = getMacro(padInfo.getPadCell("CORNER1"));
				if(m) len+=m->getWidth()*100;
				m = getMacro(padInfo.getPadCell("CORNER3"));
				if(m) len+=m->getWidth()*100;
			}
			if(side=="B") {
				m = getMacro(padInfo.getPadCell("CORNER2"));
				if(m) len+=m->getWidth()*100;
				m = getMacro(padInfo.getPadCell("CORNER4"));
				if(m) len+=m->getWidth()*100;
			}
			if((side=="L")||(side=="R")) if(padframeh<len) padframeh=len;
			if((side=="T")||(side=="B")) if(padframew<len) padframew=len;
		}

		padframeleft = (coreleft+(corew/2))-(padframew/2);
		padframeright = (coreleft+(corew/2))+(padframew/2);
		padframebottom = (corebottom+(coreh/2))-(padframeh/2);
		padframetop = (corebottom+(coreh/2))+(padframeh/2);

		cellName = padInfo.getPadCell("CORNER1");  // left upper corner
		m = getMacro(cellName);
		mw = ((m)?m->getWidth():100)*100;
		mh = ((m)?m->getHeight():100)*100;
		if(mh>height_corner) height_corner = mh;
		instanceBox["CORNER1"]=QRectF(padframeleft,padframetop-mh,mw,mh);

		cellName = padInfo.getPadCell("CORNER2"); // left lower corner
		m = getMacro(cellName);
		mw = ((m)?m->getWidth():100)*100;
		mh = ((m)?m->getHeight():100)*100;
		if(mh>height_corner) height_corner = mh;
		instanceBox["CORNER2"]=QRectF(padframeleft,padframebottom,mw,mh);

		cellName = padInfo.getPadCell("CORNER3"); // right upper corner
		m = getMacro(cellName);
		mw = ((m)?m->getWidth():100)*100;
		mh = ((m)?m->getHeight():100)*100;
		if(mh>height_corner) height_corner = mh;
		instanceBox["CORNER3"]=QRectF(padframeright-mw,padframetop-mh,mw,mh);

		cellName = padInfo.getPadCell("CORNER4"); // right lower corner
		m = getMacro(cellName);
		mw = ((m)?m->getWidth():100)*100;
		mh = ((m)?m->getHeight():100)*100;
		if(mh>height_corner) height_corner = mh;
		instanceBox["CORNER4"]=QRectF(padframeright-mw,padframebottom,mw,mh);

		foreach(QString side, sides) {
			mx = (side=="R")?padframeright-height_corner:padframeleft;
			my = (side=="T")?padframetop-height_corner:padframebottom;
			if((side=="R")||(side=="L")) my+=height_corner;
			if((side=="T")||(side=="B")) mx+=height_corner;
			for(int i=0;i<padInfo.getSideLength();i++) {
				padName=side+QString::number(i+1);
				cellName = padInfo.getPadCell(padName);
				m = getMacro(cellName);
				switch(instanceOrientationMapping[padName]) {
					case 6:
					case 7:
						mh = ((m)?m->getWidth():100)*100;
						mw = ((m)?m->getHeight():100)*100;
						break;
					default:
						mw = ((m)?m->getWidth():100)*100;
						mh = ((m)?m->getHeight():100)*100;
						break;
				}
				instanceBox[padName]=QRectF(mx,my,mw,mh);
				mx+=((side=="T")||(side=="B"))?mw:0;
				my+=((side=="R")||(side=="L"))?mh:0;
			}
		}

		QMap<QString,int> cellCounters;
		foreach(QString padName, instanceBox.keys()) {
			cellName = padInfo.getPadCell(padName);
			if(!cellCounters.contains(cellName)) cellCounters[cellName]=0;
			indexedCellInstance[padName]=cellName+"_"+QString::number(cellCounters[cellName]+1);
			cellCounters[cellName]++;
		}

		if(pl1file.open(QIODevice::WriteOnly|QIODevice::Append)) {
			QTextStream pl1stream(&pl1file);
			foreach(QString padName, instanceBox.keys()) {
				pl1stream << indexedCellInstance[padName];
				pl1stream << " ";
				pl1stream << QString::number(instanceBox[padName].x());
				pl1stream << " ";
				pl1stream << QString::number(instanceBox[padName].y());
				pl1stream << " ";
				pl1stream << QString::number(instanceBox[padName].x()+instanceBox[padName].width());
				pl1stream << " ";
				pl1stream << QString::number(instanceBox[padName].y()+instanceBox[padName].height());
				pl1stream << " ";
				pl1stream << QString::number(instanceOrientationMapping[padName]);
				pl1stream << " 1";
				pl1stream << endl;
			}
			pl1file.close();
		} else {
			qDebug() << pl1path << " can't be opened";
			return;
		}

		QMap<QString,QMap<QString,QMap<QString,QPointF>>> signalMacroPinPosition;

		qreal x,y;
		foreach(QString padName, instanceBox.keys()) {
			cellName = padInfo.getPadCell(padName);
			m = getMacro(cellName);
			if(m) {
				foreach(QString pin, m->getPinNames()) {
					p = m->getPin(pin);
					if(!p) continue; // can't place this thing
					pinSignal = padInfo.getPadPinSignal(padName,pin);
					if(pinSignal=="None") continue; // place holder
					if(pinSignal==QString()) continue; // none
					pinCenter = p->getCenter()*100;
					x = pinCenter.x();
					y = pinCenter.y();
					if(instanceOrientationMapping[padName]==3) { // South
						px=-x;
						py=-y;
						px+=instanceBox[padName].width();
						py+=instanceBox[padName].height();
					} else if(instanceOrientationMapping[padName]==7) { // East
						px=-y;
						py=x;
						px+=instanceBox[padName].width();
						px=-px;
						py=-py;
						px+=instanceBox[padName].height();
						py+=instanceBox[padName].width();
					} else if(instanceOrientationMapping[padName]==6) { // West
						px=-y;
						py=x;
						px+=instanceBox[padName].height();
						py+=instanceBox[padName].width();
					} else {
						px=x;
						py=y;
					}
					px+=instanceBox[padName].x();
					py+=instanceBox[padName].y();
					signalMacroPinPosition[pinSignal][padName][pin]=QPointF(px,py);
				}
			}
		}

		QString twpinName;
		QMap<QString,QPointF> twpinPosition;
		QMap<QString,int> twpinOrientationMapping;
		QMap<QString,QString> twpinSignalMapping;
		QVector<QStringList> prerouteNodeGroups;
		qreal cored,padframed;
		cored = (corew>coreh)?corew:coreh;
		padframed = (padframew>padframeh)?padframew:padframeh;

		foreach(QString signal, signalMacroPinPosition.keys()) {
			if(getPowerNets().contains(signal)) continue; // VDDs
			if(getGroundNets().contains(signal)) continue; // VSSs
			foreach(QString padName	, signalMacroPinPosition[signal].keys()) {
				foreach(QString pin	, signalMacroPinPosition[signal][padName].keys()) {
					orient = instanceOrientationMapping[padName];
					px = signalMacroPinPosition[signal][padName][pin].x();
					py = signalMacroPinPosition[signal][padName][pin].y();
					twpinName = "twpin_"+signal;
					twpinPosition[twpinName]=QPointF(px,py);
					twpinOrientationMapping[twpinName]=orient;
					twpinSignalMapping[twpinName]=signal;
				}
			}
		}

		QString pinName;
		QMap<QString,QPointF> preRouteNodePosition;
		QMap<QString,int> preRouteNodeOrientation;
		QMap<QString,QString> preRouteSignalMapping;
		QStringList preRouteGroup;
		qreal origpx, origpy;
		qreal origpx2, origpy2;
		int numPoints = 5;
		int width = 10;
		qreal dx,dy;
		foreach(QString signal, signalMacroPinPosition.keys()) {
			if(getPowerNets().contains(signal)||getGroundNets().contains(signal)) {
				foreach(QString padName	, signalMacroPinPosition[signal].keys()) {
					foreach(QString pin	, signalMacroPinPosition[signal][padName].keys()) {
						orient = instanceOrientationMapping[padName];
						origpx2 = signalMacroPinPosition[signal][padName][pin].x();
						origpy2 = signalMacroPinPosition[signal][padName][pin].y();
						if(orient==3) { // South
							for(int j=0;j<width;j++) {
								preRouteGroup = QStringList();
								origpx = origpx2-width/2+j;
								origpy = origpy2;
								dx = coreleft-origpx;
								dy = corebottom-origpy;

								pinName = signal+QString::number(j+1);
								px=origpx;
								py=origpy;
								preRouteNodePosition[pinName]=QPointF(px,py);
								preRouteNodeOrientation[pinName]=orient;
								preRouteSignalMapping[pinName]=signal;
								preRouteGroup.append(pinName);

								pinName = signal+QString::number(j+2);
								px=origpx;
								py=corebottom;
								preRouteNodePosition[pinName]=QPointF(px,py);
								preRouteNodeOrientation[pinName]=orient;
								preRouteSignalMapping[pinName]=signal;
								preRouteGroup.append(pinName);

								prerouteNodeGroups.append(preRouteGroup);
							}
						} else if(orient==7) { // East
						} else if(orient==6) { // West
						} else { // North
						}
					}
				}
			} else {
				foreach(QString padName	, signalMacroPinPosition[signal].keys()) {
					foreach(QString pin	, signalMacroPinPosition[signal][padName].keys()) {
						preRouteGroup = QStringList();
						orient = instanceOrientationMapping[padName];
						origpx = signalMacroPinPosition[signal][padName][pin].x();
						origpy = signalMacroPinPosition[signal][padName][pin].y();
						if(orient==3) { // South
							dx = coreleft-origpx;
							dy = corebottom-origpy;
							/*for(int i=0;i<numPoints;i++) {
								pinName = signal+QString::number(i);
								px=origpx+i*(dx/numPoints);
								py=origpy+i*(dy/numPoints);
								py+=prerouteNodeGroups.count()*100;
								preRouteNodePosition[pinName]=QPointF(px,py);
								preRouteNodeOrientation[pinName]=orient;
								preRouteSignalMapping[pinName]=signal;
								preRouteGroup.append(pinName);
							}*/
						} else if(orient==7) { // East
						} else if(orient==6) { // West
						} else { // North
							/*pinName = signal+QString::number(i);
							preRouteNodePosition[pinName]=QPointF(px,py);
							preRouteNodeOrientation[pinName]=orient;
							preRouteSignalMapping[pinName]=signal;
							preRouteGroup.append(pinName);
							py-=2000;*/
						}
						prerouteNodeGroups.append(preRouteGroup);
					}
				}
			}
		}

		if(pl2file.open(QIODevice::WriteOnly|QIODevice::Append)) {
			QTextStream pl2stream(&pl2file);
			foreach(QString twpinName, twpinPosition.keys()) {
				pl2stream << twpinName << " ";
				pl2stream << twpinPosition[twpinName].x()-pw/2;
				pl2stream << " ";
				pl2stream << twpinPosition[twpinName].y()-ph/2;
				pl2stream << " ";
				pl2stream << twpinPosition[twpinName].x()+pw/2;
				pl2stream << " ";
				pl2stream << twpinPosition[twpinName].y()+ph/2;
				pl2stream << " ";
				pl2stream << QString::number(twpinOrientationMapping[twpinName]);
				pl2stream << " 0 0";
				pl2stream << endl;
			}
			pl2file.close();
		} else {
			qDebug() << pl1path << " can't be opened";
			return;
		}

		j = 0;
		if(pinfile.open(QIODevice::WriteOnly | QIODevice::Append)) {
			QTextStream pinstream(&pinfile);
			foreach(QString signal, signalMacroPinPosition.keys()) {
				foreach(QString padName	, signalMacroPinPosition[signal].keys()) {
					foreach(QString pin	, signalMacroPinPosition[signal][padName].keys()) {
						pinstream << signal << " ";
						pinstream << j << " ";
						pinstream << indexedCellInstance[padName] << " ";
						pinstream << pin << " ";
						pinstream << signalMacroPinPosition[signal][padName][pin].x();
						pinstream << " ";
						pinstream << signalMacroPinPosition[signal][padName][pin].y();
						pinstream << " ";
						pinstream << QString::number(instanceOrientationMapping[padName]);
						pinstream << " 1 1";
						pinstream << endl;
						j++;
					}
				}
			}
			foreach(QString twpinName, twpinPosition.keys()) {
				pinstream << twpinSignalMapping[twpinName] << " ";
				pinstream << j << " ";
				pinstream << "twpin_" << twpinSignalMapping[twpinName] << " ";
				pinstream << twpinSignalMapping[twpinName] << " ";
				pinstream << twpinPosition[twpinName].x() << " ";
				pinstream << twpinPosition[twpinName].y() << " ";
				pinstream << QString::number(twpinOrientationMapping[padName]);
				pinstream << " 1 1";
				pinstream << endl;
				j++;
			}
			pinfile.close();
		} else {
			qDebug() << pinpath << " can't be opened";
			return;
		}

		if(preroutefile.open(QIODevice::WriteOnly)) {
			QTextStream preroutestream(&preroutefile);
			preroutestream << "core ";
			preroutestream << coreleft << " " << coreright << " " << corebottom << " " << coretop << endl;
			preroutestream << "padframe ";
			preroutestream << padframeleft << " " << padframeright << " " << padframebottom << " " << padframetop << endl;
			foreach(QString pinName, preRouteNodePosition.keys()) {
				preroutestream << "position " << pinName;
				preroutestream << " " << preRouteNodePosition[pinName].x();
				preroutestream << " " << preRouteNodePosition[pinName].y();
				preroutestream << endl;
			}
			foreach(QStringList group, prerouteNodeGroups) {
				if(group.count()) {
					preroutestream << "net " << preRouteSignalMapping[group[0]];
					foreach(QString pin, group) {
						preroutestream << " " << pin;
					}
					preroutestream << endl;
				}
			}
			preroutefile.close();
		} else {
			qDebug() << preroutepath << " can't be opened";
			return;
		}
	}
}

QString Project::getPadInfoFile()
{
	return QDir(getSourceDir()).filePath(getTopLevel()+".pads");
}

QString Project::getPadFrameFile()
{
	return QDir(getLayoutDir()).filePath("padframe.mag");
}