set(HS2CLIENT_LIB_NAMES hs2client)

if (DEFINED ENV{HS2CLIENT_ROOT})
	set(HS2CLIENT_ROOT_DIR "$ENV{HS2CLIENT_ROOT}")
endif()

if (DEFINED HS2CLIENT_ROOT_DIR)
	if (NOT EXISTS "${HS2CLIENT_ROOT_DIR}")
		message(FATAL_ERROR "Please make sure that the directory '${HS2CLIENT_ROOT_DIR}' exists.")
	endif()

	set(HS2CLIENT_INC_DIR "${HS2CLIENT_ROOT_DIR}/include")
	set(HS2CLIENT_LIB_DIR "${HS2CLIENT_ROOT_DIR}/lib")	
elseif (DEFINED ENV{HS2CLIENT_INC_DIR} AND DEFINED ENV{HS2CLIENT_LIB_DIR})
	set(HS2CLIENT_INC_DIR "$ENV{HS2CLIENT_INC_DIR}")
	set(HS2CLIENT_LIB_DIR "$ENV{HS2CLIENT_LIB_DIR}")
else()
	message("Unable to find 'hs2client'. Please set the 'HS2CLIENT_ROOT_DIR' variable (environment or cmake cache) or, the 'HS2CLIENT_INC_DIR' and 'HS2CLIENT_LIB_DIR' variables if you wish to add 'soci-hs2client' support.")
	return()
endif()

if (NOT EXISTS "${HS2CLIENT_INC_DIR}" OR "${HS2CLIENT_LIB_DIR}")
	message(FATAL_ERROR "Please make sure that the directories, '${HS2CLIENT_INC_DIR}' and '${HS2CLIENT_LIB_DIR}' exist and they are part of a proper 'hs2client' installation.")
endif()

if (NOT EXISTS "${HS2CLIENT_INC_DIR}/hs2client/api.h" OR NOT EXISTS "${HS2CLIENT_LIB_DIR}/libhs2client.so")
	message(FATAL_ERROR "Please make sure that the directories, '${HS2CLIENT_INC_DIR}' and '${HS2CLIENT_LIB_DIR}' contain a valid 'hs2client' installation.")
endif()

find_path(HS2CLIENT_INCLUDE_DIR hs2client/api.h "${HS2CLIENT_INC_DIR}")
find_library(HS2CLIENT_LIBRARIES NAMES ${HS2CLIENT_LIB_NAMES} PATHS "${HS2CLIENT_LIB_DIR}")

message("HS2Client Include Dir = '${HS2CLIENT_INCLUDE_DIR}'")

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Hs2client DEFAULT_MSG HS2CLIENT_LIBRARIES HS2CLIENT_INCLUDE_DIR)

mark_as_advanced(HS2CLIENT_INCLUDE_DIR HS2CLIENT_LIBRARIES)