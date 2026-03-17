
#include <catch2/catch_amalgamated.hpp>

#include <shim/cli.h>

#include <vector>
#include <string>

/*
    Helper class for simulating argc and argv
*/
class Args {
public:
    Args(std::initializer_list<std::string> args) {
        // Add first arg as program
        _storage.emplace_back("program");
        // Now add given args
        for (auto& a : args) _storage.push_back(a);
        // Now construct pointer array
        for (auto& s : _storage) _args.push_back(s.data());
    }

    int argc() {return static_cast<int>(_storage.size());}
    char** argv() {return _args.data();}
private:
    std::vector<std::string> _storage;
    std::vector<char*> _args;
};

/*
    Catch2 provide the main for us so let's start declareing tests!
 */

/*
    Flag Tests
*/
TEST_CASE("Flags", "[flag]") {

    /*
        Test Cases:

        - Flag defaults to false when not specified
        - Flag defaults to default true when not specified
        - Flag default true can be overridden via --longname=false
        - Flag set explicitly with no value throws i.e. --flag=
        - Flag can be set explicitly via --longname=true
        - Flag can be set explicitly via --longname=false
        - Flag can be set explicitly via case-insensitive --longname=TrUe
        - Flag can be set explicitly via case-insensitive --longname=FALse
        - Parser throws for invalid flag value --longname = maybe
        - Flag can be set to true via short name
        - Flag can be set to true via long name
        - Flag can be accessed from results via short or long name when set by short name
        - Flag can be accessed from results via short or long name when set by long name
        - has() returns true if seen
        - has() returns false if not seen
        - has() throws for unknown flag
        - count() is 0 when not set
        - count() is 1 when set
        - count() tracks repeatable flag via short name i.e. -vvv -> 3
        - count() tracks repeatable flag via long name i.e. --verbose --verbose -> 3
        - count() tracks repeatable flag via long and short name i.e. -vv --verbose -> 3
    */

    // Some helpful re-usable defaults
    Args no_args{};

    SECTION("Flag defaults false") {
        // Cli
        shm::cli cli("test");
        cli.flag("verbose");
        // Parse
        auto result = cli.parse(no_args.argc(), no_args.argv());
        // Verify
        REQUIRE_FALSE(result.get<bool>("verbose"));
    }

    SECTION("Flag defaults true when specified") {
        // Cli
        shm::cli cli("test");
        cli.flag("verbose").defaults(true);
        // Parse
        auto result = cli.parse(no_args.argc(), no_args.argv());
        // Verify
        REQUIRE(result.get<bool>("verbose"));
    }

    SECTION("Flag default true can be overridden explicitly") {
        // Args
        Args args{"--verbose=false"};
        // Cli
        shm::cli cli("test");
        cli.flag("verbose").defaults(true);
        // Parse
        auto result = cli.parse(args.argc(), args.argv());
        // Verify
        REQUIRE_FALSE(result.get<bool>("verbose"));
    }

    SECTION("Flag set explicitly true") {
        // Args
        Args args{"--verbose=true"};
        // Cli
        shm::cli cli("test");
        cli.flag("verbose");
        // Parse
        auto result = cli.parse(args.argc(), args.argv());
        // Verify
        REQUIRE(result.get<bool>("verbose"));
    }

    SECTION("Flag set explicitly false") {
        // Args
        Args args{"--verbose=false"};
        // Cli
        shm::cli cli("test");
        cli.flag("verbose");
        // Parse
        auto result = cli.parse(args.argc(), args.argv());
        // Verify
        REQUIRE_FALSE(result.get<bool>("verbose"));
    }

    SECTION("empty explicit flag throws") {
        Args args{"--count="};
        shm::cli cli("test");
        cli.flag("count");
        REQUIRE_THROWS(cli.parse(args.argc(), args.argv()));
    }

    SECTION("Flag set explicitly true case-insensitive") {
        // Args
        Args args{"--verbose=TrUe"};
        // Cli
        shm::cli cli("test");
        cli.flag("verbose");
        // Parse
        auto result = cli.parse(args.argc(), args.argv());
        // Verify
        REQUIRE(result.get<bool>("verbose"));
    }

    SECTION("Flag set explicitly false case-insensitive") {
        // Args
        Args args{"--verbose=FALse"};
        // Cli
        shm::cli cli("test");
        cli.flag("verbose");
        // Parse
        auto result = cli.parse(args.argc(), args.argv());
        // Verify
        REQUIRE_FALSE(result.get<bool>("verbose"));
    }

    SECTION("Parser throws for invalid explicit value") {
        // Args
        Args args{"--verbose=maybe"};
        // Cli
        shm::cli cli("test");
        cli.flag("verbose");
        // Parse
        REQUIRE_THROWS(cli.parse(args.argc(), args.argv()));
    }

    SECTION("Flag set via short name") {
        // Args
        Args args{"-v"};
        // Cli
        shm::cli cli("test");
        cli.flag('v', "verbose");
        // Parse
        auto result = cli.parse(args.argc(), args.argv());
        // Verify
        REQUIRE(result.get<bool>("verbose"));
    }

    SECTION("Flag set via long name") {
        // Args
        Args args{"--verbose"};
        // Cli
        shm::cli cli("test");
        cli.flag("verbose");
        // Parse
        auto result = cli.parse(args.argc(), args.argv());
        // Verify
        REQUIRE(result.get<bool>("verbose"));
    }

    SECTION("Flag set via short accessible from results via either name") {
        // Args
        Args args{"-v"};
        // Cli
        shm::cli cli("test");
        cli.flag('v', "verbose");
        // Parse
        auto result = cli.parse(args.argc(), args.argv());
        // Verify
        REQUIRE(result.get<bool>("v"));
        REQUIRE(result.get<bool>("verbose"));
    }

    SECTION("Flag set via long accessible from results via either name") {
        // Args
        Args args{"--verbose"};
        // Cli
        shm::cli cli("test");
        cli.flag('v', "verbose");
        // Parse
        auto result = cli.parse(args.argc(), args.argv());
        // Verify
        REQUIRE(result.get<bool>("v"));
        REQUIRE(result.get<bool>("verbose"));
    }

    SECTION("has() returns true if seen") {
        // Args
        Args args{"--verbose"};
        // Cli
        shm::cli cli("test");
        cli.flag("verbose");
        // Parse
        auto result = cli.parse(args.argc(), args.argv());
        // Verify
        REQUIRE(result.specified("verbose"));
    }

    SECTION("has() returns false if not seen") {
        // Cli
        shm::cli cli("test");
        cli.flag("verbose");
        // Parse
        auto result = cli.parse(no_args.argc(), no_args.argv());
        // Verify
        REQUIRE_FALSE(result.specified("verbose"));
    }

    SECTION("has() throws for unknown flag") {
        // Cli
        shm::cli cli("test");
        cli.flag("verbose");
        // Parse
        auto result = cli.parse(no_args.argc(), no_args.argv());
        // Verify
        REQUIRE_THROWS(result.specified("verbosity"));
    }

    SECTION("count() is 0 when not set") {
        // Cli
        shm::cli cli("test");
        cli.flag("verbose");
        // Parse
        auto result = cli.parse(no_args.argc(), no_args.argv());
        // Verify
        REQUIRE(0 == result.count("verbose"));
    }

    SECTION("count() is 1 when set") {
        // Args
        Args args{"--verbose"};
        // Cli
        shm::cli cli("test");
        cli.flag("verbose");
        // Parse
        auto result = cli.parse(args.argc(), args.argv());
        // Verify
        REQUIRE(1 == result.count("verbose"));
    }

    SECTION("count() tracks repeatable short name") {
        // Args
        Args args{"-vvv"};
        // Cli
        shm::cli cli("test");
        cli.flag('v', "verbose").repeatable();
        // Parse
        auto result = cli.parse(args.argc(), args.argv());
        // Verify
        REQUIRE(3 == result.count("verbose"));
    }

    SECTION("count() tracks repeatable long name") {
        // Args
        Args args{"--verbose", "--verbose", "--verbose"};
        // Cli
        shm::cli cli("test");
        cli.flag("verbose").repeatable();
        // Parse
        auto result = cli.parse(args.argc(), args.argv());
        // Verify
        REQUIRE(3 == result.count("verbose"));
    }

    SECTION("count() tracks repeatable flag via long and short name") {
        // Args
        Args args{"-vv", "--verbose"};
        // Cli
        shm::cli cli("test");
        cli.flag('v', "verbose").repeatable();
        // Parse
        auto result = cli.parse(args.argc(), args.argv());
        // Verify
        REQUIRE(3 == result.count("verbose"));
    }
}

/*
    Flag Spec Tests
*/
TEST_CASE("Flag Spec Validation", "[flag]") {

    /*
        Test Cases:

        - Shortname cannot be space or dash
        - Longname cannot be empty
        - Longname cannot be single character
        - Shortname cannot be duplicate
        - Longname cannot be duplicate
        - Default can only be set once
    */

    SECTION("short name cannot be space, dash or other special char") {
        shm::cli cli("test");
        REQUIRE_THROWS(cli.flag('-', "verbose"));
        REQUIRE_THROWS(cli.flag(' ', "verbose"));
        REQUIRE_THROWS(cli.flag('\t', "verbose"));
        REQUIRE_THROWS(cli.flag('\n', "verbose"));
    }

    SECTION("long name cannot be empty") {
        shm::cli cli("test");
        REQUIRE_THROWS(cli.flag(""));
    }

    SECTION("long name cannot be single character") {
        shm::cli cli("test");
        REQUIRE_THROWS(cli.flag("v"));
    }

    SECTION("short name cannot be duplicate") {
        shm::cli cli("test");
        cli.flag('v', "verbose");
        REQUIRE_THROWS(cli.flag('v', "verbosity"));
    }

    SECTION("long name cannot be duplicate") {
        shm::cli cli("test");
        cli.flag("verbose");
        REQUIRE_THROWS(cli.flag("verbose"));
    }

    SECTION("default can only be set once") {
        shm::cli cli("test");
        auto& f = cli.flag("verbose").defaults(true);
        REQUIRE_THROWS(f.defaults(false));
    }
}

/*
    Option Tests
*/

TEST_CASE("Options", "[option]") {

    /*
        Test Cases:

        - Option can be set via --longname
        - Option can be set explicitly via --longname=option
        - Option can be set via short name -o file.txt
        - Option can be set via short name unix style i.e. -ofile.txt
        - Option default value returned when not set
        - Option default overridden via cli
        - has() returns false when not seen
        - has() returns true when seen
        - has() returns false when not seen even if default supplied
        - has() throws for unknown option
        - required option not set throws at parse time
        - required option set doesn't throw
        - Option can be accessed via short or long name when set by short name
        - Option can be accessed via short or long name when set by long name
        - Option get<T>() throws if not set and no default
        - Option get<T>() throws for unknown option
        - count() is 0 when not set
        - count() is 1 when set
        - Missing value for option throws
        - get works for parseable type
        - get throws for non-parseable type
        - empty explicit option throws
        - Option get<bool>() throws
    */

    SECTION("Option can be set via longname") {
        Args args{"--output", "file.txt"};
        shm::cli cli("test");
        cli.option("output");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<std::string>("output") == "file.txt");
    }

    SECTION("Option can be set explicitly via longname") {
        Args args{"--output=file.txt"};
        shm::cli cli("test");
        cli.option("output");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<std::string>("output") == "file.txt");
    }

    SECTION("Option can be set via short name") {
        Args args{"-o", "file.txt"};
        shm::cli cli("test");
        cli.option('o', "output");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<std::string>("output") == "file.txt");
    }

    SECTION("Option can be set via short name unix style") {
        Args args{"-ofile.txt"};
        shm::cli cli("test");
        cli.option('o', "output");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<std::string>("output") == "file.txt");
    }

    SECTION("Option default value returned when not set") {
        Args args{};
        shm::cli cli("test");
        cli.option("output").defaults("stdout");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<std::string>("output") == "stdout");
    }

    SECTION("Option default overridden via cli") {
        Args args{"--output", "file.txt"};
        shm::cli cli("test");
        cli.option("output").defaults(std::string("stdout"));
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<std::string>("output") == "file.txt");
    }

    SECTION("has() returns false when not seen") {
        Args args{};
        shm::cli cli("test");
        cli.option("output");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE_FALSE(result.specified("output"));
    }

    SECTION("has() returns true when seen") {
        Args args{"--output", "file.txt"};
        shm::cli cli("test");
        cli.option("output");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.specified("output"));
    }

    SECTION("has() returns false when not seen even if default supplied") {
        Args args{};
        shm::cli cli("test");
        cli.option("output").defaults(std::string("stdout"));
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE_FALSE(result.specified("output"));
    }

    SECTION("has() throws for unknown option") {
        Args args{};
        shm::cli cli("test");
        cli.option("output").defaults(std::string("stdout"));
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE_THROWS(result.specified("wrong"));
    }

    SECTION("required option not set throws at parse time") {
        Args args{};
        shm::cli cli("test");
        cli.option("output").required();
        REQUIRE_THROWS(cli.parse(args.argc(), args.argv()));
    }

    SECTION("required option set doesn't throw") {
        Args args{"--output", "file.txt"};
        shm::cli cli("test");
        cli.option("output").required();
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<std::string>("output") == "file.txt");
    }

    SECTION("Option can be accessed via short or long name when set by short name") {
        Args args{"-o", "file.txt"};
        shm::cli cli("test");
        cli.option('o', "output");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<std::string>("o") == "file.txt");
        REQUIRE(result.get<std::string>("output") == "file.txt");
    }

    SECTION("Option can be accessed via short or long name when set by long name") {
        Args args{"--output", "file.txt"};
        shm::cli cli("test");
        cli.option('o', "output");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<std::string>("o") == "file.txt");
        REQUIRE(result.get<std::string>("output") == "file.txt");
    }

    SECTION("Option get<T>() throws if not set and no default") {
        Args args{};
        shm::cli cli("test");
        cli.option("count");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE_THROWS(result.get<int>("count"));
    }

    SECTION("Option get<T>() throws for unknown option") {
        Args args{};
        shm::cli cli("test");
        cli.option("count");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE_THROWS(result.get<int>("wrong"));
    }

    SECTION("count() is 0 when not set") {
        Args args{};
        shm::cli cli("test");
        cli.option("counter");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.count("counter") == 0);
    }

    SECTION("count() is 1 when set") {
        Args args{"--counter", "1"};
        shm::cli cli("test");
        cli.option("counter");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.count("counter") == 1);
    }

    SECTION("Missing value for option throws") {
        Args args{"--output"};
        shm::cli cli("test");
        cli.option("output");
        REQUIRE_THROWS(cli.parse(args.argc(), args.argv()));
    }

    SECTION("get works for parseable type") {
        Args args{"--count", "42"};
        shm::cli cli("test");
        cli.option("count");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<int>("count") == 42);
    }

    SECTION("get throws for non-parseable type") {
        Args args{"--count", "twelve"};
        shm::cli cli("test");
        cli.option("count");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE_THROWS(result.get<int>("count"));
    }

    SECTION("empty explicit option throws") {
        Args args{"--count="};
        shm::cli cli("test");
        cli.option("count");
        REQUIRE_THROWS(cli.parse(args.argc(), args.argv()));
    }

    SECTION("Option get<bool>() throws") {
        Args args{"--verbose", "true"};
        shm::cli cli("test");
        cli.option("verbose");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE_THROWS(result.get<bool>("verbose"));
    }
}

/*
    Repeatable Option Tests
*/

TEST_CASE("Repeatable options", "[option]") {
    /*
        Test Cases:

        - Repeatable options all collected
        - count() tracks repeated option i.e. -o file1.txt -o file2.txt
        - Non-repeatable option specified twice fails
        - get<vector<T>>() throws for non-repeatable option
        - get<T>() throws for repeatable option
        - get<vector<T>>() returns empty vector if not seen
    */

    SECTION("Repeatable options all collected") {
        Args args{"--file", "a.txt", "--file", "b.txt", "--file", "c.txt"};
        shm::cli cli("test");
        cli.option("file").repeatable();
        auto result = cli.parse(args.argc(), args.argv());
        auto files = result.get<std::vector<std::string>>("file");
        REQUIRE(files.size() == 3);
        REQUIRE(files[0] == "a.txt");
        REQUIRE(files[1] == "b.txt");
        REQUIRE(files[2] == "c.txt");
    }

    SECTION("count() tracks repeated option") {
        Args args{"--file", "a.txt", "--file", "b.txt"};
        shm::cli cli("test");
        cli.option("file").repeatable();
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.count("file") == 2);
    }

    SECTION("Non-repeatable option specified twice fails") {
        Args args{"--output", "a.txt", "--output", "b.txt"};
        shm::cli cli("test");
        cli.option("output");
        REQUIRE_THROWS(cli.parse(args.argc(), args.argv()));
    }

    SECTION("get<vector<T>>() throws for non-repeatable option") {
        Args args{"--output", "a.txt"};
        shm::cli cli("test");
        cli.option("output");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE_THROWS(result.get<std::vector<std::string>>("output"));
    }

    SECTION("get<T>() throws for repeatable option") {
        Args args{"--file", "a.txt"};
        shm::cli cli("test");
        cli.option("file").repeatable();
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE_THROWS(result.get<std::string>("file"));
    }

    SECTION("get<vector<T>>() returns empty vector if not seen") {
        Args args{};
        shm::cli cli("test");
        cli.option("file").repeatable();
        auto result = cli.parse(args.argc(), args.argv());
        auto files = result.get<std::vector<std::string>>("file");
        REQUIRE(files.empty());
    }
}


/*
    Option Expectation & Validation Tests
*/

TEST_CASE("Option expectation and validation", "[option]") {
    /*
        Test Cases:

        - expect<int> fails at parse time with string
        - expect<int> succeeds for valid value
        - expect<float> fails at parse time with string
        - expect<float> succeeds for valid value
        - multiple expectations fails
        - successful validator passes
        - failed validator throws
        - multiple validators pass
        - one failed validator throws
        - string validator
        - validator can throw its own exception
    */

    SECTION("expect<int> fails at parse time with string") {
        Args args{"--count", "abc"};
        shm::cli cli("test");
        cli.option("count").expect<int>();
        REQUIRE_THROWS(cli.parse(args.argc(), args.argv()));
    }

    SECTION("expect<int> succeeds for valid value") {
        Args args{"--count", "42"};
        shm::cli cli("test");
        cli.option("count").expect<int>();
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<int>("count") == 42);
    }

    SECTION("expect<float> fails at parse time with string") {
        Args args{"--count", "123 foo"};
        shm::cli cli("test");
        cli.option("count").expect<float>();
        REQUIRE_THROWS(cli.parse(args.argc(), args.argv()));
    }

    SECTION("expect<float> succeeds for valid value") {
        Args args{"--count", "1.2345"};
        shm::cli cli("test");
        cli.option("count").expect<float>();
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(std::abs(result.get<float>("count") - 1.2345) < 1e-6f);
    }

    SECTION("expect<double> fails at parse time with string") {
        Args args{"--count", "123.45 foo"};
        shm::cli cli("test");
        cli.option("count").expect<double>();
        REQUIRE_THROWS(cli.parse(args.argc(), args.argv()));
    }

    SECTION("expect<double> succeeds for valid value") {
        Args args{"--count", "1.2345"};
        shm::cli cli("test");
        cli.option("count").expect<double>();
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(std::abs(result.get<double>("count") - 1.2345) < 1e-6f);
    }

    SECTION("multiple expectations fails") {
        shm::cli cli("test");
        auto& o = cli.option("count").expect<int>();
        REQUIRE_THROWS(o.expect<std::string>());
    }

    SECTION("successful validator passes") {
        Args args{"--count", "5"};
        shm::cli cli("test");
        cli.option("count").validator([](int v) -> bool { return v > 0; });
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<int>("count") == 5);
    }

    SECTION("failed validator throws") {
        Args args{"--count", "-1"};
        shm::cli cli("test");
        cli.option("count").validator([](int v) { return v > 0; });
        REQUIRE_THROWS(cli.parse(args.argc(), args.argv()));
    }

    SECTION("multiple validators pass") {
        Args args{"--count", "5"};
        shm::cli cli("test");
        cli.option("count").validator([](int v) -> bool { return v > 0; }).validator([](int v) -> bool { return v < 10; });
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<int>("count") == 5);
    }

    SECTION("one failed validator throws") {
        Args args{"--count", "5"};
        shm::cli cli("test");
        cli.option("count").validator([](int v) -> bool { return v > 0; }).validator([](int v) -> bool { return v < 4; });
        REQUIRE_THROWS(cli.parse(args.argc(), args.argv()));
    }

    SECTION("string validator") {
        Args args{"--mode", "extra"};
        shm::cli cli("test");
        cli.option("mode").validator([](std::string v) -> bool { return v == "extra"; });
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<std::string>("mode") == "extra");
    }

    SECTION("validator can throw its own exception") {
        Args args{"--port", "1234"};
        shm::cli cli("test");
        cli.option("port").validator([](int port) -> bool { throw std::runtime_error("Invalid port"); });

        REQUIRE_THROWS_AS(cli.parse(args.argc(), args.argv()), std::runtime_error);

        shm::cli cli2("test");
        cli2.option("port").validator([](int port) -> bool { throw std::runtime_error("Invalid port"); });

        CHECK_THROWS_WITH(cli2.parse(args.argc(), args.argv()), "Invalid port");
    }
}

/*
    Option Spec Validation
*/

TEST_CASE("Option spec validation", "[option]") {

    /*
        Test Cases:

        - Shortname cannot be space or dash
        - Longname cannot be empty
        - Longname cannot be single character
        - Shortname cannot be duplicate
        - Longname cannot be duplicate
        - Default can only be set once
    */

    SECTION("short name cannot be space, dash or other special char") {
        shm::cli cli("test");
        REQUIRE_THROWS(cli.option('-', "verbose"));
        REQUIRE_THROWS(cli.option(' ', "verbose"));
        REQUIRE_THROWS(cli.option('\t', "verbose"));
        REQUIRE_THROWS(cli.option('\n', "verbose"));
    }

    SECTION("long name cannot be empty") {
        shm::cli cli("test");
        REQUIRE_THROWS(cli.option(""));
    }

    SECTION("long name cannot be single character") {
        shm::cli cli("test");
        REQUIRE_THROWS(cli.option("v"));
    }

    SECTION("short name cannot be duplicate") {
        shm::cli cli("test");
        cli.option('v', "verbose");
        REQUIRE_THROWS(cli.option('v', "verbosity"));
    }

    SECTION("long name cannot be duplicate") {
        shm::cli cli("test");
        cli.option("verbose");
        REQUIRE_THROWS(cli.option("verbose"));
    }

    SECTION("default can only be set once") {
        shm::cli cli("test");
        auto& f = cli.option("key").defaults("value");
        REQUIRE_THROWS(f.defaults("alternate"));
    }
}

/*
    Option Packs
*/

TEST_CASE("Short flag & option packs", "[short]") {
    /*
        Test Cases:

        - Multiple flags in a single pack
        - Multiple flags followed by option
        - Option in middle of pack throws
    */

    SECTION("Multiple flags in a single pack") {
        Args args{"-vxz"};
        shm::cli cli("test");
        cli.flag('v', "verbose");
        cli.flag('x', "extract");
        cli.flag('z', "compress");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<bool>("verbose"));
        REQUIRE(result.get<bool>("extract"));
        REQUIRE(result.get<bool>("compress"));
    }

    SECTION("Multiple flags followed by option") {
        Args args{"-vxo", "file.txt"};
        shm::cli cli("test");
        cli.flag('v', "verbose");
        cli.flag('x', "extract");
        cli.option('o', "output");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<bool>("verbose"));
        REQUIRE(result.get<bool>("extract"));
        REQUIRE(result.get<std::string>("output") == "file.txt");
    }

    SECTION("option in middle of pack throws") {
        Args args{"-vof"};
        shm::cli cli("test");
        cli.flag('v', "verbose");
        cli.option('o', "output");
        cli.flag('f', "force");
        REQUIRE_THROWS(cli.parse(args.argc(), args.argv()));
    }
}

/*
    Positional Argument Tests
*/

TEST_CASE("Positional arguments", "[positional]") {

    /*
        Test Cases:

        - Single positional arg
        - Multiple positional args
        - Positional arg with flags and options
        - Missing positional arg throws
        - Too many positional args throws
        - Positional after end of options -- marker
        - Too many positionals after end of options -- marker throws
        - Filename with - after end of options marker
        - Positional expectation

    */

    SECTION("Single positional arg") {
        Args args{"input.txt"};
        shm::cli cli("test");
        cli.positional("input");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<std::string>("input") == "input.txt");
    }

    SECTION(" Multiple positional args") {
        Args args{"input.txt", "output.txt"};
        shm::cli cli("test");
        cli.positional("input");
        cli.positional("output");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<std::string>("input") == "input.txt");
        REQUIRE(result.get<std::string>("output") == "output.txt");
    }

    SECTION("Positional arg with flags and options") {
        Args args{"--verbose", "-o", "output.txt", "input.txt"};
        shm::cli cli("test");
        cli.flag("verbose");
        cli.option('o', "output");
        cli.positional("input");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<bool>("verbose"));
        REQUIRE(result.get<std::string>("o") == "output.txt");
        REQUIRE(result.get<std::string>("input") == "input.txt");
    }

    SECTION("Missing positional arg throws") {
        Args args{};
        shm::cli cli("test");
        cli.positional("input");
        REQUIRE_THROWS(cli.parse(args.argc(), args.argv()));
    }

    SECTION("Too many positional args throws") {
        Args args{"a.txt", "b.txt"};
        shm::cli cli("test");
        cli.positional("input");
        REQUIRE_THROWS(cli.parse(args.argc(), args.argv()));
    }

    SECTION("Positional after end of options -- marker") {
        Args args{"--", "input.txt"};
        shm::cli cli("test");
        cli.positional("input");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<std::string>("input") == "input.txt");
    }

    SECTION("Too many positionals after end of options -- marker throws") {
        Args args{"--", "input.txt", "output.txt"};
        shm::cli cli("test");
        cli.positional("input");
        REQUIRE_THROWS(cli.parse(args.argc(), args.argv()));
    }

    SECTION("Filename with - after end of options marker") {
        Args args{"--", "-weirdfile.txt"};
        shm::cli cli("test");
        cli.positional("input");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<std::string>("input") == "-weirdfile.txt");
    }

    SECTION("Positional expectation") {
        Args args{"5", "hello"};
        shm::cli cli("test");
        cli.positional("count").expect<int>();
        cli.positional("word").expect<std::string>();
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<int>("count") == 5);
        REQUIRE(result.get<std::string>("word") == "hello");
    }

}

/*
    Positional Argument Spec Validation
*/

TEST_CASE("Positional Argument Spec Validation", "[positional]") {

    /*
        Test Cases:

        - Cannot mix positionals and subcommands
        - Duplicate positional name throws
    */

    SECTION("Cannot mix positionals and subcommands") {
        shm::cli cli("test");
        cli.positional("input");
        REQUIRE_THROWS(cli.subcommand("add"));
    }

    SECTION("Cannot mix positionals and subcommands") {
        shm::cli cli("test");
        auto& sub = cli.subcommand("add");
        REQUIRE_THROWS(cli.positional("input"));
    }

    SECTION("Duplicate positional name throws") {
        shm::cli cli("test");
        cli.positional("input");
        REQUIRE_THROWS(cli.positional("input"));
    }
}

/*
    Subcommand Tests
*/

TEST_CASE("Subcommands", "[subcommand]") {

    /*
        Test Cases:

        - No subcommand specified
        - Subcommand specified
        - Check which subcommand was specified
        - Get specified subcommand name
        - Check subcommand is specified
        - Get throws if no subcommand specified
        - Access arguments of subcommand
        - Access wrong subcommand name throws
        - Access subcommand when not set throws
        - Subcommand arguments not shadowed by parents
        - Unknown subcommand throws
    */

    SECTION("No subcommand specified") {
        Args args{};
        shm::cli cli("test");
        auto& sub = cli.subcommand("add");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE_FALSE(result.has_subcommand());
    }

    SECTION("Subcommand specified") {
        Args args{"add"};
        shm::cli cli("test");
        auto& sub = cli.subcommand("add");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.has_subcommand());
    }

    SECTION("Check which subcommand was specified") {
        Args args{"add"};
        shm::cli cli("test");
        auto& sub_add =cli.subcommand("add");
        auto& sub_remove =cli.subcommand("remove");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.subcommand() == "add");
        REQUIRE_FALSE(result.subcommand() == "remove");
    }

    SECTION("Get specified subcommand name") {
        Args args{"add"};
        shm::cli cli("test");
        auto& sub = cli.subcommand("add");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.subcommand() == "add");
    }

    SECTION("Check subcommand is specified") {
        Args args{"add"};
        shm::cli cli("test");
        auto& sub = cli.subcommand("add");
        auto& sub2 = cli.subcommand("remove");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.specified("add"));
        REQUIRE_FALSE(result.specified("remove"));
    }

    SECTION("Get throws if no subcommand specified") {
        Args args{};
        shm::cli cli("test");
        auto& sub = cli.subcommand("add");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE_THROWS(result.subcommand());
    }

    SECTION("Access arguments of subcommand") {
        Args args{"add", "--verbose"};
        shm::cli cli("test");
        auto& add = cli.subcommand("add");
        add.flag("verbose");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result["add"].get<bool>("verbose"));
    }

    SECTION("Access wrong subcommand throws") {
        Args args{"add"};
        shm::cli cli("test");
        auto& sub = cli.subcommand("add");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE_THROWS(result["remove"]);
    }

    SECTION("Access subcommand when not set throws") {
        Args args{};
        shm::cli cli("test");
        auto& sub = cli.subcommand("add");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE_THROWS(result["add"]);
    }

    SECTION("Subcommand arguments not shadowed by parents") {
        Args args{"--verbose=false", "add", "--verbose"};
        shm::cli cli("test");
        cli.flag("verbose");
        auto& add = cli.subcommand("add");
        add.flag("verbose");
        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE_FALSE(result.get<bool>("verbose"));
        REQUIRE(result["add"].get<bool>("verbose"));
    }

    SECTION("Unknown subcommand throws") {
        Args args{"unknown"};
        shm::cli cli("test");
        auto& sub = cli.subcommand("add");
        REQUIRE_THROWS(cli.parse(args.argc(), args.argv()));
    }
}

/*
    Subcommand Spec Validation
*/

TEST_CASE("Subcommand Spec Validation", "[subcommand]") {

    /*
        Test Cases:

        - Duplicate subcommand throws
    */

    SECTION("Duplicate subcommand throws") {
        shm::cli cli("test");
        auto& sub = cli.subcommand("add");
        REQUIRE_THROWS(cli.subcommand("add"));
    }
}

/*
    Combined tests
*/

TEST_CASE("Combined features", "[combined]") {

    /*
        Test Cases:

        - Flag, required option, repeatable option & positional arg
        - Parent flag and subcommand option
    */

    SECTION("Flag, required option, repeatable option & positional arg") {
        Args args{"-v", "--output=result.txt", "--file", "a.txt", "--file", "b.txt", "input.txt"};
        shm::cli cli("test");
        cli.flag('v', "verbose");
        cli.option('o', "output").required();
        cli.option("file").repeatable();
        cli.positional("input");
        auto result = cli.parse(args.argc(), args.argv());

        REQUIRE(result.get<bool>("verbose"));
        REQUIRE(result.get<std::string>("output") == "result.txt");
        auto files = result.get<std::vector<std::string>>("file");
        REQUIRE(files.size() == 2);
        REQUIRE(files[0] == "a.txt");
        REQUIRE(files[1] == "b.txt");
        REQUIRE(result.get<std::string>("input") == "input.txt");
    }

    SECTION("Parent flag and subcommand option") {
        Args args{"--verbose", "add", "-i", "file.txt"};
        shm::cli cli("test");
        cli.flag("verbose");
        auto& add = cli.subcommand("add");
        add.option('i', "input").required();

        auto result = cli.parse(args.argc(), args.argv());
        REQUIRE(result.get<bool>("verbose"));
        REQUIRE(result.subcommand() == "add");
        REQUIRE(result["add"].get<std::string>("input") == "file.txt");
    }

}
