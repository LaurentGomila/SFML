#include <SFML/System/String.hpp>

#include <catch2/catch_test_macros.hpp>

#include <GraphicsUtil.hpp>
#include <array>
#include <iomanip>
#include <sstream>
#include <type_traits>

#include <cassert>

namespace
{
// Return either argument depending on whether wchar_t is 16 or 32 bits
// Lets us write tests that work on both Windows where wchar_t is 16 bits
// and elsewhere where it is 32. Otherwise the tests would only work on
// one OS or the other.
template <typename T>
auto select(const std::basic_string<T>& string16, const std::basic_string<T>& string32)
{
    assert(string16 != string32 && "Invalid to select between identical inputs");
    if constexpr (sizeof(wchar_t) == 2)
        return string16;
    else
        return string32;
}

auto toHex(const char32_t character)
{
    std::ostringstream stream;
    stream << "[\\x" << std::uppercase << std::hex << std::uint32_t{character} << ']';
    return stream.str();
}
} // namespace

// Specialize StringMaker for alternative std::basic_string<T> specializations
// std::string's string conversion cannot be specialized but all other string types get special treatment
// https://github.com/catchorg/Catch2/blob/devel/docs/tostring.md#catchstringmaker-specialisation
namespace Catch
{
template <>
struct StringMaker<sf::U8String>
{
    static std::string convert(const sf::U8String& string)
    {
        std::ostringstream output;
        for (const auto character : string)
        {
            if (character >= 32 && character < 127)
                output << std::string(1, static_cast<char>(character));
            else
                output << toHex(character);
        }
        return output.str();
    }
};
} // namespace Catch

TEST_CASE("[System] sf::U8StringCharTraits")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(std::is_trivially_copy_constructible_v<sf::U8StringCharTraits>);
        STATIC_CHECK(std::is_trivially_copy_assignable_v<sf::U8StringCharTraits>);
        STATIC_CHECK(std::is_trivially_move_constructible_v<sf::U8StringCharTraits>);
        STATIC_CHECK(std::is_trivially_move_assignable_v<sf::U8StringCharTraits>);
    }

    SECTION("assign(char_type&, const char_type&)")
    {
        sf::U8StringCharTraits::char_type       c1 = 'a';
        const sf::U8StringCharTraits::char_type c2 = 'b';
        sf::U8StringCharTraits::assign(c1, c2);
        CHECK(c1 == 'b');
        CHECK(c2 == 'b');
    }

    SECTION("assign(char_type*, std::size_t, char_type)")
    {
        std::array<sf::U8StringCharTraits::char_type, 4> s = {'a', 'b', 'c', '\0'};
        CHECK(sf::U8StringCharTraits::assign(s.data(), 2, 'd') == s.data());
        CHECK(s[0] == 'd');
        CHECK(s[1] == 'd');
        CHECK(s[2] == 'c');
    }

    SECTION("eq()")
    {
        CHECK(sf::U8StringCharTraits::eq(0, 0));
        CHECK(!sf::U8StringCharTraits::eq(1, 0));
        CHECK(!sf::U8StringCharTraits::eq(0, 1));
        CHECK(sf::U8StringCharTraits::eq(1, 1));
    }

    SECTION("lt()")
    {
        CHECK(!sf::U8StringCharTraits::lt(0, 0));
        CHECK(!sf::U8StringCharTraits::lt(1, 0));
        CHECK(sf::U8StringCharTraits::lt(0, 1));
        CHECK(!sf::U8StringCharTraits::lt(1, 1));
    }

    SECTION("move()")
    {
        std::array<sf::U8StringCharTraits::char_type, 4>       s1 = {'a', 'b', 'c', '\0'};
        const std::array<sf::U8StringCharTraits::char_type, 4> s2 = {'d', 'e', 'f', '\0'};
        CHECK(sf::U8StringCharTraits::move(s1.data(), s2.data(), s2.size()) == s1.data());
        CHECK(s1[0] == 'd');
        CHECK(s1[1] == 'e');
        CHECK(s1[2] == 'f');
        CHECK(s2[0] == 'd');
        CHECK(s2[1] == 'e');
        CHECK(s2[2] == 'f');
    }

    SECTION("copy()")
    {
        std::array<sf::U8StringCharTraits::char_type, 4>       s1 = {'a', 'b', 'c', '\0'};
        const std::array<sf::U8StringCharTraits::char_type, 4> s2 = {'d', 'e', 'f', '\0'};
        CHECK(sf::U8StringCharTraits::copy(s1.data(), s2.data(), s2.size()) == s1.data());
        CHECK(s1[0] == 'd');
        CHECK(s1[1] == 'e');
        CHECK(s1[2] == 'f');
        CHECK(s2[0] == 'd');
        CHECK(s2[1] == 'e');
        CHECK(s2[2] == 'f');
    }

    SECTION("compare()")
    {
        const std::array<sf::U8StringCharTraits::char_type, 4> s1 = {'a', 'b', 'c', '\0'};
        const std::array<sf::U8StringCharTraits::char_type, 4> s2 = {'a', 'b', 'c', '\0'};
        const std::array<sf::U8StringCharTraits::char_type, 4> s3 = {'d', 'e', 'f', '\0'};
        CHECK(sf::U8StringCharTraits::compare(s1.data(), s2.data(), s1.size()) == 0);
        CHECK(sf::U8StringCharTraits::compare(s1.data(), s3.data(), s1.size()) < 0);
        CHECK(sf::U8StringCharTraits::compare(s3.data(), s1.data(), s3.size()) > 0);
    }

    SECTION("length()")
    {
        const std::array<sf::U8StringCharTraits::char_type, 2> s1 = {'a', '\0'};
        const std::array<sf::U8StringCharTraits::char_type, 6> s2 = {'a', 'b', 'c', 'd', 'e', '\0'};
        CHECK(sf::U8StringCharTraits::length(s1.data()) == 1);
        CHECK(sf::U8StringCharTraits::length(s2.data()) == 5);
    }

    SECTION("find()")
    {
        const std::array<sf::U8StringCharTraits::char_type, 5> s = {'a', 'b', 'c', 'd', 'e'};
        CHECK(*sf::U8StringCharTraits::find(s.data(), s.size(), 'a') == 'a');
        CHECK(sf::U8StringCharTraits::find(s.data(), s.size(), 'f') == nullptr);
    }

    SECTION("to_char_type()")
    {
        CHECK(sf::U8StringCharTraits::to_char_type(sf::U8StringCharTraits::int_type{0}) ==
              sf::U8StringCharTraits::char_type{0});
        CHECK(sf::U8StringCharTraits::to_char_type(sf::U8StringCharTraits::int_type{1}) ==
              sf::U8StringCharTraits::char_type{1});
        CHECK(sf::U8StringCharTraits::to_char_type(sf::U8StringCharTraits::int_type{10}) ==
              sf::U8StringCharTraits::char_type{10});
    }

    SECTION("to_int_type()")
    {
        CHECK(sf::U8StringCharTraits::to_int_type(sf::U8StringCharTraits::char_type{0}) ==
              sf::U8StringCharTraits::int_type{0});
        CHECK(sf::U8StringCharTraits::to_int_type(sf::U8StringCharTraits::char_type{1}) ==
              sf::U8StringCharTraits::int_type{1});
        CHECK(sf::U8StringCharTraits::to_int_type(sf::U8StringCharTraits::char_type{10}) ==
              sf::U8StringCharTraits::int_type{10});
    }

    SECTION("eq_int_type()")
    {
        CHECK(sf::U8StringCharTraits::eq_int_type(sf::U8StringCharTraits::int_type{0}, sf::U8StringCharTraits::int_type{0}));
        CHECK(sf::U8StringCharTraits::eq_int_type(sf::U8StringCharTraits::int_type{1}, sf::U8StringCharTraits::int_type{1}));
        CHECK(sf::U8StringCharTraits::eq_int_type(sf::U8StringCharTraits::int_type{10},
                                                  sf::U8StringCharTraits::int_type{10}));
    }

    SECTION("eof()")
    {
        CHECK(sf::U8StringCharTraits::eof() == ~sf::U8StringCharTraits::int_type{0});
    }

    SECTION("not_eof()")
    {
        CHECK(sf::U8StringCharTraits::not_eof(sf::U8StringCharTraits::int_type{0}) == sf::U8StringCharTraits::int_type{0});
        CHECK(sf::U8StringCharTraits::not_eof(sf::U8StringCharTraits::int_type{1}) == sf::U8StringCharTraits::int_type{1});
        CHECK(sf::U8StringCharTraits::not_eof(sf::U8StringCharTraits::int_type{10}) == sf::U8StringCharTraits::int_type{10});
        CHECK(sf::U8StringCharTraits::not_eof(sf::U8StringCharTraits::eof()) != sf::U8StringCharTraits::eof());
    }
}

TEST_CASE("[System] sf::String")
{
    using namespace std::string_literals;

    SECTION("Type traits")
    {
        STATIC_CHECK(!std::is_constructible_v<sf::String, std::nullptr_t>);
        STATIC_CHECK(!std::is_constructible_v<sf::String, std::nullptr_t, const std::locale&>);
        STATIC_CHECK(std::is_copy_constructible_v<sf::String>);
        STATIC_CHECK(std::is_copy_assignable_v<sf::String>);
        STATIC_CHECK(std::is_nothrow_move_constructible_v<sf::String>);
        STATIC_CHECK(std::is_nothrow_move_assignable_v<sf::String>);
    }

    SECTION("Construction")
    {
        SECTION("Default constructor")
        {
            const sf::String string;
            CHECK(std::string(string).empty());
            CHECK(std::wstring(string).empty());
            CHECK(string.toAnsiString().empty());
            CHECK(string.toWideString().empty());
            CHECK(string.toUtf8().empty());
            CHECK(string.toUtf16().empty());
            CHECK(string.toUtf32().empty());
            CHECK(string.getSize() == 0);
            CHECK(string.isEmpty());
            CHECK(string.getData() != nullptr);
        }

        SECTION("ANSI character constructor")
        {
            const sf::String string = 'a';
            CHECK(std::string(string) == "a"s);
            CHECK(std::wstring(string) == L"a"s);
            CHECK(string.toAnsiString() == "a"s);
            CHECK(string.toWideString() == L"a"s);
            CHECK(string.toUtf8() == sf::U8String{'a'});
            CHECK(string.toUtf16() == u"a"s);
            CHECK(string.toUtf32() == U"a"s);
            CHECK(string.getSize() == 1);
            CHECK(!string.isEmpty());
            CHECK(string.getData() != nullptr);
        }

        SECTION("ANSI C string constructor")
        {
            SECTION("Nullptr")
            {
                const sf::String string = static_cast<char*>(nullptr);
                CHECK(std::string(string).empty());
                CHECK(std::wstring(string).empty());
                CHECK(string.toAnsiString().empty());
                CHECK(string.toWideString().empty());
                CHECK(string.toUtf8().empty());
                CHECK(string.toUtf16().empty());
                CHECK(string.toUtf32().empty());
                CHECK(string.getSize() == 0);
                CHECK(string.isEmpty());
                CHECK(string.getData() != nullptr);
            }

            SECTION("Non-empty string")
            {
                const sf::String string = "def";
                CHECK(std::string(string) == "def"s);
                CHECK(std::wstring(string) == L"def"s);
                CHECK(string.toAnsiString() == "def"s);
                CHECK(string.toWideString() == L"def"s);
                CHECK(string.toUtf8() == sf::U8String{'d', 'e', 'f'});
                CHECK(string.toUtf16() == u"def"s);
                CHECK(string.toUtf32() == U"def"s);
                CHECK(string.getSize() == 3);
                CHECK(!string.isEmpty());
                CHECK(string.getData() != nullptr);
            }
        }

        SECTION("ANSI string constructor")
        {
            const sf::String string = "ghi"s;
            CHECK(std::string(string) == "ghi"s);
            CHECK(std::wstring(string) == L"ghi"s);
            CHECK(string.toAnsiString() == "ghi"s);
            CHECK(string.toWideString() == L"ghi"s);
            CHECK(string.toUtf8() == sf::U8String{'g', 'h', 'i'});
            CHECK(string.toUtf16() == u"ghi"s);
            CHECK(string.toUtf32() == U"ghi"s);
            CHECK(string.getSize() == 3);
            CHECK(!string.isEmpty());
            CHECK(string.getData() != nullptr);
        }

        SECTION("Wide character constructor")
        {
            const sf::String string = L'\xFA';
            CHECK(std::string(string) == select("\xFA"s, "\0"s));
            CHECK(std::wstring(string) == L"\xFA"s);
            CHECK(string.toAnsiString() == select("\xFA"s, "\0"s));
            CHECK(string.toWideString() == L"\xFA"s);
            CHECK(string.toUtf8() == sf::U8String{0xC3, 0xBA});
            CHECK(string.toUtf16() == u"\xFA"s);
            CHECK(string.toUtf32() == U"\xFA"s);
            CHECK(string.getSize() == 1);
            CHECK(!string.isEmpty());
            CHECK(string.getData() != nullptr);
        }

        SECTION("Wide C string constructor")
        {
            SECTION("Nullptr")
            {
                const sf::String string = static_cast<wchar_t*>(nullptr);
                CHECK(std::string(string).empty());
                CHECK(std::wstring(string).empty());
                CHECK(string.toAnsiString().empty());
                CHECK(string.toWideString().empty());
                CHECK(string.toUtf8().empty());
                CHECK(string.toUtf16().empty());
                CHECK(string.toUtf32().empty());
                CHECK(string.getSize() == 0);
                CHECK(string.isEmpty());
                CHECK(string.getData() != nullptr);
            }

            SECTION("Non-empty string")
            {
                const sf::String string = L"j\xFAl";
                CHECK(std::string(string) == select("j\xFAl"s, "j\0l"s));
                CHECK(std::wstring(string) == L"j\xFAl"s);
                CHECK(string.toAnsiString() == select("j\xFAl"s, "j\0l"s));
                CHECK(string.toWideString() == L"j\xFAl"s);
                CHECK(string.toUtf8() == sf::U8String{'j', 0xC3, 0xBA, 'l'});
                CHECK(string.toUtf16() == u"j\xFAl"s);
                CHECK(string.toUtf32() == U"j\xFAl"s);
                CHECK(string.getSize() == 3);
                CHECK(!string.isEmpty());
                CHECK(string.getData() != nullptr);
            }
        }

        SECTION("Wide string constructor")
        {
            const sf::String string = L"mno\xFA"s;
            CHECK(std::string(string) == select("mno\xFA"s, "mno\0"s));
            CHECK(std::wstring(string) == L"mno\xFA"s);
            CHECK(string.toAnsiString() == select("mno\xFA"s, "mno\0"s));
            CHECK(string.toWideString() == L"mno\xFA"s);
            CHECK(string.toUtf8() == sf::U8String{'m', 'n', 'o', 0xC3, 0XBA});
            CHECK(string.toUtf16() == u"mno\xFA"s);
            CHECK(string.toUtf32() == U"mno\xFA"s);
            CHECK(string.getSize() == 4);
            CHECK(!string.isEmpty());
            CHECK(string.getData() != nullptr);
        }

        SECTION("UTF-32 character constructor")
        {
            const sf::String string = U'🐌';
            CHECK(std::string(string) == "\0"s);
            CHECK(std::wstring(string) == select(L""s, L"🐌"s));
            CHECK(string.toAnsiString() == "\0"s);
            CHECK(string.toWideString() == select(L""s, L"🐌"s));
            CHECK(string.toUtf8() == sf::U8String{0xF0, 0x9F, 0x90, 0x8C});
            CHECK(string.toUtf16() == u"🐌"s);
            CHECK(string.toUtf32() == U"🐌"s);
            CHECK(string.getSize() == 1);
            CHECK(!string.isEmpty());
            CHECK(string.getData() != nullptr);
        }

        SECTION("UTF-32 C string constructor")
        {
            SECTION("Nullptr")
            {
                const sf::String string = static_cast<char32_t*>(nullptr);
                CHECK(std::string(string).empty());
                CHECK(std::wstring(string).empty());
                CHECK(string.toAnsiString().empty());
                CHECK(string.toWideString().empty());
                CHECK(string.toUtf8().empty());
                CHECK(string.toUtf16().empty());
                CHECK(string.toUtf32().empty());
                CHECK(string.getSize() == 0);
                CHECK(string.isEmpty());
                CHECK(string.getData() != nullptr);
            }

            SECTION("Non-empty string")
            {
                const sf::String string = U"🐌rs";
                CHECK(std::string(string) == "\0rs"s);
                CHECK(std::wstring(string) == select(L"rs"s, L"🐌rs"s));
                CHECK(string.toAnsiString() == "\0rs"s);
                CHECK(string.toWideString() == select(L"rs"s, L"🐌rs"s));
                CHECK(string.toUtf8() == sf::U8String{0xF0, 0x9F, 0x90, 0x8C, 'r', 's'});
                CHECK(string.toUtf16() == u"🐌rs"s);
                CHECK(string.toUtf32() == U"🐌rs"s);
                CHECK(string.getSize() == 3);
                CHECK(!string.isEmpty());
                CHECK(string.getData() != nullptr);
            }
        }

        SECTION("UTF-32 string constructor")
        {
            const sf::String string = U"tuv🐌"s;
            CHECK(std::string(string) == "tuv\0"s);
            CHECK(std::wstring(string) == select(L"tuv"s, L"tuv🐌"s));
            CHECK(string.toAnsiString() == "tuv\0"s);
            CHECK(string.toWideString() == select(L"tuv"s, L"tuv🐌"s));
            CHECK(string.toUtf8() == sf::U8String{'t', 'u', 'v', 0xF0, 0x9F, 0x90, 0x8C});
            CHECK(string.toUtf16() == u"tuv🐌"s);
            CHECK(string.toUtf32() == U"tuv🐌"s);
            CHECK(string.getSize() == 4);
            CHECK(!string.isEmpty());
            CHECK(string.getData() != nullptr);
        }
    }

    SECTION("fromUtf8()")
    {
        SECTION("Nominal")
        {
            constexpr std::array<std::uint8_t, 4> characters{'w', 'x', 'y', 'z'};
            const sf::String                      string = sf::String::fromUtf8(characters.begin(), characters.end());
            CHECK(std::string(string) == "wxyz"s);
            CHECK(std::wstring(string) == L"wxyz"s);
            CHECK(string.toAnsiString() == "wxyz"s);
            CHECK(string.toWideString() == L"wxyz"s);
            CHECK(string.toUtf8() == sf::U8String{'w', 'x', 'y', 'z'});
            CHECK(string.toUtf16() == u"wxyz"s);
            CHECK(string.toUtf32() == U"wxyz"s);
            CHECK(string.getSize() == 4);
            CHECK(!string.isEmpty());
            CHECK(string.getData() != nullptr);
        }

        SECTION("Insufficient input")
        {
            constexpr std::array<std::uint8_t, 1> characters{251};
            const sf::String                      string = sf::String::fromUtf8(characters.begin(), characters.end());
            constexpr char32_t                    defaultReplacementCharacter = 0;
            CHECK(string.getSize() == 1);
            CHECK(string[0] == defaultReplacementCharacter);
        }
    }

    SECTION("fromUtf16()")
    {
        constexpr std::array<char16_t, 4> characters{0xF1, 'x', 'y', 'z'};
        const sf::String                  string = sf::String::fromUtf16(characters.begin(), characters.end());
        CHECK(std::string(string) == select("\xF1xyz"s, "\0xyz"s));
        CHECK(std::wstring(string) == L"\xF1xyz"s);
        CHECK(string.toAnsiString() == select("\xF1xyz"s, "\0xyz"s));
        CHECK(string.toWideString() == L"\xF1xyz"s);
        CHECK(string.toUtf8() == sf::U8String{0xC3, 0xB1, 'x', 'y', 'z'});
        CHECK(string.toUtf16() == u"\xF1xyz"s);
        CHECK(string.toUtf32() == U"\xF1xyz"s);
        CHECK(string.getSize() == 4);
        CHECK(!string.isEmpty());
        CHECK(string.getData() != nullptr);
    }

    SECTION("fromUtf32()")
    {
        constexpr std::array<char32_t, 4> characters{'w', U'🐌', 'y', 'z'};
        const sf::String                  string = sf::String::fromUtf32(characters.begin(), characters.end());
        CHECK(std::string(string) == "w\0yz"s);
        CHECK(std::wstring(string) == select(L"wyz"s, L"w🐌yz"s));
        CHECK(string.toAnsiString() == "w\0yz"s);
        CHECK(string.toWideString() == select(L"wyz"s, L"w🐌yz"s));
        CHECK(string.toUtf8() == sf::U8String{'w', 0xF0, 0x9F, 0x90, 0x8C, 'y', 'z'});
        CHECK(string.toUtf16() == u"w🐌yz"s);
        CHECK(string.toUtf32() == U"w🐌yz"s);
        CHECK(string.getSize() == 4);
        CHECK(!string.isEmpty());
        CHECK(string.getData() != nullptr);
    }

    SECTION("clear()")
    {
        sf::String string("you'll never guess what happens when you call clear()");
        string.clear();
        CHECK(string == sf::String());
        CHECK(string.getSize() == 0);
    }

    SECTION("erase()")
    {
        sf::String string("what if i want a shorter string?");
        string.erase(0, 8);
        string.erase(string.getSize() - 1, 1);
        CHECK(string == "i want a shorter string");
        CHECK(string.getSize() == 23);
    }

    SECTION("insert()")
    {
        sf::String string("please insert text");
        string.insert(7, "don't ");
        CHECK(string == "please don't insert text");
        CHECK(string.getSize() == 24);
    }

    SECTION("find()")
    {
        const sf::String string("a little bit of this and a little bit of that");
        CHECK(string.find("a little bit") == 0);
        CHECK(string.find("a little bit", 15) == 25);
        CHECK(string.find("a little bit", 1'000) == sf::String::InvalidPos);
        CHECK(string.find("no way you find this") == sf::String::InvalidPos);
    }

    SECTION("replace()")
    {
        sf::String string("sfml is the worst");
        string.replace(12, 5, "best!");
        CHECK(string == "sfml is the best!");
        string.replace("the", "THE");
        CHECK(string == "sfml is THE best!");
    }

    SECTION("substring()")
    {
        const sf::String string("let's get some substrings");
        CHECK(string.substring(0) == "let's get some substrings");
        CHECK(string.substring(10) == "some substrings");
        CHECK(string.substring(10, 4) == "some");
        CHECK_THROWS_AS((void)string.substring(1'000), std::out_of_range);
        CHECK_THROWS_AS((void)string.substring(420, 69), std::out_of_range);
    }

    SECTION("begin() and end() const")
    {
        const sf::String string("let's test the const iterators");
        CHECK(*string.begin() == 'l');
        CHECK(*(string.end() - 1) == 's');
        for (const auto character : string)
            CHECK(character != 0);
    }

    SECTION("begin() and end()")
    {
        sf::String string("let's test the iterators");
        CHECK(*string.begin() == 'l');
        CHECK(*(string.end() - 1) == 's');
        for (auto& character : string)
            character = 'x';
        CHECK(string == "xxxxxxxxxxxxxxxxxxxxxxxx");
    }

    SECTION("Operators")
    {
        SECTION("operator+=")
        {
            sf::String string;
            string += sf::String("xyz");
            CHECK(string.toAnsiString() == "xyz"s);
        }

        SECTION("operator[] const")
        {
            const sf::String string("the quick brown fox");
            CHECK(string[0] == 't');
            CHECK(string[10] == 'b');
        }

        SECTION("operator[]")
        {
            sf::String string("the quick brown fox");
            CHECK(string[0] == 't');
            string[1] = 'x';
            CHECK(string[1] == 'x');
        }

        SECTION("operator==")
        {
            CHECK(sf::String() == sf::String());
            CHECK_FALSE(sf::String() == sf::String(' '));
        }

        SECTION("operator!=")
        {
            CHECK(sf::String() != sf::String(' '));
            CHECK_FALSE(sf::String() != sf::String());
        }

        SECTION("operator<")
        {
            CHECK(sf::String('a') < sf::String('b'));
            CHECK_FALSE(sf::String() < sf::String());
        }

        SECTION("operator>")
        {
            CHECK(sf::String('b') > sf::String('a'));
            CHECK_FALSE(sf::String() > sf::String());
        }

        SECTION("operator<=")
        {
            CHECK(sf::String() <= sf::String());
            CHECK(sf::String('a') <= sf::String('b'));
        }

        SECTION("operator>=")
        {
            CHECK(sf::String() >= sf::String());
            CHECK(sf::String('b') >= sf::String('a'));
        }

        SECTION("operator+")
        {
            CHECK(sf::String() + sf::String() == sf::String());
            CHECK(sf::String("abc") + sf::String("def") == sf::String("abcdef"));
        }
    }
}
