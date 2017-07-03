#include "sqlmake.h"
#include <stdio.h>

namespace serverframe
{

/*******************
class comm
*******************/
std::string comm::itos(int n)
{
	char sz[20] = { 0 };
	sprintf(sz, "%d", n);
	return sz;
}

std::string comm::ftos(double f)
{
	char sz[20] = { 0 };
	sprintf(sz, "%f", f);
	return sz;
}

void comm::string_replace(std::string &s, const std::string &source, const std::string &target)
{
	std::string::size_type pos = 0;
	std::string::size_type a = source.size();
	std::string::size_type b = target.size();
	while ((pos = s.find(source, pos)) != std::string::npos)
	{
		s.replace(pos, a, target);
		pos += b;
	}
}

std::vector<std::string> comm::split(std::string str, std::string pattern)
{
	std::string::size_type pos;
	std::vector<std::string> result;
	str += pattern;
	std::string::size_type size = str.size();

	for (int i = 0; i < (int)size; i++)
	{
		pos = str.find(pattern, i);
		if (pos < size)
		{
			std::string s = str.substr(i, pos - i);
			result.push_back(s);
			i = pos + pattern.size() - 1;
		}
	}
	return result;
}

std::string comm::groupbysplit(std::vector<std::string> &v, std::string pattern)
{
	std::string str;
	std::vector<std::string>::iterator it = v.begin();
	for (; it != v.end(); it++)
	{
		if (str.empty())
		{
			str = "'" + *it + "'";
		}
		else
		{
			str += pattern + "'" + *it + "'";
		}
	}
	return str;
}

std::string comm::round_price(const std::string &price, int dotnum)
{
	if (dotnum < 1)
	{
		return "";
	}

	std::string ret;

	int find = price.find('.');
	if (find != (int)std::string::npos)
	{
		ret = (price + "0000").substr(0, find + dotnum + 1);
	}
	else
	{
		ret = price;
	}

	return ret;
}

std::string comm::round_price(double price, int dotnum)
{
	if (dotnum < 1)
	{
		return "";
	}

	char ret[100] = {0};
	std::string format = "%." + itos(dotnum) + "lf";
	sprintf(ret, format.c_str(), price);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
sqlmaker::sqlmaker()
	: m_maketype(mtinsert)
{

}

sqlmaker::sqlmaker(const std::string &tablename, makertype maketype)
    : m_maketype(maketype)
{
	if (mtinsert == maketype)
	{
		m_sql = "insert into " + tablename + "({%1}) values({%2});";
	}
	else if (mtupdate == maketype)
	{
		m_sql = "update " + tablename + " set {%1} {%2}";
	}
	else if (mtdelete == maketype)
	{
		m_sql = "delete from " + tablename + " {%1}";
	}
}

sqlmaker::~sqlmaker()
{

}

std::string sqlmaker::sql()
{
	if (mtinsert == m_maketype)
	{
		comm::string_replace(m_sql, "{%1}", m_field);
		comm::string_replace(m_sql, "{%2}", m_value);
	}
	else if (mtupdate == m_maketype)
	{
		comm::string_replace(m_sql, "{%1}", m_update);
		comm::string_replace(m_sql, "{%2}", m_condition);
	}
	else if (mtdelete == m_maketype)
	{
		comm::string_replace(m_sql, "{%1}", m_condition);
	}
	return m_sql;
}

void sqlmaker::add_field(const std::string &fieldname, const std::string &value)
{
	append_element(m_field, fieldname);
	append_element(m_value, "'" + value + "'");
}

void sqlmaker::add_field(const std::string &fieldname, int value)
{
	append_element(m_field, fieldname);
	append_element(m_value, comm::itos(value));
}

void sqlmaker::add_field(const std::string &fieldname, double value)
{
	append_element(m_field, fieldname);
	append_element(m_value, comm::ftos(value));
}

void sqlmaker::update_field(const std::string &fieldname, const std::string &value)
{
	append_element(m_update, fieldname + "='" + value + "'");
}

void sqlmaker::update_field(const std::string &fieldname, int value)
{
	append_element(m_update, fieldname + "=" + comm::itos(value));
}

void sqlmaker::update_field(const std::string &fieldname, double value)
{
	append_element(m_update, fieldname + "=" + comm::ftos(value));
}

void sqlmaker::condition_field(const std::string &fieldname, const std::string &value)
{
	make_sqlcondition(m_condition, fieldname + "='" + value + "'");
}

void sqlmaker::condition_field(const std::string &fieldname, int value)
{
	make_sqlcondition(m_condition, fieldname + "=" + comm::itos(value));
}

void sqlmaker::condition_field(const std::string &fieldname, double value)
{
	make_sqlcondition(m_condition, fieldname + "=" + comm::ftos(value));
}

void sqlmaker::add_field_date_time(const std::string &fieldname, datetimetype type)
{
	append_element(m_field, fieldname);
	if (datetime == type)
	{
		append_element(m_value, "now()");
	}
	else if (date == type)
	{
		append_element(m_value, "curdate()");
	}
	else if (time == type)
	{
		append_element(m_value, "curtime()");
	}
}

void sqlmaker::update_field_date_time(const std::string &fieldname, datetimetype type)
{
	append_element(m_field, fieldname);
	if (datetime == type)
	{
		append_element(m_update, fieldname + "=now()");
	}
	else if (date == type)
	{
		append_element(m_update, fieldname + "=curdate()");
	}
	else if (time == type)
	{
		append_element(m_update, fieldname + "=curtime()");
	}
}

void sqlmaker::append_element(std::string &current, const std::string &append)
{
	if (current.empty())
	{
		current += append;
	}
	else
	{
		current += "," + append;
	}
}

void sqlmaker::make_sqlcondition(std::string &current, const std::string &append)
{
	if (current.empty())
	{
		current = " where " + append;
	}
	else
	{
		current += " and " + append;
	}
}

}
