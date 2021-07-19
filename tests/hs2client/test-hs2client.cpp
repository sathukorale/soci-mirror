//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "soci/soci.h"
#include "soci/hs2client/soci-hs2client.h"

// Normally the tests would include common-tests.h here, but we can't run any
// of the tests registered there, so instead include CATCH header directly.
#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <numeric>
#include <thread>
#include <sstream>

using namespace soci;

std::string connectString;
backend_factory const &backEnd = *soci::factory_hs2client();

// NOTE:
// This file is supposed to serve two purposes:
// 1. To be a starting point for implementing new tests (for new backends).
// 2. To exercise (at least some of) the syntax and try the SOCI library
//    against different compilers, even in those environments where there
//    is no database. SOCI uses advanced template techniques which are known
//    to cause problems on different versions of popular compilers, and this
//    test is handy to verify that the code is accepted by as many compilers
//    as possible.
//
// Both of these purposes mean that the actual code here is meaningless
// from the database-development point of view. For new tests, you may wish
// to remove this code and keep only the general structure of this file.

struct Person
{
    int id;
    std::string firstName;
    std::string lastName;
};

namespace soci
{
    template<> struct type_conversion<Person>
    {
        typedef values base_type;
        static void from_base(values & /* r */, indicator /* ind */,
            Person & /* p */)
        {
        }
    };

	namespace testing
	{
		namespace hs2client
		{
			class Utilities
			{
				public:
					static void CreateDatabase(soci::session& sql)
					{
						sql << "drop database if exists hs2client_test_db cascade";
						sql << "create database hs2client_test_db";
					}

					static void CreateTable(soci::session& sql)
					{
						sql << "use hs2client_test_db";
						sql << "create table hs2client_test_table (int_col int, string_col string)";
					}

					static void CreateTableWithAllColumnTypes(soci::session& sql)
					{
						sql << "use hs2client_test_db";
						sql << "create table hs2client_all_types_table (tinyint_col tinyint, smallint_col smallint, int_col int, bigint_col bigint, double_col double, string_col string, varchar_col varchar, timestamp_col bigint, char_col tinyint)";
					}

					static void InsertSampleData(soci::session& sql, std::vector<int> int_data, std::vector<std::string> string_data)
					{
						sql << "insert into hs2client_test_table VALUES (:int_values, :string_values)", use(int_data, "int_values"), use(string_data, "string_values");
					}

					static std::unique_ptr<soci::session> OpenSession(const std::string& connectionString)
					{
						return std::unique_ptr<soci::session>(new soci::session(backEnd, connectionString));
					}
			};
		}
	}
}

TEST_CASE("ServiceTests-TestConnect", "Successfully connecting to the HS2 service and executing some queries.")
{
	std::string validConnectionString = connectString; // We have to assume that the provided string is valid.
	std::unique_ptr<soci::session> sql;
	REQUIRE_NOTHROW(sql = soci::testing::hs2client::Utilities::OpenSession(validConnectionString));

	std::vector<std::string> databases(5);

	*sql << "drop database if exists hs2client_test_db cascade";
	*sql << "create database hs2client_test_db";
	*sql << "show databases", into(databases);

	REQUIRE(databases.size() > 0);
	REQUIRE(std::find(databases.begin(), databases.end(), "hs2client_test_db") != databases.end());
}

TEST_CASE("ServiceTests-TestFailedConnect", "Failing to connect to the invalid HS2 service mentioned by the invalid connection string specified.")
{
	std::string invalidConnectionString = "host=localhost, port=60000, connection-timeout=5000, user=user, protocol-version=6";

	std::unique_ptr<soci::session> sql;
	REQUIRE_THROWS(sql = soci::testing::hs2client::Utilities::OpenSession(invalidConnectionString));
}

TEST_CASE("SessionTests-TestSessionConfig", "Checking whether session parameters are processed properly")
{
	std::unique_ptr<soci::session> sql;
	CHECK_NOTHROW(sql = soci::testing::hs2client::Utilities::OpenSession(connectString));

	soci::testing::hs2client::Utilities::CreateDatabase(*sql);
	soci::testing::hs2client::Utilities::CreateTable(*sql);

	CHECK(connectString.find("database") == std::string::npos);

	std::string connectStringWithDbSpecified = connectString + ", database=hs2client_test_db";
	std::string selectStatement = "select * from hs2client_test_table";

	soci::session sql1(backEnd, connectStringWithDbSpecified);
	REQUIRE_NOTHROW(sql1 << selectStatement); // This should work

	soci::session sql2(backEnd, connectString);
	REQUIRE_THROWS(sql2 << selectStatement); // However this should not
}

TEST_CASE("OperationTests-TestFetch", "Inserting dummy data into a table, and fetching them")
{
	std::unique_ptr<soci::session> sql;
	CHECK_NOTHROW(sql = soci::testing::hs2client::Utilities::OpenSession(connectString));

	soci::testing::hs2client::Utilities::CreateDatabase(*sql);
	soci::testing::hs2client::Utilities::CreateTable(*sql);

	soci::testing::hs2client::Utilities::InsertSampleData(*sql, std::vector<int>({ 1, 2, 3, 4 }), std::vector<std::string>({ "a", "b", "c", "d" }));

	std::vector<int> int_data(2);
	std::vector<std::string> string_data(2);

	statement stm = (sql->prepare << "select int_col, string_col from hs2client_test_table order by int_col", into(int_data), into(string_data));
	REQUIRE_NOTHROW(stm.execute());

	REQUIRE(stm.fetch());
	REQUIRE(int_data == std::vector<int>({ 1, 2 }));
	REQUIRE(string_data == std::vector<std::string>({ "a", "b" }));

	REQUIRE(stm.fetch());
	REQUIRE(int_data == std::vector<int>({ 3, 4 }));
	REQUIRE(string_data == std::vector<std::string>({ "c", "d" }));

	REQUIRE_FALSE(stm.fetch());
}

TEST_CASE("OperationTests-MultipleSingularInserts-ParametersByName", "Adding multiple records however by inserting them one buy one. However the data 'use'd are addressed by their parameter names.")
{
	std::unique_ptr<soci::session> sql;
	CHECK_NOTHROW(sql = soci::testing::hs2client::Utilities::OpenSession(connectString));

	soci::testing::hs2client::Utilities::CreateDatabase(*sql);
	soci::testing::hs2client::Utilities::CreateTable(*sql);

	std::vector<int> expected_int_values;
	std::vector<std::string> expected_string_values;

	for (int i = 0; i < 10; i++)
	{
		std::string string_value = "something" + std::to_string(i);
		*sql << "insert into hs2client_test_table VALUES (:int_value, :string_value)", use(i, "int_value"), use(string_value, "string_value");

		expected_int_values.push_back(i);
		expected_string_values.push_back(string_value);
	}

	std::vector<int> fetched_int_values(10);
	std::vector<std::string> fetched_string_values(10);

	statement st = (sql->prepare << "select int_col, string_col from hs2client_test_table order by int_col", into(fetched_int_values), into(fetched_string_values));
	st.execute();
	st.fetch();

	REQUIRE(fetched_int_values == expected_int_values);
	REQUIRE(fetched_string_values == expected_string_values);
	REQUIRE_FALSE(st.fetch());
}

TEST_CASE("OperationTests-MultipleSingularInserts-ParametersByPosition", "Adding multiple records however by inserting them one buy one. However the data 'use'd are provided in the proper order.")
{
	std::unique_ptr<soci::session> sql;
	CHECK_NOTHROW(sql = soci::testing::hs2client::Utilities::OpenSession(connectString));

	soci::testing::hs2client::Utilities::CreateDatabase(*sql);
	soci::testing::hs2client::Utilities::CreateTable(*sql);

	std::vector<int> expected_int_values;
	std::vector<std::string> expected_string_values;

	for (int i = 0; i < 10; i++)
	{
		std::string string_value = "something" + std::to_string(i);
		*sql << "insert into hs2client_test_table VALUES (:int_value, :string_value)", use(i), use(string_value);

		expected_int_values.push_back(i);
		expected_string_values.push_back(string_value);
	}

	std::vector<int> fetched_int_values(10);
	std::vector<std::string> fetched_string_values(10);

	statement st = (sql->prepare << "select int_col, string_col from hs2client_test_table order by int_col", into(fetched_int_values), into(fetched_string_values));
	st.execute();
	st.fetch();

	REQUIRE(fetched_int_values == expected_int_values);
	REQUIRE(fetched_string_values == expected_string_values);
	REQUIRE(st.fetch() == false);
}

TEST_CASE("OperationTests-BulkInsert-ParametersByName", "Adding multiple records in bulk. However the data 'use'd are addressed by their parameter names.")
{
	std::unique_ptr<soci::session> sql;
	CHECK_NOTHROW(sql = soci::testing::hs2client::Utilities::OpenSession(connectString));

	soci::testing::hs2client::Utilities::CreateDatabase(*sql);
	soci::testing::hs2client::Utilities::CreateTable(*sql);

	std::vector<int> expected_int_values;
	std::vector<std::string> expected_string_values;

	for (int i = 0; i < 10; i++)
	{
		expected_int_values.push_back(i);
		expected_string_values.push_back("something" + std::to_string(i));
	}

	*sql << "insert into hs2client_test_table VALUES (:int_value, :string_value)", use(expected_int_values, "int_value"), use(expected_string_values, "string_value");

	std::vector<int> fetched_int_values(10);
	std::vector<std::string> fetched_string_values(10);

	statement st = (sql->prepare << "select int_col, string_col from hs2client_test_table order by int_col", into(fetched_int_values), into(fetched_string_values));
	st.execute();
	st.fetch();

	REQUIRE(fetched_int_values == expected_int_values);
	REQUIRE(fetched_string_values == expected_string_values);
	REQUIRE_FALSE(st.fetch());
}

TEST_CASE("OperationTests-BulkInsert-ParametersByPosition", "Adding multiple records in bulk. However the data 'use'd are provided in the proper order.")
{
	std::unique_ptr<soci::session> sql;
	CHECK_NOTHROW(sql = soci::testing::hs2client::Utilities::OpenSession(connectString));

	soci::testing::hs2client::Utilities::CreateDatabase(*sql);
	soci::testing::hs2client::Utilities::CreateTable(*sql);

	std::vector<int> expected_int_values;
	std::vector<std::string> expected_string_values;

	for (int i = 0; i < 10; i++)
	{
		expected_int_values.push_back(i);
		expected_string_values.push_back("something" + std::to_string(i));
	}

	*sql << "insert into hs2client_test_table VALUES (:int_value, :string_value)", use(expected_int_values), use(expected_string_values);

	std::vector<int> fetched_int_values(10);
	std::vector<std::string> fetched_string_values(10);

	statement st = (sql->prepare << "select int_col, string_col from hs2client_test_table order by int_col", into(fetched_int_values), into(fetched_string_values));
	st.execute();
	st.fetch();

	REQUIRE(fetched_int_values == expected_int_values);
	REQUIRE(fetched_string_values == expected_string_values);
	REQUIRE_FALSE(st.fetch());
}

TEST_CASE("OperationTests-FetchSingularEntry", "Fetching a single record")
{
	std::unique_ptr<soci::session> sql;
	CHECK_NOTHROW(sql = soci::testing::hs2client::Utilities::OpenSession(connectString));

	soci::testing::hs2client::Utilities::CreateDatabase(*sql);
	soci::testing::hs2client::Utilities::CreateTable(*sql);

	soci::testing::hs2client::Utilities::InsertSampleData(*sql, std::vector<int>({ 1, 2, 3, 4 }), std::vector<std::string>({ "a", "b", "c", "d" }));

	int int_value;
	std::string string_value;

	*sql << "select int_col, string_col from hs2client_test_table where int_col=3", into(int_value), into(string_value);

	REQUIRE(int_value == 3);
	REQUIRE(string_value == "c");
}

TEST_CASE("OperationTests-FetchMultipleEntries", "Fetching multiple records")
{
	std::unique_ptr<soci::session> sql;
	CHECK_NOTHROW(sql = soci::testing::hs2client::Utilities::OpenSession(connectString));

	soci::testing::hs2client::Utilities::CreateDatabase(*sql);
	soci::testing::hs2client::Utilities::CreateTable(*sql);

	soci::testing::hs2client::Utilities::InsertSampleData(*sql, std::vector<int>({ 1, 2, 3, 4 }), std::vector<std::string>({ "a", "b", "c", "d" }));

	std::vector<int> int_data(2);
	std::vector<std::string> string_data(2);

	*sql << "select int_col, string_col from hs2client_test_table order by int_col", into(int_data), into(string_data);

	REQUIRE(int_data.size() == 2);
	REQUIRE(int_data[0] == 1);
	REQUIRE(int_data[1] == 2);

	REQUIRE(string_data.size() == 2);
	REQUIRE(string_data[0] == "a");
	REQUIRE(string_data[1] == "b");
}

TEST_CASE("OperationTests-FetchSingularEntryButUsingUseElementToFilter", "Fetching a single record by specifying the filter value (used by the where clause) via an use element")
{
	std::unique_ptr<soci::session> sql;
	CHECK_NOTHROW(sql = soci::testing::hs2client::Utilities::OpenSession(connectString));

	soci::testing::hs2client::Utilities::CreateDatabase(*sql);
	soci::testing::hs2client::Utilities::CreateTable(*sql);

	soci::testing::hs2client::Utilities::InsertSampleData(*sql, std::vector<int>({ 1, 2, 3, 4 }), std::vector<std::string>({ "a", "b", "c", "d" }));

	int int_value;
	std::string string_value;
	int filter_value = 3;

	*sql << "select int_col, string_col from hs2client_test_table where int_col=:filter_value", into(int_value), into(string_value), use(filter_value);

	REQUIRE(int_value == 3);
	REQUIRE(string_value == "c");
}

TEST_CASE("OperationTests-DifferentDataTypeInsertionAndRetrieval", "Inserting all of the supported data types and retrieving them.")
{
	std::unique_ptr<soci::session> sql;
	CHECK_NOTHROW(sql = soci::testing::hs2client::Utilities::OpenSession(connectString));

	soci::testing::hs2client::Utilities::CreateDatabase(*sql);
	soci::testing::hs2client::Utilities::CreateTable(*sql);

	soci::testing::hs2client::Utilities::InsertSampleData(*sql, std::vector<int>({ 1, 2, 3, 4 }), std::vector<std::string>({ "a", "b", "c", "d" }));

	std::vector<int> int_data(2);
	std::vector<std::string> string_data(2);

	*sql << "select int_col, string_col from hs2client_test_table order by int_col", into(int_data), into(string_data);

	REQUIRE(int_data.size() == 2);
	REQUIRE(int_data[0] == 1);
	REQUIRE(int_data[1] == 2);

	REQUIRE(string_data.size() == 2);
	REQUIRE(string_data[0] == "a");
	REQUIRE(string_data[1] == "b");
}

TEST_CASE("OperationTests-CheckingDifferentTypes", "")
{
	/* Unfortunately we don't support FLOAT and DECIMAL at the moment,
	so these values will be excluded from the test */
	std::unique_ptr<soci::session> sql;
	CHECK_NOTHROW(sql = soci::testing::hs2client::Utilities::OpenSession(connectString));

	soci::testing::hs2client::Utilities::CreateDatabase(*sql);
	soci::testing::hs2client::Utilities::CreateTableWithAllColumnTypes(*sql);

	std::time_t t1 = std::time(0);

	std::tm time1 = *std::gmtime(&t1);
	std::tm time2 = *std::gmtime(&t1);
	std::tm time3 = *std::gmtime(&t1);
	std::tm time4 = *std::gmtime(&t1);

	time1.tm_sec += 1;
	time2.tm_sec += 2;
	time3.tm_sec += 3;
	time4.tm_sec += 4;

	std::vector<char> tinyint_data({ 1,2,3,4 });
	std::vector<int16_t> smallint_data({ 12,23,34,56 });
	std::vector<int32_t> int_data({ 123,234,345,456 });
	std::vector<int64_t> bigint_data({ 1234,2345,3456,4567 });
	std::vector<double> double_data({ 12.345123456,23.456123456,34.567123456,45.678123456 });
	std::vector<std::string> string_data({ "something-1", "something-2", "something-3", "something-4" });
	std::vector<std::string> varchar_data({ "something-5", "something-6", "something-7", "something-8" });
	std::vector<std::tm> timestamp_data({ time1, time2, time3, time4 });
	std::vector<char> char_data({ 'p', 'q', 'r', 's' });

	*sql << "insert into hs2client_all_types_table values (:tinyint_values, :smallint_values, :int_values, :bigint_values, :double_values, :string_values, :varchar_values, :timestamp_values, :char_values)",
			use(tinyint_data, "tinyint_values"),
			use(smallint_data, "smallint_values"),
			use(int_data, "int_values"),
			use(bigint_data, "bigint_values"),
			use(double_data, "double_values"),
			use(string_data, "string_values"),
			use(varchar_data, "varchar_values"),
			use(timestamp_data, "timestamp_values"),
			use(char_data, "char_values");

	std::vector<char> tinyint_data_r(4);
	std::vector<int16_t> smallint_data_r(4);
	std::vector<int32_t> int_data_r(4);
	std::vector<int64_t> bigint_data_r(4);
	std::vector<double> double_data_r(4);
	std::vector<std::string> string_data_r(4);
	std::vector<std::string> varchar_data_r(4);
	std::vector<std::tm> timestamp_data_r(4);
	std::vector<char> char_data_r(4);

	*sql <<	"select tinyint_col, smallint_col, int_col, bigint_col, double_col, string_col, varchar_col, timestamp_col, char_col from hs2client_all_types_table order by tinyint_col",
			into(tinyint_data_r),
			into(smallint_data_r),
			into(int_data_r),
			into(bigint_data_r),
			into(double_data_r),
			into(string_data_r),
			into(varchar_data_r),
			into(timestamp_data_r),
			into(char_data_r);

	REQUIRE(tinyint_data == tinyint_data_r);
	REQUIRE(smallint_data == smallint_data_r);
	REQUIRE(int_data == int_data_r);
	REQUIRE(bigint_data == bigint_data_r);
	REQUIRE(double_data == double_data_r);
	REQUIRE(string_data == string_data_r);
	REQUIRE(varchar_data == varchar_data_r);
	REQUIRE(char_data == char_data_r);

	for (size_t i = 0; i < timestamp_data_r.size(); i++)
	{
		int64_t time = mktime(&timestamp_data[i]);
		int64_t time_r = mktime(&timestamp_data_r[i]);

		REQUIRE(time == time_r);
	}
}

TEST_CASE("OperationTests-EscapingSingleQuote", "Checking where the soci-hs2client implementation handles single quotes")
{
	std::unique_ptr<soci::session> sql;
	CHECK_NOTHROW(sql = soci::testing::hs2client::Utilities::OpenSession(connectString));

	soci::testing::hs2client::Utilities::CreateDatabase(*sql);
	soci::testing::hs2client::Utilities::CreateTable(*sql);

	std::string value1 = "something'else";
	std::string value2 = "some'thing'else";
	std::string value3 = "'something else";
	std::string value4 = "something else'";
	std::string value5 = "'something else'";
	std::string value6 = "'something'else'";
	std::string value7 = "somethingelse\\";

	*sql << "insert into hs2client_test_table VALUES (1, :string_value)", use(value1);
	*sql << "insert into hs2client_test_table VALUES (2, :string_value)", use(value2);
	*sql << "insert into hs2client_test_table VALUES (3, :string_value)", use(value3);
	*sql << "insert into hs2client_test_table VALUES (4, :string_value)", use(value4);
	*sql << "insert into hs2client_test_table VALUES (5, :string_value)", use(value5);
	*sql << "insert into hs2client_test_table VALUES (6, :string_value)", use(value6);
	*sql << "insert into hs2client_test_table VALUES (7, :string_value)", use(value7);

	std::string fetched_value1 = "";
	std::string fetched_value2 = "";
	std::string fetched_value3 = "";
	std::string fetched_value4 = "";
	std::string fetched_value5 = "";
	std::string fetched_value6 = "";
	std::string fetched_value7 = "";

	*sql << "select string_col from hs2client_test_table where int_col=1", into(fetched_value1);
	*sql << "select string_col from hs2client_test_table where int_col=2", into(fetched_value2);
	*sql << "select string_col from hs2client_test_table where int_col=3", into(fetched_value3);
	*sql << "select string_col from hs2client_test_table where int_col=4", into(fetched_value4);
	*sql << "select string_col from hs2client_test_table where int_col=5", into(fetched_value5);
	*sql << "select string_col from hs2client_test_table where int_col=6", into(fetched_value6);
	*sql << "select string_col from hs2client_test_table where int_col=7", into(fetched_value7);

	REQUIRE(fetched_value1 == value1);
	REQUIRE(fetched_value2 == value2);
	REQUIRE(fetched_value3 == value3);
	REQUIRE(fetched_value4 == value4);
	REQUIRE(fetched_value5 == value5);
	REQUIRE(fetched_value6 == value6);
	REQUIRE(fetched_value7 == value7);
}

TEST_CASE("OperationTests-EscapingSingleQuoteInBulkInserting", "Checking where the soci-hs2client implementation handles single quotes")
{
	std::unique_ptr<soci::session> sql;
	CHECK_NOTHROW(sql = soci::testing::hs2client::Utilities::OpenSession(connectString));

	soci::testing::hs2client::Utilities::CreateDatabase(*sql);
	soci::testing::hs2client::Utilities::CreateTable(*sql);

	std::vector<int> int_values;
	std::vector<std::string> string_values;

	int_values.push_back(1); string_values.push_back("something'else");
	int_values.push_back(2); string_values.push_back("some'thing'else");
	int_values.push_back(3); string_values.push_back("'something else");
	int_values.push_back(4); string_values.push_back("something else'");
	int_values.push_back(5); string_values.push_back("'something else'");
	int_values.push_back(6); string_values.push_back("'something'else'");

	*sql << "insert into hs2client_test_table VALUES (:int_values, :string_values)", use(int_values), use(string_values);

	std::string fetched_value1 = "";
	std::string fetched_value2 = "";
	std::string fetched_value3 = "";
	std::string fetched_value4 = "";
	std::string fetched_value5 = "";
	std::string fetched_value6 = "";

	*sql << "select string_col from hs2client_test_table where int_col=1", into(fetched_value1);
	*sql << "select string_col from hs2client_test_table where int_col=2", into(fetched_value2);
	*sql << "select string_col from hs2client_test_table where int_col=3", into(fetched_value3);
	*sql << "select string_col from hs2client_test_table where int_col=4", into(fetched_value4);
	*sql << "select string_col from hs2client_test_table where int_col=5", into(fetched_value5);
	*sql << "select string_col from hs2client_test_table where int_col=6", into(fetched_value6);

	REQUIRE(fetched_value1 == string_values[0]);
	REQUIRE(fetched_value2 == string_values[1]);
	REQUIRE(fetched_value3 == string_values[2]);
	REQUIRE(fetched_value4 == string_values[3]);
	REQUIRE(fetched_value5 == string_values[4]);
	REQUIRE(fetched_value6 == string_values[5]);
}

TEST_CASE("OperationTests-SupportingODBCStyleArguments-MultipleSingularInserts", "Supporting the '?' from ODBC")
{
	std::unique_ptr<soci::session> sql;
	CHECK_NOTHROW(sql = soci::testing::hs2client::Utilities::OpenSession(connectString));

	soci::testing::hs2client::Utilities::CreateDatabase(*sql);
	soci::testing::hs2client::Utilities::CreateTable(*sql);

	std::vector<int> expected_int_values;
	std::vector<std::string> expected_string_values;

	for (int i = 0; i < 10; i++)
	{
		std::string string_value = "something" + std::to_string(i);
		*sql << "insert into hs2client_test_table VALUES (?, ?)", use(i), use(string_value);

		expected_int_values.push_back(i);
		expected_string_values.push_back(string_value);
	}

	std::vector<int> fetched_int_values(10);
	std::vector<std::string> fetched_string_values(10);

	statement st = (sql->prepare << "select int_col, string_col from hs2client_test_table order by int_col", into(fetched_int_values), into(fetched_string_values));
	st.execute();
	st.fetch();

	REQUIRE(fetched_int_values == expected_int_values);
	REQUIRE(fetched_string_values == expected_string_values);
	REQUIRE(st.fetch() == false);
}

TEST_CASE("OperationTests-SupportingODBCStyleArguments-BulkInserts", "Supporting the '?' from ODBC")
{
	std::unique_ptr<soci::session> sql;
	CHECK_NOTHROW(sql = soci::testing::hs2client::Utilities::OpenSession(connectString));

	soci::testing::hs2client::Utilities::CreateDatabase(*sql);
	soci::testing::hs2client::Utilities::CreateTable(*sql);

	std::vector<int> expected_int_values;
	std::vector<std::string> expected_string_values;

	for (int i = 0; i < 10; i++)
	{
		expected_int_values.push_back(i);
		expected_string_values.push_back("something" + std::to_string(i));
	}

	*sql << "insert into hs2client_test_table VALUES (?, ?)", use(expected_int_values), use(expected_string_values);

	std::vector<int> fetched_int_values(10);
	std::vector<std::string> fetched_string_values(10);

	statement st = (sql->prepare << "select int_col, string_col from hs2client_test_table order by int_col", into(fetched_int_values), into(fetched_string_values));
	st.execute();
	st.fetch();

	REQUIRE(fetched_int_values == expected_int_values);
	REQUIRE(fetched_string_values == expected_string_values);
	REQUIRE_FALSE(st.fetch());
}

TEST_CASE("OperationTests-SupportingODBCStyleArguments-Filtering", "Supporting the '?' from ODBC")
{
	std::unique_ptr<soci::session> sql;
	CHECK_NOTHROW(sql = soci::testing::hs2client::Utilities::OpenSession(connectString));

	soci::testing::hs2client::Utilities::CreateDatabase(*sql);
	soci::testing::hs2client::Utilities::CreateTable(*sql);

	soci::testing::hs2client::Utilities::InsertSampleData(*sql, std::vector<int>({ 12, 23, 34, 45 }), std::vector<std::string>({ "value_a", "value_b", "value_c", "value_d" }));
	
	int fetched_int_value = 0;
	std::string fetched_string_value = "";

	int int_filter = 34;
	*sql << "select int_col, string_col from hs2client_test_table where int_col=?", into(fetched_int_value), into(fetched_string_value), use(int_filter);

	REQUIRE(fetched_int_value == 34);
	REQUIRE(fetched_string_value == "value_c");

	std::string string_filter = "value_b";
	*sql << "select int_col, string_col from hs2client_test_table where string_col=?", into(fetched_int_value), into(fetched_string_value), use(string_filter);

	REQUIRE(fetched_int_value == 23);
	REQUIRE(fetched_string_value == "value_b");
}

TEST_CASE("OperationTests-MultipleExchangesOnTheSameStatement", "Using the same statement with different values exchanged to retrieve different data")
{
	std::unique_ptr<soci::session> sql;
	CHECK_NOTHROW(sql = soci::testing::hs2client::Utilities::OpenSession(connectString));

	soci::testing::hs2client::Utilities::CreateDatabase(*sql);
	soci::testing::hs2client::Utilities::CreateTable(*sql);

	soci::testing::hs2client::Utilities::InsertSampleData(*sql, std::vector<int>({ 1, 2, 3, 4 }), std::vector<std::string>({ "a", "b", "c", "d" }));

	std::string string_value;
	int filter_value = 3;

	statement st = (sql->prepare << "select string_col from hs2client_test_table where int_col=:filter_value");

	filter_value = 3;
	{
		st.exchange(into(string_value));
		st.exchange(use(filter_value));

		st.define_and_bind();
		st.execute(false);
		st.fetch();
		st.bind_clean_up();

		REQUIRE(string_value == "c");
	}

	filter_value = 2;
	{
		st.exchange(into(string_value));
		st.exchange(use(filter_value));

		st.define_and_bind();
		st.execute(false);
		st.fetch();
		st.bind_clean_up();

		REQUIRE(string_value == "b");
	}
}


int main(int argc, char** argv)
{

#ifdef _MSC_VER
    // Redirect errors, unrecoverable problems, and assert() failures to STDERR,
    // instead of debug message window.
    // This hack is required to run assert()-driven tests by Buildbot.
    // NOTE: Comment this 2 lines for debugging with Visual C++ debugger to catch assertions inside.
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif //_MSC_VER

    if (argc >= 2)
    {
        connectString = argv[1];

        // Replace the connect string with the process name to ensure that
        // CATCH uses the correct name in its messages.
        argv[1] = argv[0];

        argc--;
        argv++;
    }
    else
    {
        std::cout << "usage: " << argv[0]
          << " connectstring [test-arguments...]\n"
            << "example: " << argv[0]
            << " \'connect_string_for_hs2client_backend\'\n";
        std::exit(1);
    }

    return Catch::Session().run(argc, argv);
}
