###############################################################################
#
# This file is part of CMake configuration for SOCI library
#
# Copyright (C) 2010 Mateusz Loskot
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#
###############################################################################
set (CMAKE_CXX_STANDARD 11)

soci_backend(hs2client
			 DEPENDS Hs2client
			 DESCRIPTION "SOCI backend skeleton for new backends development"
			 AUTHORS "Maciej Sobczak, Stephen Hutton"
			 MAINTAINERS "Maciej Sobczak")
