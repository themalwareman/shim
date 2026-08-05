#pragma once

/*
    SPDX-License-Identifier: MIT
    Copyright (c) 2026 themalwareman

    shim - small, header-only C++ utilities
    https://github.com/themalwareman/shim

    shm::dotenv - a compatible dotenv parser based on the
                    original spec: https://github.com/motdotla/dotenv
*/

#include "buffer.h"

#include <string>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <map>
#include <regex>
#include <algorithm>
#include <cctype>
#include <array>
#include <ranges>


namespace shm {

    class dotenv {
    public:

        /*
            // Static Methods
            static dotenv parse(std::string_view text)
            static dotenv load(const std::filesystem::path& path)

            // Constructors
            explicit dotenv()

            // Element Access
            [[nodiscard]] std::string& at(const std::string& key)
            [[nodiscard]] const std::string& at(const std::string& key) const
            [[nodiscard]] std::string& operator[](const std::string& key)
            [[nodiscard]] std::string& operator[](std::string&& key)

            // Iterators
            [[nodiscard]] auto begin() noexcept
            [[nodiscard]] auto begin() const noexcept
            [[nodiscard]] auto cbegin() const noexcept
            [[nodiscard]] auto end() noexcept
            [[nodiscard]] auto end() const noexcept
            [[nodiscard]] auto cend() const noexcept

            // Capacity
            [[nodiscard]] bool empty() const
            [[nodiscard]] std::size_t size() const

            // Lookup
            [[nodiscard]] bool contains(const std::string& key) const
        */

        /*
            Static Methods
        */
        static dotenv parse(std::string_view text) {
            // Construct our return value
            dotenv retval;

            /*
                To ensure correct parsing of the dotenv file the plan is to match the parsing code
                found in the official npm package as closely as possible.

                ref:  https://github.com/motdotla/dotenv

                Their default parser is the regex parser which looks like it hasn't been modified
                in many years so is hopefully stable if a little slow
             */

            // Copy the regexes from the official GitHub. Construct them here once as construction is expensive
            static const auto _line = std::regex(R"((?:^|^)\s*(?:export\s+)?([\w.-]+)(?:\s*=\s*?|:\s+?)(\s*'(?:\\'|[^'])*'|\s*"(?:\\"|[^"])*"|\s*`(?:\\`|[^`])*`|[^#\r\n]+)?\s*(?:#.*)?(?:$|$))", std::regex::multiline);
            static const auto _newline_normalizer = std::regex(R"(\r\n?)");
            static const auto _quote_strip = std::regex(R"(^(['"`])([\s\S]*)\1$)", std::regex::multiline);
            static const auto _line_feed = std::regex(R"(\\n)");
            static const auto _carriage_return = std::regex(R"(\\r)");

            // Lambda for finding first non whitespace character
            auto not_space = [](unsigned char c) {
                return not std::isspace(c);
            };

            // Get string_view as modifiable string and normalize line endings in the process
            std::string normalized_text;

            // We use an older overload of regex_replace as when being called from our load method the string_view isn't guaranteed to be null terminated
            std::regex_replace(std::back_inserter(normalized_text), text.begin(), text.end(), _newline_normalizer, "\n");

            // Iterate the file match by match
            for (std::sregex_iterator it{normalized_text.begin(), normalized_text.end(), _line}; it != std::sregex_iterator(); ++it) {
                // Grab the match
                auto& match = *it;

                // Pull the key
                auto key = match[1].str();

                // Now the value, default undefined/empty to empty string
                auto value = match[2].length() != 0 ? match[2].str() : "";

                // Remove whitespace from value
                value.erase(value.begin(), std::ranges::find_if(value, not_space));
                value.erase(std::ranges::find_if(value | std::views::reverse, not_space).base(), value.end());

                if (not value.empty()) {
                    // Save off the possibility of it being double-quoted
                    const char maybeDoubleQuote = value[0];

                    // Remove the surrounding quotes
                    value = std::regex_replace(value, _quote_strip, "$2");

                    // If it was double-quoted then expand newlines
                    if ('"' == maybeDoubleQuote) {
                        value = std::regex_replace(value, _line_feed, "\n");
                        value = std::regex_replace(value, _carriage_return, "\r");
                    }
                }

                // Finally store key
                retval[key] = value;
            }

            return retval;
        }

        static dotenv load(const std::filesystem::path& path) {
            // Declare our return value
            dotenv retval{};

            // Check that the specified file actually exists
            if (std::filesystem::exists(path)) {

                // Grab the file size
                const auto fileSize = std::filesystem::file_size(path);

                // Allocate the read buffer
                shm::buffer<char> fileBuffer(fileSize);

                // Construct the input stream and attempt to read the file
                if (auto input = std::ifstream(path, std::ios::binary); input.read(fileBuffer.data(), fileSize)) {

                    // Grab a string_view of the raw bytes
                    auto view = std::string_view{fileBuffer.data(), fileBuffer.size()};

                    /*
                        I don't love this UTF-8 BOM removal however it seems this is where C++'s
                        and javascript's regex engines diverge. Javascript's implementation of
                        \s will match on the BOM whereas C++'s wont which means leaving it in
                        causes us to miss the env var specified on the first line. For now we
                        will just remove it ourselves.
                    */
                    static constexpr std::array _utf8_bom = {'\xEF', '\xBB', '\xBF'};

                    // Remove utf-8 bom if present
                    if (fileBuffer.size() >= 3 && std::ranges::equal(fileBuffer.first(3), _utf8_bom)) {
                        view.remove_prefix(3);
                    }

                    // Forward to our static parse method
                    retval = dotenv::parse(view);
                }
                else {
                    throw std::runtime_error("failed to read the specified file");
                }
            }
            else {
                throw std::runtime_error("file at specified path does not exist");
            }

            return retval;
        }

        // Constructor
        dotenv() = default;

        /*
            Element Access
        */
        [[nodiscard]] std::string& at(const std::string& key) {
            return _env_vars.at(key);
        }

        [[nodiscard]] const std::string& at(const std::string& key) const {
            return _env_vars.at(key);
        }

        [[nodiscard]] std::string& operator[](const std::string& key) {
            return _env_vars[key];
        }

        [[nodiscard]] std::string& operator[](std::string&& key) {
            return _env_vars[std::move(key)];
        }

        /*
            Iterators
        */
        [[nodiscard]] auto begin() noexcept {
            return _env_vars.begin();
        }

        [[nodiscard]] auto begin() const noexcept {
            return _env_vars.begin();
        }

        [[nodiscard]] auto cbegin() const noexcept {
            return _env_vars.cbegin();
        }

        [[nodiscard]] auto end() noexcept {
            return _env_vars.end();
        }

        [[nodiscard]] auto end() const noexcept {
            return _env_vars.end();
        }

        [[nodiscard]] auto cend() const noexcept {
            return _env_vars.cend();
        }

        /*
            Capacity
        */
        [[nodiscard]] bool empty() const {
            return _env_vars.empty();
        }

        [[nodiscard]] std::size_t size() const {
            return _env_vars.size();
        }

        /*
            Lookup
        */
        [[nodiscard]] bool contains(const std::string& key) const {
            return _env_vars.contains(key);
        }

    private:
        // Environment variable store
        std::map<std::string, std::string> _env_vars;
    };
}
