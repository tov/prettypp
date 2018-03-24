#pragma once

#include <algorithm>
#include <cassert>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace pretty {

template<class Output = std::ostream>
class base_renderer
{
protected:
    Output& out_;

public:
    explicit base_renderer(Output& out) : out_(out) {}

    /// Writes the given string.
    void write(std::string_view sv);

    /// Writes a single character.
    void write(char c);

    /// Writes a newline followed by the given indentation.
    void newline(int indent);
};

/// A renderer that ignores annotations.
template<class Output = std::ostream>
class no_annotation_renderer : private base_renderer<Output>
{
public:
    using super = base_renderer<Output>;
    using super::base_renderer;
    using super::write;
    using super::newline;

    template<class Annot>
    void push_annotation(const Annot&);

    /// Leaves an annotation.
    void pop_annotation();
};

/// A renderer that expects annotations to be pairs whose elements can be
/// stream-inserted before and after the annotated text.
template<
        class AnnotText,
        class Output = std::ostream
>
class simple_annotation_renderer : base_renderer<Output>
{
private:
    using super = base_renderer<Output>;
    using super::out_;

    std::vector<AnnotText*> annot_stack_;

public:
    using super::base_renderer;
    using super::write;
    using super::newline;

    /// Enters an annotation.
    void push_annotation(const std::pair<AnnotText, AnnotText>&);

    /// Leaves an annotation.
    void pop_annotation();
};

/////
///// IMPLEMENTATION
/////

template<class Output>
void base_renderer<Output>::write(std::string_view sv)
{
    out_.write(sv.data(), sv.size());
}

template<class Output>
void base_renderer<Output>::write(char c)
{
    out_.write(&c, 1);
}


template<class Output>
void
base_renderer<Output>::newline(int indent)
{
    static const std::string SPACES(80, ' ');

    out_.write("\n", 1);

    while (indent > 0) {
        int amount = std::min(indent, int(SPACES.size()));
        out_.write(SPACES.data(), amount);
        indent -= amount;
    }
}

template<class Output>
template<class Annot>
void no_annotation_renderer<Output>::push_annotation(const Annot&)
{ }

template<class Output>
void no_annotation_renderer<Output>::pop_annotation()
{ }

template<class AnnotText, class Output>
void simple_annotation_renderer<AnnotText, Output>::push_annotation(
        const std::pair<AnnotText, AnnotText>& annot)
{
    out_ << annot.first;
    annot_stack_.push_back(&annot.second);
}

template<class AnnotText, class Output>
void simple_annotation_renderer<AnnotText, Output>::pop_annotation()
{
    assert( !annot_stack_.empty() );
    out_ << *annot_stack_.back();
    annot_stack_.pop_back();
}

}
