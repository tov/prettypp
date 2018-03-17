#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

/// Pretty-printing combinators.
namespace pretty {

class no_annotation {};

/// A document, parameterized by character type and string allocator.
template<
        class Annot,
        class CharT,
        class Traits = std::char_traits<CharT>,
        class Allocator = std::allocator<CharT>
>
class basic_document
{
public:
    using text_type = std::basic_string<CharT, Traits, Allocator>;
    using text_view_type = std::basic_string_view<CharT, Traits>;

    using annot_type =
        std::conditional<std::is_same_v<void, Annot>,
            no_annotation,
            Annot>;

private:
    struct owned_text_ { text_type s; };
    struct nil_ {};
    struct line_ {};
    struct append_ { basic_document first, second; };
    struct group_ { basic_document document; };
    struct nest_ { int amount; basic_document document; };
    struct annot_ { annot_type annot; basic_document document; };

    using repr_ = std::variant<
            owned_text_,
            nil_,
            line_,
            append_,
            group_,
            nest_,
            annot_
    >;

    std::unique_ptr<repr_> pimpl_;

    template <class... Arg>
    explicit basic_document(Arg&& ...);

    enum class mode_ { breaking, flat };

    struct cmd_
    {
        int indent;
        mode_ mode;
        const basic_document* doc;
    };

    using cmd_stack_ = std::vector<cmd_>;

    static bool fits(cmd_ next,
                     const cmd_stack_& todo,
                     cmd_stack_& stack,
                     int space_remaining);

public:
    /// Constructs the empty (nil) document.
    basic_document() : basic_document(nil_ {}) {}
    /// Deep-copy constructs a document.
    basic_document(const basic_document&);
    /// Deep-copy assigns a document.
    basic_document& operator=(const basic_document&);
    /// Move-constructs a document.
    basic_document(basic_document&&) noexcept = default;
    /// Move-assigns a document.
    basic_document& operator=(basic_document&&) noexcept = default;

    /// Equivalent to `std::move`.
    basic_document move();

    /// Constructs a text document, emplacing the string.
    template <class... Arg>
    static basic_document text(Arg&& ...);

    /// Constructs a line-break document.
    static basic_document line();

    /// Appends two documents.
    basic_document append(basic_document) &&;

    /// The group operation, which suppresses line breaks when possible.
    basic_document group() &&;

    /// Nests by the given amount.
    basic_document nest(int amount) &&;

    /// Emplaces an annotation on a document.
    template <class... Arg>
    basic_document annotate(Arg&&...) &&;

    /// Render to an ostream.
    void render(std::ostream&, int width) const;
};

using document = basic_document<void, char>;

/////
///// Implementations
/////

template<class Annot, class CharT, class Traits, class Allocator>
basic_document<Annot, CharT, Traits, Allocator>::basic_document(
        const basic_document& other)
        : basic_document(*other.pimpl_)
{ }

template<class Annot, class CharT, class Traits, class Allocator>
auto basic_document<Annot, CharT, Traits, Allocator>::operator=(
        const basic_document& other) -> basic_document&
{
    *pimpl_ = *other.pimpl_;
    return *this;
}

template<class Annot, class CharT, class Traits, class Allocator>
auto basic_document<Annot, CharT, Traits, Allocator>::move() -> basic_document
{
    return std::move(*this);
}

template<class Annot, class CharT, class Traits, class Allocator>
template<class... Arg>
auto basic_document<Annot, CharT, Traits, Allocator>::text(Arg&& ... arg) -> basic_document
{
    return basic_document(owned_text_ { text_type(std::forward<Arg>(arg)...) });
}

template<class Annot, class CharT, class Traits, class Allocator>
template<class... Arg>
basic_document<Annot, CharT, Traits, Allocator>::basic_document(Arg&& ... arg)
        : pimpl_{std::make_unique<repr_>(std::forward<Arg>(arg)...)}
{ }

template<class Annot, class CharT, class Traits, class Allocator>
auto basic_document<Annot, CharT, Traits, Allocator>::line() -> basic_document
{
    return basic_document(line_ {});
}

template<class Annot, class CharT, class Traits, class Allocator>
auto basic_document<Annot, CharT, Traits, Allocator>::append(
        basic_document next)&& -> basic_document
{
    return basic_document(append_ { std::move(*this), std::move(next) });
}

template<class Annot, class CharT, class Traits, class Allocator>
auto basic_document<Annot, CharT, Traits, Allocator>::group() && -> basic_document
{
    return basic_document(group_ { std::move(*this) });
}

template<class Annot, class CharT, class Traits, class Allocator>
auto basic_document<Annot, CharT, Traits, Allocator>::nest(int amount) && -> basic_document
{
    return basic_document(nest_ {amount, std::move(*this) });
}

template<class Annot, class CharT, class Traits, class Allocator>
template<class... Arg>
auto basic_document<Annot, CharT, Traits, Allocator>::annotate(Arg&&... arg) && -> basic_document
{
    return basic_document(annot_ { Annot(std::forward<Arg>(arg)...),
                                   std::move(*this) });
}

template<class Annot, class CharT, class Traits, class Allocator>
bool
basic_document<Annot, CharT, Traits, Allocator>::fits(
        cmd_ next,
        const cmd_stack_& todo,
        cmd_stack_& stack,
        int space_remaining)
{
    auto todo_begin = todo.rbegin();
    auto todo_end   = todo.rend();
    stack.clear();
    stack.push_back(next);

    while (space_remaining >= 0) {
        if (stack.empty()) {
            if (todo_begin == todo_end) return true;
            else stack.push_back(*todo_begin++);
        } else {
            cmd_ cmd {stack.back()};
            stack.pop_back();

            struct Fits_visitor
            {
                const cmd_& cmd;
                cmd_stack_& stack;
                int& space_remaining;

                bool operator()(nil_) const
                {
                    return false;
                }

                bool operator()(const append_& app) const
                {
                    stack.push_back(cmd_{ cmd.indent, cmd.mode, &app.second });
                    stack.push_back(cmd_{ cmd.indent, cmd.mode, &app.first });
                    return false;
                }

                bool operator()(const owned_text_& text) const
                {
                    // This only works for ASCII :(
                    space_remaining -= text.s.size();
                    return false;
                }

                bool operator()(line_) const
                {
                    switch (cmd.mode) {
                        case mode_::breaking:
                            return true;
                        case mode_::flat:
                            --space_remaining;
                            return false;
                    }
                }

                bool operator()(const group_& group) const
                {
                    stack.push_back(cmd_{ cmd.indent, cmd.mode, &group.document });
                    return false;
                }

                bool operator()(const nest_& nest) const
                {
                    stack.push_back(cmd_{ cmd.indent + nest.amount, cmd.mode, &nest.document });
                    return false;
                }

                bool operator()(const annot_& annot) const
                {
                    stack.push_back(cmd_{ cmd.indent, cmd.mode, &annot.document });
                    return false;
                }
            };

            if (std::visit(Fits_visitor{cmd, stack, space_remaining},
                           *cmd.doc->pimpl_))
                return true;
        }
    }

    return false;
}

template<class Annot, class CharT, class Traits, class Allocator>
void basic_document<Annot, CharT, Traits, Allocator>::render(
        std::ostream& out, const int width) const
{
    int pos { 0 };
    cmd_stack_ stack { cmd_{ 0, mode_::breaking, this } };
    cmd_stack_ aux_stack;

    while (!stack.empty()) {
        cmd_ cmd = stack.back();
        stack.pop_back();

        struct Render_visitor
        {
            int& pos;
            const int width;
            cmd_& cmd;
            cmd_stack_& stack;
            cmd_stack_& aux_stack;
            std::ostream& out;

            void operator()(nil_) const
            { }

            void operator()(const append_& app) const
            {
                stack.push_back(cmd_{cmd.indent, cmd.mode, &app.second});
                stack.push_back(cmd_{cmd.indent, cmd.mode, &app.first});
            }

            void operator()(const owned_text_& text) const
            {
                out << text.s;
                pos += text.s.size();
            }

            void operator()(line_) const
            {
                switch (cmd.mode) {
                    case mode_::breaking:
                        out << '\n';
                        for (size_t i = 0; i < cmd.indent; ++i) out << ' ';
                        pos = cmd.indent;
                        break;
                    case mode_::flat:
                        out << ' ';
                        ++pos;
                        break;
                }
            }

            void operator()(const group_& group) const
            {
                cmd_ next { cmd.indent, mode_::flat, &group.document };

                if (cmd.mode == mode_::breaking && !fits(next, stack, aux_stack, width - pos))
                    next.mode = mode_::breaking;

                stack.push_back(next);
            }

            void operator()(const nest_& nest) const
            {
                stack.push_back(cmd_{cmd.indent + nest.amount, cmd.mode, &nest.document});
            }

            void operator()(const annot_& annot) const
            {
                stack.push_back(cmd_{cmd.indent, cmd.mode, &annot.document});
            }
        };

        std::visit(Render_visitor{pos, width, cmd, stack, aux_stack, out},
                   *cmd.doc->pimpl_);
    }

}

}
