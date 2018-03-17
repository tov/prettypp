#include "pretty.h"
#include <catch.hpp>

using namespace pretty;

template class ::pretty::basic_document<void, char>;

TEST_CASE("trivial")
{
    CHECK( true );
}

TEST_CASE("construct nil")
{
    document nil_doc;
}

TEST_CASE("construct hello world")
{
    document d = document::text("hello ").append(document::text("world"));
}
