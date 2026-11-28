#include "original_town_data.h"

namespace s2ne
{
auto try_load_towns_json(const std::filesystem::path& root) -> Result<std::vector<uint8_t>>
{
    const auto path = root / "json" / "towns.json";
    if (!std::filesystem::exists(path))
    {
        return Result<std::vector<uint8_t>>::success({});
    }
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Cannot open town JSON: " + path.string()));
    }
    std::string text(std::istreambuf_iterator<char>(input), {});
    auto json = nlohmann::json::parse(text, nullptr, false);
    if (json.is_discarded() || !json.is_object() || !json.contains("records") || !json.at("records").is_array())
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Invalid town JSON format: " + path.string()));
    }
    const auto& records = json.at("records");
    if (records.size() > 65535)
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Too many town JSON records: " + path.string()));
    }
    std::vector<uint8_t> result(2 + records.size() * 14, 0);
    auto put_word = [&](size_t p, unsigned value)
    {
        if (value > 65535 || p + 1 >= result.size()) { return false; }
        result[p] = uint8_t(value);
        result[p + 1] = uint8_t(value >> 8);
        return true;
    };
    auto put_dword = [&](size_t p, size_t value)
    {
        if (value > 0xFFFFFFFFu || p + 3 >= result.size()) { return false; }
        result[p] = uint8_t(value);
        result[p + 1] = uint8_t(value >> 8);
        result[p + 2] = uint8_t(value >> 16);
        result[p + 3] = uint8_t(value >> 24);
        return true;
    };
    if (!put_word(0, unsigned(records.size())))
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Invalid town JSON count: " + path.string()));
    }
    for (size_t i = 0; i < records.size(); ++i)
    {
        const auto& source = records.at(i);
        if (!source.is_object() || source.value("id", 0u) != i + 1 || !source.contains("name") || !source.at("name").is_string() || !source.contains("x") || !source.contains("y") || !source.contains("services_allowed") || !source.contains("offers"))
        {
            return Result<std::vector<uint8_t>>::failure(make_error("Invalid town JSON record: " + path.string()));
        }
        const auto& offers = source.at("offers");
        if (!offers.is_object())
        {
            return Result<std::vector<uint8_t>>::failure(make_error("Invalid town JSON offers: " + path.string()));
        }
        const std::array<const char*, 3> keys{ "weapons", "armor", "items" };
        std::array<std::vector<unsigned>, 3> values;
        for (size_t category = 0; category < keys.size(); ++category)
        {
            if (!offers.contains(keys[category]) || !offers.at(keys[category]).is_array() || offers.at(keys[category]).size() > 255)
            {
                return Result<std::vector<uint8_t>>::failure(make_error("Invalid town JSON offer list: " + path.string()));
            }
            for (const auto& item : offers.at(keys[category]))
            {
                if (!item.is_number_unsigned() || item.get<unsigned>() > 65535) { return Result<std::vector<uint8_t>>::failure(make_error("Invalid town JSON item: " + path.string())); }
                values[category].push_back(item.get<unsigned>());
            }
        }
        auto encode = [&](const std::string& utf8) -> Result<std::vector<uint8_t>>
        {
            if (utf8.empty()) { return Result<std::vector<uint8_t>>::success({}); }
            std::vector<uint8_t> name;
            if (!utf8_to_big5(utf8, name)) { return Result<std::vector<uint8_t>>::failure(make_error("Town name is not CP950 encodable: " + path.string())); }
            return Result<std::vector<uint8_t>>::success(std::move(name));
        };
        auto encoded = encode(source.at("name").get<std::string>());
        if (!encoded) { return encoded; }
        const auto& name = encoded.value();
        size_t pointer = result.size();
        result.insert(result.end(), name.begin(), name.end());
        result.push_back(0);
        for (const auto& list : values)
        {
            for (unsigned item : list)
            {
                result.push_back(uint8_t(item));
                result.push_back(uint8_t(item >> 8));
            }
        }
        if (!source.contains("rumors") || !source.at("rumors").is_array() || source.at("rumors").size() != 2)
        {
            return Result<std::vector<uint8_t>>::failure(make_error("Town JSON requires two rumors: " + path.string()));
        }
        for (const auto& rumor : source.at("rumors"))
        {
            if (!rumor.is_string()) { return Result<std::vector<uint8_t>>::failure(make_error("Invalid town JSON rumor: " + path.string())); }
            auto text = encode(rumor.get<std::string>());
            if (!text) { return text; }
            result.insert(result.end(), text.value().begin(), text.value().end());
            result.push_back(0);
        }
        size_t record = 2 + i * 14;
        if (!put_dword(record, pointer) || !put_word(record + 6, source.at("x").get<unsigned>()) || !put_word(record + 8, source.at("y").get<unsigned>()) || !put_word(record + 10, source.at("services_allowed").get<bool>() ? 1 : 0))
        {
            return Result<std::vector<uint8_t>>::failure(make_error("Invalid town JSON coordinates: " + path.string()));
        }
        for (size_t category = 0; category < values.size(); ++category) { result[record + 11 + category] = uint8_t(values[category].size()); }
    }
    return Result<std::vector<uint8_t>>::success(std::move(result));
}
}    //namespace s2ne
