#include "pretty.h"
#include <catch.hpp>
#include <sstream>

using namespace pretty;

template class ::pretty::annotated_document<void>;

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

std::string render_string(const document& doc, int width)
{
    std::ostringstream out;
    no_annotation_renderer<> renderer(out);
    doc.render(renderer, width);
    return out.str();
}

TEST_CASE("hello world wrapping")
{
    document d = document::text("hello")
            .append(document::line())
            .append(document::view("world"))
            .group();

    std::ostringstream out;

    CHECK( render_string(d, 12) == "hello world" );
    CHECK( render_string(d, 11) == "hello world" );
    CHECK( render_string(d, 10) == "hello\nworld" );
    CHECK( render_string(d, 6)  == "hello\nworld" );
}
