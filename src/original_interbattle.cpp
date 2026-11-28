#include "original_interbattle.h"

namespace s2ne
{
auto OriginalInterbattle::caption(std::vector<uint8_t>& image, unsigned id, int x, int y) const -> void
{
    const auto text = font_.text_bytes(id);
    // Font helper uses global colors; the PCX menu owns its palette.
    for (unsigned ink : { 2u, 3u })
    {
        std::vector<uint8_t> mask(image.size());
        int delta = ink == 2 ? 1 : 0;
        font_.draw_small(mask, text, x + delta, y + delta, ink);
        for (size_t p = 0; p < mask.size(); p += 4)
        {
            if (mask[p + 3])
            {
                for (unsigned k = 0; k < 3; ++k)
                {
                    const unsigned color = opening_palette_channel(uint8_t(caption_palette_[ink * 3 + k] >> 2));
                    const unsigned alpha = mask[p + 3];
                    image[p + k] = uint8_t((color * alpha + unsigned(image[p + k]) * (255 - alpha) + 127) / 255);
                }
                image[p + 3] = 255;
            }
        }
    }
}

OriginalInterbattle::OriginalInterbattle(OriginalMenu font, InitializationTag) : font_(std::move(font)) {}

auto OriginalInterbattle::initialize(const std::filesystem::path& root, OriginalDisc disc, unsigned resource) -> s2ne::Result<bool>
{
    if (resource != 12 && resource != 14 && resource != 15 && resource != 13)
    {
        return s2ne::Result<bool>::failure(make_error("Unsupported original interbattle background"));
    }
    auto dat_result = OriginalDiscResources(root).try_dat_entry(disc, 12);
    if (!dat_result)
    {
        return s2ne::Result<bool>::failure(dat_result.error());
    }
    auto dat = std::move(dat_result).value();
    if (dat.size() < 769 || dat[dat.size() - 769] != 12) { return s2ne::Result<bool>::failure(make_error("Missing hub caption palette")); }
    caption_palette_.assign(dat.end() - 768, dat.end());
    auto external = texture_resources.load_image_scaled("interbattle", resource, screen_width, screen_height);
    if (!external) { return s2ne::Result<bool>::failure(external.error()); }
    wide_background_ = external.value().pixels;
    set_fixed_page_background(wide_background_);
    background_ = fixed_page_center();
    return s2ne::Result<bool>::success(true);
}

OriginalInterbattle::OriginalInterbattle(const std::filesystem::path& root, OriginalDisc disc, unsigned resource) : font_(root)
{
    auto result = initialize(root, disc, resource);
    if (!result)
    {
        fatal(result.error());
    }
}

auto OriginalInterbattle::create(const std::filesystem::path& root, OriginalDisc disc, unsigned resource) -> s2ne::Result<std::unique_ptr<OriginalInterbattle>>
{
    auto font = OriginalMenu::create(root);
    if (!font) { return s2ne::Result<std::unique_ptr<OriginalInterbattle>>::failure(font.error()); }
    auto result = std::make_unique<OriginalInterbattle>(std::move(font).value(), InitializationTag{});
    auto ready = result->initialize(root, disc, resource);
    if (!ready)
    {
        return s2ne::Result<std::unique_ptr<OriginalInterbattle>>::failure(ready.error());
    }
    return s2ne::Result<std::unique_ptr<OriginalInterbattle>>::success(std::move(result));
}

auto OriginalInterbattle::background() const -> const std::vector<uint8_t>&
{
    if (!wide_background_.empty()) { fixed_page_background = wide_background_; }
    return background_;
}

auto OriginalInterbattle::town_services_allowed(const std::filesystem::path& root, unsigned town) -> bool
{
    auto pal = read_shared_resource(root / "SUPERSP.PAL").value();
    auto bin = read_shared_resource(root / "SUPERSP.BIN").value();
    size_t table = dword(pal, 872) - 14;
    if (!town || town > word(bin, table)) { return false; }
    return bin.at(table + 2 + (town - 1) * 14 + 10) == 1;
}

auto OriginalInterbattle::try_interbattle_town(const std::filesystem::path& root, unsigned town) -> s2ne::Result<unsigned>
{
    if (town >= 201)
    {
        const auto path = root / "json" / ("stage-" + std::to_string(town) + ".json");
        std::ifstream input(path, std::ios::binary);
        if (!input) { return s2ne::Result<unsigned>::failure(make_error("Cannot open third-route interbattle JSON: " + path.string())); }
        auto stage = nlohmann::json::parse(input, nullptr, false);
        if (stage.is_discarded() || !stage.is_object())
        {
            return s2ne::Result<unsigned>::failure(make_error("Invalid third-route interbattle JSON: " + path.string()));
        }
        if (!stage.contains("route") || !stage.at("route").is_object()
            || !stage.at("route").contains("interbattle_town_id")
            || !stage.at("route").at("interbattle_town_id").is_number_unsigned()
            || stage.at("route").at("interbattle_town_id").get<uint64_t>() > std::numeric_limits<unsigned>::max())
        {
            return s2ne::Result<unsigned>::failure(make_error("Missing or invalid route.interbattle_town_id: " + path.string()));
        }
        town = stage.at("route").at("interbattle_town_id").get<unsigned>();
    }
    return s2ne::Result<unsigned>::success(town);
}

auto OriginalInterbattle::try_configure_town(const std::filesystem::path& root, unsigned town) -> s2ne::Result<bool>
{
    const auto resolved = try_interbattle_town(root, town);
    if (!resolved)
    {
        return s2ne::Result<bool>::failure(resolved.error());
    }
    town = resolved.value();
    services_allowed_ = town_services_allowed(root, town);
    selected_ = services_allowed_ ? 1 : 4;
    pending_ = !services_allowed_;
    return s2ne::Result<bool>::success(true);
}

auto OriginalInterbattle::configure_town(const std::filesystem::path& root, unsigned town) -> bool
{
    auto result = try_configure_town(root, town);
    if (!result) { fatal(result.error()); }
    return result.value();
}

auto OriginalInterbattle::services_allowed() const -> bool { return services_allowed_; }

auto OriginalInterbattle::draw_caption(std::vector<uint8_t>& image, unsigned id, int x, int y) const -> void { caption(image, id, x, y); }

auto OriginalInterbattle::selected() const -> int { return selected_; }

auto OriginalInterbattle::closed() const -> bool { return closed_; }

auto OriginalInterbattle::pending() const -> bool { return pending_; }

auto OriginalInterbattle::hit(int x, int y) -> int
{
    for (unsigned i = 0; i < rectangles.size(); ++i)
    {
        auto r = rectangles[i];
        if (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom)
        {
            return int(i) + 1;
        }
    }
    return 0;
}

auto OriginalInterbattle::mouse(int x, int y, bool left_click) -> void
{
    if (closed_ || pending_ || !services_allowed_)
    {
        return;
    }
    pointer_x_ = x;
    pointer_y_ = y;
    int row = hit(x, y);
    selected_ = row;
    if (row)
    {
        if (left_click)
        {
            pending_ = true;
        }
    }
}

auto OriginalInterbattle::select_option(unsigned index) -> bool
{
    if (closed_ || pending_ || index >= labels.size() || (!services_allowed_ && index != 3)) { return false; }
    selected_ = int(index + 1);
    pending_ = true;
    return true;
}

auto OriginalInterbattle::scan(unsigned code) -> void
{
    if (closed_ || pending_ || !services_allowed_)
    {
        return;
    }
    // 410A3F: Left AND Right AND Tab all advance; Escape is disabled at hub.
    if (code == 0x4B || code == 0x4D || code == 0x0F)
    {
        selected_ = selected_ % 4 + 1;
        auto r = rectangles[size_t(selected_ - 1)];
        pointer_x_ = (r.left + r.right) / 2;
        pointer_y_ = (r.top + r.bottom) / 2;
    }
    else if ((code == 0x1C || code == 0x39) && selected_ > 0)
    {
        pending_ = true;
    }
}

auto OriginalInterbattle::request() const -> std::optional<Request>
{
    if (!pending_ || closed_)
    {
        return std::nullopt;
    }
    size_t row = size_t(selected_ - 1);
    return Request{ unsigned(selected_), labels[row], targets[row] };
}

auto OriginalInterbattle::dispatch(const std::function<Result(Request)>& adapter) -> bool
{
    auto req = request();
    if (!req || !adapter)
    {
        return false;
    }
    Result result = adapter(*req);
    if (result == Result::unsupported)
    {
        return false;
    }
    if (result == Result::enter_battle && req->option != 4)
    {
        return false;
    }
    pending_ = false;
    closed_ = result == Result::enter_battle || result == Result::exit_to_title;
    return true;
}

auto OriginalInterbattle::draw(bool all_captions) const -> std::vector<uint8_t>
{
    auto image = background();
    if (all_captions)
    {
        for (size_t i = 0; i < labels.size(); ++i)
        {
            if (i < 3 && !services_allowed_) { continue; }
            auto r = rectangles[i];
            auto t = font_.text_bytes(labels[i]);
            int x = std::clamp((r.left + r.right) / 2 - int(t.size()) * 4, 0, 639 - int(t.size()) * 8);
            caption(image, labels[i], x, (r.top + r.bottom) / 2);
        }
    }
    else if (selected_ > 0 && (selected_ == 4 || services_allowed_))
    {
        auto text = font_.text_bytes(labels[size_t(selected_ - 1)]);
        int x = std::clamp(pointer_x_ - int(text.size()) * 4, 0, 639 - int(text.size()) * 8);
        caption(image, labels[size_t(selected_ - 1)], x, std::max(30, pointer_y_ - 30));
    }
    return image;
}
}    //namespace s2ne
