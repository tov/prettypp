#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <type_traits>
#include <variant>

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
auto basic_document<Annot, CharT, Traits, Allocator>::nest(int amount)&& -> basic_document
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

}
