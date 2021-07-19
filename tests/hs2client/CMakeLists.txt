###############################################################################
#
# This file is part of CMake configuration for SOCI library
#
# Copyright (C) 2010-2013 Mateusz Loskot <mateusz@loskot.net>
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
###############################################################################
set (CMAKE_CXX_STANDARD 11)

soci_backend_test(
  BACKEND Hs2client
  DEPENDS Hs2client
  SOURCE test-hs2client.cpp ${SOCI_TESTS_COMMON}
  CONNSTR "host=172.25.31.43, port=47150, connection-timeout=10, user=user, protocol-version=6")
