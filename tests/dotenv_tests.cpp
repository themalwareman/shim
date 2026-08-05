
#include <catch2/catch_amalgamated.hpp>

#include <shim/dotenv.h>

#include <vector>
#include <string>

TEST_CASE("Compatability", "[dotenv][compatability]") {

    SECTION("Env") {
        shm::dotenv dotenv = shm::dotenv::load("..\\..\\tests\\env_files\\.env");

        CHECK(dotenv["BASIC"] == "basic");
        CHECK(dotenv["AFTER_LINE"] == "after_line");
        CHECK(dotenv["EMPTY"] == "");
        CHECK(dotenv["EMPTY_SINGLE_QUOTES"] == "");
        CHECK(dotenv["EMPTY_DOUBLE_QUOTES"] == "");
        CHECK(dotenv["EMPTY_BACKTICKS"] == "");
        CHECK(dotenv["SINGLE_QUOTES"] == "single_quotes");
        CHECK(dotenv["SINGLE_QUOTES_SPACED"] == "    single quotes    ");
        CHECK(dotenv["DOUBLE_QUOTES"] == "double_quotes");
        CHECK(dotenv["DOUBLE_QUOTES_SPACED"] == "    double quotes    ");
        CHECK(dotenv["DOUBLE_QUOTES_INSIDE_SINGLE"] == "double \"quotes\" work inside single quotes");
        CHECK(dotenv["DOUBLE_QUOTES_WITH_NO_SPACE_BRACKET"] == "{ port: $MONGOLAB_PORT}");
        CHECK(dotenv["SINGLE_QUOTES_INSIDE_DOUBLE"] == "single 'quotes' work inside double quotes");
        CHECK(dotenv["BACKTICKS_INSIDE_SINGLE"] == "`backticks` work inside single quotes");
        CHECK(dotenv["BACKTICKS_INSIDE_DOUBLE"] == "`backticks` work inside double quotes");
        CHECK(dotenv["BACKTICKS"] == "backticks");
        CHECK(dotenv["BACKTICKS_SPACED"] == "    backticks    ");
        CHECK(dotenv["DOUBLE_QUOTES_INSIDE_BACKTICKS"] == "double \"quotes\" work inside backticks");
        CHECK(dotenv["SINGLE_QUOTES_INSIDE_BACKTICKS"] == "single 'quotes' work inside backticks");
        CHECK(dotenv["DOUBLE_AND_SINGLE_QUOTES_INSIDE_BACKTICKS"] == "double \"quotes\" and single 'quotes' work inside backticks");
        CHECK(dotenv["EXPAND_NEWLINES"] == "expand\nnew\nlines");
        CHECK(dotenv["DONT_EXPAND_UNQUOTED"] == "dontexpand\\nnewlines");
        CHECK(dotenv["DONT_EXPAND_SQUOTED"] == "dontexpand\\nnewlines");
        CHECK(dotenv["COMMENTS"] != "ignores commented lines");
        CHECK(dotenv["INLINE_COMMENTS"] == "inline comments");
        CHECK(dotenv["INLINE_COMMENTS_SINGLE_QUOTES"] == "inline comments outside of #singlequotes");
        CHECK(dotenv["INLINE_COMMENTS_DOUBLE_QUOTES"] == "inline comments outside of #doublequotes");
        CHECK(dotenv["INLINE_COMMENTS_BACKTICKS"] == "inline comments outside of #backticks");
        CHECK(dotenv["INLINE_COMMENTS_SPACE"] == "inline comments start with a");
        CHECK(dotenv["EQUAL_SIGNS"] == "equals==");
        CHECK(dotenv["RETAIN_INNER_QUOTES"] == "{\"foo\": \"bar\"}");
        CHECK(dotenv["RETAIN_INNER_QUOTES_AS_STRING"] == "{\"foo\": \"bar\"}");
        CHECK(dotenv["RETAIN_INNER_QUOTES_AS_BACKTICKS"] == "{\"foo\": \"bar\'s\"}");
        CHECK(dotenv["TRIM_SPACE_FROM_UNQUOTED"] == "some spaced out string");
        CHECK(dotenv["USERNAME"] == "therealnerdybeast@example.tld");
        CHECK(dotenv["SPACED_KEY"] == "parsed");
        CHECK(dotenv["EXPORT_IS_DECLARED"] == "parsed");
        CHECK(dotenv["EXPORT_IS_DECLARED_WITH_SPACING"] == "parsed");
        CHECK(dotenv["EXPORT_IS_DECLARED_WITH_SOME_VALUE"] == "some_value");
        CHECK(dotenv["EXPORT_IS_DECLARED_WITH_SOME_VALUE_SPACED"] == "some_value");
        CHECK(dotenv["EXPORT_IS_DECLARED_WITH_SOME_VALUE_AND_SPACING"] == "some_value");

        {
            auto tmp = shm::dotenv::parse("BUFFER=true");
            CHECK(tmp["BUFFER"] == "true");
        }

        {
            auto tmp = shm::dotenv::parse("DUP=one\nDUP=two");
            CHECK(tmp["DUP"] == "two");
        }

        {
            auto tmp = shm::dotenv::parse("SERVER=localhost\rPASSWORD=password\rDB=tests\r");
            CHECK(tmp["SERVER"] == "localhost");
            CHECK(tmp["PASSWORD"] == "password");
            CHECK(tmp["DB"] == "tests");
        }

        {
            auto tmp = shm::dotenv::parse("SERVER=localhost\nPASSWORD=password\nDB=tests\n");
            CHECK(tmp["SERVER"] == "localhost");
            CHECK(tmp["PASSWORD"] == "password");
            CHECK(tmp["DB"] == "tests");
        }

        {
            auto tmp = shm::dotenv::parse("SERVER=localhost\r\nPASSWORD=password\r\nDB=tests\r\n");
            CHECK(tmp["SERVER"] == "localhost");
            CHECK(tmp["PASSWORD"] == "password");
            CHECK(tmp["DB"] == "tests");
        }

    }

    SECTION("Multiline") {
        shm::dotenv dotenv = shm::dotenv::load("..\\..\\tests\\env_files\\.env.multiline");

        CHECK(dotenv["BASIC"] == "basic");
        CHECK(dotenv["AFTER_LINE"] == "after_line");
        CHECK(dotenv["EMPTY"] == "");
        CHECK(dotenv["SINGLE_QUOTES"] == "single_quotes");
        CHECK(dotenv["SINGLE_QUOTES_SPACED"] == "    single quotes    ");
        CHECK(dotenv["DOUBLE_QUOTES"] == "double_quotes");
        CHECK(dotenv["DOUBLE_QUOTES_SPACED"] == "    double quotes    ");
        CHECK(dotenv["EXPAND_NEWLINES"] == "expand\nnew\nlines");
        CHECK(dotenv["DONT_EXPAND_UNQUOTED"] == "dontexpand\\nnewlines");
        CHECK(dotenv["DONT_EXPAND_SQUOTED"] == "dontexpand\\nnewlines");
        CHECK(dotenv["COMMENTS"] != "ignores commented lines");
        CHECK(dotenv["EQUAL_SIGNS"] == "equals==");
        CHECK(dotenv["RETAIN_INNER_QUOTES"] == "{\"foo\": \"bar\"}");
        CHECK(dotenv["RETAIN_INNER_QUOTES_AS_STRING"] == "{\"foo\": \"bar\"}");
        CHECK(dotenv["TRIM_SPACE_FROM_UNQUOTED"] == "some spaced out string");
        CHECK(dotenv["USERNAME"] == "therealnerdybeast@example.tld");
        CHECK(dotenv["SPACED_KEY"] == "parsed");
        CHECK(dotenv["MULTI_DOUBLE_QUOTED"] == "THIS\nIS\nA\nMULTILINE\nSTRING");
        CHECK(dotenv["MULTI_SINGLE_QUOTED"] == "THIS\nIS\nA\nMULTILINE\nSTRING");
        CHECK(dotenv["MULTI_BACKTICKED"] == "THIS\nIS\nA\n\"MULTILINE\'S\"\nSTRING");

        CHECK(dotenv["MULTI_PEM_DOUBLE_QUOTED"] == R"(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAnNl1tL3QjKp3DZWM0T3u
LgGJQwu9WqyzHKZ6WIA5T+7zPjO1L8l3S8k8YzBrfH4mqWOD1GBI8Yjq2L1ac3Y/
bTdfHN8CmQr2iDJC0C6zY8YV93oZB3x0zC/LPbRYpF8f6OqX1lZj5vo2zJZy4fI/
kKcI5jHYc8VJq+KCuRZrvn+3V+KuL9tF9v8ZgjF2PZbU+LsCy5Yqg1M8f5Jp5f6V
u4QuUoobAgMBAAE=
-----END PUBLIC KEY-----)");

        CHECK(dotenv["BASIC"] == "basic");

        {
            auto tmp = shm::dotenv::parse("BUFFER=true");
            CHECK(tmp["BUFFER"] == "true");
        }

        {
            auto tmp = shm::dotenv::parse("SERVER=localhost\rPASSWORD=password\rDB=tests\r");
            CHECK(tmp["SERVER"] == "localhost");
            CHECK(tmp["PASSWORD"] == "password");
            CHECK(tmp["DB"] == "tests");
        }

        {
            auto tmp = shm::dotenv::parse("SERVER=localhost\nPASSWORD=password\nDB=tests\n");
            CHECK(tmp["SERVER"] == "localhost");
            CHECK(tmp["PASSWORD"] == "password");
            CHECK(tmp["DB"] == "tests");
        }

        {
            auto tmp = shm::dotenv::parse("SERVER=localhost\r\nPASSWORD=password\r\nDB=tests\r\n");
            CHECK(tmp["SERVER"] == "localhost");
            CHECK(tmp["PASSWORD"] == "password");
            CHECK(tmp["DB"] == "tests");
        }
    }

    SECTION("Bom") {
        shm::dotenv dotenv = shm::dotenv::load("..\\..\\tests\\env_files\\.env.bom");

        CHECK(dotenv["BASIC"] == "basic");
    }

    SECTION("Empty") {
        shm::dotenv dotenv = shm::dotenv::load("..\\..\\tests\\env_files\\.env.empty");

        CHECK(dotenv.empty());
    }
}

TEST_CASE("Static loaders", "[dotenv][static]") {
    /*
        Test Cases:

        - Load from file
    */

    // SECTION("Load")
    // {
    //     shm::dotenv dotenv = shm::dotenv::load("..\\..\\tests\\.env");
    //
    //     CHECK(dotenv["BASIC"] == "win");
    // }
}