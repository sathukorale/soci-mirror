//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_HS2CLIENT_SOURCE
#include "soci/hs2client/soci-hs2client.h"

using namespace soci;
using namespace soci::details;

#define FETCH_AND_PUT_VALUE(data_type, column_value, column_index, indicators, row_count)	\
		std::vector<data_type>* data_vector = (static_cast<std::vector<data_type>*>(_data)); \
		if (container_row_index == 0) \
		{ \
			data_vector->clear(); \
			data_vector->resize(row_count); \
		} \
		\
		if (column_value->length() > container_row_index && column_value->IsNull(segment_row_index) == false) \
		{ \
			if (data_vector->size() <= static_cast<size_t>(container_row_index)) \
				data_vector->resize(container_row_index + 1); \
			\
			(*data_vector)[container_row_index] = column_value->GetData(segment_row_index); \
			if (indicators != nullptr) indicators[container_row_index] = i_ok; \
		} \
		else if (indicators != nullptr) indicators[container_row_index] = i_null;

void hs2client_vector_into_type_backend::define_by_pos(int& position, void* data, exchange_type type)
{
	_data = data;
	_type = type;
	_position = position++;
}

void hs2client_vector_into_type_backend::pre_fetch()
{
    // ...
}

void hs2client_vector_into_type_backend::post_fetch(bool got_data, indicator* ind)
{
	if (!got_data) return;

	/* Again according to http://soci.sourceforge.net/doc/master/api/backend/,
	this index usually start with '1' */
	int column_index = _position - 1;

	const hs2client_statement_backend::segmented_result& segment = _statement.get_current_results();
	const hs2client::ColumnarRowSet& current_results = segment.get_results();

	/* We don't really have a direct way of getting the result length.
	So we are going to read the first row to get an idea. The issue
	with this method is that, each column could return different
	result lengths. Unfortunately we don't have any other way of doing this
	*/
	int row_count = segment.get_segment_size();
	uint32_t segment_row_index = segment.get_segment_start_offset();
	uint32_t container_row_index = 0;

	for (; segment_row_index < segment.get_segment_end_offset(); segment_row_index++, container_row_index++)
	{
		switch (_type)
		{
			case x_char:
			{
				std::unique_ptr<hs2client::ByteColumn> col = current_results.GetCol<hs2client::ByteColumn>(column_index);
				FETCH_AND_PUT_VALUE(char, col, column_index, ind, row_count);
			}
			break;

			case x_stdstring:
			{
				std::unique_ptr<hs2client::StringColumn> col = current_results.GetCol<hs2client::StringColumn>(column_index);
				FETCH_AND_PUT_VALUE(std::string, col, column_index, ind, row_count);
			}
			break;

			case x_short:
			{
				std::unique_ptr<hs2client::Int16Column> col = current_results.GetCol<hs2client::Int16Column>(column_index);
				FETCH_AND_PUT_VALUE(int16_t, col, column_index, ind, row_count);
			}
			break;

			case x_integer:
			{
				std::unique_ptr<hs2client::Int32Column> col = current_results.GetCol<hs2client::Int32Column>(column_index);
				FETCH_AND_PUT_VALUE(int32_t, col, column_index, ind, row_count);
			}
			break;

			case x_long_long:
			{
				std::unique_ptr<hs2client::Int64Column> col = current_results.GetCol<hs2client::Int64Column>(column_index);
				FETCH_AND_PUT_VALUE(int64_t, col, column_index, ind, row_count);
			}
			break;

			case x_unsigned_long_long:
			{
				// TODO : Not entire sure whether there should be a warning here.
				std::unique_ptr<hs2client::Int64Column> col = current_results.GetCol<hs2client::Int64Column>(column_index);
				FETCH_AND_PUT_VALUE(int64_t, col, column_index, ind, row_count);
			}
			break;

			case x_double:
			{
				std::unique_ptr<hs2client::DoubleColumn> col = current_results.GetCol<hs2client::DoubleColumn>(column_index);
				FETCH_AND_PUT_VALUE(double, col, column_index, ind, row_count);
			}
			break;

			case x_stdtm:
			{
				/* We expect the date & time columns to be of type long(int64),
				and we expect the time in UTC, GMT, 0 in both directions */
				std::unique_ptr<hs2client::Int64Column> column_value = current_results.GetCol<hs2client::Int64Column>(column_index);
				std::vector<std::tm>* data_vector = (static_cast<std::vector<std::tm>*>(_data));

				if (container_row_index == 0)
				{ 
					data_vector->clear(); 
					data_vector->resize(row_count);
				} 
				
				if (column_value->length() > container_row_index && column_value->IsNull(segment_row_index) == false)
				{
					time_t time = column_value->GetData(segment_row_index);
					(*data_vector)[container_row_index] = *localtime(&time);

					if (ind != nullptr) ind[container_row_index] = i_ok;
				} 
				else if (ind != nullptr) ind[container_row_index] = i_null;
			}
			break;
			break;

			default:
				throw soci_error("Into element used with non-supported type.");
		}
	}
}

namespace utilities
{
	template <typename T>
	void resize_vector(void *p, std::size_t sz)
	{
		std::vector<T> *v = static_cast<std::vector<T> *>(p);
		v->resize(sz);
	}

	template <typename T>
	std::size_t get_vector_size(void *p)
	{
		std::vector<T> *v = static_cast<std::vector<T> *>(p);
		return v->size();
	}
}

void hs2client_vector_into_type_backend::resize(std::size_t sz)
{
	switch (_type)
	{
		case x_char:				utilities::resize_vector<char>(_data, sz); break;
		case x_short:				utilities::resize_vector<short>(_data, sz); break;
		case x_integer:				utilities::resize_vector<int>(_data, sz); break;
		case x_long_long:			utilities::resize_vector<long long>(_data, sz); break;
		case x_unsigned_long_long:	utilities::resize_vector<unsigned long long>(_data, sz);break;
		case x_double:				utilities::resize_vector<double>(_data, sz); break;
		case x_stdstring:			utilities::resize_vector<std::string>(_data, sz); break;
		case x_stdtm:				utilities::resize_vector<std::tm>(_data, sz); break;

		default: throw soci_error("Into vector element used with non-supported type.");
	}
}

std::size_t hs2client_vector_into_type_backend::size()
{
	switch (_type)
	{
		case x_char:				return utilities::get_vector_size<char>(_data); break;
		case x_short:				return utilities::get_vector_size<short>(_data); break;
		case x_integer:				return utilities::get_vector_size<int>(_data); break;
		case x_long_long:			return utilities::get_vector_size<long long>(_data); break;
		case x_unsigned_long_long:	return utilities::get_vector_size<unsigned long long>(_data); break;
		case x_double:				return utilities::get_vector_size<double>(_data); break;
		case x_stdstring:			return utilities::get_vector_size<std::string>(_data); break;
		case x_stdtm:				return utilities::get_vector_size<std::tm>(_data); break;
		default: throw soci_error("Into vector element used with non-supported type.");
	}
}

void hs2client_vector_into_type_backend::clean_up()
{
    // ...
}