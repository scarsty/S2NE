#include "simple_cc.h"

namespace s2ne
{
auto SimpleCC::utf8_length(unsigned char value) -> int
{
    if (value < 0x80) { return 1; }
    if ((value & 0xe0) == 0xc0) { return 2; }
    if ((value & 0xf0) == 0xe0) { return 3; }
    if ((value & 0xf8) == 0xf0) { return 4; }
    return 1;
}

auto SimpleCC::write(const std::string& key, const std::string& value) -> void
{
    std::function<void(const std::string&, Trie&)> insert = [&](const std::string& rest, Trie& node)
    {
        const int length = utf8_length(static_cast<unsigned char>(rest.front()));
        const auto part = rest.substr(0, length);
        if (rest.size() == static_cast<size_t>(length))
        {
            node.children[part].value = value;
            return;
        }
        insert(rest.substr(static_cast<size_t>(length)), node.children[part]);
    };
    if (!key.empty()) { insert(key, root_); }
}

auto SimpleCC::shared() -> SimpleCC&
{
    static SimpleCC instance;
    return instance;
}

auto SimpleCC::configure_once(const std::filesystem::path& directory, bool enabled) -> void
{
    auto& instance = shared();
    instance.enabled_ = enabled;
    if (enabled && !instance.initialized_)
    {
        instance.initialized_ = instance.init({ (directory / "TSCharacters.txt").string(), (directory / "TSPhrases.txt").string() });
    }
}

auto SimpleCC::enabled() -> bool { return shared().enabled_; }

auto SimpleCC::big5_to_utf8(const std::vector<uint8_t>& text) -> std::string
{
    if (text.empty()) { return {}; }
    auto converted = convert_text(text, false);
    if (!converted) { return {}; }
    return { converted->begin(), converted->end() };
}

auto SimpleCC::init(const std::vector<std::string>& files) -> bool
{
    bool loaded = false;
    for (const auto& file : files)
    {
        std::FILE* stream = std::fopen(file.c_str(), "rb");
        if (!stream) { continue; }
        std::fseek(stream, 0, SEEK_END);
        const long length = std::ftell(stream);
        std::fseek(stream, 0, SEEK_SET);
        if (length < 0)
        {
            std::fclose(stream);
            continue;
        }
        std::string content(static_cast<size_t>(length), '\0');
        std::fread(content.data(), 1, content.size(), stream);
        std::fclose(stream);
        size_t cursor = 0;
        while (cursor < content.size())
        {
            const auto separator = content.find('\t', cursor);
            if (separator == std::string::npos) { break; }
            const auto line_end = content.find_first_of("\r\n", separator + 1);
            if (line_end == std::string::npos) { break; }
            auto value = content.substr(separator + 1, line_end - separator - 1);
            if (const auto space = value.find(' '); space != std::string::npos) { value.resize(space); }
            write(content.substr(cursor, separator - cursor), value);
            loaded = true;
            cursor = line_end + 1;
        }
    }
    return loaded;
}

auto SimpleCC::convert(const std::string& source) const -> std::string
{
    std::string output;
    size_t cursor = 0;
    while (cursor < source.size())
    {
        const Trie* node = &root_;
        size_t position = cursor, matched = 0;
        const std::string* replacement = nullptr;
        while (position < source.size())
        {
            const int length = utf8_length(static_cast<unsigned char>(source[position]));
            const auto part = source.substr(position, static_cast<size_t>(length));
            const auto found = node->children.find(part);
            if (found == node->children.end()) { break; }
            position += static_cast<size_t>(length);
            node = &found->second;
            if (!node->value.empty())
            {
                matched = position - cursor;
                replacement = &node->value;
            }
        }
        if (replacement)
        {
            output += *replacement;
            cursor += matched;
        }
        else
        {
            const int length = utf8_length(static_cast<unsigned char>(source[cursor]));
            output.append(source, cursor, static_cast<size_t>(length));
            cursor += static_cast<size_t>(length);
        }
    }
    return output;
}

auto SimpleCC::empty() const -> bool { return root_.children.empty(); }
}    //namespace s2ne
