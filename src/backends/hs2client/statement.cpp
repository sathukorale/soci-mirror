//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_HS2CLIENT_SOURCE

#include <time.h>
#include <unistd.h>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include "soci/hs2client/soci-hs2client.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

/* The way we are fetching the modified count does not feel right. 
   So define or remove 'ENABLE_MODIFIED_COUNT_FETCHING' to use or 
   disable that functionality */
// #define ENABLE_MODIFIED_COUNT_FETCHING

using namespace soci;
using namespace soci::details;

std::atomic_ulong hs2client_statement_backend::_variable_index(0);

hs2client_statement_backend::hs2client_statement_backend::sql_statement::sql_statement(const std::string& sql_statement) :
_query(sql_statement),
_query_chunks(),
_names(),
_parameter_values_by_index(),
_parameter_values_by_name()
{
}

hs2client_statement_backend::sql_statement::~sql_statement()
{
	for (std::pair<int, std::vector<char*>> itr : _parameter_values_by_index)
	{
		for (char* data : itr.second)
			if (data != nullptr)
				delete[] data;

		itr.second.clear();
	}

	for (std::pair<std::string, std::vector<char*>> itr : _parameter_values_by_name)
	{
		for (char* data : itr.second)
			if (data != nullptr)
				delete[] data;

		itr.second.clear();
	}

	_parameter_values_by_index.clear();
	_parameter_values_by_name.clear();
}

/* This is a copy-paste of whats on 'soci\src\backends\mysql\statement.cpp',
although this has been modified to ignore ':=' situation. */
void hs2client_statement_backend::sql_statement::prepare()
{
	_query_chunks.clear();
	enum { eNormal, eInQuotes, eInName } state = eNormal;

	std::string name;
	bool escaped = false;

	_query_chunks.push_back("");

	for (std::string::const_iterator it = _query.begin(), end = _query.end(); it != end; ++it)
	{
		switch (state)
		{
		case eNormal:
			if (*it == '\'')
			{
				_query_chunks.back() += *it;
				state = eInQuotes;
			}
			else if (*it == ':') /* The start of a SOCI SQL parameter */
			{
				state = eInName;
			}
			else if (*it == '?') /* Supporting ODBC style '?' position by index parameters */
			{
				std::string variable_name = "";
				generate_variable_name(variable_name);

				_names.push_back(variable_name);
				_query_chunks.push_back("");
			}
			else // regular character, stay in the same state
			{
				_query_chunks.back() += *it;
			}
			break;

		case eInQuotes:
			if (*it == '\'' && !escaped)
			{
				_query_chunks.back() += *it;
				state = eNormal;
			}
			else // regular quoted character
			{
				_query_chunks.back() += *it;
			}
			escaped = *it == '\\' && !escaped;
			break;

		case eInName:
			if (std::isalnum(*it) || *it == '_')
			{
				name += *it;
			}
			else // end of name
			{
				_names.push_back(name);
				name.clear();

				_query_chunks.push_back("");
				_query_chunks.back() += *it;

				state = eNormal;
			}
			break;
		}
	}

	if (state == eInName)
	{
		_names.push_back(name);
	}
}

/* This is a copy-paste of whats on 'soci\src\backends\mysql\statement.cpp' */
void hs2client_statement_backend::sql_statement::get_statement(std::string& out_statement, int index)
{
	if (index == -1) 
	{
		out_statement = _query;
		return; /* returns the original query as is. */
	}

	std::vector<char*> values;
	fill_parameter_values_into_list(values, index);

	if (_query_chunks.size() != values.size() &&
		_query_chunks.size() != values.size() + 1)
	{
		throw soci_error("Wrong number of parameters.");
	}

	out_statement = "";
	std::vector<std::string>::const_iterator ci = _query_chunks.begin();
	std::vector<char*>::const_iterator end = values.end();

	for (std::vector<char*>::const_iterator pi = values.begin(); pi != end; ++ci, ++pi)
	{
		out_statement += *ci;
		out_statement += *pi;
	}

	if (ci != _query_chunks.end())
	{
		out_statement += *ci;
	}
}

const std::string& hs2client_statement_backend::sql_statement::get_parameter_name(int index) const
{
	return _names[index];
}

/* These two methods will be called by 'standard-use-type', 'standard-into-type',
  'vector-use-type' and 'vector-into-type' depending on the query.
*/
void hs2client_statement_backend::sql_statement::add_parameter_value(int index, char* value)
{
	_parameter_values_by_index[index].push_back(value);
}

void hs2client_statement_backend::sql_statement::add_parameter_value(const std::string& parameter_name, char* value)
{
	_parameter_values_by_name[parameter_name].push_back(value);
}

void hs2client_statement_backend::sql_statement::fill_parameter_values_into_list(std::vector<char*>& values, int index)
{
	values.clear();

	bool usesParameterByName = _parameter_values_by_name.empty() == false;
	bool usesParameterByIndex = _parameter_values_by_index.empty() == false;

	if (usesParameterByName && usesParameterByIndex)
		throw new std::runtime_error("A statement cannot have both, values by name and position. It should be one or the other.");

	if (usesParameterByName)
	{
		for (std::string& name : _names)
		{
			values.push_back(_parameter_values_by_name[name][index]);
		}
	}
	else if (usesParameterByIndex)
	{
		for (std::pair<int, std::vector<char*>> itr : _parameter_values_by_index)
		{
			values.push_back(itr.second[index]);
		}
	}
}

hs2client_statement_backend::hs2client_statement_backend(hs2client_session_backend &session) : 
_session(session),
_current_statement(nullptr),
_current_operation(nullptr),
_bulk_read_size(session.get_bulk_read_size()),
_just_described(false),
_affected_row_count(-1),
_current_result(_bulk_read_size)
{
}

soci::hs2client_statement_backend::~hs2client_statement_backend()
{
	clean_up();

	if (_current_statement.get() != nullptr)
	{
		_current_statement.release();
	}
}

void hs2client_statement_backend::alloc()
{
    // TODO : We might not have to do anything here. But better to check again.
}

void hs2client_statement_backend::clean_up()
{
	if (_current_operation.get() != nullptr)
	{
		_current_operation->Close();
		_current_operation.release();
	}

	if (_current_statement != nullptr) _current_statement->clear_all_parameter_values();
	_current_result.reset();
}

void hs2client_statement_backend::prepare(const std::string& query, statement_type /* eType */)
{
	_current_statement = std::unique_ptr<sql_statement>(new sql_statement(query));
	_current_statement->prepare();
}

void hs2client_statement_backend::execute_statements(int number)
{
	if (_session.is_connected() == false) throw soci_error("Unable to execute query via a closed connection.");

	std::vector<std::string*> statements_to_execute;
	if (_use_element_count == element_count::none)
	{
		std::string* statement = new std::string();
		_current_statement->get_statement(*statement);

		statements_to_execute.push_back(statement);
	}
	else /* Just of readability's sake */
	{
		for (int i = 0; i < number; i++)
		{
			std::string* statement = new std::string();
			_current_statement->get_statement(*statement, i);

			statements_to_execute.push_back(statement);
		}
	}

	for (std::string* statement : statements_to_execute)
	{
		if (_current_operation.get() != nullptr)
			_current_operation->Close();

		_session.execute_statement(*statement, &_current_operation);

		hs2client::Operation::State status = wait_for_operation_to_complete(*_current_operation);
		if (status != hs2client::Operation::State::FINISHED)
		{
			std::stringstream str_error;
			str_error << "Failed to execute query/statement '" << *statement << "'";

			delete statement;
			throw soci_error(str_error.str());
		}

		#ifdef ENABLE_MODIFIED_COUNT_FETCHING
			std::string profile_output_string;
			_current_operation->GetProfile(&profile_output_string);

			int modified_count, error_count;
			get_affected_row_count_from_profile(profile_output_string, modified_count, error_count);

			_affected_row_count = modified_count;
			if (error_count > 0)
			{
				std::stringstream str_error;
				str_error << "Some errors were thrown during the execution of query/statement '" << *statement << "'. Please consult the log for more details.";

				delete statement;
				throw soci_error(str_error.str());
			}
		#else
			_affected_row_count = -1;
		#endif

		delete statement;
	}
}

/* The parameter 'number' is somewhat misleading. The following is the much better description
found at http://soci.sourceforge.net/doc/master/api/backend/.

Called to execute the query; if number is zero, the intent is not to exchange data with the
user-provided objects (into and use elements); positive values mean the number of rows to
exchange (more than 1 is used only for bulk operations).

Initially we are going to process bulk inserts as separate individual queries. So,
TODO : Fix the bulk entry situation
*/
statement_backend::exec_fetch_result hs2client_statement_backend::execute(int number)
{
	if (_use_element_count == element_count::many && _into_element_count != element_count::none)
		throw new std::runtime_error("Unsupported operations. As of yet, the use of bulk into's and use's at the same time are not supported.");
	
	_has_more_data = false;
	int numberOfExecutions = (number <= 0) ? 1 : ((_use_element_count == element_count::many) ? number : 1);

	_current_result.reset();

	if (_just_described)
	{
		/* Apparently the query already ran */
		_just_described = false;
	}
	else
	{
		execute_statements(numberOfExecutions);
	}

	std::vector<hs2client::ColumnDesc> columns;
	_current_operation->GetResultSetMetadata(&columns);

	bool is_a_fetch_query = (columns.empty() == false); // Whether the query fetches data

	/* bulk executions shouldn't be returning anything. */
	if (is_a_fetch_query)
	{
		_has_more_data = true;

		if (_use_element_count == element_count::many) 
			throw soci_error("The query shouldn't have returned any data but it did. (Reason='This is a bulk execution, thus this operation is not supported')");
	}

    return is_a_fetch_query ? (number > 0 ? fetch(number) : ef_success) : ef_no_data;
}

statement_backend::exec_fetch_result hs2client_statement_backend::fetch(int number)
{
	_last_fetch_count = 0;

 	if (_current_operation.get() == nullptr) throw soci_error("Unable to fetch data as no current operation exists.");
	if (_session.is_connected() == false) throw soci_error("Unable to fetch results via an disconnected connection.");

	bool is_data_available = (_current_result.is_buffer_empty() == false) || (_current_result.is_more_data_pending());
	if ((is_data_available == false) && _current_result.is_initial_attempt() == false) return ef_no_data;

	if (_current_result.is_buffer_empty())
	{
		bool found_data = _current_result.fetch_next(*_current_operation, number);
		if (found_data == false) return ef_no_data;
	}

	_current_result.move_segment(number);
	_last_fetch_count = _current_result.get_segment_size();

	return ef_success;
}

long long hs2client_statement_backend::get_affected_rows()
{
	return _affected_row_count;
}

int hs2client_statement_backend::get_number_of_rows()
{
    return _last_fetch_count; /* This method cannot be supported with impala, especially due to the way impala was designed. */
}

std::string hs2client_statement_backend::get_parameter_name(int index) const
{
	if (_current_statement->is_parameter_available(index))
		return _current_statement->get_parameter_name(index);
	return "";
}

std::string hs2client_statement_backend::rewrite_for_procedure_call(std::string const &query)
{
    return query;
}

int hs2client_statement_backend::prepare_for_describe()
{
	execute_statements(1);
	_just_described = true;

	std::vector<hs2client::ColumnDesc> column_descriptions;
	_current_operation->GetResultSetMetadata(&column_descriptions);

	return column_descriptions.size();
}

/* According to the details at, https://impala.apache.org/docs/build/html/topics/impala_datatypes.html */
soci::data_type map_data_type(hs2client::ColumnType::TypeId data_type)
{
	switch (data_type)
	{
		case hs2client::ColumnType::TypeId::BOOLEAN:
		case hs2client::ColumnType::TypeId::TINYINT:
		case hs2client::ColumnType::TypeId::SMALLINT:
		case hs2client::ColumnType::TypeId::INT:
			return soci::data_type::dt_integer;

		case hs2client::ColumnType::TypeId::BIGINT:
			return soci::data_type::dt_long_long;

		case hs2client::ColumnType::TypeId::DECIMAL:
		case hs2client::ColumnType::TypeId::FLOAT:
		case hs2client::ColumnType::TypeId::DOUBLE:
			return soci::data_type::dt_double;

		case hs2client::ColumnType::TypeId::CHAR:
		case hs2client::ColumnType::TypeId::VARCHAR:
		case hs2client::ColumnType::TypeId::STRING:
			return soci::data_type::dt_string;

		/* Apparently impala does not support the type DATE, but apparently HIVE does. 
		   https://stackoverflow.com/questions/48265651/can-hive-tables-that-contain-date-type-columns-be-queried-using-impala */
		case hs2client::ColumnType::TypeId::DATE:
		case hs2client::ColumnType::TypeId::TIMESTAMP:
			return soci::data_type::dt_date;

		case hs2client::ColumnType::TypeId::BINARY:
			return soci::data_type::dt_blob;

		/* TODO : These types are not supported at the moment. Because I am not entirely sure how these values fit into 
		          whats provided by SOCI. Should check if more types can be given compatibility with SOCI 
		*/
		case hs2client::ColumnType::TypeId::ARRAY:
		case hs2client::ColumnType::TypeId::MAP:
		case hs2client::ColumnType::TypeId::STRUCT:
		case hs2client::ColumnType::TypeId::UNION:
		case hs2client::ColumnType::TypeId::USER_DEFINED:
		case hs2client::ColumnType::TypeId::NULL_TYPE:
		default:
			throw soci_error("Unsupported types"); 
	}
}

void hs2client_statement_backend::describe_column(int column_index, data_type& data_type, std::string& column_name)
{
	if (_current_operation.get() == nullptr)
	{
		// TODO : Have to check whether this is even a valid situation. Perhaps change the generic 
		//	      throw here to something proper.
		throw soci_error("Cannot describe column without any active operations");
	}

	std::vector<hs2client::ColumnDesc> column_descriptions;
	_current_operation->GetResultSetMetadata(&column_descriptions);

	// Because apparently these column indexes start from 1
	column_index -= 1;

	if (static_cast<size_t>(column_index) >= column_descriptions.size())
	{
		std::stringstream str_error;
		str_error << "Invalid column index. (ColumnIndex='" << column_index << "', ColumnCount='" << column_descriptions.size() << "')";

		// TODO : To check the implications/complications of throwing this error
		throw soci_error(str_error.str());
	}

	hs2client::ColumnDesc& column = column_descriptions[column_index];

	column_name = column.column_name();
	data_type = map_data_type(column.type()->type_id());
}

/* 'make_into_type_backend' is about statements where the statement fetches a single entry of data and puts 
	them onto variable provided by the 'into' section/method. 

	An example would be (grabbed from, http://soci.sourceforge.net/doc/release/3.2/statements.html):
		int i;
		statement st = (sql.prepare << "select value from numbers order by value", into(i));
*/
hs2client_standard_into_type_backend * hs2client_statement_backend::make_into_type_backend()
{
	_into_element_count = element_count::one;
    return new hs2client_standard_into_type_backend(*this);
}

/* 'make_use_type_backend' is about statements where the statement sends the value provided by the 'use' 
	section/method, so that it can be used to replace a parameter in the SQL statement

	An example would be (grabbed from, http://soci.sourceforge.net/doc/release/3.2/statements.html):
	int i;
	statement st = (sql.prepare << "insert into numbers(value) values(:val)", use(i));
*/
hs2client_standard_use_type_backend * hs2client_statement_backend::make_use_type_backend()
{
	_use_element_count = element_count::one;
    return new hs2client_standard_use_type_backend(*this);
}

/* Same situation as with the 'make_into_type_backend', however with multiple fetched entries.

   Please check the http://soci.sourceforge.net/doc/release/3.2/statements.html#bulk section 
   for details explanations and examples.
*/
hs2client_vector_into_type_backend * hs2client_statement_backend::make_vector_into_type_backend()
{
	_into_element_count = element_count::many;
    return new hs2client_vector_into_type_backend(*this);
}

/* Same situation as with the 'make_use_type_backend', however with bulk inserts.

   Please check the http://soci.sourceforge.net/doc/release/3.2/statements.html#bulk section
   for details explanations and examples. 
*/
hs2client_vector_use_type_backend * hs2client_statement_backend::make_vector_use_type_backend()
{
	_use_element_count = element_count::many;
    return new hs2client_vector_use_type_backend(*this);
}

hs2client::Operation::State hs2client_statement_backend::wait_for_operation_to_complete(hs2client::Operation& operation)
{
	hs2client::Operation::State current_state = hs2client::Operation::State::UNKNOWN;
	while (current_state == hs2client::Operation::State::PENDING ||
		   current_state == hs2client::Operation::State::RUNNING ||
		   current_state == hs2client::Operation::State::UNKNOWN)
	{
		operation.GetState(&current_state);
		usleep(50);
	}

	return current_state;
}

/* To fetch the affected count we have to do some shady stuff. Not entirely sure how I feel about it.
Looks like this is the same method 'impyla' uses. Please check the https://github.com/cloudera/impyla/issues/209
for more details. */
bool hs2client_statement_backend::get_affected_row_count_from_profile(const std::string& profile, int& modified_count, int &error_count)
{
	boost::char_separator<char> sep("\n");

	static std::string key_num_modified_rows = "NumModifiedRows";
	static std::string key_num_row_errors = "NumRowErrors";

	modified_count = -1;
	error_count = -1;

	bool modified_count_found = false;
	bool error_count_found = false;

	BOOST_FOREACH(std::string line, boost::tokenizer<boost::char_separator<char>>(profile, sep))
	{
		std::vector<std::string> key_value_item;
		boost::split(key_value_item, line, boost::is_any_of("="));

		if (key_value_item.size() != 2) continue;

		boost::algorithm::trim(key_value_item[0]);
		boost::algorithm::trim(key_value_item[1]);

		if (key_value_item[0] == key_num_modified_rows)
		{
			try
			{
				modified_count = std::stoi(key_value_item[1]);
				modified_count_found = true;
			}
			catch (std::exception& ex) { modified_count = -1; }
		}
		else if (key_value_item[0] == key_num_row_errors)
		{
			try
			{
				error_count = std::stoi(key_value_item[1]);
				error_count_found = true;
			}
			catch (std::exception& ex) { error_count = -1; }
		}

		if (modified_count_found && error_count_found) break;
	}

	return modified_count_found || error_count_found;
}

uint32_t hs2client_statement_backend::get_fetched_row_count(const hs2client::Operation& operation, const hs2client::ColumnarRowSet& results)
{
	std::vector<hs2client::ColumnDesc> columns;
	operation.GetResultSetMetadata(&columns);

	if (columns.empty()) return 0;

	switch (columns[0].type()->type_id())
	{
		case hs2client::ColumnType::TypeId::BOOLEAN: return results.GetCol<hs2client::BoolColumn>(0)->length();
		case hs2client::ColumnType::TypeId::TINYINT: return results.GetCol<hs2client::ByteColumn>(0)->length();
		case hs2client::ColumnType::TypeId::SMALLINT: return results.GetCol<hs2client::Int16Column>(0)->length();
		case hs2client::ColumnType::TypeId::INT: return results.GetCol<hs2client::Int32Column>(0)->length();
		case hs2client::ColumnType::TypeId::BIGINT: return results.GetCol<hs2client::Int64Column>(0)->length();
		case hs2client::ColumnType::TypeId::DECIMAL: return results.GetCol<hs2client::DoubleColumn>(0)->length();
		case hs2client::ColumnType::TypeId::FLOAT: return results.GetCol<hs2client::DoubleColumn>(0)->length();
		case hs2client::ColumnType::TypeId::DOUBLE: return results.GetCol<hs2client::DoubleColumn>(0)->length();
		case hs2client::ColumnType::TypeId::CHAR: return results.GetCol<hs2client::StringColumn>(0)->length();
		case hs2client::ColumnType::TypeId::VARCHAR: return results.GetCol<hs2client::StringColumn>(0)->length();
		case hs2client::ColumnType::TypeId::STRING: return results.GetCol<hs2client::StringColumn>(0)->length();

		default: 
		{
			std::stringstream strError;
			strError << "Unsupported column type (ColumnType='" << columns[0].type()->ToString() << "')";

			throw new std::runtime_error(strError.str());
		}
	}
}