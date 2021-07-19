# Hs2client Backend Reference

SOCI backend for accessing Hive Server 2 services (specifically the impala HS2 Thrift service). This backend heavily depends on the existing FOSS project at [cloudera/hs2client](https://github.com/cloudera/hs2client).

## Prerequisites

### Supported Versions
The versions supported by the depending HS2Clien libraries. Please consult Apache or Cloudera on these matters.

### Tested Platforms

> Please note that the HS2Client library does not compiler under Windows

### Required Client Libraries

The SOCI HS2Client backend depends on the HS2Client libary at [cloudera/hs2client](https://github.com/cloudera/hs2client). However as of yet, no official releases are available for direct use. Thus building that library immediately falls on to you, the user. 

Once built and installed (to the location of your choosing), you are required to set the **HS2CLIENT_ROOT** variable (environment or CMake), or if the libraries and headers are in their own separate directories you can target them individualy using the **HS2CLIENT_LIB_DIR** and **HS2CLIENT_INC_DIR** variables (again environment or cmake).

For example:
```
export HS2CLIENT_ROOT=/home/sample_user/hs2client-installation/usr/local
```
OR
```
export HS2CLIENT_INCLUDE_DIR=/home/sample_user/hs2client-installation/usr/local/include
export HS2CLIENT_LIB_DIR=/home/sample_user/hs2client-installation/usr/local/lib64
```

## Connecting to the Database

You can create a _soci::session_ in the following ways

```
session sql(hs2client, "host=127.0.0.1, port=21050, connection-timeout=10, user=user, protocol-version=6");
```

Or

```
session sql("hs2client", "host=127.0.0.1, port=21050, connection-timeout=10, user=user, protocol-version=6");
```

Or

```
session sql("hs2client://host=127.0.0.1, port=21050, connection-timeout=10, user=user, protocol-version=6");
```

The following parameters are supported under the connection string

|Paramter Name|Required|DefaultValue|Description|
|-------------|--------|------------|-----------|
|host|Yes||The host name or ip address of the Hive Server 2 service|
|port|Yes|The port used by the Hive Server 2 service|
|user|Yes|YTD|
|connection-timeout|No|5|Self explanatory|
|protocol-version|No|6|The only version supported by the HS2Client library is 7.|
|database|No||The database to automatically switch after/at connection.|

## SOCI Feature Support

### Dynamic Binding

The api fully supports both the '[binding by position](../binding.md#binding-by-position)' and 
'[binding by name](../binding.md#binding-by-name)' capabilities.

However since Impala nor HS2Client supports this feature, this is merely an emulation of that feature happening on the soci side.

### Bulk Operations

### Transactions

Unfortunately since Impala does not support transactions, and since each operation is considered a self-commit operation this feature is not supported.

### BLOB Data Type

Unfortunately as of right now this type is not supported. 

### RowID Data Type

The `rowid` functionality is not supported by the Impala backend.

## Backend-specific extensions

None.

## Configuration options

None.
