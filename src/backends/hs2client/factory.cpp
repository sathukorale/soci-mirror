//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_HS2CLIENT_SOURCE
#include "soci/hs2client/soci-hs2client.h"
#include "soci/backend-loader.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;

// concrete factory for Empty concrete strategies
hs2client_session_backend* hs2client_backend_factory::make_session(
     connection_parameters const& parameters) const
{
     return new hs2client_session_backend(parameters);
}

hs2client_backend_factory const soci::hs2client;

extern "C"
{

// for dynamic backend loading
SOCI_HS2CLIENT_DECL backend_factory const* factory_hs2client()
{
    return &soci::hs2client;
}

SOCI_HS2CLIENT_DECL void register_factory_hs2client()
{
    soci::dynamic_backends::register_backend("hs2client", soci::hs2client);
}

} // extern "C"
