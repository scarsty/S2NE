#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <deque>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <windows.h>
namespace s2ne
{
class StdioProtocol
{
    HANDLE input_ = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE output_ = GetStdHandle(STD_OUTPUT_HANDLE);
    std::string partial_;
    std::deque<std::string> lines_;
    bool oversized_{};
    bool eof_{};

public:
    using Json = nlohmann::json;
    bool eof() const { return eof_ && lines_.empty(); }
    void respond(const Json& response)
    {
        const auto bytes = response.dump(-1, ' ', false, Json::error_handler_t::replace) + "\n";
        DWORD written{};
        if (!WriteFile(output_, bytes.data(), DWORD(bytes.size()), &written, nullptr) || written != bytes.size()) { eof_ = true; }
    }
    void feed(std::string_view bytes)
    {
        for (char c : bytes)
        {
            if (c == '\n')
            {
                if (oversized_) { lines_.emplace_back(); }
                else
                {
                    lines_.push_back(std::move(partial_));
                }
                partial_.clear();
                oversized_ = false;
            }
            else if (!oversized_)
            {
                if (partial_.size() == 65536)
                {
                    oversized_ = true;
                    partial_.clear();
                }
                else
                {
                    partial_ += c;
                }
            }
        }
    }
    void poll()
    {
        if (eof_ || lines_.size() >= 16) { return; }
        DWORD available{};
        if (GetFileType(input_) == FILE_TYPE_PIPE)
        {
            if (!PeekNamedPipe(input_, nullptr, 0, nullptr, &available, nullptr))
            {
                finish();
                return;
            }
            if (!available) { return; }
        }
        else if (GetFileType(input_) == FILE_TYPE_DISK) { available = 4096; }
        else
        {
            respond({ { "ok", false }, { "error", "stdin must be a pipe or file" } });
            finish();
            return;
        }
        char bytes[4096];
        DWORD read{};
        if (!ReadFile(input_, bytes, std::min<DWORD>(available, sizeof(bytes)), &read, nullptr) || !read)
        {
            finish();
            return;
        }
        feed(std::string_view(bytes, read));
    }
    void finish()
    {
        if (!partial_.empty() || oversized_)
        {
            lines_.push_back(std::move(partial_));
            partial_.clear();
        }
        eof_ = true;
    }
    std::optional<Json> next()
    {
        if (lines_.empty()) { return {}; }
        auto line = std::move(lines_.front());
        lines_.pop_front();
        auto parsed = Json::parse(line, nullptr, false);
        if (parsed.is_discarded() || !parsed.is_object()) { return Json{ { "protocol_error", "invalid JSON object or oversized line" } }; }
        return parsed;
    }
};
}    //namespace s2ne
