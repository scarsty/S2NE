#pragma once
#include "text_encoding.h"
#include <cstdio>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace s2ne
{
class SimpleCC
{
    struct Trie
    {
        std::string value;
        std::unordered_map<std::string, Trie> children;
    };
    Trie root_;
    bool initialized_{};
    bool enabled_{};

    static int utf8_length(unsigned char value);
    void write(const std::string& key, const std::string& value);

public:
    static SimpleCC& shared();
    static void configure_once(const std::filesystem::path& directory, bool enabled);
    static bool enabled();
    static std::string big5_to_utf8(const std::vector<uint8_t>& text);
    bool init(const std::vector<std::string>& files);
    std::string convert(const std::string& source) const;
    bool empty() const;
};
}    //namespace s2ne
