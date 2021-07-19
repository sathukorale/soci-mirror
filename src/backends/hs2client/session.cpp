//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.// boost.org/LICENSE_1_0.txt)
//

#define SOCI_HS2CLIENT_SOURCE

#include <hs2client/api.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

#include "soci/hs2client/soci-hs2client.h"
#include "soci/connection-parameters.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;

bool cast_to_int(const std::string& value, int& int_value)
{
	try
	{
		int_value = boost::lexical_cast<int>(value);
		return true;
	}
	catch (const boost::bad_lexical_cast&)
	{
		return false;
	}
}

/* Breaks a string with ',' or ';' separated parameters/values and puts each key value pair into the provided map */
void recognize_parameters(const std::string& connection_string, std::map<std::string, std::string>& parameters)
{
	parameters.clear();

	std::vector<std::string> key_value_items;
	std::vector<std::string> key_value_items_inner;

	boost::split(key_value_items, connection_string, boost::is_any_of(",;"));

	for (std::string& key_value_string : key_value_items)
	{
		if (key_value_string.empty()) continue; // Ignoring since this is empty

		key_value_items_inner.clear();
		boost::split(key_value_items_inner, key_value_string, boost::is_any_of("="));

		if (key_value_items_inner.size() != 2)
		{
			std::stringstream str_error;
			str_error << "Malformed connection string (segment='" << key_value_string << "', connecting-string='" << connection_string << "')";

			throw soci_error(str_error.str());
		}

		boost::algorithm::to_lower(key_value_items_inner[0]);

		boost::algorithm::trim(key_value_items_inner[0]);
		boost::algorithm::trim(key_value_items_inner[1]);

		parameters[key_value_items_inner[0]] = key_value_items_inner[1];
	}
}

bool contains(const std::map<std::string, std::string>& parameters, const std::string& item_name, std::string& output_value)
{
	auto itr_parameters = parameters.find(item_name);
	if (itr_parameters != parameters.end())
	{
		output_value = itr_parameters->second;
		return true;
	}
	return false;
}

/* The parameters 'connectionString', 'host' and 'portNumber', 'user' are all required, while the 'connectionTimeout' and the 'protocolVersion'
are not. Thus if either of these 'optional' parameters are not mentioned under the connection string, they will default to the
values mentioned on the first two line of this method. */
void parse_connection_string(const std::string& connection_string, std::string& host, int& port_number, std::string& user, int& connection_timeout, int& protocol_version, int& bulk_read_size, std::shared_ptr<hs2client::HS2ClientConfig>& security_configs, boost::optional<std::string>* database = nullptr)
{
	connection_timeout = 0;
	protocol_version = static_cast<int>(hs2client::ProtocolVersion::HS2CLIENT_PROTOCOL_V7);
	bulk_read_size = 10000;
	if (database != nullptr) *database = boost::none;

	std::map<std::string, std::string> parameters;
	recognize_parameters(connection_string, parameters);

	static const char* param_host = "host";
	static const char* param_port = "port";
	static const char* param_user = "user";
	static const char* param_connection_timeout = "connection-timeout";
	static const char* param_protocol_version = "protocol-version";
	static const char* param_database = "database";
	static const char* param_bulk_read_size = "rowsfetchedperblock";

	// security related settings (SSL)
	static const char* param_enable_ssl = "ssl";
	static const char* param_allow_hostname_cn_mismatch = "allowhostnamecnmismatch";
	static const char* param_is_self_signed_cert = "allowselfsignedservercert";
	static const char* param_certification_location = "trustedcerts";
	static const char* param_tls_version = "min_tls";

	// security related settings (SASL)
	static const char* param_auth_mech = "authmech";
	static const char* param_krb_realm = "krbrealm";
	static const char* param_krb_enable_service_principal_canonicalization = "serviceprincipalcanonicalization";
	static const char* param_krb_fqdn = "krbfqdn";
	static const char* param_krb_service_name = "krbservicename";
	static const char* param_common_uid = "uid"; // this can be the principal or the username (for plain authentication)
	static const char* param_common_pwd = "pwd";
	static const char* param_krb_keytab_file_path = "defaultkeytabfile";
	static const char* param_krb_use_keytab = "usekeytab";

	(void)param_krb_enable_service_principal_canonicalization;

	std::string tmp_value = "";

	if (contains(parameters, param_host, host) == false) throw soci_error("The required parameter 'host' missing.");
	if (contains(parameters, param_user, user) == false) user = "user";
	if (contains(parameters, param_database, tmp_value) && database != nullptr) *database = tmp_value;
	if (contains(parameters, param_port, tmp_value) == false) throw soci_error("The required parameter 'port' missing");

	if (cast_to_int(tmp_value, port_number) == false) throw soci_error("Invalid connection string (Details='port number should must be a positive number')");
	if (port_number < 0 || port_number > 65535) throw soci_error("Invalid connection string (Details='invalid port number')");

	if (contains(parameters, param_connection_timeout, tmp_value))
		if (cast_to_int(tmp_value, connection_timeout) == false)
			throw soci_error("Invalid connection string (Details='the connection timeout should be a positive number')");

	if (contains(parameters, param_protocol_version, tmp_value))
	{
		if (cast_to_int(tmp_value, protocol_version) == false) throw soci_error("Invalid connection string (Details='the protocol version should be a positive number.')");

		if (protocol_version != static_cast<int>(hs2client::ProtocolVersion::HS2CLIENT_PROTOCOL_V6) &&
			protocol_version != static_cast<int>(hs2client::ProtocolVersion::HS2CLIENT_PROTOCOL_V7)) throw soci_error("Invalid connection string (Details='Unsupported protocol version'");
	}

	if (contains(parameters, param_bulk_read_size, tmp_value))
		if (cast_to_int(tmp_value, bulk_read_size) == false)
			throw soci_error("Invalid connection string (Details='Bulk read size (RowsFetchedPerBlock) should be a positive number')");

	if (contains(parameters, param_enable_ssl, tmp_value) && tmp_value == "1")
	{
		security_configs.reset(new hs2client::HS2ClientConfig());

		security_configs->SetOption(hs2client::Service::Params::paramSslEnable, "1");

		if (contains(parameters, param_tls_version, tmp_value)) security_configs->SetOption(hs2client::Service::Params::paramTlsVersion, tmp_value);
		if (contains(parameters, param_is_self_signed_cert, tmp_value)) security_configs->SetOption(hs2client::Service::Params::paramSslSelfSigned, tmp_value);
		if (contains(parameters, param_certification_location, tmp_value)) security_configs->SetOption(hs2client::Service::Params::paramCertificateLocation, tmp_value);
		if (contains(parameters, param_allow_hostname_cn_mismatch, tmp_value)) security_configs->SetOption(hs2client::Service::Params::paramDisablePeerValidation, tmp_value);
	}

	int auth_mech = 0;
	if (contains(parameters, param_auth_mech, tmp_value) && cast_to_int(tmp_value, auth_mech))
	{
		/* Accepted values for AuthMech are 1 for Kerberos and 2 for Simple authentication */
		if (auth_mech < 0 || auth_mech > 2)
			throw soci_error("Invalid connection string (Details='AuthMech parameter should be either 1 [Kerberos] or 2 [Simple]')");

		if (auth_mech != 0 && (security_configs.get() == nullptr)) /* Which is for authentication disabled */
			security_configs.reset(new hs2client::HS2ClientConfig());

		if (auth_mech == 1)
		{
			security_configs->SetOption(hs2client::Service::Params::paramAuthMechanism, "kerberos");
			if (contains(parameters, param_krb_realm, tmp_value)) security_configs->SetOption(hs2client::Service::Params::paramKerberosRealm, tmp_value);
			if (contains(parameters, param_krb_fqdn, tmp_value)) security_configs->SetOption(hs2client::Service::Params::paramKerberosFqdn, tmp_value);
			if (contains(parameters, param_krb_service_name, tmp_value)) security_configs->SetOption(hs2client::Service::Params::paramKerberosServiceName, tmp_value);
			if (contains(parameters, param_common_uid, tmp_value)) security_configs->SetOption(hs2client::Service::Params::paramCommonUsername, tmp_value);

			if (contains(parameters, param_krb_use_keytab, tmp_value))
			{
				int use_keytab = -1;
				if ((cast_to_int(tmp_value, use_keytab) == false) || (use_keytab != 0 && use_keytab != 1))
					throw soci_error("Invalid connection string (Details='The UseKeyTab parameter only expects 1 [True] or 0 [False]')");

				if (use_keytab == 1)
				{
					if (contains(parameters, param_krb_keytab_file_path, tmp_value)) security_configs->SetOption(hs2client::Service::Params::paramKerberosKeytabPath, tmp_value);
				}
				else
				{
					if (contains(parameters, param_common_pwd, tmp_value)) security_configs->SetOption(hs2client::Service::Params::paramCommonPassword, tmp_value);
				}
			}
			else /* This is up-to interpretation lets say. We'll read both values and let the hs2client library decide which is given priority */
			{
				if (contains(parameters, param_krb_keytab_file_path, tmp_value)) security_configs->SetOption(hs2client::Service::Params::paramKerberosKeytabPath, tmp_value);
				if (contains(parameters, param_common_pwd, tmp_value)) security_configs->SetOption(hs2client::Service::Params::paramCommonPassword, tmp_value);
			}
		}
		else if (auth_mech == 2)
		{
			security_configs->SetOption(hs2client::Service::Params::paramAuthMechanism, "simple");
			if (contains(parameters, param_common_uid, tmp_value)) security_configs->SetOption(hs2client::Service::Params::paramCommonUsername, tmp_value);
			if (contains(parameters, param_common_pwd, tmp_value)) security_configs->SetOption(hs2client::Service::Params::paramCommonPassword, tmp_value);
		}
	}
}

/* A typical hs2client connection string is expected to look like this. The separator can either be ',' or the ';'
host=<host-name or ip-address>, port=<port-number>, connection-timeout=<connection-timeout>, user=<user>, protocol-version=<protocol-version>

However the parameters, 'connection-timeout' and 'protocol-version' are optional and will default to,
- connection-timeout = 5
- protocol-version   = 6 (which corresponds to ProtocolVersion::HS2CLIENT_PROTOCOL_V7. You can find the rest at 'hs2client\service.h')
*/
hs2client_session_backend::hs2client_session_backend(const connection_parameters& params) :
_service(nullptr),
_session(nullptr),
_bulk_read_size(10000)
{
	std::string host;
	int port;
	std::string user;
	int connection_timeout = 0;
	int protocol_version = static_cast<int>(hs2client::ProtocolVersion::HS2CLIENT_PROTOCOL_V7);
	int bulk_read_size = 10000;
	boost::optional<std::string> database;
	std::shared_ptr<hs2client::HS2ClientConfig> security_configs;

	parse_connection_string(params.get_connect_string(), host, port, user, connection_timeout, protocol_version, bulk_read_size, security_configs, &database);

	hs2client::HS2ClientConfig config;
	if (database) config.SetOption("use:database", database.get());

	_bulk_read_size = bulk_read_size;

	hs2client::Status status = hs2client::Service::Connect(host, port, connection_timeout, static_cast<hs2client::ProtocolVersion>(protocol_version), security_configs, &_service);
	if (status.ok() == false)
	{
		std::stringstream str_error;
		str_error << "Failed to connect to the Hive Server 2 service identified by the provided connection string. (ConnectionString='" << params.get_connect_string() << "', Details='" << status.GetMessage() << "')";

		throw soci_error(str_error.str());
	}

	if ((status = _service->OpenSession(user, config, &_session)).ok() == false)
	{
		std::stringstream str_error;
		str_error << "Failed to open a session with the connected Hive Server 2 service identified by the provided connection string. (ConnectionString='" << params.get_connect_string() << "', Details='" + status.GetMessage() + "')";

		throw soci_error(str_error.str());
	}
}

hs2client_session_backend::~hs2client_session_backend()
{
    clean_up();
}

hs2client::Status hs2client_session_backend::execute_statement(const std::string& statement, std::unique_ptr<hs2client::Operation>* operation) const
{
	return _session->ExecuteStatement(statement, operation);
}

void hs2client_session_backend::begin()
{
	throw soci_error("Unsupported operation. (Details='HS2/Impala does not support BEGIN operations. Further reading, https://docs.cloudera.com/runtime/7.0.3/impala-reference/topics/impala-transactions.html')");
}

void hs2client_session_backend::commit()
{
	throw soci_error("Unsupported operation. (Details='HS2/Impala does not support COMMIT operations. Further reading, https://docs.cloudera.com/runtime/7.0.3/impala-reference/topics/impala-transactions.html')");
}

void hs2client_session_backend::rollback()
{
	throw soci_error("Unsupported operation. (Details='HS2/Impala does not support ROLLBACK operations. Further reading, https://docs.cloudera.com/runtime/7.0.3/impala-reference/topics/impala-transactions.html')");
}

void hs2client_session_backend::clean_up()
{
	_session.release();
	_service.release();
}

hs2client_statement_backend * hs2client_session_backend::make_statement_backend()
{
    return new hs2client_statement_backend(*this);
}

hs2client_rowid_backend * hs2client_session_backend::make_rowid_backend()
{
    return new hs2client_rowid_backend(*this);
}

hs2client_blob_backend * hs2client_session_backend::make_blob_backend()
{
    return new hs2client_blob_backend(*this);
}

bool hs2client_session_backend::is_connected() const
{
	if (_service.get() == nullptr) return false;
	return _service->IsConnected();
}