#pragma once

#include "renderers.h"

#include <memory>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

/// Pretty-printing combinators.
namespace pretty {

class no_annotation {};

/// A document, parameterized by character type and string allocator.
template<class Annot>
class annotated_document
{
public:
    using text_type = std::string;
    using text_view_type = std::string_view;

    using annot_type =
        std::conditional<std::is_same_v<void, Annot>,
            no_annotation,
            Annot>;

private:
    struct owned_text_ { text_type s; };
    struct borrowed_text_ { text_view_type sv; };
    struct nil_ {};
    struct line_ {};
    struct append_ { annotated_document first, second; };
    struct group_ { annotated_document document; };
    struct nest_ { int amount; annotated_document document; };
    struct annot_ { annot_type annot; annotated_document document; };

    using repr_ = std::variant<
            owned_text_,
            borrowed_text_,
            nil_,
            line_,
            append_,
            group_,
            nest_,
            annot_
    >;

    std::unique_ptr<repr_> pimpl_;

    template <class... Arg>
    explicit annotated_document(Arg&& ...);

    enum class mode_ { breaking, flat };

    struct cmd_
    {
        int indent;
        mode_ mode;
        const annotated_document* doc;
    };

    using cmd_stack_ = std::vector<cmd_>;

    static bool fits(cmd_ next,
                     const cmd_stack_& todo,
                     cmd_stack_& stack,
                     int space_remaining);

public:
    /// Constructs the empty (nil) document.
    annotated_document() : annotated_document(nil_ {}) {}
    /// Deep-copy constructs a document.
    annotated_document(const annotated_document&);
    /// Deep-copy assigns a document.
    annotated_document& operator=(const annotated_document&);
    /// Move-constructs a document.
    annotated_document(annotated_document&&) noexcept = default;
    /// Move-assigns a document.
    annotated_document& operator=(annotated_document&&) noexcept = default;

    /// Equivalent to `std::move`.
    annotated_document move();

    /// Constructs a text document, emplacing the string.
    template <class... Arg>
    static annotated_document text(Arg&& ...);

    /// Constructs a text view document.
    static annotated_document view(text_view_type sv);

    /// Constructs a line-break document.
    static annotated_document line();

    /// Appends two documents.
    annotated_document append(annotated_document) &&;

    /// The group operation, which suppresses line breaks when possible.
    annotated_document group() &&;

    /// Nests by the given amount.
    annotated_document nest(int amount) &&;

    /// Emplaces an annotation on a document.
    template <class... Arg>
    annotated_document annotate(Arg&&...) &&;

    /// Render to a generic renderer.
    template <class Renderer>
    void render(Renderer&, int width) const;
};

using document = annotated_document<void>;

/////
///// Implementations
/////

template<class Annot>
annotated_document<Annot>::annotated_document(
        const annotated_document& other)
        : annotated_document(*other.pimpl_)
{ }

template<class Annot>
auto annotated_document<Annot>::operator=(
        const annotated_document& other) -> annotated_document&
{
    *pimpl_ = *other.pimpl_;
    return *this;
}

template<class Annot>
auto annotated_document<Annot>::move() -> annotated_document
{
    return std::move(*this);
}

template<class Annot>
template<class... Arg>
auto annotated_document<Annot>::text(Arg&& ... arg) -> annotated_document
{
    return annotated_document(owned_text_ { text_type(std::forward<Arg>(arg)...) });
}

template<class Annot>
auto annotated_document<Annot>::view(
        annotated_document::text_view_type sv) -> annotated_document
{
    return annotated_document(borrowed_text_ { sv });
}

template<class Annot>
template<class... Arg>
annotated_document<Annot>::annotated_document(Arg&& ... arg)
        : pimpl_{std::make_unique<repr_>(std::forward<Arg>(arg)...)}
{ }

template<class Annot>
auto annotated_document<Annot>::line() -> annotated_document
{
    return annotated_document(line_ {});
}

template<class Annot>
auto annotated_document<Annot>::append(
        annotated_document next)&& -> annotated_document
{
    return annotated_document(append_ { std::move(*this), std::move(next) });
}

template<class Annot>
auto annotated_document<Annot>::group() && -> annotated_document
{
    return annotated_document(group_ { std::move(*this) });
}

template<class Annot>
auto annotated_document<Annot>::nest(int amount) && -> annotated_document
{
    return annotated_document(nest_ {amount, std::move(*this) });
}

template<class Annot>
template<class... Arg>
auto annotated_document<Annot>::annotate(Arg&&... arg) && -> annotated_document
{
    return annotated_document(annot_ { Annot(std::forward<Arg>(arg)...),
                                   std::move(*this) });
}

template<class Annot>
bool
annotated_document<Annot>::fits(
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

                bool operator()(borrowed_text_ text) const
                {
                    space_remaining -= text.sv.size();
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

template <class Annot>
template <class Renderer>
void annotated_document<Annot>::render(
        Renderer& out, const int width) const
{
    int pos { 0 };
    cmd_stack_ stack { cmd_{ 0, mode_::breaking, this } };
    cmd_stack_ aux_stack;
    std::vector<size_t> annot_stack;

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
            std::vector<size_t>& annot_stack;
            Renderer& out;

            void operator()(nil_) const
            { }

            void operator()(const append_& app) const
            {
                stack.push_back(cmd_{cmd.indent, cmd.mode, &app.second});
                stack.push_back(cmd_{cmd.indent, cmd.mode, &app.first});
            }

            void operator()(const owned_text_& text) const
            {
                out.write(text.s);
                pos += text.s.size();
            }

            void operator()(borrowed_text_ text) const {
                out.write(text.sv);
                pos += text.sv.size();
            }

            void operator()(line_) const
            {
                switch (cmd.mode) {
                    case mode_::breaking:
                        out.newline(cmd.indent);
                        pos = cmd.indent;
                        break;
                    case mode_::flat:
                        out.write(' ');
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
                out.push_annotation(annot.annot);
                annot_stack.push_back(stack.size());
                stack.push_back(cmd_{cmd.indent, cmd.mode, &annot.document});
            }
        };

        std::visit(Render_visitor{pos, width, cmd, stack,
                                  aux_stack, annot_stack, out},
                   *cmd.doc->pimpl_);

        if (!annot_stack.empty() && annot_stack.back() == stack.size()) {
            annot_stack.pop_back();
            out.pop_annotation();
        }
    }
}

}
