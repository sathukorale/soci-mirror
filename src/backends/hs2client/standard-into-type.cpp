//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_HS2CLIENT_SOURCE
#include "soci/hs2client/soci-hs2client.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;

#define FETCH_AND_SET_VALUE(data_type, column_type, column_index, row_index, ind, statment_obj)	{ \
																									std::unique_ptr<column_type> col = results.GetCol<column_type>(column_index); \
																									if (col->length() > row_index && col->IsNull(row_index) == false) \
																									{ \
																										(*(static_cast<data_type*>(_data))) = col->GetData(row_index); \
																										if (ind != nullptr) *ind = i_ok; \
																									} \
																									else if (ind != nullptr) *ind = i_null; \
																								} 

void hs2client_standard_into_type_backend::define_by_pos(int& position, void* data, exchange_type type)
{
	_data = data;
	_type = type;
	_position = position++;
}

void hs2client_standard_into_type_backend::pre_fetch()
{
    // ...
}

void hs2client_standard_into_type_backend::post_fetch(bool got_data, bool called_from_fetch, indicator* ind)
{
	/* According to 'http://soci.sourceforge.net/doc/master/api/backend/'
	   this is considered an end-of-row condition. So we can ignore this. */
	if (called_from_fetch && !got_data) return;

	/* Again according to http://soci.sourceforge.net/doc/master/api/backend/,
	   this index usually start with '1' */
	int column_index = _position - 1;
	const hs2client_statement_backend::segmented_result& segment = _statement.get_current_results();
	const hs2client::ColumnarRowSet& results = segment.get_results();

	/* Because here we only interested in one result and that's the first one. */
	int row_index = segment.get_segment_start_offset();

	switch (_type)
	{
		case x_char:
			FETCH_AND_SET_VALUE(char, hs2client::ByteColumn, column_index, row_index, ind, _statement);
			break;
		case x_stdstring:
			FETCH_AND_SET_VALUE(std::string, hs2client::StringColumn, column_index, row_index, ind, _statement);
			break;
		case x_short:
			FETCH_AND_SET_VALUE(int16_t, hs2client::Int16Column, column_index, row_index, ind, _statement);
			break;
		case x_integer:
			FETCH_AND_SET_VALUE(int32_t, hs2client::Int32Column, column_index, row_index, ind, _statement);
			break;
		case x_long_long:
			FETCH_AND_SET_VALUE(int64_t, hs2client::Int64Column, column_index, row_index, ind, _statement);
			break;
		case x_unsigned_long_long:
			// TODO : Not entire sure whether there should be a warning here.
			FETCH_AND_SET_VALUE(int64_t, hs2client::Int64Column, column_index, row_index, ind, _statement);
			break;
		case x_double:
			FETCH_AND_SET_VALUE(double, hs2client::DoubleColumn, column_index, row_index, ind, _statement);
			break;
		case x_stdtm:
		{
			/* We expect the date & time columns to be of type long(int64),
			and we expect the time in UTC, GMT, 0 in both directions */
			std::unique_ptr<hs2client::Int64Column> col = results.GetCol<hs2client::Int64Column>(column_index);
			if (col->length() > row_index && col->IsNull(row_index) == false) 
			{
				time_t time = col->GetData(row_index);
				(*(static_cast<std::tm*>(_data))) = *localtime(&time);

				if (ind != nullptr) *ind = i_ok;
			} 
			else if (ind != nullptr) *ind = i_null;
		}
		break;
		default:
			throw soci_error("Into element used with non-supported type.");
	}
}

void hs2client_standard_into_type_backend::clean_up()
{
    // ...
}
