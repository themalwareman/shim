# shm::cli

## :bookmark: Overview

`shm::cli` is a lightweight, modern C++ command-line with a fluent API. The core design of cli can be summarized as text-first. CLI values are always text, so rather than forcing you to declare types for everything up front, `shm::cli` stores everything strings and  lets you declare what type you want at lookup time. If you want type safety at parse-time, you can easily opt in with expectations.

---

## :rocket: Quick Example

```cpp

#include "shim/cli.h"

int main(int argc, char* argv[]) {

    shm::cli cli("app", "Does something useful");
    cli.version("1.0.0")

    cli.flag('v', "verbose").help("Enable verbose output").repeatable();

    auto add = cli.subcommand("add", "Add something");
    add.option('i', "item").required().help("The item to add");

    auto result = cli.parse(argc, argv);

    if (result.get<bool>("verbose")) {
        // Verbose mode
    }

    if (result.has_subcommand() && result.subcommand() == "add") {
        auto item = result["add"].get<std::string>("item");
    }
}
```

---

## :package: API


### Building the spec

`shm::cli` is the top-level object. It owns the entire cli spec and handles parsing.

```cpp
shm::cli cli("name");
shm::cli cli("name", "description");
```

`cli` is non-copyable and non-movable, construct it once and keep it in scope. The name and description provided here will show up in the cli help output. Not that whilst building the spec both `std::logic_error` and `std::invalid_argument` may be thrown if the contract is violated or invalid values are given.

---

### Flags

Flags are boolean arguments, their presence on the command line implicitly means `true`. They can also be set explicitly via `--flag=true` or `--flag=false`, which is essential in case you need to override a flag that's been given a default of `true`. The point of flags over using options is to prevent a user having to explicitly specify the value i.e. `--flag` instead of `--flag true` which changes the parsing semantics.

```cpp
cli.flag("verbose");
cli.flag('v', "verbose");
```

Flags can be given either a long name or both a short name and a long name. I decided against short name only declarations because I thought it led to poorly documented clis.

#### Fluent Configuration

Flags also come with the following fluent configuration:

```cpp
help(std::string_view help)
repeatable(bool repeatable = true)
defaults(bool value)
```

i.e. 

```cpp
cli.flag('v', "verbose")
    .help("The help text for the flag")
    .repeatable() // Allows a flag to be repeated on the command line, think -vvv
    .defaults(true); // All flags default to false unless specified
```

Note you can also use `.repeatable(false)` & `.defaults(false)` which whilst it is already the default behavior, allows you to be truly explicit in your cli spec. Making a flag repeatable allows it to be defined multiple times. It's final value will be the result of parsing its last occurrence in the command line, however the point of making a flag repeatable is to allow you to later query the count. i.e. using a flag for verbosity and making it repeatable allows `-vvv` on the command line which produces a count of 3. This would allow you to give the user more granular control over the verbosity level.

#### Parsing Behavior

| Input             | Result |
|-------------------|-----------------------------|
| `--verbose`       | `true`                      |
| `--verbose=true`  | `true`                      |
| `--verbose=false` | `false` (explicit override) |
| `-v`              | `true`                      |

---

### Options

Options, unlike flags, take a value. The value can be anything and is stored as a string. It is parsed to the requested type at retrieval time unless expectations are set or validators are used.

```cpp
cli.flag("output");
cli.flag('o', "output");
```

For the same reason as flags there is no short name only option. Options may also be set using the syntax `--option=value`

#### Fluent Configuration

```cpp
help(std::string_view help)
repeatable(bool repeatable = true)
required(bool required = true)

template<typename T>
requires ...
defaults(T value)

template <typename T>
requires ...
expect()

template <typename F>
requires ...
validator(F&& validator)
```

i.e.

```cpp
cli.option("port")
    .required()
    .repeatable(false) // This is the default but you're allowed to be explicit
    .expect<int>()
    .validator([](int p) { return 0 < p && p <= 65535; });
```

There's a few things to unpack here. The fluent apis `help()` & `repeatable()` work the same as for flags, they let you set the help text and indicate if an option is allowed to be specified multiple times or not. The next few functions are worth diving into.

#### `defaults(T value)`

Given our cli is text first, we allow you to pass any type to defaults that satisfies (among a few other restraints) the concept:

```cpp
template<typename T>
concept stringable = requires(std::ostringstream& s, T& v) {s << v;};
```

which simply means as long as there exists a stringstream operator, such that we can shove the provided type into a stringstream to convert it to a string, then you can set that as the default value.

So going back to our port example:

```cpp
cli.option("port").defaults(1234);
```

This works because int has a stringstream operator, which allows us to store the cli default as the string "1234". Remember, we're a text first cli. You'll be able to convert it back to an int later when you pull it out of the parse_result.

#### `expect<T>()`

Given the cli design is text-first, you initially lose the ability to guarantee that an option is convertible to the requested type later down the line. i.e. `--port hi` would initially be valid, so later when you try to extract `hi` as an `int`, everything blows up. That's where expectations come in. You can provide an expected type, and at parse-time when we encounter the value, we'll check the conversion is valid, and throw if its not. This allows:

```cpp
cli.option("port").expect<int>()
```

Which guarantees later on, that extracting the port option as an int will be valid and safe. So what's the catch? Not much, just a concept that needs to be satisfied:

```cpp
template <typename T>
concept parseable = requires(std::istringstream& s, T& v) { s >> v; };
```

For us to verify the conversion to the expected type, we need there to exist an `istringstream` operator that allows us to convert the string to the expected type. The standard provides this for most of the core types such as `float` & `int` etc. Although its worth noting our parsing to `int`, `float` & `double` is a bit stronger than the standard `istringstream` operator.

#### `validator(F&& validator)`

The validator api allows you to go beyond a type expectation, it allows you to provide a unary callable predicate (a callable function that takes one argument and returns a bool), to indicate whether or not the given value is acceptable. For example see the validator lambda above for port, it guarantees that the int given on the command line falls within the valid range of port numbers.

We use some cool templates to inspect the unary callable such that, the argument the callable takes must satisfy the `parseable` concept, just like `expect<T>()`. This guarantees that we can attempt to convert the text on the cli into the type expected by your callable. Note that this is essentially doing an implicit `expect<T>()`. On top of that we guarantee that you only take 1 argument, and also return a bool indicating if the value is acceptable or not.

You may throw an exception from your validator yourself which we'll let through, but if you return false we'll throw an exception for you anyway.

Failed expectations or validations will throw a `std::runtime_error`.

Note that multiple validators may be specified however only one expectation is allowed.

---

### Short Flag/Option Packs

- Multiple flags may be given at the same time
    - `-abc` is equivalent to `-a -b -c`
- Options may be specified unix style
    - `-ffilename` is equivalent to `-f filename`
- Options are allowed at the end of a short name option pack
    - `-abcf filename` is equivalent to `-a -b -c -f filename`
    - Note `-abcffilename` is disallowed as it leads to ambiguity

---

### Positionals

Positional arguments are matched by their position in the argument list, in the order they were registered. They are always required.

```cpp
shm::cli cli("transformer", "transforms input to output");
cli.positional("in");
cli.positional("out");
```

The above spec indicates you would like a cli of the form:

`transformer.exe <in> <out>`

Positional names must not clash with flags or options as they are pulled from the parse_result by name rather than by index. To avoid a positional argument being mistaken for an option you can use the end of options marker `--` to force all remaining tokens to be treated as positional arguments. For example if your filename began with a hyphen you would need:

`transformer.exe -- -myfilein myfile.out`

Without the `--` marker, the parser would assume `-myfilein` is a short option pack.


#### Fluent Configuration

Just like options, positional arguments can be given expectations and validators. i.e.

```cpp
cli.positional("probability").validator([](float r) { return 0 <= r && r <= 1;});
```

Remember using a validator that expects a float implicitly means the same as `.expect<float>()`.

---

### Subcommands

Subcommands create a nested command scope. Each subcommand owns its own flags and options (and maybe positional arguments too) independently of its parent. Note I say maybe for positional arguments. This is because you cannot mix positional arguments and subcommands at the same level because we have no way of knowing which is which.

```cpp
auto& add = cli.subcommand("add", "Add a file");
add.option('i', "input").help("File to add");
add.flag('f', "force");

auto& rm = cli.subcommand("rm", "Remove a file");
rm.positional("path");
```

Each layer in your cli spec can have either subcommands or positional arguments. Say we allowed:

```cpp
shm::cli cli("program", "description");
cli.subcommand("add", "add an item");
cli.positional("input");
```

If the cli is `program add`, did you mean the subcommand `add` or was `add` the value for your positional `input`? It's ambiguous so we don't allow it.

---

### Naming Rules

- Every flag & option must have a long name.
- Short names are optional but must be a printable, non-dash, non-space character.
- Long names cannot begin with `-`, contain `=`, whitespace or non-printable characters.
- All names (long, short, positional, subcommand) must be unique within a command scope.

### Parsing

Parsing can only be done once as it modifies the cli object & the cli object must be kept in scope whilst the parse_result object is alive. i.e. Do not return the parse_result out the function where the cli object is declared, this is because the parse_result internally holds a reference to the cli spec. This is because the parse_result needs to know about the spec to stop you doing stupid things with the parse_result.

Parsing is done like so:

```cpp
auto result = cli.parse(argc, argv);
```

The `parse` method on cli accepts the raw argc and argv parameters that get given to you `main` function. Any errors encountered during parsing will result in a `std::runtime_error` being thrown that explains the issue. 


### Accessing Results

#### `get<T>(name)`

Retrieves the value of a flag, option or positional argument as type `T`. Note that this may throw if the parsing to type `T` fails. If you don't want to handle that here then use an expectation to guarantee the conversion is valid at parse-time.

```cpp
// Flags must always be retrieved as a bool
bool verbose = result.get<bool>("verbose");
// The count may be retrieved for flags and options
// Any non-repeatable flag or option (that isn't required) will always return 0 or 1
int count = result.count("verbose");
// Repeatable options must be collected as a vector of T
auto files = result.get<std::vector<std::string>>("file");
```

`get<bool>()` on an option will throw, you should have used a flag if you wanted a bool. `get<T>()` where `T` is not a bool will throw on a flag, flags must be retrieved as a bool.

If an option was not specified and no default was given then `get<T>(option)` will throw. If an option has no default you can check to see if it was explicitly specified:

```cpp
bool specified = result.specified("file")
```

Note that it's valid for a non-required, repeatable option, to return an empty vector.

#### `specified(name)`

Returns true if the argument was explicitly provided on the command line. Returns `false` if the value is coming from a default.

Specified can also be used to check if a particular subcommand was given.


#### `count(name)`

Returns how many times a flag or option was seen. For non-repeatable arguments this is always 0 or 1. This cannot be called on positional arguments as they are always required and can only be specified once.

#### `has_subcommand()`

Returns true is any subcommand was specified.

#### `subcommand()`

Returns the name of the specified subcommand. Throws if no subcommand was specified.

#### `operator[](name)`

Returns the nested parse_result scoped to the named subcommand, giving access to its flags and options etc.

```cpp
if (result.subcommand() == "add") {
    auto sub = result["add"];
    auto input = sub.get<std::string>("input");
    // Could also just have
    auto value = result["add"].get<std::string>("input");
}
```

This throws if the named subcommand was not the one that was invoked.

### Automatic features

#### Help

`-h/--help` is automatically registered on every command and subcommand. Passing it prints the generated help text for that command and exits. Help cannot be disabled without modifying the code. Yes this is opinionated but I think all clis should have help, so doing it for you feels much easier than you having to constantly check to see if help was specified.

Generated help output includes the usage, description, flags, options with type hints, required/default annotations & a subcommand listing where applicable. e.g.

```
Usage: vcs [OPTIONS] <COMMAND>

A modern version control system

Flags:
  -h, --help                   Print help
      --version                Print version
  -v, --verbose                Enable verbose output (repeatable)
      --no-color               Disable colored output

Options:
  -C, --directory VALUE        Run as if started in this directory (default: .)

Commands:
  add                          Add file contents to the index
  commit                       Record changes to the repository
  init                         Initialise a new repository
  remote                       Manage set of tracked repositories

Run 'vcs <COMMAND> --help' for more information on a command.
```

#### Version

Calling:

```cpp
shm::cli cli("myapp");
cli.version("1.0.0");
```

will register a `--version` flag on the top level command. If seen it will print the version string as:

`{app name} version {version string}` i.e. `myapp version 1.0.0`.

The version and help flags are checked for before enforcing the required attribute so that they function even if required options are absent.

### Exceptions

All user errors (missing required args, unknown flags, type mismatches & validation failures) throw `std::runtime_error` with a descriptive message. Logic errors or bad names in spec construction will throw a `std::logic_error` or `std::invalid_argument`.

Help and version flags trigger a `std::exit(0)` directly rather than throwing so that no exception handling is required for those code paths.

### Full Example

```cpp
#include "shim/cli.h"

int main(int argc, char* argv[]) {

    shm::cli cli("archive", "A simple file archiver");
    cli.version("2.0.0");
    cli.flag('v', "verbose").help("Enable verbose output").repeatable();

    auto& add = cli.subcommand("add", "Add files to the archive");
    add.option('a', "archive").help("Target archive").required().expect<std::string>();
    add.option('f', "file").help("File to add").required().repeatable();
    add.flag('c', "compress").help("Compress when adding");

    auto& list = cli.subcommand("list", "List archive contents");
    list.option('a', "archive").help("Archive to list").required().expect<std::string>();

    try {
        auto result = cli.parse(argc, argv);

        if (!result.has_subcommand()) {
            std::cerr << "No subcommand specified. Use --help for usage.\n";
            return 1;
        }

        bool verbose = result.get<bool>("verbose");

        if (result.subcommand() == "add") {
            auto sub = result["add"];
            auto archive = sub.get<std::string>("archive");
            auto files = sub.get<std::vector<std::string>>("file");
            bool compress = sub.get<bool>("compress");
            // ...
        }
        else if (result.subcommand() == "list") {
            auto sub = result["list"];
            auto archive = sub.get<std::string>("archive");
            // ...
        }
    }
    catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
```