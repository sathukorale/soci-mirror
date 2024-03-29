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


hs2client_blob_backend::hs2client_blob_backend(hs2client_session_backend &session)
    : session_(session)
{
    // ...
}

hs2client_blob_backend::~hs2client_blob_backend()
{
    // ...
}

std::size_t hs2client_blob_backend::get_len()
{
    // ...
    return 0;
}

std::size_t hs2client_blob_backend::read(
    std::size_t /* offset */, char * /* buf */, std::size_t /* toRead */)
{
    // ...
    return 0;
}

std::size_t hs2client_blob_backend::write(
    std::size_t /* offset */, char const * /* buf */,
    std::size_t /* toWrite */)
{
    // ...
    return 0;
}

std::size_t hs2client_blob_backend::append(
    char const * /* buf */, std::size_t /* toWrite */)
{
    // ...
    return 0;
}

void hs2client_blob_backend::trim(std::size_t /* newLen */)
{
    // ...
}
