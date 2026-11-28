#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "simple_cc.h"
#include <deque>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <poll.h>
#include <unistd.h>
#endif
namespace s2ne
{
class StdioProtocol
{
#ifdef _WIN32
    HANDLE input_ = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE output_ = GetStdHandle(STD_OUTPUT_HANDLE);
#else
    int input_ = STDIN_FILENO;
    int output_ = STDOUT_FILENO;
#endif
    std::string partial_;
    std::deque<std::string> lines_;
    bool oversized_{};
    bool eof_{};

public:
    using Json = nlohmann::json;
    static std::string original_text(const Json& bytes);
    static void describe(Json& value);
    static std::optional<std::string> mismatch(const Json& state, const Json& expected);
    static void verify_response(Json& response);
    class Progress
    {
        Json previous_;
        uint64_t changed_at_{};

    public:
        Json sample(Json state, uint64_t now);
    };
    bool eof() const;
    void respond(const Json& response);
    void feed(std::string_view bytes);
    void poll();
    void finish();
    std::optional<Json> next();
};
}    //namespace s2ne
