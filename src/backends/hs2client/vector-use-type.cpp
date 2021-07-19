//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_HS2CLIENT_SOURCE
#include "soci/hs2client/soci-hs2client.h"

#include <cstring>
#include <limits>
#include "soci-exchange-cast.h"
#include "soci-dtocstr.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;

/* Most of the following content is directly copied from the 'backends\mysql\vector-use-type.cpp' file since
situation with 'mysql' is somewhat similar to 'hs2client' */

void hs2client_vector_use_type_backend::bind_by_pos(int& position, void* data, exchange_type type)
{
	_data = data;
	_type = type;
	_position = position++;
}

void hs2client_vector_use_type_backend::bind_by_name(const std::string& name, void* data, exchange_type type)
{
	_data = data;
	_type = type;
	_name = name;
}

void hs2client_vector_use_type_backend::pre_use(const indicator* ind)
{
	if (_position > 0)
	{
		_statement.get_current_statement().clear_parameter_values(_position);
	}
	else
	{
		_statement.get_current_statement().clear_parameter_values(_name);
	}

	const std::size_t vsize = size();
	for (size_t i = 0; i != vsize; ++i)
	{
		char* buf;

		// the data in vector can be either i_ok or i_null
		if (ind != NULL && ind[i] == i_null)
		{
			buf = new char[5];
			std::strcpy(buf, "NULL");
		}
		else
		{
			// allocate and fill the buffer with text-formatted client data
			switch (_type)
			{
				/* We have to treat char as int8_t */
				case x_char:
				{
					std::vector<char> *pv = static_cast<std::vector<char> *>(_data);
					std::vector<char> &v = *pv;

					const std::size_t buf_size = std::numeric_limits<char>::digits10 + 3;
					buf = new char[buf_size];

					snprintf(buf, buf_size, "%d", static_cast<int>(exchange_type_cast<x_char>(&v[i])));
				}
				break;

				case x_stdstring:
				{
					std::vector<std::string> *pv = static_cast<std::vector<std::string> *>(_data);
					std::vector<std::string> &v = *pv;

					const std::string& s = v[i];
					soci::utils::escape_string_to_buffer(s, buf);
				}
				break;

				case x_short:
				{
					std::vector<short> *pv = static_cast<std::vector<short> *>(_data);
					std::vector<short> &v = *pv;
					const std::size_t buf_size = std::numeric_limits<short>::digits10 + 3;
					buf = new char[buf_size];

					snprintf(buf, buf_size, "%d", static_cast<int>(v[i]));
				}
				break;

				case x_integer:
				{
					std::vector<int> *pv = static_cast<std::vector<int> *>(_data);
					std::vector<int> &v = *pv;
					const std::size_t buf_size = std::numeric_limits<int>::digits10 + 3;
					buf = new char[buf_size];

					snprintf(buf, buf_size, "%d", v[i]);
				}
				break;

				case x_long_long:
				{
					std::vector<long long> *pv = static_cast<std::vector<long long> *>(_data);
					std::vector<long long> &v = *pv;
					const std::size_t buf_size = std::numeric_limits<long long>::digits10 + 3;
					buf = new char[buf_size];

					snprintf(buf, buf_size, "%" LL_FMT_FLAGS "d", v[i]);
				}
				break;

				case x_unsigned_long_long:
				{
					std::vector<unsigned long long> *pv = static_cast<std::vector<unsigned long long> *>(_data);
					std::vector<unsigned long long> &v = *pv; 
					const std::size_t buf_size = std::numeric_limits<unsigned long long>::digits10 + 3;
					buf = new char[buf_size];

					snprintf(buf, buf_size, "%" LL_FMT_FLAGS "u", v[i]);
				}
				break;

				case x_double:
				{
					std::vector<double> *pv = static_cast<std::vector<double> *>(_data);
					std::vector<double> &v = *pv;

					const std::string s = double_to_cstring(v[i]);

					buf = new char[s.size() + 1];
					std::strcpy(buf, s.c_str());
				}
				break;

				case x_stdtm:
				{
					std::vector<std::tm> *pv = static_cast<std::vector<std::tm> *>(_data);
					std::vector<std::tm> &v = *pv;

					std::tm& time = exchange_type_cast<x_stdtm>(&v[i]);
					const std::size_t buf_size = std::numeric_limits<unsigned long long>::digits10 + 3;

					buf = new char[buf_size];
					snprintf(buf, buf_size, "%li", mktime(&time));
				}
				break;

				default:
					throw soci_error("Use vector element used with non-supported type.");
			}
		}

		if (_position > 0)
		{
			// binding by position
			_statement.get_current_statement().add_parameter_value(_position, buf);
		}
		else
		{
			// binding by name
			_statement.get_current_statement().add_parameter_value(_name, buf);
		}
	}
}

std::size_t hs2client_vector_use_type_backend::size()
{
	std::size_t sz = 0;

	switch (_type)
	{
		case x_char:			   sz = static_cast<std::vector<char>*>(_data)->size(); break;
		case x_short:			   sz = static_cast<std::vector<short>*>(_data)->size(); break;
		case x_integer:			   sz = static_cast<std::vector<int>*>(_data)->size(); break;
		case x_long_long:		   sz = static_cast<std::vector<long long>*>(_data)->size(); break;
		case x_unsigned_long_long: sz = static_cast<std::vector<unsigned long long>*>(_data)->size(); break;
		case x_double:			   sz = static_cast<std::vector<double>*>(_data)->size(); break;
		case x_stdstring:		   sz = static_cast<std::vector<std::string>*>(_data)->size(); break;
		case x_stdtm:			   sz = static_cast<std::vector<std::tm>*>(_data)->size(); break;

		default: throw soci_error("Use vector element used with non-supported type.");
	}

	return sz;
}

void hs2client_vector_use_type_backend::clean_up()
{
}
