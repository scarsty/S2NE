#include "resource_result.h"

namespace s2ne
{
auto read_resource(const std::filesystem::path& path) -> Result<std::vector<uint8_t>>
{
    StartupTrace trace("Resource");
    auto resolved = path;
    if (!std::filesystem::exists(resolved) && std::filesystem::exists(resolved.parent_path().parent_path() / resolved.filename()))
    {
        resolved = resolved.parent_path().parent_path() / resolved.filename();
    }

    if (resolved.filename() == "SUPERSP.PRG")
    {
        return ResourceBundle(resolved.parent_path()).try_prg_image();
    }
    std::ifstream file(resolved, std::ios::binary | std::ios::ate);
    if (!file)
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Cannot open resource: " + path.string()));
    }
    const auto length = file.tellg();
    if (length < 0 || uint64_t(length) > std::vector<uint8_t>().max_size())
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Invalid resource length: " + path.string()));
    }
    std::vector<uint8_t> bytes(size_t(length), uint8_t{});
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), std::streamsize(bytes.size())))
    {
        return Result<std::vector<uint8_t>>::failure(make_error("Cannot read resource: " + path.string()));
    }
    trace.mark(resolved.filename().string() + " bytes=" + std::to_string(bytes.size()));
    return Result<std::vector<uint8_t>>::success(std::move(bytes));
}

auto read_shared_resource(const std::filesystem::path& path, ResourceDomain domain) -> Result<SharedBytes>
{
    auto resolved = path;
    if (!std::filesystem::exists(resolved) && std::filesystem::exists(resolved.parent_path().parent_path() / resolved.filename())) { resolved = resolved.parent_path().parent_path() / resolved.filename(); }
    const bool prg = resolved.filename() == "SUPERSP.PRG";
    const auto identity = prg ? "domain=" + std::to_string(unsigned(domain)) + ";palette=" + std::filesystem::weakly_canonical(resolved.parent_path() / "SUPERSP.PAL").string() : "";
    return load_shared_resource(resolved, identity, [&]() -> Result<std::vector<uint8_t>>
        {
            if (prg) { return ResourceBundle(resolved.parent_path()).try_prg_image(domain); }
            return read_resource(resolved);
        });
}

ResourceReader::ResourceReader(std::span<const uint8_t> bytes) : bytes_(bytes) {}

auto ResourceReader::extent(size_t position, size_t count) -> bool
{
    if (position > bytes_.size() || count > bytes_.size() - position)
    {
        reject("Truncated resource field");
        return false;
    }
    return true;
}

auto ResourceReader::number(size_t position, unsigned count) -> unsigned
{
    if (!extent(position, count))
    {
        return 0;
    }
    unsigned value = 0;
    for (unsigned i = 0; i < count; ++i)
    {
        value |= unsigned(bytes_[position + i]) << (8 * i);
    }
    return value;
}

auto ResourceReader::reject(std::string message) -> void
{
    if (!error_)
    {
        error_ = make_error(std::move(message));
    }
}

ResourceReader::operator bool() const { return !error_; }

auto ResourceReader::error() const -> const Error& { return *error_; }

auto read_combat_resources(const std::filesystem::path& root) -> Result<CombatResources>
{
    auto bin = read_shared_resource(root / "SUPERSP.BIN"), pal = read_shared_resource(root / "SUPERSP.PAL");
    if (!bin)
    {
        return Result<CombatResources>::failure(bin.error());
    }
    if (!pal)
    {
        return Result<CombatResources>::failure(pal.error());
    }
    CombatResources result{ std::move(bin).value(), std::move(pal).value(), 0 };
    ResourceReader p(result.pal), b(result.bin);
    unsigned address = p.number(920, 4);
    if (!p)
    {
        return Result<CombatResources>::failure(p.error());
    }
    if (address < 20)
    {
        return Result<CombatResources>::failure(make_error("Invalid combat configuration address"));
    }
    size_t c = address - 20;
    c += 2 + 4 * size_t(b.number(c));
    c += 2 + b.number(c);
    c += 2;
    if (!b.extent(c, 256) || !b)
    {
        return Result<CombatResources>::failure(b.error());
    }
    result.config = c;
    return Result<CombatResources>::success(std::move(result));
}
}    //namespace s2ne
