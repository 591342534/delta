/*****************************************************************************
 data_query_manager Copyright (c) 2016. All Rights Reserved.

 FileName: sqlmake.cpp
 Version: 1.0
 Date: 2016.10.27

 History:
 ericsheng     2016.10.27   1.0     Create
 ******************************************************************************/

#ifndef SQLMAKE_H__
#define SQLMAKE_H__

#include <string>
#include <vector>
#include "base/dictionary.h"

namespace serverframe
{

class comm
{
public:
	static std::string itos(int n);
	static std::string ftos(double f);
	static void string_replace(std::string &s, const std::string &source, const std::string &target);
	static std::vector<std::string> split(std::string str, std::string pattern);
	static std::string groupbysplit(std::vector<std::string> &v, std::string pattern);
	static std::string round_price(const std::string &price, int dotnum = 4);
	static std::string round_price(double price, int dotnum = 4);
};

class sqlmaker
{
public:
	enum makertype
	{
		mtinsert = 0,
		mtupdate,
		mtselect,
		mtdelete
	};
	enum datetimetype
	{
		datetime = 0,
		date,
		time
	};
	enum datatype
	{
		dtstring = 0,
		dtint,
		dtdouble
	};
public:
	sqlmaker();
	sqlmaker(const std::string &tablename, makertype maketype = mtinsert);
	virtual ~sqlmaker();

public:
	std::string sql();
	void add_field(const std::string &fieldname, const std::string &value);
	void add_field(const std::string &fieldname, int value);
	void add_field(const std::string &fieldname, double value);
	void add_field_date_time(const std::string &fieldname, datetimetype type);
	void update_field(const std::string &fieldname, const std::string &value);
	void update_field(const std::string &fieldname, int value);
	void update_field(const std::string &fieldname, double value);
	void update_field_date_time(const std::string &fieldname, datetimetype type);
	void condition_field(const std::string &fieldname, const std::string &value);
	void condition_field(const std::string &fieldname, int value);
	void condition_field(const std::string &fieldname, double value);

protected:
	void append_element(std::string &current, const std::string &append);
	void make_sqlcondition(std::string &current, const std::string &append);

private:
	int m_maketype;
	std::string m_sql;
	std::string m_field;
	std::string m_value;
	std::string m_condition;
	std::string m_update;
};


}

#endif
