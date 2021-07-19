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
#include <time.h>
#include <algorithm>
#include "soci-exchange-cast.h"
#include "soci-dtocstr.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;

/* Most of the following content is directly copied from the 'backends\mysql\standard-use-type.cpp' file since
   situation with 'mysql' is somewhat similar to 'hs2client' */

void hs2client_standard_use_type_backend::bind_by_pos(int& position, void* data, exchange_type type, bool /*readOnly*/)
{
	_data = data;
	_type = type;
	_position = position++;
}

void hs2client_standard_use_type_backend::bind_by_name(const std::string& name, void* data, exchange_type type, bool /*readOnly*/)
{
	_data = data;
	_type = type;
	_name = name;
}

void hs2client_standard_use_type_backend::pre_use(const indicator* ind)
{
	if (ind != NULL && *ind == i_null)
	{
		_buf = new char[5];
		std::strcpy(_buf, "NULL");
	}
	else
	{
		switch (_type)
		{
			/* We have to treat char as int8_t */
			case x_char:			
			{
				const std::size_t buf_size = std::numeric_limits<char>::digits10 + 3;
				_buf = new char[buf_size];

				snprintf(_buf, buf_size, "%d", static_cast<int>(exchange_type_cast<x_char>(_data)));
			}
			break;

			case x_stdstring:
			{
				const std::string& s = exchange_type_cast<x_stdstring>(_data);
				soci::utils::escape_string_to_buffer(s, _buf);
			}
			break;

			case x_short:
			{
				const std::size_t buf_size = std::numeric_limits<short>::digits10 + 3;
				_buf = new char[buf_size];

				snprintf(_buf, buf_size, "%d", static_cast<int>(exchange_type_cast<x_short>(_data)));
			}
			break;

			case x_integer:				
			{
				const std::size_t buf_size = std::numeric_limits<int>::digits10 + 3;
				_buf = new char[buf_size];
				
				snprintf(_buf, buf_size, "%d", exchange_type_cast<x_integer>(_data));
			}
			break;

			case x_long_long:			
			{				
				const std::size_t buf_size = std::numeric_limits<long long>::digits10 + 3;
				_buf = new char[buf_size];

				snprintf(_buf, buf_size, "%" LL_FMT_FLAGS "d", exchange_type_cast<x_long_long>(_data));
			}
			break;

			case x_unsigned_long_long:
			{
				const std::size_t buf_size = std::numeric_limits<unsigned long long>::digits10 + 3;
				_buf = new char[buf_size];

				snprintf(_buf, buf_size, "%" LL_FMT_FLAGS "u", exchange_type_cast<x_unsigned_long_long>(_data));
			}
			break;

			case x_double:			
			{				
				const double d = exchange_type_cast<x_double>(_data);
				const std::string s = double_to_cstring(d);

				_buf = new char[s.size() + 1];
				std::strcpy(_buf, s.c_str());
			}
			break;

			case x_stdtm:			
			{
				std::tm& time = exchange_type_cast<x_stdtm>(_data);
				const std::size_t buf_size = std::numeric_limits<unsigned long long>::digits10 + 3;

				_buf = new char[buf_size];
				snprintf(_buf, buf_size, "%li", mktime(&time));
			}
			break;

			default:
				throw soci_error("Use element used with non-supported type.");
		}
	}

	if (_position > 0)
	{
		// binding by index, although we are just emulating it
		_statement.get_current_statement().clear_parameter_values(_position);
		_statement.get_current_statement().add_parameter_value(_position, _buf);
	}
	else
	{
		// binding by name
		_statement.get_current_statement().clear_parameter_values(_name);
		_statement.get_current_statement().add_parameter_value(_name, _buf);
	}
}

void hs2client_standard_use_type_backend::post_use(bool /* got_data */, indicator * /* ind */)
{
    // TODO : Yet to check what we have to do here. Check the stuff mentioned under 'mysql'
}

void hs2client_standard_use_type_backend::clean_up()
{
	// Nothing to do here, because all the new '_buf'
	// allocations are sent to the _statement.get_current_statement()
	// and they should be deleted there.
}
