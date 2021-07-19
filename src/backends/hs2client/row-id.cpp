//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#define SOCI_HS2CLIENT_SOURCE
#include "soci/hs2client/soci-hs2client.h"

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;


hs2client_rowid_backend::hs2client_rowid_backend(hs2client_session_backend & /* session */)
{
    // ...
}

hs2client_rowid_backend::~hs2client_rowid_backend()
{
    // ...
}
