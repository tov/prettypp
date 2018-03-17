#pragma once

#include <algorithm>
#include <cassert>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace pretty {

/// A renderer that ignores annotations.
template<
        class CharT = char,
        class Traits = std::char_traits<CharT>,
        class Output = std::basic_ostream<CharT, Traits>
>
class no_annotation_renderer
{
protected:
    Output& out_;

public:
    explicit no_annotation_renderer(Output& out)
            : out_(out) {}

    /// Writes the given string.
    void write(std::basic_string_view<CharT, Traits> sv);

    /// Writes a single character.
    void write(CharT c);

    /// Writes a newline followed by the given indentation.
    void newline(int indent);

    /// Enters an annotation (no-op).
    template<class Annot>
    void push_annotation(const Annot&);

    /// Leaves an annotation.
    void pop_annotation();
};

/// A renderer that expects annotations to be pairs whose elements can be
/// stream-inserted before and after the annotated text.
template<
        class AnnotText,
        class CharT = char,
        class Traits = std::char_traits<CharT>,
        class Output = std::basic_ostream<CharT, Traits>
>
class simple_annotation_renderer : no_annotation_renderer<CharT, Traits, Output>
{
private:
    using super = no_annotation_renderer<CharT, Traits, Output>;
    using super::out_;

    std::vector<AnnotText*> annot_stack_;

public:
    using super::no_annotation_renderer;
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

template<class CharT, class Traits, class Output>
void no_annotation_renderer<CharT, Traits, Output>::write(
        std::basic_string_view<CharT, Traits> sv)
{
    out_.write(sv.data(), sv.size());
}

template<class CharT, class Traits, class Output>
void no_annotation_renderer<CharT, Traits, Output>::write(CharT c)
{
    out_.write(&c, 1);
}


template<class CharT, class Traits, class Output>
void
no_annotation_renderer<CharT, Traits, Output>::newline(int indent)
{
    static const std::basic_string<CharT, Traits>
            SPACES(80, CharT(' '));

    out_.write("\n", 1);

    while (indent > 0) {
        int amount = std::min(indent, int(SPACES.size()));
        out_.write(SPACES.data(), amount);
        indent -= amount;
    }
}

template<class CharT, class Traits, class Output>
template<class Annot>
void no_annotation_renderer<CharT, Traits, Output>::push_annotation(
        const Annot&)
{ }

template<class CharT, class Traits, class Output>
void no_annotation_renderer<CharT, Traits, Output>::pop_annotation()
{ }

template<class AnnotText, class CharT, class Traits, class Output>
void
simple_annotation_renderer<AnnotText, CharT, Traits, Output>::push_annotation(
        const std::pair<AnnotText, AnnotText>& annot)
{
    out_ << annot.first;
    annot_stack_.push_back(&annot.second);
}

template<class AnnotText, class CharT, class Traits, class Output>
void
simple_annotation_renderer<AnnotText, CharT, Traits, Output>::pop_annotation()
{
    assert( !annot_stack_.empty() );
    out_ << *annot_stack_.back();
    annot_stack_.pop_back();
}

}
