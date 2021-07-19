//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SOCI_HS2CLIENT_H_INCLUDED
#define SOCI_HS2CLIENT_H_INCLUDED

#ifdef _WIN32
# ifdef SOCI_DLL
#  ifdef SOCI_HS2CLIENT_SOURCE
#   define SOCI_HS2CLIENT_DECL __declspec(dllexport)
#  else
#   define SOCI_HS2CLIENT_DECL __declspec(dllimport)
#  endif // SOCI_HS2CLIENT_SOURCE
# endif // SOCI_DLL
#endif // _WIN32
//
// If SOCI_HS2CLIENT_DECL isn't defined yet define it now
#ifndef SOCI_HS2CLIENT_DECL
# define SOCI_HS2CLIENT_DECL
#endif

#include <hs2client/api.h>
#include <soci/soci-backend.h>

#include <cstddef>
#include <string>
#include <boost/optional.hpp>
#include <atomic>

#define ENABLE_STRING_ESCAPING

namespace soci
{
	class utils
	{
		public:
			static double time_in_nanoseconds(const timespec& time)
			{
				return (time.tv_sec * 1000000000.0) + time.tv_nsec;
			}

			static double get_current_time()
			{
				timespec time;
				clock_gettime(CLOCK_REALTIME, &time);

				return time_in_nanoseconds(time);
			}

			static void print_time(const std::string& message, double time_in_ns)
			{
				std::cout << message << "'" << (time_in_ns / 1000000.0) << " ms')" << std::endl;
			}

			static void escape_string_to_buffer(const std::string& s, char* &buffer)
			{
				/*buffer = new char[s.size() + 3]{ '\0' };
				buffer[0] = buffer[s.size() + 1] = '\'';

				strncpy(buffer + 1, s.c_str(), s.size());*/

				std::vector<size_t> offsets;

				size_t offset = -1;
				while ((offset = s.find("'", offset + 1)) != std::string::npos)
					if (offset == 0 || s[offset - 1] != '\\') // If it isn't already escaped, but the check is flawed
						offsets.push_back(offset);

				size_t escape_character_count = offsets.size();

				if (offsets.empty() || *offsets.rbegin() != s.length() - 1)
					offsets.push_back(s.length());

				if (s.at(s.length() - 1) != '\\')
				{
					buffer = new char[s.length() + escape_character_count + 3]{ '\0' };
					buffer[0] = buffer[s.length() + escape_character_count + 1] = '\'';
				}
				else
				{
					buffer = new char[s.length() + escape_character_count + 3 + 1]{ '\0' };
					buffer[0] = buffer[s.length() + escape_character_count + 2] = '\'';
					buffer[s.length() + escape_character_count + 1] = '\\';
				}

				size_t previous_offset = 0;
				size_t current_offset = 0;

				for (size_t i = 0; i < offsets.size(); i++)
				{
					current_offset = offsets.at(i);

					size_t i_offset = (i == 0 ? 0 : 1);
					char* dest = buffer + 1 /* because the beginning ' */ + previous_offset + i + i_offset /* the number of \ we added */;
					const char* source = s.c_str() + previous_offset + i_offset;
					size_t length = current_offset - previous_offset - i_offset;

					memcpy(dest, source, length);

					if (current_offset != s.length())
					{
						*(dest + length) = '\\';
						*(dest + length + 1) = '\'';
					}

					previous_offset = current_offset;
				}
			}
	};

struct hs2client_statement_backend;

struct SOCI_HS2CLIENT_DECL hs2client_standard_into_type_backend : details::standard_into_type_backend
{
	public:
		hs2client_standard_into_type_backend(hs2client_statement_backend &st) : 
		_statement(st),
		_data(nullptr),
		_type(),
		_position(0),
		_name(""),
		_buf(nullptr)
		{
		}

		void define_by_pos(int& position, void* data, details::exchange_type type) SOCI_OVERRIDE;

		void pre_fetch() SOCI_OVERRIDE;
		void post_fetch(bool got_data, bool called_from_fetch, indicator* ind) SOCI_OVERRIDE;

		void clean_up() SOCI_OVERRIDE;

	protected:
		hs2client_statement_backend& _statement;

		void* _data;
		details::exchange_type _type;
		int _position;
		std::string _name;
		char* _buf;
};

struct SOCI_HS2CLIENT_DECL hs2client_vector_into_type_backend : details::vector_into_type_backend
{
	public:
		hs2client_vector_into_type_backend(hs2client_statement_backend &st) : 
		_statement(st),
		_data(nullptr),
		_type(),
		_position(0),
		_name(""),
		_buf(nullptr)
		{
		}

		void define_by_pos(int& position, void* data, details::exchange_type type) SOCI_OVERRIDE;

		void pre_fetch() SOCI_OVERRIDE;
		void post_fetch(bool got_data, indicator* ind) SOCI_OVERRIDE;

		void resize(std::size_t sz) SOCI_OVERRIDE;
		std::size_t size() SOCI_OVERRIDE;

		void clean_up() SOCI_OVERRIDE;

	protected:
		hs2client_statement_backend& _statement;

		void* _data;
		details::exchange_type _type;
		int _position;
		std::string _name;
		char* _buf;
};

struct SOCI_HS2CLIENT_DECL hs2client_standard_use_type_backend : details::standard_use_type_backend
{
	public:
		hs2client_standard_use_type_backend(hs2client_statement_backend &st) : 
		_statement(st),
		_data(nullptr),
		_type(),
		_position(0),
		_name(""),
		_buf(nullptr)
		{
		}

		void bind_by_pos(int& position, void* data, details::exchange_type type, bool read_only) SOCI_OVERRIDE;
		void bind_by_name(std::string const& name, void* data, details::exchange_type type, bool read_only) SOCI_OVERRIDE;

		void pre_use(indicator const* ind) SOCI_OVERRIDE;
		void post_use(bool got_data, indicator* ind) SOCI_OVERRIDE;

		void clean_up() SOCI_OVERRIDE;

	protected:
		hs2client_statement_backend& _statement;

		void* _data;
		details::exchange_type _type;
		int _position;
		std::string _name;
		char* _buf;
};

struct SOCI_HS2CLIENT_DECL hs2client_vector_use_type_backend : details::vector_use_type_backend
{
	public:
		hs2client_vector_use_type_backend(hs2client_statement_backend &st) : 
		_statement(st) ,
		_data(nullptr),
		_type(),
		_position(0),
		_name("")
		{
		}

		void bind_by_pos(int& position, void* data, details::exchange_type type) SOCI_OVERRIDE;
		void bind_by_name(std::string const& name, void* data, details::exchange_type type) SOCI_OVERRIDE;

		void pre_use(indicator const* ind) SOCI_OVERRIDE;

		std::size_t size() SOCI_OVERRIDE;

		void clean_up() SOCI_OVERRIDE;

	protected:
		hs2client_statement_backend& _statement;

		void* _data;
		details::exchange_type _type;
		int _position;
		std::string _name;
};

struct hs2client_session_backend;
struct SOCI_HS2CLIENT_DECL hs2client_statement_backend : details::statement_backend
{
	friend class hs2client_standard_into_type_backend;
	friend class hs2client_vector_into_type_backend;

	private:
		static std::atomic_ulong _variable_index;

		enum class element_count
		{
			none,
			one,
			many
		};

		class sql_statement
		{
			public:
				sql_statement(const std::string& sql_statement);
				~sql_statement();

				void prepare();

				void clear_parameter_values(int position)
				{
					std::map<int, std::vector<char*>>::iterator itr = _parameter_values_by_index.find(position);
					if (itr == _parameter_values_by_index.end()) return;

					for (char* data : itr->second)
						if (data != nullptr) 
							delete[] data;

					itr->second.clear();
				}

				void clear_parameter_values(const std::string& name)
				{
					std::map<std::string, std::vector<char*>>::iterator itr = _parameter_values_by_name.find(name);
					if (itr == _parameter_values_by_name.end()) return;

					for (char* data : itr->second)
						if (data != nullptr) 
							delete[] data;

					itr->second.clear();
				}

				void clear_all_parameter_values()
				{
					for (auto itr : _parameter_values_by_index)
						for (char* data : itr.second)
							if (data != nullptr)
								delete[] data;

					for (auto itr : _parameter_values_by_name)
						for (char* data : itr.second)
							if (data != nullptr)
								delete[] data;

					_parameter_values_by_index.clear();
					_parameter_values_by_name.clear();
				}

				void add_parameter_value(int index, char* value);
				void add_parameter_value(const std::string& parameter_name, char* value);

				void get_statement(std::string& out_statement, int index = -1);
				const std::string& get_parameter_name(int index) const;
				inline bool is_parameter_available(int index) const { return (static_cast<size_t>(index) < _names.size()); }

			private:
				std::string _query = "";
				std::vector<std::string> _query_chunks = {};
				std::vector<std::string> _names = {};

				std::map<int, std::vector<char*>> _parameter_values_by_index = {};
				std::map<std::string, std::vector<char*>> _parameter_values_by_name = {};

				void fill_parameter_values_into_list(std::vector<char*>& values, int index = 0);
		};

		class segmented_result
		{
			private:
				uint32_t _segment_start_offset;
				uint32_t _segment_end_offset;
				uint32_t _segment_size;

				uint32_t _fetched_record_count;
				uint32_t _bulk_read_size;
				bool _has_more_data;

				bool _is_initial_attempt;
				uint64_t _total_fetched_record_count;
				std::unique_ptr<hs2client::ColumnarRowSet> _current_result;

			public:
				segmented_result(uint32_t bulk_read_size = 10000) :
				_segment_start_offset(0),
				_segment_end_offset(0),
				_segment_size(0),
				_fetched_record_count(0),
				_bulk_read_size(bulk_read_size),
				_has_more_data(true),
				_is_initial_attempt(true),
				_total_fetched_record_count(0),
				_current_result(nullptr)
				{
				}

				inline uint32_t get_segment_start_offset() const { return _segment_start_offset; }

				inline uint32_t get_segment_end_offset() const { return _segment_end_offset; }

				inline uint32_t get_segment_size() const { return _segment_size; }

				inline bool is_initial_attempt() const { return _is_initial_attempt; }

				inline bool is_buffer_empty() const { return _segment_end_offset >= _fetched_record_count; }

				inline bool is_more_data_pending() const { return _has_more_data; }

				inline uint32_t get_total_fetched_record_count() const { return _total_fetched_record_count; }

				inline const hs2client::ColumnarRowSet& get_results() const { return *_current_result; }

				inline bool fetch_next(hs2client::Operation& operation, int container_size)
				{
					bool data_pending = is_more_data_pending();
					cleanup();

					_segment_size = container_size;
					if (_segment_size > _bulk_read_size) _bulk_read_size = _segment_size;

					if (data_pending == false) return false;

					operation.Fetch(_bulk_read_size, hs2client::FetchOrientation::NEXT, &_current_result, &_has_more_data);

					/* AFAIK Fetching is a blocking operation and most likely doesn't require this. But... */
					hs2client::Operation::State status = wait_for_operation_to_complete(operation);
					if (status != hs2client::Operation::State::FINISHED)
					{
						// TODO : We probably have to handle this properly.
						throw soci_error("Failed to fetch results.");
					}
					
					_fetched_record_count = hs2client_statement_backend::get_fetched_row_count(operation, *_current_result);
					_total_fetched_record_count += _fetched_record_count;

					return (_fetched_record_count > 0);
				}

				inline bool move_segment(uint32_t container_size)
				{
					if (_segment_end_offset > 0) _segment_start_offset = _segment_end_offset;
					_segment_end_offset += container_size;

					if (_segment_start_offset > _fetched_record_count)
					{
						_segment_size = 0;
						_segment_start_offset = _segment_end_offset = _fetched_record_count;
						return false;
					}
					else if (_segment_end_offset > _fetched_record_count)
					{
						_segment_end_offset = _fetched_record_count;
					}

					_segment_size = _segment_end_offset - _segment_start_offset;
					return true;
				}

				inline void reset()
				{
					cleanup();

					_total_fetched_record_count = 0;
					_is_initial_attempt = 0;
				}

			private:
				inline void cleanup()
				{
					_segment_start_offset = 0;
					_segment_end_offset = 0;
					_segment_size = 0;

					_fetched_record_count = 0;
					_has_more_data = true;
					_current_result.reset(nullptr);
				}
		};

		static void generate_variable_name(std::string& variable_name)
		{
			std::stringstream str_variable_name;
			str_variable_name << "variable_" << _variable_index.fetch_add(1, std::memory_order_relaxed);

			variable_name = str_variable_name.str();
		}

	public:
		hs2client_statement_backend(hs2client_session_backend &session);
		~hs2client_statement_backend();

		void alloc() SOCI_OVERRIDE;
		void clean_up() SOCI_OVERRIDE;
		void prepare(std::string const& query, details::statement_type e_type) SOCI_OVERRIDE;

		exec_fetch_result execute(int number) SOCI_OVERRIDE;
		exec_fetch_result fetch(int number) SOCI_OVERRIDE;

		long long get_affected_rows() SOCI_OVERRIDE;
		int get_number_of_rows() SOCI_OVERRIDE;
		std::string get_parameter_name(int index) const SOCI_OVERRIDE;

		std::string rewrite_for_procedure_call(std::string const& query) SOCI_OVERRIDE;

		int prepare_for_describe() SOCI_OVERRIDE;
		void describe_column(int column_index, data_type& dtype, std::string& column_name) SOCI_OVERRIDE;

		hs2client_standard_into_type_backend* make_into_type_backend() SOCI_OVERRIDE;
		hs2client_standard_use_type_backend* make_use_type_backend() SOCI_OVERRIDE;
		hs2client_vector_into_type_backend* make_vector_into_type_backend() SOCI_OVERRIDE;
		hs2client_vector_use_type_backend* make_vector_use_type_backend() SOCI_OVERRIDE;

		sql_statement& get_current_statement() { return *_current_statement; }

	protected:
		hs2client_session_backend& _session;
		std::unique_ptr<sql_statement> _current_statement = nullptr;
		std::unique_ptr<hs2client::Operation> _current_operation = nullptr;

		uint32_t _bulk_read_size = 10000;
		bool _just_described = false;
		int _affected_row_count = -1;
		bool _has_more_data = false;
		int _last_fetch_count = 0;
		element_count _use_element_count = element_count::none;
		element_count _into_element_count = element_count::none;

		segmented_result _current_result;

		const segmented_result& get_current_results() { return _current_result; }
		void execute_statements(int number);

		static bool get_affected_row_count_from_profile(const std::string& profile, int& modified_count, int& error_count);
		static hs2client::Operation::State wait_for_operation_to_complete(hs2client::Operation& operation);
		static uint32_t get_fetched_row_count(const hs2client::Operation& operation, const hs2client::ColumnarRowSet& results);
};

struct hs2client_rowid_backend : details::rowid_backend
{
    hs2client_rowid_backend(hs2client_session_backend &session);

    ~hs2client_rowid_backend() SOCI_OVERRIDE;
};

struct hs2client_blob_backend : details::blob_backend
{
    hs2client_blob_backend(hs2client_session_backend& session);

    ~hs2client_blob_backend() SOCI_OVERRIDE;

    std::size_t get_len() SOCI_OVERRIDE;
    std::size_t read(std::size_t offset, char* buf, std::size_t to_read) SOCI_OVERRIDE;
    std::size_t write(std::size_t offset, char const* buf, std::size_t to_write) SOCI_OVERRIDE;
    std::size_t append(char const* buf, std::size_t to_write) SOCI_OVERRIDE;
    void trim(std::size_t new_len) SOCI_OVERRIDE;

    hs2client_session_backend& session_;
};

struct hs2client_session_backend : details::session_backend
{
	friend class hs2client_statement_backend;

	public:
		hs2client_session_backend(connection_parameters const& parameters);
		~hs2client_session_backend() SOCI_OVERRIDE;

		void begin() SOCI_OVERRIDE;
		void commit() SOCI_OVERRIDE;
		void rollback() SOCI_OVERRIDE;

		std::string get_dummy_from_table() const SOCI_OVERRIDE { return std::string(); }

		std::string get_backend_name() const SOCI_OVERRIDE { return "hs2client"; }

		void clean_up();

		hs2client_statement_backend* make_statement_backend() SOCI_OVERRIDE;
		hs2client_rowid_backend* make_rowid_backend() SOCI_OVERRIDE;
		hs2client_blob_backend* make_blob_backend() SOCI_OVERRIDE;

		bool is_connected() const;

	protected:
		std::unique_ptr<hs2client::Service> _service;
		std::unique_ptr<hs2client::Session> _session;
		uint32_t _bulk_read_size;

		uint32_t get_bulk_read_size() const { return _bulk_read_size; }

		hs2client::Status execute_statement(const std::string& statement, std::unique_ptr<hs2client::Operation>* operation) const;
};

struct SOCI_HS2CLIENT_DECL hs2client_backend_factory : backend_factory
{
    hs2client_backend_factory() {}
    hs2client_session_backend* make_session(connection_parameters const& parameters) const SOCI_OVERRIDE;
};

extern SOCI_HS2CLIENT_DECL hs2client_backend_factory const hs2client;

extern "C"
{

// for dynamic backend loading
SOCI_HS2CLIENT_DECL backend_factory const* factory_hs2client();
SOCI_HS2CLIENT_DECL void register_factory_hs2client();

} // extern "C"

} // namespace soci

#endif // SOCI_HS2CLIENT_H_INCLUDED
