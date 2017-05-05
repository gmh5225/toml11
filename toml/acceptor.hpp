#ifndef TOML11_ACCEPTOR
#define TOML11_ACCEPTOR
#include <type_traits>
#include <iterator>
#include <limits>
#include "exception.hpp"

namespace toml
{

template<typename charT, charT c>
struct is_charactor
{
    typedef charT value_type;
    constexpr static value_type target = c;

    template<typename Iterator, class = typename std::enable_if<
        std::is_same<typename std::iterator_traits<Iterator>::value_type,
                     value_type>::value>::type>
    constexpr static Iterator invoke(Iterator iter)
    {
        return *iter == c ? std::next(iter) : iter;
    }
};

template<typename charT, charT lw, charT up>
struct is_in_range
{
    typedef charT value_type;
    constexpr static value_type upper = up;
    constexpr static value_type lower = lw;
    static_assert(lower <= upper, "lower <= upper");

    template<typename Iterator, class = typename std::enable_if<
        std::is_same<typename std::iterator_traits<Iterator>::value_type,
                     value_type>::value>::type>
    constexpr static Iterator invoke(Iterator iter)
    {
        return (lower <= *iter && *iter <= upper) ? std::next(iter) : iter;
    }
};

template<typename headT, typename ... condT>
struct is_one_of
{
    typedef typename headT::value_type value_type;
    static_assert(
        std::is_same<value_type, typename is_one_of<condT...>::value_type>::value,
        "different value_type");

    template<typename Iterator, class = typename std::enable_if<
        std::is_same<typename std::iterator_traits<Iterator>::value_type,
                     value_type>::value>::type>
    static Iterator invoke(Iterator iter)
    {
        const Iterator tmp = headT::invoke(iter);
        return (tmp != iter) ? tmp : is_one_of<condT...>::invoke(iter);
    }
};
template<typename tailT>
struct is_one_of<tailT>
{
    typedef typename tailT::value_type value_type;

    template<typename Iterator, class = typename std::enable_if<
        std::is_same<typename std::iterator_traits<Iterator>::value_type,
                     value_type>::value>::type>
    static Iterator invoke(Iterator iter)
    {
        const Iterator tmp = tailT::invoke(iter);
        return (tmp != iter) ? tmp : iter;
    }
};

// just a wrapper for maybe_ignored
template<typename condT>
struct is_ignorable
{
    typedef typename condT::value_type value_type;

    template<typename Iterator, class = typename std::enable_if<
        std::is_same<typename std::iterator_traits<Iterator>::value_type,
                     value_type>::value>::type>
    static Iterator invoke(Iterator iter)
    {
        const Iterator tmp = condT::invoke(iter);
        return (tmp != iter) ? tmp : iter;
    }
};

template<typename condT>
struct maybe_ignored : std::false_type{};
template<typename condT>
struct maybe_ignored<is_ignorable<condT>> : std::true_type{};

template<typename headT, typename ... condT>
struct is_chain_of_impl
{
    typedef typename headT::value_type value_type;
    static_assert(std::is_same<value_type,
                      typename is_chain_of_impl<condT...>::value_type>::value,
                  "different value_type");

    constexpr static bool ignorable = maybe_ignored<headT>::value;

    template<typename Iterator, class = typename std::enable_if<
        std::is_same<typename std::iterator_traits<Iterator>::value_type,
                     value_type>::value>::type>
    static Iterator invoke(Iterator iter, Iterator rollback)
    {
        const Iterator tmp = headT::invoke(iter);
        return (tmp == iter && not ignorable) ? rollback :
                is_chain_of_impl<condT...>::invoke(tmp, rollback);
    }
};

template<typename tailT>
struct is_chain_of_impl<tailT>
{
    typedef typename tailT::value_type value_type;
    constexpr static bool ignorable = maybe_ignored<tailT>::value;

    template<typename Iterator, class = typename std::enable_if<
        std::is_same<typename std::iterator_traits<Iterator>::value_type,
                     value_type>::value>::type>
    static Iterator invoke(Iterator iter, Iterator rollback)
    {
        const Iterator tmp = tailT::invoke(iter);
        return (tmp == iter) ? (ignorable ? iter : rollback) : tmp;
    }
};

template<typename headT, typename ...condT>
struct is_chain_of
{
    typedef typename is_chain_of_impl<headT, condT...>::value_type value_type;

    template<typename Iterator, class = typename std::enable_if<
        std::is_same<typename std::iterator_traits<Iterator>::value_type,
                     value_type>::value>::type>
    static Iterator invoke(Iterator iter)
    {
        return is_chain_of_impl<headT, condT...>::invoke(iter, iter);
    }
};

constexpr inline std::size_t repeat_infinite(){return 0ul;}

template<typename condT, std::size_t N>
struct is_repeat_of
{
    typedef typename condT::value_type value_type;

    template<typename Iterator, class = typename std::enable_if<
        std::is_same<typename std::iterator_traits<Iterator>::value_type,
                     value_type>::value>::type>
    static Iterator invoke(Iterator iter)
    {
        const Iterator rollback = iter;
        Iterator tmp;
        for(auto i=0ul; i<N; ++i)
        {
            tmp = condT::invoke(iter);
            if(tmp == iter) return rollback;
            iter = tmp;
        }
        return iter;
    }
};

template<typename condT>
struct is_repeat_of<condT, 0>
{
    typedef typename condT::value_type value_type;

    template<typename Iterator, class = typename std::enable_if<
        std::is_same<typename std::iterator_traits<Iterator>::value_type,
                     value_type>::value>::type>
    static Iterator invoke(Iterator iter)
    {
        Iterator tmp = condT::invoke(iter);
        while(tmp != iter)
        {
            iter = tmp;
            tmp = condT::invoke(iter);
        }
        return iter;
    }
};

template<typename headT, typename ... tailT>
struct is_none_of
{
    typedef typename headT::value_type value_type;
    static_assert(
        std::is_same<value_type, typename is_one_of<tailT...>::value_type>::value,
        "different value_type");

    template<typename Iterator, class = typename std::enable_if<
        std::is_same<typename std::iterator_traits<Iterator>::value_type,
                     value_type>::value>::type>
    static Iterator invoke(Iterator iter)
    {
        const Iterator tmp = headT::invoke(iter);
        return (tmp != iter) ? iter : is_none_of<tailT...>::invoke(iter);
    }
};

template<typename tailT>
struct is_none_of<tailT>
{
    typedef typename tailT::value_type value_type;

    template<typename Iterator, class = typename std::enable_if<
        std::is_same<typename std::iterator_traits<Iterator>::value_type,
                     value_type>::value>::type>
    static Iterator invoke(Iterator iter)
    {
        const Iterator tmp = tailT::invoke(iter);
        return (tmp != iter) ? iter : std::next(iter);
    }
};

template<typename notT, typename butT>
struct is_not_but
{
    typedef typename notT::value_type value_type;
    static_assert(
        std::is_same<value_type, typename butT::value_type>::value,
        "different value type");

    template<typename Iterator, class = typename std::enable_if<
        std::is_same<typename std::iterator_traits<Iterator>::value_type,
                     value_type>::value>::type>
    static Iterator invoke(Iterator iter)
    {
        return (iter != notT::invoke(iter)) ? iter : butT::invoke(iter);
    }
};

template<typename charT>
using is_space      = is_charactor<charT, ' '>;
template<typename charT>
using is_tab        = is_charactor<charT, '\t'>;
template<typename charT>
using is_number     = is_in_range<charT, '0', '9'>;
template<typename charT>
using is_lowercase  = is_in_range<charT, 'a', 'z'>;
template<typename charT>
using is_uppercase  = is_in_range<charT, 'A', 'Z'>;
template<typename charT>
using is_alphabet   = is_one_of<is_lowercase<charT>, is_uppercase<charT>>;
template<typename charT>
using is_hex = is_one_of<is_number<charT>, is_in_range<charT, 'a', 'f'>,
                         is_in_range<charT, 'A', 'F'>>;
template<typename charT>
using is_whitespace = is_one_of<is_space<charT>, is_tab<charT>>;
template<typename charT>
using is_newline    = is_one_of<is_charactor<charT, '\n'>,
    is_chain_of<is_charactor<charT, '\r'>, is_charactor<charT, '\n'>>>;
template<typename charT>
using is_barekey_component = is_one_of<is_alphabet<charT>, is_number<charT>,
    is_charactor<charT, '_'>, is_charactor<charT, '-'>>;
template<typename charT>
using is_barekey = is_repeat_of<is_barekey_component<charT>, repeat_infinite()>;
template<typename charT>
using is_comment =
    is_chain_of<
        is_charactor<charT, '#'>,
        is_repeat_of<is_none_of<is_newline<charT>>, repeat_infinite()>,
        is_newline<charT>
    >;

template<typename charT>
using is_basic_inline_string_component =
    is_one_of<
        is_none_of< is_in_range<charT, '\0', '\31'>, is_charactor<charT, '\"'>,
                    is_charactor<charT, '\\'>, is_newline<charT>>,
        is_chain_of<is_charactor<charT, '\\'>, is_charactor<charT, '\"'>>,
        is_chain_of<is_charactor<charT, '\\'>, is_charactor<charT, '\\'>>,
        is_chain_of<is_charactor<charT, '\\'>, is_charactor<charT, 'b'>>,
        is_chain_of<is_charactor<charT, '\\'>, is_charactor<charT, 't'>>,
        is_chain_of<is_charactor<charT, '\\'>, is_charactor<charT, 'n'>>,
        is_chain_of<is_charactor<charT, '\\'>, is_charactor<charT, 'f'>>,
        is_chain_of<is_charactor<charT, '\\'>, is_charactor<charT, 'r'>>,
        is_chain_of<is_charactor<charT, '\\'>, is_charactor<charT, 'u'>,
                    is_repeat_of<is_hex<charT>, 4>>,
        is_chain_of<is_charactor<charT, '\\'>, is_charactor<charT, 'U'>,
                    is_repeat_of<is_hex<charT>, 8>>
        >;
template<typename charT>
using is_basic_inline_string =
    is_not_but<
        is_repeat_of<is_charactor<charT, '\"'>, 3>, // not multiline
        is_chain_of<
            is_charactor<charT, '\"'>,
            is_ignorable<is_repeat_of<is_basic_inline_string_component<charT>,
                                      repeat_infinite()>>,
            is_charactor<charT, '\"'>
        >
    >;
template<typename charT>
using is_basic_multiline_string_component =
    is_one_of<
        is_none_of< is_in_range<charT, '\0', '\31'>,
                    is_repeat_of<is_charactor<charT, '\"'>, 3>,
                    is_charactor<charT, '\\'>>,
        is_newline<charT>,
        is_chain_of<is_charactor<charT, '\\'>, is_newline<charT>>,
        is_chain_of<is_charactor<charT, '\\'>, is_charactor<charT, '\"'>>,
        is_chain_of<is_charactor<charT, '\\'>, is_charactor<charT, '\\'>>,
        is_chain_of<is_charactor<charT, '\\'>, is_charactor<charT, 'b'>>,
        is_chain_of<is_charactor<charT, '\\'>, is_charactor<charT, 't'>>,
        is_chain_of<is_charactor<charT, '\\'>, is_charactor<charT, 'n'>>,
        is_chain_of<is_charactor<charT, '\\'>, is_charactor<charT, 'f'>>,
        is_chain_of<is_charactor<charT, '\\'>, is_charactor<charT, 'r'>>,
        is_chain_of<is_charactor<charT, '\\'>, is_charactor<charT, 'u'>,
                    is_repeat_of<is_hex<charT>, 4>>,
        is_chain_of<is_charactor<charT, '\\'>, is_charactor<charT, 'U'>,
                    is_repeat_of<is_hex<charT>, 8>>
        >;
template<typename charT>
using is_basic_multiline_string =
    is_chain_of<
        is_repeat_of<is_charactor<charT, '\"'>, 3>,
        is_ignorable<is_repeat_of<is_basic_multiline_string_component<charT>,
                                  repeat_infinite()>>,
        is_repeat_of<is_charactor<charT, '\"'>, 3>
    >;

template<typename charT>
using is_literal_inline_string_component =
    is_none_of<is_in_range<charT, '\0', '\31'>, is_charactor<charT, '\''>>;

template<typename charT>
using is_literal_inline_string =
    is_not_but<
        is_repeat_of<is_charactor<charT, '\''>, 3>,
        is_chain_of<
            is_charactor<charT, '\''>,
            is_ignorable<is_repeat_of<is_literal_inline_string_component<charT>,
                                      repeat_infinite()>>,
            is_charactor<charT, '\''>
        >
    >;

template<typename charT>
using is_literal_multiline_string_component =
    is_one_of<
        is_none_of<is_in_range<charT, '\0', '\31'>,
                   is_repeat_of<is_charactor<charT, '\''>, 3>>,
        is_newline<charT>
    >;

template<typename charT>
using is_literal_multiline_string =
    is_chain_of<
        is_repeat_of<is_charactor<charT, '\''>, 3>,
        is_ignorable<is_repeat_of<is_literal_multiline_string_component<charT>,
                                  repeat_infinite()>>,
        is_repeat_of<is_charactor<charT, '\''>, 3>
    >;

template<typename charT>
using is_sign = is_one_of<is_charactor<charT, '+'>, is_charactor<charT, '-'>>;
template<typename charT>
using is_nonzero_number = is_in_range<charT, '1', '9'>;

template<typename charT>
using is_integer_component =
    is_not_but<
        is_repeat_of<is_charactor<charT, '_'>, 2>,
        is_one_of<
            is_charactor<charT, '_'>, is_number<charT>
        >
    >;
template<typename charT>
using is_integer =
    is_chain_of<
        is_ignorable<is_sign<charT>>,
        is_one_of<
            is_charactor<charT, '0'>,
            is_chain_of<
                is_nonzero_number<charT>,
                is_ignorable<is_repeat_of<is_integer_component<charT>,
                                          repeat_infinite()>
                >
            >
        >
    >;

template<typename charT>
using is_fractional_part =
    is_chain_of<
        is_charactor<charT, '.'>,
        is_repeat_of<is_integer_component<charT>, repeat_infinite()>
    >;
template<typename charT>
using is_exponent_part =
    is_chain_of<
        is_one_of<is_charactor<charT, 'e'>, is_charactor<charT, 'E'>>,
        is_integer<charT>
    >;
template<typename charT>
using is_float =
    is_one_of<
        is_chain_of<
            is_integer<charT>,
            is_fractional_part<charT>,
            is_exponent_part<charT>
        >,
        is_chain_of<
            is_integer<charT>,
            is_fractional_part<charT>
        >,
        is_chain_of<
            is_integer<charT>,
            is_exponent_part<charT>
        >
    >;

template<typename charT>
using is_boolean =
    is_one_of<
        is_chain_of<
            is_charactor<charT, 't'>,
            is_charactor<charT, 'r'>,
            is_charactor<charT, 'u'>,
            is_charactor<charT, 'e'>
        >,
        is_chain_of<
            is_charactor<charT, 'f'>,
            is_charactor<charT, 'a'>,
            is_charactor<charT, 'l'>,
            is_charactor<charT, 's'>,
            is_charactor<charT, 'e'>
        >
    >;

template<typename charT>
using is_local_time =
    is_chain_of<
        is_repeat_of<is_number<charT>, 2>,
        is_charactor<charT, ':'>,
        is_repeat_of<is_number<charT>, 2>,
        is_charactor<charT, ':'>,
        is_repeat_of<is_number<charT>, 2>,
        is_ignorable<
            is_chain_of<
                is_charactor<charT, '.'>,
                is_repeat_of<is_number<charT>, repeat_infinite()>
            >
        >
    >;

template<typename charT>
using is_local_date =
    is_chain_of<
        is_repeat_of<is_number<charT>, 4>,
        is_charactor<charT, '-'>,
        is_repeat_of<is_number<charT>, 2>,
        is_charactor<charT, '-'>,
        is_repeat_of<is_number<charT>, 2>
    >;

template<typename charT>
using is_local_date_time =
    is_chain_of<
        is_local_date<charT>,
        is_charactor<charT, 'T'>,
        is_local_time<charT>
    >;

template<typename charT>
using is_offset =
    is_one_of<
        is_charactor<charT, 'Z'>,
        is_chain_of<
            is_sign<charT>,
            is_repeat_of<is_number<charT>, 2>,
            is_charactor<charT, ':'>,
            is_repeat_of<is_number<charT>, 2>
        >
    >;

template<typename charT>
using is_offset_date_time =
    is_chain_of<
        is_local_date_time<charT>,
        is_offset<charT>
    >;

template<typename charT>
using is_fundamental_type =
    is_one_of<
        is_basic_inline_string<charT>,
        is_basic_multiline_string<charT>,
        is_literal_inline_string<charT>,
        is_literal_multiline_string<charT>,
        is_integer<charT>,
        is_float<charT>,
        is_boolean<charT>,
        is_offset_date_time<charT>,
        is_local_date_time<charT>,
        is_local_date<charT>,
        is_local_time<charT>
    >;

template<typename charT>
using is_skippable_in_array =
    is_repeat_of<
        is_one_of<is_whitespace<charT>, is_newline<charT>, is_comment<charT>>,
        repeat_infinite()
    >;

template<typename charT>
struct is_inline_table;

template<typename charT>
using is_key =
    is_one_of<
        is_barekey<charT>,
        is_basic_inline_string<charT>,
        is_literal_inline_string<charT>
    >;

template<typename charT>
using is_skippable_in_inline_table =
    is_repeat_of<is_whitespace<charT>, repeat_infinite()>;


template<typename charT>
struct is_array
{
    typedef charT value_type;

    template<typename Iterator, class = typename std::enable_if<
        std::is_same<typename std::iterator_traits<Iterator>::value_type,
                     value_type>::value>::type>
    static Iterator invoke(Iterator iter)
    {
        typedef is_one_of<is_fundamental_type<charT>,
                          is_array<charT>, is_inline_table<charT>> is_component;

        typedef is_chain_of<
            is_charactor<charT, '['>,
            is_ignorable<
                is_repeat_of<
                    is_chain_of<
                        is_ignorable<is_skippable_in_array<charT>>,
                        is_component,
                        is_ignorable<is_skippable_in_array<charT>>,
                        is_charactor<charT, ','>
                    >,
                    repeat_infinite()
                >
            >,
            is_ignorable<
                    is_chain_of<
                        is_ignorable<is_skippable_in_array<charT>>,
                        is_component,
                        is_ignorable<is_skippable_in_array<charT>>,
                        is_ignorable<is_charactor<charT, ','>>
                    >
                >,
            is_ignorable<is_skippable_in_array<charT>>,
            is_charactor<charT, ']'>
        > entity;
        return entity::invoke(iter);
    }
};

template<typename charT>
struct is_inline_table
{
    typedef charT value_type;

    template<typename Iterator, class = typename std::enable_if<
        std::is_same<typename std::iterator_traits<Iterator>::value_type,
                     value_type>::value>::type>
    static Iterator invoke(Iterator iter)
    {
        typedef is_one_of<is_fundamental_type<charT>,
                          is_array<charT>, is_inline_table<charT>> is_component;
        typedef is_chain_of<
            is_ignorable<is_skippable_in_inline_table<charT>>,
            is_key<charT>,
            is_ignorable<is_skippable_in_inline_table<charT>>,
            is_charactor<charT, '='>,
            is_ignorable<is_skippable_in_inline_table<charT>>,
            is_component,
            is_ignorable<is_skippable_in_inline_table<charT>>
        > is_inline_key_value_pair;

        typedef is_chain_of<
            is_charactor<charT, '{'>,
            is_ignorable<
                is_repeat_of<
                    is_chain_of<
                        is_ignorable<is_skippable_in_inline_table<charT>>,
                        is_inline_key_value_pair,
                        is_ignorable<is_skippable_in_inline_table<charT>>,
                        is_charactor<charT, ','>
                    >,
                    repeat_infinite()
                >
            >,
            is_ignorable<
                    is_chain_of<
                        is_ignorable<is_skippable_in_inline_table<charT>>,
                        is_inline_key_value_pair,
                        is_ignorable<is_skippable_in_inline_table<charT>>,
                        is_ignorable<is_charactor<charT, ','>>
                    >
                >,
            is_ignorable<is_skippable_in_inline_table<charT>>,
            is_charactor<charT, '}'>
            > entity;
        return entity::invoke(iter);
    }
};








}//toml
#endif// TOML11_ACCEPTOR
