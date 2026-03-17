#pragma once

/*
    SPDX-License-Identifier: MIT
    Copyright (c) 2026 themalwareman

    shim — small, header-only C++ utilities
    https://github.com/themalwareman/shim

    shm::cli - a lightweight, modern C++ command-line parser with subcommand support
*/


/*
    Todo:
        Quote handling
        We could expand expect to have multiple types and guarantee that at least one works? not sure if thats any benfit though
            Could provide a non throwing is_valid<T>?
        Remove the dump method eventually after proving a way to dump the cli results to json or if we no longer need it
*/

/*
    Maintenance Notes:

    Abstract:
        I think I had a bit of a personal brainwave but that's very much up to the reader's opinion. A lot
        of command lines force you to specify the type in the spec but that removes the ability to be flexible.
        You might specify something is an int but for the sake of some maths or something want to pick it up
        as a float. Well why not just specify what you want to pick it up as, when you pick it up? Lazily
        parse the option into the asked for type? We can also solve guarantees with expectations and validations
        which you'll see shortly or in the docs.

        expect<T>() which guarantees the coder that the cli value is parseable to the type T by checking it at parse
        time and throwing if it's not possible. This basically gives you back explicit typing if you want.

    std::deque for args storage:
        deque is used rather than vector because the spec-building methods
        like flag and option return references into _args to enable fluent
        chaining. deque guarantees that push/emplace_back never invalidates
        existing references, whereas a vector, which could reallocate on growth,
        might. Lookup used from the _lookup map used to use reference wrappers
        which is fine, because the references are stable. But it started to get
        a bit messy and is more prone to future issues if we try to enable things
        like copy. Instead, it now uses indexes into the deque.

        e.g.

        auto& f = cli.flag('v', "verbose");
        cli.option("output");  // Could reallocate _args vector
        f.repeatable();        // dangling reference if reallocation happened

    std::less<> in map templates:
        using a transparent comparator for map objects enables lookup via
        a string_view rather than having to construct a full std::string
        for every lookup. Given we're working with the cli where we use a
        lot of string_views this enables us to save some extra allocations.

    std::variant & std::visit:
        This mechanism greatly simplifies storing different classes that inherit
        functionality via CRTP. We could inherit from yet another base but that
        makes casting back to the correct type pretty awful so I think variants
        are our cleanest option.

    specified:
        This function on the result object was hard to name, because the intent is being
        able to check if the user explicitly specified a command line arg. "has" is a bit
        off because "has" feels true for a default, "seen" was another option but felt a bit weak.
        The final decision was to settle on specified.

    Decisions:

    Should we allow 0/1, yes/no, on/off for explicit flag overrides?
        No. I think this leads to more command line noise with no real benefit. The docs
            will state that explicit flag overrides are true/false and I think only allowing
            that is more consistent across the board. You can also name a flag slightly
            differently such that true/false makes sense. Or if you want text options
            you can use an option, put it in the help, and use a validator to enforce it.

    Should flags be repeatable by default?
        No. Initially I thought it made sense to allow this, but I think anything less explicit
        just becomes more confusing. For flags like verbose it makes sense for a user to indicate
        that the flag is repeatable. This also helps with the usage doc generation as its obvious
        which flags are repeatable. We can still allow the use of count on a non-repeatable flag,
        but it'll either just be 0 or 1. It also helps throw errors when a user might accidentally
        be specifying a flag twice.

    Should get<bool> be allowed on an option?
        No. I realize this is an opinionated decision, and it's not strictly the libraries business
        to tell you how to use it. However, I think if a user is trying to get an option as a bool
        we can help nudge them towards using a flag and think about what they're doing. Also using
        an option for a bool when we have flags feels like a cli smell, and they should rethink it anyway.

    Should we allow positional args to be called the same thing as a flag or option?
        No. Originally I did allow this because there's no reason you can't allow it, the only thing is
        you then need an additional api into the result object i.e. .positional<T>("name") instead of get.
        However, the more I think about the more it's similar to the opinionated get<bool> on option decision.
        Enforcing that design allows us to simplify the api into a unified get<T>() method which is nice.
        But also I think it pushes the user away from a cli smell because tbh having --source x and then
        in the docs having a positional <source> is pretty confusing and probably bad anyway.

    Should we allow subcommands to be called the same thing as a flag or option?
        No. For basically identical logic to above.

    Should long names be mandatory?
        Yes. Long names are essential to partly documenting the option -s by itself means nothing
        but -s --store is much more meaningful. Flags also always need them to allow explicit overriding
        of defaults so we'll unify that and say that everything must have a long name. But short names
        are optional so you don't unnecessarily use up the short name namespace when you don't want one.

    Should --help, & -h be reserved and should we automatically provide help parsing?
        No. I thought about this, while its close to the cli smell decisions above we'll
        leave it for know and let the user decide how they want to expose help. What we will
        do though is provide a .help() method on the cli object which dumps the spec in a sensible
        format. We can always revisit this. Maybe for now we add an .auto_help method to do this?

    Should the parse time storage sit alongside the spec?
        Yes. Originally the parse_result has its own storage, but to provide a nice sensible API
        it ended up having to know a decent amount about the spec. Like is a flag or option repeatable,
        and be populated with default values etc. Whilst you could argue that the parse_result was
        storing a distilled version of the spec it created a lot of duplication in what is intended
        to be a smaller header only library. Instead, a decision was made to put the parse_time storage
        alongside the spec. The parse function then fills in the spec. This way we can also have the
        individual spec objects enforce correctness rather than having the parse_result have to handle
        checking if it should be allowed to give a repeated value to something. To avoid duplicating the
        spec and having to have the parse_result own the spec instead we only allow parsing once and
        the parse_result then has a reference to the spec. We'll document lifetimes clearly in the docs.

    Should a conflicting repeated flag throw?
        Yes. I think if someone uses a repeatable flag and mixes the values true and false then
        its very likely an error. If there's ever a good reason it would be required we can
        revisit this but for now I think it should constitute an error.

    Should help be automatic?
        Yes. I realize it's another opinion, but I don't personally see any real argument
        for a CLI not having help. A potential argument would be you want to repurpose
        -h but in all honesty its so universal that that's probably bad cli design. I
        may add a way to opt out in the future, but for now it can be ripped out manually.

    Should we provide automatic version help?
        Yes. cli versions are common enough that having the boilerplate done for you is
        super helpful. Note this is opt-in in case you really don't want it. It can be setup
        by calling the version function on the top level CLI object. There were a few adjustments
        to make it work, but I think they're all worthwhile. It'll auto register the flag
        and handle the printing of the version and exiting.

*/

#include <string>
#include <algorithm>
#include <string_view>
#include <stdexcept>
#include <sstream>
#include <type_traits>
#include <format>
#include <vector>
#include <functional>
#include <map>
#include <utility>
#include <memory>
#include <deque>
#include <variant>
#include <iostream>
#include <charconv>

namespace shm {

    namespace detail {
        namespace cli {

            /*
                We need some helper templates to extract types from callables.
            */
            template<typename>
            struct function_traits;

            // PTS for plain function types
            template<typename R, typename... Args>
            struct function_traits<R(Args...)>
            {
                using return_type = R;
                using arg_types = std::tuple<Args...>;
                static constexpr std::size_t arg_count = sizeof...(Args);

                template<std::size_t N>
                using nth_arg = std::tuple_element_t<N, arg_types>;
            };

            // PTS for const member function pointers
            template<typename R, typename C, typename... Args>
            struct function_traits<R(C::*)(Args...) const> {
                using return_type = R;
                using arg_types = std::tuple<Args...>;
                static constexpr std::size_t arg_count = sizeof...(Args);
                template<std::size_t N>
                using nth_arg = std::tuple_element_t<N, arg_types>;
            };

            // PTS for non const member function pointers
            template<typename R, typename C, typename... Args>
            struct function_traits<R(C::*)(Args...)> {
                using return_type = R;
                using arg_types = std::tuple<Args...>;
                static constexpr std::size_t arg_count = sizeof...(Args);
                template<std::size_t N>
                using nth_arg = std::tuple_element_t<N, arg_types>;
            };

            // Redirection for lambda types to evaluate their operator() instead of the anonymous lambda type
            template<typename F>
            requires requires { &F::operator(); }
            struct function_traits<F> : function_traits<decltype(&F::operator())> {};

            /*
                Concepts required for validators
            */
            template<typename F>
            concept returns_bool = std::same_as<bool, typename function_traits<F>::return_type>;

            template<typename F>
            concept unary_callable = function_traits<F>::arg_count == 1;

            /*
                We will likely strengthen the limits on the parsing, but I think this is
                a good bare minimum of what's required.
            */
            template <typename T>
            concept parseable = requires(std::istringstream& s, T& v) { s >> v; };

            /*
                When setting defaults we want the concept of stringable which is
                a user can specify any value that is stringable i.e. convertible
                to string. This can be used as the default and then will be
                converted back to whatever they want later when they ask for it.
             */
            template<typename T>
            concept stringable = requires(std::ostringstream& s, T& v) {s << v;};

            /*
                To disambiguate the get overloads on the parse_result we need to be able
                to tell if a template type is a vector or not
             */
            template<typename T>
            concept is_vector = requires { typename T::value_type; } &&
                std::same_as<T, std::vector<typename T::value_type>>;

            /*
                Custom Exceptions
            */

            /*
                The verifiability CRTP add-on knows nothing about the name of what failed
                validation so to provide additional context we want to throw specific exceptions
                that can be caught and re-thrown by the derived classes. We also want our own
                exceptions so that if a user provided validator throws we let their exception
                bubble up.
             */
            class expectation_error : public std::runtime_error {
            public:
                explicit expectation_error(std::string_view message)
                    : std::runtime_error(std::string(message)) {}
            };

            class validation_error : public std::runtime_error {
            public:
                explicit validation_error(std::string_view message)
                    : std::runtime_error(std::string(message)) {}
            };

            /*
                For parsing we want to catch any exceptions thrown by the C++ stl
                and rethrow our own specific parser exception for clarity.
            */
            class parse_error : public std::runtime_error {
            public:
                explicit parse_error(std::string_view message)
                    : std::runtime_error(std::string(message)) {}
            };

            /*
                An exception we throw if we detect the help flag whilst parsing
             */
            class help_error : public std::runtime_error {
            public:
                explicit help_error(std::string_view message)
                    : std::runtime_error(std::string(message)) {}
            };

            // This function attempts to provide more helpful type names for exception messages
            template<typename T>
            constexpr std::string_view type_name() {
                if constexpr (std::is_same_v<T, int>)         return "int";
                else if constexpr (std::is_same_v<T, float>)  return "float";
                else if constexpr (std::is_same_v<T, double>)  return "double";
                else if constexpr (std::is_same_v<T, std::string>) return "string";
                else if constexpr (std::is_same_v<T, bool>)   return "bool";
                else return typeid(T).name(); // fallback for unknown types
            }

            /*
                The base parsing method is istringstream >>, that's the bare minimum,
                and it's easy for people to extend without messing with the internals
                of this header. What we can do though is strengthen some of them a bit
                more. For example istringstream would parse 12f3 as 12 if asking for
                an int but for a cli that should fail.
            */
            template<typename T>
            T parse(std::string_view sv) {
                T retval;
                std::istringstream iss{std::string{sv}};
                iss >> retval;

                if (iss.fail() || not iss.eof()) {
                    throw parse_error(std::format("Failed to parse '{}' as '{}'", sv, type_name<T>()));
                }

                return retval;
            }

            // Strengthen int parsing
            template<>
            inline int parse<int>(std::string_view sv) {
                int value;
                auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);

                if (ec != std::errc() || ptr != (sv.data() + sv.size())) {
                    throw parse_error(std::format("Failed to parse '{}' as '{}'", sv, type_name<int>()));
                }

                return value;
            }

            // Strengthen float parsing
            template<>
            inline float parse<float>(std::string_view sv) {
                float value;
                auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);

                if (ec != std::errc() || ptr != (sv.data() + sv.size())) {
                    throw parse_error(std::format("Failed to parse '{}' as '{}'", sv, type_name<float>()));
                }

                return value;
            }

            // Strengthen double parsing
            template<>
            inline double parse<double>(std::string_view sv) {
                double value;
                auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);

                if (ec != std::errc() || ptr != (sv.data() + sv.size())) {
                    throw parse_error(std::format("Failed to parse '{}' as '{}'", sv, type_name<double>()));
                }

                return value;
            }

            /*
                For explicit flag evaluation like --flag=true we want case-insensitive string comparison
                to support false,False,true,True etc.
            */
            inline bool iequals(std::string_view a, std::string_view b)
            {
                return a.size() == b.size() &&
                       std::ranges::equal(a, b, [](unsigned char c1, unsigned char c2) {
                           return std::tolower(c1) == std::tolower(c2);
                       });
            }

            /*
                This helpability CRTP add-on allows something to inherit fluent chaining
                for adding a help string. We need a public method for setting the help,
                a protected method for retrieving the help and then private storage which
                is an implementation detail.
            */
            template<typename Derived>
            class helpability  {
                /*
                    // Configuration
                    Derived& help(std::string_view help)

                    // Access
                    [[nodiscard]] std::string_view help() const
                */
                public:
                    Derived& help(std::string_view help) { _help = help; return static_cast<Derived&>(*this); }
                protected:
                    // Fun bug - without the explicit string_view constructor on help.value() the ternary promotes the string literal
                    // to a string to get the consistent return type, then we return a string_view to a temporary which is obviously
                    // an issue.
                    [[nodiscard]] std::string_view help() const { return _help.has_value() ? std::string_view{_help.value()} : "(undocumented)";}
                private:
                    // Could just default this to "(undocumented)", but in future might want to know if it was ever set.
                    std::optional<std::string> _help;
            };

            /*
                This repeatability CRTP add-on provides fluent chaining for indicating if
                an argument is repeatable or not. We have our public method for setting
                teh repeatability state, a protected method for checking that state and
                the private storage which is an implementation detail.
             */
            template<typename Derived>
            class repeatability  {
                /*
                    // Configuration
                    Derived& repeatable(bool repeatable = true)

                    // Access
                    [[nodiscard]] bool is_repeatable() const noexcept
                */
            public:
                // The shared fluent api, both options and flags can have help and be repeatable
                // Think --file 1.txt --file 2.txt or -v -vv -vvv
                Derived& repeatable(bool repeatable = true) {_repeatable = repeatable; return static_cast<Derived&>(*this); }
            protected:
                [[nodiscard]] bool is_repeatable() const noexcept {return _repeatable;}
            private:
                // Non-repeatable by default
                bool _repeatable = false;
            };


            /*
                Given our CLI library is about being text first, this verifiability CRTP add-on
                is about providing a mechanism for guaranteeing certain type conversions will
                be valid upon later access. We provide the concept of expectation which guarantees
                at parse-time that a conversion to the expected type will be tested, throwing upon
                failure which guarantees it's accessible as the expected type later. We also provide
                validators which implicitly give you the same guarantee as an expectation by converting
                the type to the type of the parameter of the unary callable validator. But also
                allow further validation by allowing the user to provide a unary callable predicate
                that can return false if conditions aren't met which results in a throw. There are more
                verbose docs below.
            */
            template<typename Derived>
            class verifiability {
                /*
                    // Configuration
                    template <typename T>
                    requires (not std::is_reference_v<T>) && detail::cli::parseable<T>
                    Derived& expect()

                    template <typename F>
                    requires detail::cli::unary_callable<F> &&
                        detail::cli::returns_bool<F> &&
                        detail::cli::parseable<typename detail::cli::function_traits<F>::template nth_arg<0>>
                    Derived& validator(F&& validator)

                    // Access
                    [[nodiscard]] bool has_expectation() const
                    [[nodiscard]] std::string expected_type() const
                    void verify(std::string_view sv) const
                */
            public:

                /*
                    Expectation
                    This is just a guarantee that during parsing this conversion will be attempted so that
                    if parsing succeeds i.e. doesn't throw, attempting to collect the option later as this
                    type T will succeed. i.e.

                    If an option has .expect<int>() later when you do result.get<int>("option") that's
                    guaranteed to work.
                */
                template <typename T>
                requires (not std::is_reference_v<T>) && detail::cli::parseable<T>
                Derived& expect()
                {
                    if (_expectation) {
                        throw std::logic_error{"An expected type has already been specified for this option; only one type can be expected"};
                    }

                    /*
                        Setting an expectation should guarantee that at parse time we attempt
                        to parse the cli value into the expected type and we succeed. If we
                        generate a parse error then the parsing should fail.
                    */
                    _expectation = [](std::string_view sv) -> void { detail::cli::parse<T>(sv); };
                    _expected_type = type_name<T>();

                    return static_cast<Derived&>(*this);
                }

                /*
                    Validation:

                    A validator must be:
                        a unary callable - accept 1 parameter
                        return bool - indicate whether the validation of the option succeeded or not
                        parseable first arg - we must be able to parse the cli string_view into the expected validator arg type
                */
                template <typename F>
                requires detail::cli::unary_callable<F> &&
                    detail::cli::returns_bool<F> &&
                    detail::cli::parseable<typename detail::cli::function_traits<F>::template nth_arg<0>>
                Derived& validator(F&& validator) {
                    /*
                        Validators are like an implicit expect because we need to be able to parse
                        the cli value into the type they want as their parameter. But they can also
                        perform validation on the parsed value.
                    */
                    using first_arg_type = typename detail::cli::function_traits<F>::template nth_arg<0>;
                    auto validator_wrapper = [fn = std::forward<F>(validator)](std::string_view sv) -> bool { return fn(detail::cli::parse<first_arg_type>(sv)); };
                    _validators.push_back(std::move(validator_wrapper));

                    return static_cast<Derived&>(*this);
                }
            protected:

                [[nodiscard]] bool has_expectation() const {
                    return _expectation.has_value();
                }

                [[nodiscard]] std::string expected_type() const {
                    return _expected_type;
                }

                void verify(std::string_view sv) const {
                    // Check for expectations
                    if (_expectation) {
                        try {
                            (*_expectation)(sv);
                        }
                        catch (const parse_error& e) {
                            throw expectation_error(e.what());
                        }
                    }

                    for (auto& validator : _validators) {
                        try {
                            if (not validator(sv)) {
                                throw validation_error(std::format("Validator predicate failed for value '{}'", sv));
                            }
                        }
                        catch (const parse_error& e) {
                            throw validation_error(std::format("Could not convert '{}' to the validators expected type: {}", sv, e.what()));
                        }
                    }
                }
            private:
                // Expectation
                std::optional<std::function<void(std::string_view)>> _expectation;
                std::string _expected_type;

                // Validation
                std::vector<std::function<bool(std::string_view)>> _validators;
            };

            /*
                For our cli parsing we need to distinguish between flags and options
                as unless explicitly specified via --flag=value, flags don't come
                with a value because the value as a bool is implicit.

                To share the common code between options and flags we have this
                argument base class. However, to enable fluent chaining when setting
                up the cli like:

                cli.option("test").repeatable().help("Help text for test")

                We need the shared functionality to be able to return a reference
                to their base class otherwise anytime you call a shared function
                you inadvertently promote to the base and potentially lose access
                to some setters/methods.

                To solve this we use CRTP so the base class can correctly cast itself
                back to the derived type to enable correct fluent chaining. Originally
                for joint storage this also meant we needed a non template base but that
                added a lot of complexity for storage that could be solved via the use
                of a variant.

                A lot of the shared functionality has now been broken out into separate
                CRTP classes so that the logic is more cleanly grouped. But this argument
                one is specifically for both flags and options as it provides the short
                and long name logic. Hence, why its naming is a little different to the
                other mix-ins.
            */
            template<typename Derived>
            class argument {
                /*
                    // Construction
                    argument(char short_name, std::string_view long_name)
                    argument(std::string_view long_name)

                    // Access
                    [[nodiscard]] bool has_short_name() const noexcept
                    [[nodiscard]] char short_name() const
                    [[nodiscard]] const std::string& long_name() const noexcept
                 */
            protected:
                /*
                    We protect the constructor so that it is only accessible to the base class
                    as we don't want users accidentally creating this base type.
                */
                argument(char short_name, std::string_view long_name) : _short_name(short_name), _long_name(long_name) {
                    validate_short_name(short_name);
                    validate_long_name(long_name);
                }

                /*
                    We want to allow long names without short names as the short name space is limited
                    and the user may have many cli args
                */
                argument(std::string_view long_name) : _long_name(long_name) {
                    validate_long_name(long_name);
                }

                // Allow us to identify if this argument has a short name set as not all arguments have one
                [[nodiscard]] bool has_short_name() const noexcept {return _short_name.has_value();}

                // Getters for the short and long names
                [[nodiscard]] char short_name() const {return _short_name.value();}
                [[nodiscard]] const std::string& long_name() const noexcept {return _long_name;}

            private:
                /*
                    Private implementation details that validates the long and short names
                    to some spec. Using this shared CRTP base allows us to centralise this
                    logic into one place.
                */
                void validate_short_name(char short_name) {
                    if (not short_name || short_name == '-' || std::isspace(static_cast<unsigned char>(short_name)) || not std::isprint(static_cast<unsigned char>(short_name))) {
                        throw std::invalid_argument("invalid short option character - must be printable, non space, non dash & non null");
                    }
                }

                void validate_long_name(std::string_view long_name) {
                    if (long_name.empty()) {
                        throw std::invalid_argument("long name cannot be empty");
                    }
                    if (long_name.length() < 2) {
                        throw std::invalid_argument("long name cannot be a single character");
                    }
                    if (long_name.front() == '-') {
                        throw std::invalid_argument("long name cannot begin with '-'");
                    }
                    if (std::ranges::any_of(long_name, [](unsigned char c) {
                        return std::isspace(c) || (not std::isprint(c)) || c == '=';
                    })) {
                        throw std::invalid_argument("long name contains invalid characters");
                    }
                }

            private:
                // Storage is a private implementation detail
                // Every argument must have a long name
                std::string _long_name;
                // Short names are optional
                std::optional<char> _short_name;
            };
        }
    }

    class flag : public detail::cli::argument<flag>,
                    public detail::cli::helpability<flag>,
                    public detail::cli::repeatability<flag> {
        /*
            We don't want the user instantiating a flag directly so we
            made the constructor private and we make the command class
            a friend so that it can call our private constructor. The
            parse_result class also need to be able to interrogate the
            spec but we don't want to expose the access methods to the
            user API either, so we also make it a friend. This is the
            same for both option and positional too.
        */
        friend class command;
        friend class cli;
        friend class parse_result;

        /*
            // Constructors
            explicit flag(std::string_view long_name);
            explicit flag(char short_name, std::string_view long_name);

            // Configuration
            flag& defaults(bool value)

            // Access
            [[nodiscard]] bool has_explicit_default() const noexcept
            [[nodiscard]] bool default_value() const
            [[nodiscard]] bool value() const
            [[nodiscard]] bool seen() const noexcept
            [[nodiscard]] size_t count() const noexcept

            // Parse-time Modification
            void set(bool value)
        */
    private:
        // Constructors are private to avoid them being created manually by the user
        explicit flag(std::string_view long_name) : argument(long_name) { }
        explicit flag(char short_name, std::string_view long_name) : argument(short_name, long_name) { }

    public:
        // Only public extention to the user is allowing them to set a bool default
        flag& defaults(bool value) {
            if (_default.has_value()) {
                throw std::logic_error("A default value has already been specified for this flag");
            }
            _default = value;
            return *this;
        }

    private:
        /*
            These methods are private as they are only required by the command class, which is already
            a friend, to allow it to extract information about the flag
        */
        // This will be used for help docs so that we can indicate and explicit vs implicit default for flags
        [[nodiscard]] bool has_explicit_default() const noexcept {return _default.has_value();}
        // Give the explicit default if we have one, otherwise flags default to false
        [[nodiscard]] bool default_value() const {return _default.has_value() ? _default.value() : false; }

        // Parsetime helpers
        // Return a value if one was explicitly set at parse time otherwise return the default
        [[nodiscard]] bool value() const { return _value.has_value() ? _value.value() : default_value(); }
        // Was this flag seen at parse time?
        [[nodiscard]] bool seen() const noexcept {return _count > 0;}
        // How many times did we see this flag at parse time -> think -vvv
        [[nodiscard]] size_t count() const noexcept {return _count;}
        // At parse time we allow setting the bool, last set wins essentially if this flag is repeatable
        void set(bool value) {
            if (not is_repeatable() && _value.has_value()) {
                throw std::runtime_error(std::format("Flag '{}' is not repeatable but was specified multiple times", long_name()));
            }
            if (is_repeatable() && _value.has_value() && (_value.value() != value)) {
                throw std::runtime_error(std::format("Flag '{}' is repeatable but was set to a conflicting value, previous: '{}', new: '{}'", long_name(), _value.value(), value));
            }
            _value = value;
            _count++;
        }
    private:
        // We use an optional here to allow us to distinguish an implicit vs explicit false default
        std::optional<bool> _default;

        // Parsetime storage
        std::optional<bool> _value;
        size_t _count = 0;
    };

    class option : public detail::cli::argument<option>,
                    public detail::cli::helpability<option>,
                    public detail::cli::verifiability<option>,
                    public detail::cli::repeatability<option> {
        friend class command;
        friend class cli;
        friend class parse_result;

        /*
            // Constructors
            explicit option(std::string_view long_name);
            explicit option(char short_name, std::string_view long_name);

            // Configuration
            option& required(bool required = true) noexcept
            template<typename T>
            requires (not std::is_reference_v<T>) &&
                (not std::is_pointer_v<T> || (std::is_same_v<T, char*> || std::is_same_v<T, const char*>)) &&
                detail::cli::stringable<T>
            option& defaults(T value)

            // Access
            [[nodiscard]] bool has_default() const noexcept
            [[nodiscard]] const std::string& default_value() const
            [[nodiscard]] bool is_required() const noexcept
            [[nodiscard]] bool seen() const noexcept
            [[nodiscard]] size_t count() const noexcept
            [[nodiscard]] const std::vector<std::string>& values() const noexcept

            // Parse-time Modification
            void set(std::string_view value)
        */
    private:
        // Constructors are private to avoid them being created manually by the user
        explicit option(std::string_view long_name) : argument(long_name) { }
        explicit option(char short_name, std::string_view long_name) : argument(short_name, long_name) { }


    public:
        // Allow an option to be set as required
        option& required(bool required = true) noexcept {_required = required; return *this; }

        // Default
        // Anything that is stringable i.e. convertible to a string can be provided as a default
        // Todo: maybe we relax reference? unsure
        template<typename T>
        requires (not std::is_reference_v<T>) &&
            (not std::is_pointer_v<T> || (std::is_same_v<T, char*> || std::is_same_v<T, const char*>)) &&
            detail::cli::stringable<T>
        option& defaults(T value) {
            if (_default.has_value()) {
                throw std::logic_error("A default value has already been specified for this option");
            }
            std::ostringstream oss;
            oss << value;
            _default = oss.str();
            return *this;
        }

    private:
        // Check if the option has a default
        [[nodiscard]] bool has_default() const noexcept {return _default.has_value();}
        // Grab the default value
        [[nodiscard]] const std::string& default_value() const {return _default.value();}

        [[nodiscard]] bool is_required() const noexcept {return _required;}

        // Parsetime helpers
        // Did we see this option at parsetime
        [[nodiscard]] bool seen() const noexcept {return not _values.empty();}
        // How many times did we see it
        [[nodiscard]] size_t count() const noexcept {return _values.size();}
        // Access values
        [[nodiscard]] const std::vector<std::string>& values() const noexcept {return _values;}

        // Set the option, handle repeatable logic too
        void set(std::string_view value) {
            if (not is_repeatable() && not _values.empty()) {
                throw std::runtime_error(std::format("Option '{}' is not repeatable but was specified multiple times", long_name()));
            }
            /*
                Verification throws two specific errors, we catch and rethrow these adding more
                context i.e. the option name. We use these two specific errors so that if a user
                decided to throw in their validator we let their exception bubble up.
            */
            try {
                verify(value);
            }
            catch (const detail::cli::expectation_error& e) {
                throw std::runtime_error(std::format("Expectation on option '{}' failed: {}", long_name(), e.what()));
            }
            catch (const detail::cli::validation_error& e) {
                throw std::runtime_error(std::format("Validation on option '{}' failed: {}", long_name(), e.what()));
            }
            _values.emplace_back(value);
        }

    private:
        // Options can be required but aren't required by default
        bool _required   = false;

        // Default
        // todo: future - Would we ever want multiple defaults for a repeatable option?
        std::optional<std::string> _default;

        // Parsetime storage - vector required in case option is repeatable
        std::vector<std::string> _values;
    };

    class positional : public detail::cli::helpability<positional>,
                        public detail::cli::verifiability<positional> {
        friend class command;
        friend class cli;
        friend class parse_result;

        /*
            // Constructors
            explicit positional(std::string_view name)

            // Access
            [[nodiscard]] const std::string& name() const noexcept
            [[nodiscard]] bool seen() const noexcept
            [[nodiscard]] const std::string& value() const

            // Parse-time Modification
            void set(std::string_view value)
        */
    private:
        explicit positional(std::string_view name) : _name(name) { }

        // Positionals have no specific configuration so we only have access
        [[nodiscard]] const std::string& name() const noexcept { return _name; }
        [[nodiscard]] bool seen() const noexcept { return _value.has_value(); }
        [[nodiscard]] const std::string& value() const { return _value.value(); }

        // Parsetime helpers
        void set(std::string_view value) {
            try {
                verify(value);
            }
            catch (const detail::cli::expectation_error& e) {
                throw std::runtime_error(std::format("Expectation on positional '{}' failed: {}", name(), e.what()));
            }
            catch (const detail::cli::validation_error& e) {
                throw std::runtime_error(std::format("Validation on positional '{}' failed: {}", name(), e.what()));
            }
            _value = value;
        }

    private:
        std::string _name;

        // Parsetime storage
        std::optional<std::string> _value;
    };

     /*
        Command is the recursive class. The top level cli object inherits from this to
        form the base command which is essentially nameless and then each subcommand
        becomes a nested command object of that base command. Each command owns its own
        flags and options and validates the parsing of those, before delegating to any
        subcommands if necessary. The reason for the separate cli class that inherits this
        is to avoid exposing a parse function on each command object that might confuse
        the user.
    */
    class command {
        // Parse result needs to interrogate the spec
        friend class parse_result;

        using argument = std::variant<flag, option, positional>;

        /*
            // Constructors
            explicit command(std::string_view name, std::string_view description = {});

            // Help
            [[nodiscard]] std::string help() const

            // Parsing
            void parse(int argc, char* argv[])

            // Subcommands
            command& subcommand(std::string_view name, std::string_view description = {})

            // Flags
            flag& flag(std::string_view long_name);
            flag& flag(char short_name, std::string_view long_name);

            // Options
            option& option(std::string_view long_name);
            option& option(char short_name, std::string_view long_name);

            // Positionals
            positional& positional(std::string_view name)

            // Private Helpers
            void ensure_unique(std::string_view name)

            // Parse-time Helpers
            [[nodiscard]] bool has_active_subcommand() const noexcept
            [[nodiscard]] std::string_view active_subcommand() const
            [[nodiscard]] bool is_known_subcommand(std::string_view name) const
            [[nodiscard]] const command& get_subcommand(std::string_view name) const

            // Protected Helpers For Access From cli
            [[nodiscard]] const argument& resolve(std::string_view name) const
            [[nodiscard]] argument& resolve(std::string_view name)
            [[nodiscard]] const std::string& name() const noexcept
        */

    protected:
        // Constructor and parse are protected so they are not publicly accessible but can be called by the cli base class
        explicit command(std::string_view name, std::string_view description = {}, std::string_view path = {}) : _name(name), _description(description), _path(path) {
            // See maintenance notes, decision was made to make help automatic
            flag('h', "help").help("Print help");
        }

        void parse(int argc, char* argv[]) {

            // If/when we come across positional args we need to keep track of which index we're at
            // I think we could figure this out by walking the list and looking at whats already been seen
            // but that's a fair bit slower.
            size_t positional_index = 0;

            // Walk each token in argv using an index tracker as sometimes a single loop
            // will consume multiple tokens i.e. --option value will be consumed at the
            // same time.
            for (size_t i = 0; i < argc; i++) {
                // Grab the current token as a string_view
                std::string_view token = argv[i];

                // Have we hit the end of options marker? in which case everything left is positional
                if ("--" == token) {
                    // Walk all remaining tokens
                    for (size_t k = i + 1; k < argc; k++) {
                        if (positional_index >= _positionals.size()) {
                            throw std::runtime_error("Too many positional arguments provided");
                        }
                        // Set the positional and advance the index
                        std::get<shm::positional>(_args[_positionals[positional_index++]]).set(argv[k]);
                    }
                    break;
                }
                // We didn't hit end of options marker so if the token starts -- we're looking at a long name option or flag
                if (token.starts_with("--")) {
                    /*
                        Possible:
                            --long: flag
                            --long_flag=true: allowed to overwrite defaults
                            --file filename
                            --file=filename
                     */
                    // First we need to extract the name, but we have two major cases, using an equals sign or not
                    auto eq_pos = token.find("=");
                    if (std::string_view::npos == eq_pos) {
                        // No equals sign, the entire name should be a flag or an option
                        // Take substring skipping over -- which should be the long name
                        std::string_view argument = token.substr(2);
                        // Attempt to resolve the long name to an argument and visit it
                        std::visit([&](auto& var) {
                            using T = std::decay_t<decltype(var)>;
                            if constexpr (std::is_same_v<T, shm::flag>) {
                                // It's a long flag -> set it to true
                                var.set(true);
                            }
                            else if constexpr (std::is_same_v<T, shm::option>) {
                                // It's an option, grab the next arg if we have one
                                if (i + 1 < argc) {
                                    std::string_view next = argv[++i];
                                    var.set(next);
                                }
                                else {
                                    throw std::runtime_error(std::format("Ran out of arguments while parsing option '{}'", argument));
                                }
                            }
                            else if constexpr (std::is_same_v<T, shm::positional>) {
                                throw std::runtime_error(std::format("'{}' is a positional argument, not a flag or option", argument));
                            }
                            else {
                                static_assert(sizeof(T) == 0, "Unhandled argument type in parse");
                            }
                        }, resolve(argument));

                    }
                    else {
                        // We found an equals sign, split the token into argument and value
                        std::string_view argument = token.substr(2, eq_pos - 2);
                        std::string_view value = token.substr(eq_pos + 1);

                        // Throw if they gave no option
                        if (value.empty()) {
                            throw std::runtime_error(std::format("Argument '{}' was explicitly specified but no value was given", argument));
                        }

                        std::visit([&](auto& var) {
                            using T = std::decay_t<decltype(var)>;
                            if constexpr (std::is_same_v<T, shm::flag>) {
                                // It's a long flag -> validate that the value is true or false
                                if (detail::cli::iequals("true", value)) {
                                    var.set(true);
                                }
                                else if (detail::cli::iequals("false", value)) {
                                    var.set(false);
                                }
                                else {
                                    throw std::runtime_error(std::format("Unrecognized flag value '{}' for '{}', expected true or false", value, argument));
                                }
                            }
                            else if constexpr (std::is_same_v<T, shm::option>) {
                                var.set(value);
                            }
                            else if constexpr (std::is_same_v<T, shm::positional>) {
                                throw std::runtime_error(std::format("'{}' is a positional argument, not a flag or option", argument));
                            }
                            else {
                                static_assert(sizeof(T) == 0, "Unhandled argument type in parse");
                            }
                        }, resolve(argument));
                    }
                }
                else if (token.starts_with("-")) {
                    // It's a short option
                    /*
                        Possible:
                            -a: flag
                            -abc: multiple flags
                            -f filename: option
                            -ffilename: option
                            -abcf filename: flags followed by option
                     */
                    // Iterate over each character in the short pack
                    for (size_t j = 1; j < token.length(); j++) {
                        // Get that token as a string_view for lookups
                        std::string_view short_name = token.substr(j, 1);

                        std::visit([&](auto& var) {
                            using T = std::decay_t<decltype(var)>;
                            if constexpr (std::is_same_v<T, shm::flag>) {
                                // It's a flag, the presence indicates true
                                var.set(true);
                            }
                            else if constexpr (std::is_same_v<T, shm::option>) {
                                // It's an option
                                // Is this the first character in the short option pack?
                                const bool first_in_pack = (j == 1);
                                // Is this also the last character in the short option pack?
                                const bool last_in_pack  = (j + 1 == token.length());
                                // Both can be true if its bnoth the last and the first

                                if (first_in_pack) {
                                    // This is first short name in sequence meaning either we have
                                    // -f filename or -ffilename
                                    if (last_in_pack) {
                                        // -f filename
                                        // Last token -> grab next arg
                                        if (i + 1 < argc) {
                                            std::string_view next = argv[++i];
                                            var.set(next);
                                        }
                                        else {
                                            throw std::runtime_error(std::format("Ran out of arguments while parsing option '{}'", short_name));
                                        }
                                    }
                                    else {
                                        // -ffilename
                                        // Grab rest of token as value
                                        std::string_view value = token.substr(j+1);
                                        var.set(value);
                                        // Trigger break from parsing short options as we've consumed the pack
                                        // Can't break explicitly as we're inside a std::visit lambda
                                        j = token.length();
                                    }
                                }
                                else {
                                    // Option but not first, so should be -abcf filename i.e. we should be at end of short group
                                    if (last_in_pack) {
                                        // Yep, so grab next arg as option value
                                        if (i + 1 < argc) {
                                            std::string_view next = argv[++i];
                                            var.set(next);
                                        }
                                        else {
                                            throw std::runtime_error(std::format("Ran out of arguments while parsing option '{}'", short_name));
                                        }
                                    }
                                    else {
                                        throw std::runtime_error(std::format("Option '{}' cannot appear in the middle of a short name pack", short_name));
                                    }
                                }
                            }
                            else if constexpr (std::is_same_v<T, shm::positional>) {
                                throw std::runtime_error(std::format("'{}' is a positional argument, not a flag or option", short_name));
                            }
                            else {
                                static_assert(sizeof(T) == 0, "Unhandled argument type in parse");
                            }
                        }, resolve(short_name));
                    }
                }
                else {
                    // Not an option or flag should be subcommand or positional
                    if (not _commands.empty()) {
                        // This layer uses commands
                        auto it = _commands.find(token);
                        if (it != _commands.end()) {
                            // Forward parsing of rest of args into subcommand
                            _active_subcommand = std::string(token);
                            it->second.parse(argc - (i + 1), &argv[i + 1]);
                            // The rest of the args were consumed by the subcommand so break now
                            break;
                        }
                        else {
                            throw std::runtime_error(std::format("Unknown subcommand '{}'", token));
                        }
                    }
                    else {
                        // This layer uses positionals and we just hit one
                        if (positional_index >= _positionals.size()) {
                            throw std::runtime_error(std::format("Unexpected positional argument '{}'", token));
                        }
                        std::get<shm::positional>(_args[_positionals[positional_index++]]).set(token);
                    }
                }
            }

            // Parsing should now be complete

            // Help check sits naturally here, after parsing but before returning for validation. We may have done some
            // unnecessary legwork but this is much cleaner than my other solutions. Throwing is the easiest way to surface
            // help at the correct level.
            if (std::get<shm::flag>(resolve("help")).seen()) {
                throw detail::cli::help_error(help());
            }

        }

        // Validate required options were specified & all positionals were provided
        void validate() {
            for (auto& arg : _args) {
                std::visit([&](auto& var) {
                    using T = std::decay_t<decltype(var)>;
                    if constexpr (std::is_same_v<T, shm::option>) {
                        if (var.is_required() && not var.seen()) {
                            throw std::runtime_error(std::format("Required option '{}' was not specified", var.long_name()));
                        }
                    }
                    else if constexpr (std::is_same_v<T, shm::positional>) {
                        if (not var.seen()) {
                            throw std::runtime_error(std::format("Required positional '{}' was not provided", var.name()));
                        }
                    }
                }, arg);
            }

            // Recurse validation if subcommand specified
            if (_active_subcommand.has_value()) {
                _commands.at(_active_subcommand.value()).validate();
            }
        }

    private:

        [[nodiscard]] std::string help() const {
            std::ostringstream help;

            // Usage line
            help << std::format("Usage: {}", _path);
            // Does this subcommand have any flags or options, if so add [OPTIONS] to the usage line
            if (_has_flags || _has_options) {
                help << " [OPTIONS]";
            }
            // Same for subcommands or positionals
            if (not _commands.empty()) {
                help << " <COMMAND>";
            }
            else {
                // If no subcommands then we may have positionals
                for (auto& idx : _positionals) {
                    auto& pos = std::get<shm::positional>(_args[idx]);
                    help << std::format(" <{}>", pos.name());
                }
            }
            help << "\n";

            // Description
            if (not _description.empty()) {
                help << "\n" << _description << "\n";
            }

            // Positional Help
            if (not _positionals.empty()) {
                help << "\nPositionals:\n";
                for (auto& idx : _positionals) {
                    auto& pos = std::get<shm::positional>(_args[idx]);
                    help << std::format("  {:<28} {}\n", std::format("<{}>", pos.name()), pos.help());
                }
            }

            // Flags
            if (_has_flags) {
                help << "\nFlags:\n";
                for (auto& arg : _args) {
                    std::visit([&](auto& var) {
                        using T = std::decay_t<decltype(var)>;
                        if constexpr (std::is_same_v<T, shm::flag>) {
                            std::string name_column = var.has_short_name() ?
                                std::format("-{}, --{}", var.short_name(), var.long_name())
                                    : std::format("    --{}", var.long_name());

                            help << std::format("  {:<28} {}{}{}\n", name_column, var.help(),
                                var.has_explicit_default() ? std::format(" (default: {})", var.default_value()) : "",
                                var.is_repeatable() ? " (repeatable)" : "");
                        }
                    }, arg);
                }
            }

            // Options
            if (_has_options) {
                help << "\nOptions:\n";
                for (auto& arg : _args) {
                    std::visit([&](auto& var) {
                        using T = std::decay_t<decltype(var)>;
                        if constexpr (std::is_same_v<T, shm::option>) {
                            std::string type_hint = var.has_expectation()
                                ? std::string(var.expected_type())
                                : "VALUE";

                            std::string name_column = var.has_short_name() ?
                                std::format("-{}, --{} {}", var.short_name(), var.long_name(), type_hint)
                                : std::format("    --{} {}", var.long_name(), type_hint);

                            help << std::format("  {:<28} {}{}{}{}\n", name_column, var.help(),
                                var.is_required() ? " (required)" : "",
                                var.has_default() ? std::format(" (default: {})", var.default_value()) : "",
                                var.is_repeatable() ? " (repeatable)" : "");
                        }
                    }, arg);
                }
            }

            // Subcommands section
            if (not _commands.empty()) {
                help << "\nCommands:\n";
                for (auto& [name, cmd] : _commands) {
                    help << std::format("  {:<28} {}\n", name, cmd._description);
                }
                help << std::format("\nRun '{} <COMMAND> --help' for more information on a command.\n", _path);
            }

            return help.str();
        }

    public:

        // Subcommands
        command& subcommand(std::string_view name, std::string_view description = {}) {
            if (not _positionals.empty()) {
                throw std::logic_error("Cannot mix subcommands and positional arguments");
            }

            ensure_unique(name);

            auto insert = _commands.emplace(std::string{name}, shm::command(name, description, std::format("{} {}", _path, name)));

            return insert.first->second;
        }

        // Flags
        shm::flag& flag(std::string_view long_name) {
            // Ensure unique naming
            ensure_unique(long_name);
            // Add argument to our arg storage
            auto& ref = _args.emplace_back(shm::flag(long_name));
            // Save off the index lookup
            _lookup.emplace(std::string(long_name), _args.size()-1);

            _has_flags = true;

            return std::get<shm::flag>(ref);
        }

        shm::flag& flag(char short_name, std::string_view long_name) {
            ensure_unique(std::string_view{&short_name, 1});
            ensure_unique(long_name);

            // Add argument to our arg storage
            auto& ref = _args.emplace_back(shm::flag(short_name, long_name));

            // Save off the lookup
            _lookup.emplace(std::string({short_name}), _args.size()-1);
            _lookup.emplace(std::string(long_name), _args.size()-1);

            _has_flags = true;

            return std::get<shm::flag>(ref);

        }

        // Options
        shm::option& option(std::string_view long_name) {
            ensure_unique(long_name);

            auto& ref = _args.emplace_back(shm::option(long_name));
            // Save off the lookup
            _lookup.emplace(std::string(long_name), _args.size()-1);

            _has_options = true;

            return std::get<shm::option>(ref);
        }

        shm::option& option(char short_name, std::string_view long_name) {
            ensure_unique(std::string_view{&short_name, 1});
            ensure_unique(long_name);

            // Add argument to our arg storage
            auto& ref = _args.emplace_back(shm::option(short_name, long_name));

            // Save off the lookup
            _lookup.emplace(std::string({short_name}), _args.size()-1);
            _lookup.emplace(std::string(long_name), _args.size()-1);

            _has_options = true;

            return std::get<shm::option>(ref);
        }

        shm::positional& positional(std::string_view name) {
            // Can't mix positionals and subcommands
            if (not _commands.empty()) {
                throw std::logic_error("Cannot mix positional arguments and subcommands");
            }

            // We decided positionals cant have name clashes with args for clarity
            ensure_unique(name);

            auto& ref = _args.emplace_back(shm::positional(name));

            // Save off the lookup
            _lookup.emplace(std::string(name), _args.size()-1);
            // For positional we need a lookup by index registered too
            _positionals.emplace_back(_args.size()-1);

            return std::get<shm::positional>(ref);
        }

    private:
        /*
            Private helpers to clean up other function logic
        */

        // Ensure the uniqueness of long and short names across options, flags, positionals and subcommands
        void ensure_unique(std::string_view name) {
            // Check flags, options & positionals
            if (auto it = _lookup.find(name); it != _lookup.end()) {
                // Visit to find out what type already owns this name
                std::visit([&](auto& var) {
                    using T = std::decay_t<decltype(var)>;
                    if constexpr (std::is_same_v<T, shm::flag>) {
                        throw std::logic_error(std::format("'{}' is already registered as a flag", name));
                    }
                    else if constexpr (std::is_same_v<T, shm::option>) {
                        throw std::logic_error(std::format("'{}' is already registered as an option", name));
                    }
                    else if constexpr (std::is_same_v<T, shm::positional>) {
                        throw std::logic_error(std::format("'{}' is already registered as a positional argument", name));
                    }
                    else {
                        static_assert(sizeof(T) == 0, "Unhandled argument type in ensure_unique");
                    }
                }, _args[it->second]);
            }
            // Check commands too
            if (_commands.contains(name)) {
                throw std::logic_error(std::format("'{}' is already registered as a subcommand", name));
            }
        }

        // Helpers for parse_result
        // At parse-time did we run into a subcommand?
        [[nodiscard]] bool has_active_subcommand() const noexcept {
            return _active_subcommand.has_value();
        }

        // If we did, what subcommand was active?
        [[nodiscard]] std::string_view active_subcommand() const {
            return _active_subcommand.value();
        }

        // Is this a valid subcommand name for any known subcommands of this command?
        [[nodiscard]] bool is_known_subcommand(std::string_view name) const {
            return _commands.contains(name);
        }

        // Pass me the subcommand's command class i.e. the spec
        [[nodiscard]] const command& get_subcommand(std::string_view name) const {
            auto it = _commands.find(name);
            if (it == _commands.end()) {
                throw std::runtime_error(std::format("Unknown subcommand '{}'", name));
            }
            return it->second;
        }

    protected:
        /*
            Simplifies the lookup of a flag, option or positional argument which is done often
            for std::visit. We have a const version used internally for lookup, but we need a
            non-const version for modification at parse-time because we moved the storage
            alongside the spec.
        */
        [[nodiscard]] const argument& resolve(std::string_view name) const {
            auto it = _lookup.find(name);
            if (it == _lookup.end()) {
                throw std::logic_error(std::format("No argument named '{}' registered in the cli specification", name));
            }
            return _args[it->second];
        }

        [[nodiscard]] argument& resolve(std::string_view name) {
            auto it = _lookup.find(name);
            if (it == _lookup.end()) {
                throw std::runtime_error(std::format("Unrecognized argument '{}'", name));
            }
            return _args[it->second];
        }

        [[nodiscard]] const std::string& name() const noexcept {
            return _name;
        }

    private:
        // Arguments
        std::deque<argument> _args;

        // Lookup from short or long names
        std::map<std::string, size_t, std::less<>> _lookup;

        // Positionals need to be looked up by index in which they occur at parse time
        std::vector<size_t> _positionals;

        // Subcommands
        std::map<std::string, command, std::less<>> _commands;

        // For help
        std::string _name;
        std::string _description;
        std::string _path;

        // Parsetime storage
        // During parsing did we encounter a subcommand? And which one was it?
        std::optional<std::string> _active_subcommand;

        // Storing this information makes help building easier
        bool _has_flags = false;
        bool _has_options = false;
    };

    /*
        The parse_result is now essentially a thin wrapper over the command
        spec classes because we moved the runtime storage alongside the spec.
        This made a lot of things cleaner as originally to provide helpful
        errors and correct access to the result we needed to know a decent
        amount about the spec, i.e. we should reject get<std::string> on a
        bool flag etc. This meant we had to distill the relevant parts of the
        spec into the result object which resulted in a lot of code duplication.
        In our new setup the result has access to the full spec which removes all
        of that code duplication for the distillation of the spec into the result.
        It does now impose a limit of the fact that a command spec can only be parsed
        once but we can enforce that and realistically there's no reason a cli
        should need to be parsed multiple times. At least for now, we can always revisit
        this in the future if a reason shows itself.
    */
    class parse_result {
        /*
            We don't want a user instantiating their own result so make the constructor
            private and make cli a friend.
        */
        friend class cli;

        /*
            // Constructors
            explicit parse_result(const command& spec)

            // Access
            [[nodiscard]] bool specified(std::string_view name) const
            [[nodiscard]] size_t count(std::string_view name) const

            template<typename T>
            requires (not std::is_reference_v<T>) && (not detail::cli::is_vector<T>) && detail::cli::parseable<T>
            [[nodiscard]] T get(std::string_view name) const

            template<typename T>
            requires (not std::is_reference_v<T>) && (detail::cli::is_vector<T>) &&
                detail::cli::parseable<typename T::value_type>
            [[nodiscard]] T get(std::string_view name) const

            [[nodiscard]] bool has_subcommand() const noexcept
            [[nodiscard]] std::string_view subcommand() const

            [[nodiscard]] parse_result operator[] (std::string_view name) const

            void dump(size_t indent = 0) const
        */

    private:
        explicit parse_result(const command& spec) : _spec(spec) { }

    // Public api for querying the result object
    public:
        // Has this option/flag/positional/subcommand been seen?
        // Should be safe as the spec guarantees uniqueness
        [[nodiscard]] bool specified(std::string_view name) const {
            if (_spec.is_known_subcommand(name)) {
                return _spec.has_active_subcommand() && _spec.active_subcommand() == name;
            }
            // If it wasn't the subcommand it should be an argument or positional
            return std::visit([](auto& var) -> bool {
                return var.seen();
            }, _spec.resolve(name));
        }

        // Count returns how many times an option or argument was seen, non-repeatable arguments will always return 0 or 1
        [[nodiscard]] size_t count(std::string_view name) const {
            return std::visit([&](auto& var) -> size_t {
                using V = std::decay_t<decltype(var)>;
                if constexpr (std::is_same_v<V, positional>) {
                    throw std::runtime_error(std::format("count() cannot be called on positional '{}'", name));
                }
                else if constexpr (std::is_same_v<V, flag> || std::is_same_v<V, option>) {
                    return var.count();
                }
                else {
                    static_assert(sizeof(V) == 0, "Unhandled type in count()");
                }
            }, _spec.resolve(name));
        }

        /*
            Templated extraction
        */
        template<typename T>
        requires (not std::is_reference_v<T>) && (not detail::cli::is_vector<T>) && detail::cli::parseable<T>
        [[nodiscard]] T get(std::string_view name) const {
            return std::visit([&](auto& var) -> T {
                // Need to know var type to validate T
                using V = std::decay_t<decltype(var)>;
                if constexpr (std::is_same_v<V, flag>) {
                    // Flags must be collected as a bool
                    if constexpr(std::is_same_v<T, bool>) {
                       return var.value();
                    }
                    else {
                        throw std::runtime_error(std::format("Flags like '{}' must be retrieved via get<bool>()", name));
                    }
                }
                else if constexpr(std::is_same_v<V, option>) {
                    if constexpr (std::is_same_v<T, bool>) {
                        throw std::runtime_error(std::format("Options like '{}' should not be collected via get<bool>(), use a flag instead", name));
                    }
                    else if (var.is_repeatable()) {
                        throw std::runtime_error(std::format("Option '{}' is repeatable, use get<std::vector<T>>()", name));
                    }
                    else if (var.seen()) {
                        return detail::cli::parse<T>(var.values().front());
                    }
                    else if (var.has_default()) {
                        return shm::detail::cli::parse<T>(var.default_value());
                    }
                    else {
                        throw std::runtime_error(std::format("Option '{}' was not specified and has no default", name));
                    }
                }
                else if constexpr(std::is_same_v<V, positional>) {
                    // The spec should guarantee that all positional args have been seen, but I want an explicit exception
                    // for if we ever change logic and there's a chance I'd miss this.
                    if (not var.seen()) {
                        throw std::runtime_error(std::format("Positional '{}' was not provided", name));
                    }
                    return detail::cli::parse<T>(var.value());
                }
                else {
                    static_assert(sizeof(V) == 0, "Unhandled type in get<T>()");
                }
            }, _spec.resolve(name));
        }

        template<typename T>
        requires (not std::is_reference_v<T>) && (detail::cli::is_vector<T>) &&
            detail::cli::parseable<typename T::value_type>
        [[nodiscard]] T get(std::string_view name) const {
            return std::visit([&](auto& var) -> T {
                using V = std::decay_t<decltype(var)>;
                if constexpr (std::is_same_v<V, flag>) {
                    throw std::runtime_error(std::format("Flag '{}' must be retrieved via get<bool>()", name));
                }
                else if constexpr(std::is_same_v<V, option>) {
                    if (not var.is_repeatable()) {
                        throw std::runtime_error(std::format("Option '{}' is not repeatable, use get<T>()", name));
                    }
                    T extract;
                    if (var.seen()) {
                        for (auto& elem : var.values()) {
                            extract.emplace_back(shm::detail::cli::parse<typename T::value_type>(elem));
                        }
                    }
                    else if (var.has_default()) {
                        extract.emplace_back(shm::detail::cli::parse<typename T::value_type>(var.default_value()));
                    }
                    return extract;
                }
                else if constexpr(std::is_same_v<V, positional>) {
                    throw std::runtime_error(std::format("Positional '{}' is not repeatable, use get<T>()", name));
                }
                else {
                    static_assert(sizeof(V) == 0, "Unhandled variant type in get<std::vector<T>>()");
                }
            }, _spec.resolve(name));
        }

        // Was any subcommand specified?
        [[nodiscard]] bool has_subcommand() const noexcept {
            return _spec.has_active_subcommand();
        }

        // Get the name of the active subcommand
        [[nodiscard]] std::string_view subcommand() const {
            if (not _spec.has_active_subcommand()) {
                throw std::runtime_error("No subcommand was specified");
            }
            return _spec.active_subcommand();
        }

        // Access the active subcommand result
        [[nodiscard]] parse_result operator[] (std::string_view name) const {
            if (_spec._commands.empty()) {
                throw std::runtime_error(std::format("'{}' has no subcommands registered", _spec._name));
            }
            if (not _spec.has_active_subcommand()) {
                throw std::runtime_error("No subcommand was specified");
            }
            if (_spec.active_subcommand() != name) {
                throw std::runtime_error(std::format("Subcommand '{}' was not specified, '{}' was", name, _spec.active_subcommand()));
            }
            // If everything is okay return a new parse_result wrapping the subcommand
            return parse_result(_spec.get_subcommand(name));
        }

        void dump(size_t indent = 0) const {
            std::string pad(indent * 2, ' ');

            std::cout << pad << "Parse Result:\n";

            for (auto& [name, idx] : _spec._lookup) {
                std::visit([&](auto& var) {
                    using V = std::decay_t<decltype(var)>;
                    if constexpr (std::is_same_v<V, flag>) {
                        if (var.seen()) {
                            std::cout << std::format("{}  [flag]   {} = {} (count: {})\n",
                                pad, name, var.value(), var.count());
                        }
                        else {
                            std::cout << std::format("{}  [flag]   {} = {} ({})\n",
                               pad, name, var.value(), var.has_explicit_default() ? "explicit default" : "implicit default");
                        }
                    } else if constexpr (std::is_same_v<V, option>) {
                        if (var.seen()) {
                            if (var.is_repeatable()) {
                                std::cout << std::format("{}  [option] {} = [", pad, name);
                                for (auto& v : var.values()) {
                                    std::cout << std::format("'{}' ", v);
                                }
                                std::cout << "]\n";
                            } else {
                                std::cout << std::format("{}  [option] {} = '{}'\n", pad, name, var.values().front());
                            }
                        } else if (var.has_default()) {
                            if (var.is_repeatable()) {
                                std::cout << std::format("{}  [option] {} = [{}]\n", pad, name, var.default_value());
                            }
                            else {
                                std::cout << std::format("{}  [option] {} = '{}' (default)\n", pad, name, var.default_value());
                            }
                        } else {
                            std::cout << std::format("{}  [option] {} = (not set)\n", pad, name);
                        }
                    }
                    else if constexpr (std::is_same_v<V, positional>) {
                        if (var.seen()) {
                            std::cout << std::format("{}  [positional] {} = '{}'\n", pad, name, var.value());
                        } else {
                            std::cout << std::format("{}  [positional] {} = (not set)\n", pad, name);
                        }
                    }
                }, _spec._args[idx]);
            }

            if (_spec.has_active_subcommand()) {
                std::cout << std::format("\n{}Subcommand: {}\n", pad, _spec.active_subcommand());
                operator[](_spec.active_subcommand()).dump(indent + 1);
            }
        }

    private:
        const command& _spec;
    };

    /*
        cli class is the top level class that you create, the idea is that the recursive command
        class is inherited and has a protected constructor to prevent direct access by the user.
        The whole idea is to make the API as difficult as possible to misuse.
    */
    class cli : public command {
        /*
            // Constructors
            explicit cli(std::string_view name, std::string_view description = {})

            // Copy & Move are deleted

            // Parsing
            [[nodiscard]] parse_result parse(int argc, char* argv[])

            // Version
            cli& version(std::string_view version)
        */
    public:
        // Constructor
        explicit cli(std::string_view name, std::string_view description = {}) : command(name, description, name) {};

        // Non-copyable and non-movable
        cli(const cli&) = delete;
        cli& operator=(const cli&) = delete;
        cli(cli&&) = delete;
        cli& operator=(cli&&) = delete;

        [[nodiscard]] parse_result parse(int argc, char* argv[]) {
            if (_parsed) {
                throw std::logic_error("cli has already been parsed which mutates state, create a new cli object to parse again");
            }
            _parsed = true;

            try {
                // Parse in accordance with our held spec
                command::parse(argc - 1, &argv[1]);
            }
            catch (const detail::cli::help_error& e) {
                /*
                    The only exception we catch from the parser is our help exception for
                    exiting the parser loop so we can print the help and cleanly exit. We
                    do it this way rather than exiting in the parse loop directly to make
                    it easier for a user to modify the behavior if they choose to.
                */
                std::cout << e.what() << std::endl;
                std::exit(0);
            }

            // After parsing but before final validation we can now do our version check
            // Resolve is safe if has_value() is true as flag is set by same function
            if (_version.has_value() && std::get<shm::flag>(resolve("version")).seen()) {
                std::cout << std::format("{} version {}", name(), _version.value()) << std::endl;
                std::exit(0);
            }

            // Finally validate
            validate();

            // Return the parse_result which holds a reference to the spec
            return parse_result(static_cast<const command&>(*this));
        }

        cli& version(std::string_view version) {
            if (_version.has_value()) {
                throw std::logic_error("Version has already been set");
            }
            _version = version;
            // Register a --version on ourselves
            flag("version").help("Print version");
            return *this;
        }

    private:
        // Has the cli already been parsed?
        bool _parsed = false;
        std::optional<std::string> _version;
    };
}