#pragma once
#include <QStack>
#include <QHash>

#include <fstream>
#include <istream>
#include <iostream>

namespace vcd {
	enum class LogicValue {
		ONE,
		ZERO,
		UNKOWN,
		HIGHZ
	};
	std::ostream& operator<<(std::ostream& os, LogicValue val);

	class Var {
		public:
			enum class Type {
				WIRE,
				REG,
				PARAMETER
			};

			typedef int Id;

			Var() = default;
			Var(Type new_type, size_t new_width, Id new_id, std::string new_str_id, std::vector<std::string> new_hierarchical_name)
				: type_(new_type)
				, width_(new_width)
				, id_(new_id)
				, str_id_(new_str_id)
				, hierarchical_name_(new_hierarchical_name)
				{}

			Type type() const { return type_; }
			size_t width() const { return width_; }
			std::string str_id() const { return str_id_; }
			Id id() const { return id_; }
			std::vector<std::string> hierarchical_name() const { return hierarchical_name_; }
			std::string name() const { return *(--hierarchical_name_.end()); }

		private:
			Type type_;
			size_t width_;
			Id id_;
			std::string str_id_;
			std::vector<std::string> hierarchical_name_;
	};
	std::ostream& operator<<(std::ostream& os, Var::Type type);

	class TimeValue {
		public:
			TimeValue(size_t new_time=0, Var::Id new_var_id=-1, LogicValue new_value=LogicValue::UNKOWN) :
				  time_(new_time)
				, var_id_(new_var_id)
				, value_(new_value)
				{}

			size_t time() const { return time_; }
			Var::Id var_id() const { return var_id_; }
			LogicValue value() const { return value_; }

		private:
			size_t time_;
			Var::Id var_id_;
			LogicValue value_;
	};

	class Header {
		public:
			const std::string& date() const { return date_; }
			const std::string& version() const { return version_; }
			const std::string& timescale() const { return timescale_; }

			void set_date(const std::string& new_date) { date_ = new_date; }
			void set_version(const std::string& new_version) { version_ = new_version; }
			void set_timescale(const std::string& new_timescale) { timescale_ = new_timescale; }
		private:
			std::string date_;
			std::string version_;
			std::string timescale_;
	};

	class VCDScanner;
	class VCDParser;
	class VCData
	{
	public:
		VCData(QString s);

	private:
		QString streamname;
		VCDScanner *lexer;
		VCDParser *parser;
		bool trace_scanning;
		bool trace_parsing;
		bool rootSet;
	};
}
