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
    static std::string original_text(const Json& bytes)
    {
        if (!bytes.is_array()) { return {}; }
        std::string input;
        for (const auto& byte : bytes)
        {
            if (!byte.is_number_integer() || byte.get<int>() < 0 || byte.get<int>() > 255) { return {}; }
            input += char(byte.get<unsigned>());
        }
        if (input.empty()) { return {}; }
        const int count = MultiByteToWideChar(950, MB_ERR_INVALID_CHARS, input.data(), int(input.size()), nullptr, 0);
        if (!count) { return {}; }
        std::wstring wide(size_t(count), L'\0');
        MultiByteToWideChar(950, MB_ERR_INVALID_CHARS, input.data(), int(input.size()), wide.data(), count);
        const int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), count, nullptr, 0, nullptr, nullptr);
        std::string text(size_t(size), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.data(), count, text.data(), size, nullptr, nullptr);
        return text;
    }
    static void describe(Json& value)
    {
        if (value.is_array())
        {
            for (auto& entry : value) { describe(entry); }
            return;
        }
        if (!value.is_object()) { return; }
        Json additions = Json::object();
        for (auto& [key, entry] : value.items())
        {
            describe(entry);
            if (!key.ends_with("_bytes")) { continue; }
            const auto field = key.substr(0, key.size() - 6);
            auto text = original_text(entry);
            if (text.empty())
            {
                text = "unknown";
                for (const char* id : { "id", "kind", "index" })
                {
                    if (value.contains(id))
                    {
                        text += ":" + value.at(id).dump();
                        break;
                    }
                }
            }
            additions[field] = text;
        }
        value.update(additions);
        if (value.contains("name_bytes")) { value["name_encoding"] = "UTF-8 (Big5/CP950 source)"; }
    }
    static std::optional<std::string> mismatch(const Json& state, const Json& expected)
    {
        if (!expected.is_object()) { return "expected state must be an object"; }
        for (const auto& [key, value] : expected.items())
        {
            if (!state.contains(key) || state.at(key) != value) { return "state mismatch: " + key; }
        }
        return {};
    }
    static void verify_response(Json& response)
    {
        if (!response.contains("expect_after")) { return; }
        auto expected = std::move(response["expect_after"]);
        response.erase("expect_after");
        if (!response.value("ok", false)) { return; }
        if (auto error = mismatch(response.at("state"), expected))
        {
            response["ok"] = false;
            response["error"] = *error;
            response["postcondition_met"] = false;
        }
        else
        {
            response["postcondition_met"] = true;
        }
    }
    class Progress
    {
        Json previous_;
        uint64_t changed_at_{};

    public:
        Json sample(Json state, uint64_t now)
        {
            state.erase("mixed_samples");
            state.erase("music_position");
            const bool changed = previous_ != state;
            if (changed)
            {
                previous_ = std::move(state);
                changed_at_ = now;
            }
            const auto unchanged = now - changed_at_;
            return { { "changed", changed }, { "unchanged_ms", unchanged } };
        }
    };
    bool eof() const { return eof_ && lines_.empty(); }
    void respond(const Json& response)
    {
        auto described = response;
        describe(described);
        const auto bytes = described.dump(-1, ' ', false, Json::error_handler_t::replace) + "\n";
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
