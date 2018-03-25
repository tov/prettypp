#include "pretty.h"
#include <catch.hpp>
#include <memory>
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

    CHECK( render_string(d, 12) == "hello world" );
    CHECK( render_string(d, 11) == "hello world" );
    CHECK( render_string(d, 10) == "hello\nworld" );
    CHECK( render_string(d, 6)  == "hello\nworld" );
}

using Tree = std::shared_ptr<struct Tree_node>;

struct Tree_node
{
    std::string data;
    Tree left, right;
};

Tree tree_cons(const std::string& data,
               Tree left = nullptr,
               Tree right = nullptr)
{
    return std::make_shared<Tree_node>(Tree_node{data, left, right});
}

document tree2doc(const Tree& tree)
{
    if (tree) {
        return document::view(tree->data)
                .append(document::view("["))
                .append(tree2doc(tree->left)
                                .append(document::view(","))
                                .append(document::line())
                                .append(tree2doc(tree->right))
                                .group()
                                .align())
                .append(document::view("]"));
    } else {
        return document::view("[]");
    }
}

TEST_CASE("tree render")
{
    Tree tree = tree_cons("a");
    document doc = tree2doc(tree);

    CHECK( render_string(doc, 30) == "a[[], []]" );
    CHECK( render_string(doc, 6) == "a[[],\n"
                                    "  []]" );

    tree = tree_cons("this",
                     tree_cons("is"),
                     tree_cons("a", tree_cons("binary"), tree_cons("tree")));
    doc = tree2doc(tree);

    CHECK( render_string(doc, 60) == "this[is[[], []], a[binary[[], []], tree[[], []]]]");
    CHECK( render_string(doc, 30) == "this[is[[], []],\n"
                                     "     a[binary[[], []],\n"
                                     "       tree[[], []]]]");
}
