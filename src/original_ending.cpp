#include "original_ending.h"

namespace s2ne
{
auto OriginalEnding::try_class_image_validated(std::vector<uint8_t>& image, unsigned appearance) const -> s2ne::Result<bool>
{
    auto group = [&](unsigned id)
    {
        return size_t(dword(pal_, 808 + 8 * (64 + id - 1)) - 10 - (id - 1));
    };
    size_t index = group(1), frames = group(2);
    if (!appearance || appearance > word(prg_, index))
    {
        return s2ne::Result<bool>::failure(make_error("Invalid ending class appearance"));
    }
    unsigned first = word(prg_, index + 2 + (appearance - 1) * 8 + 2);
    size_t table = frames + 2 + first * 7;
    size_t p = frames + dword(prg_, table), end = p + word(prg_, table + 4);
    unsigned w = word(prg_, p) + 1, h = word(prg_, p + 2) + 1;
    p += 4;
    size_t n = 0;
    while (p < end)
    {
        unsigned op = prg_.at(p++), count = op ? op & 127 : 1, c = 0;
        if (op && !(op & 128))
        {
            c = prg_.at(p++);
        }
        while (count--)
        {
            unsigned ink = op & 128 ? prg_.at(p++) : c;
            if (n >= size_t(w) * h)
            {
                return s2ne::Result<bool>::failure(make_error("Invalid ending class image"));
            }
            int x = 225 - int(w / 2) + int(n % w), y = 180 - int(h) + int(n / w);
            ++n;
            if (ink && x >= 0 && x < 640 && y >= 0 && y < 480)
            {
                size_t q = (size_t(y) * 640 + x) * 4;
                for (unsigned k = 0; k < 3; ++k)
                {
                    image[q + k] = uint8_t(unsigned(pal_.at(ink * 3 + k)) * 255 / 63);
                }
                image[q + 3] = 255;
            }
        }
    }
    if (n != size_t(w) * h)
    {
        return s2ne::Result<bool>::failure(make_error("Truncated ending class image"));
    }

    return s2ne::Result<bool>::success(true);
}

auto OriginalEnding::try_class_image(std::vector<uint8_t>& image, unsigned appearance) const -> s2ne::Result<bool>
{
    auto output = image;
    auto result = try_class_image_validated(output, appearance);
    if (result)
    {
        image.swap(output);
    }
    return result;
}

auto OriginalEnding::class_image(std::vector<uint8_t>& image, unsigned appearance) const -> void
{
    auto result = try_class_image(image, appearance);
    if (!result)
    {
        report_error(result.error());
    }
}

auto OriginalEnding::enter_music() -> void
{
    if (done() || !midi_audio_) { return; }
    const auto& page = sequence_[page_];
    unsigned id = 0;
    // 原41F5DD在整段角色／插画序列前只播一次20，随后41EDD4统计页切13。
    if (!role_music_started_ && (page.role >= 0 || page.illustration >= 0)) { id = 20; }
    else if (page.summary) { id = 13; }
    if (id)
    {
        auto played = midi_audio_(id);
        if (!played) { report_error(played.error()); }
        else if (id == 20) { role_music_started_ = true; }
    }
}

auto OriginalEnding::enter() -> void
{
    if (movie_audio_ && stop_audio_)
    {
        auto stopped = movie_audio_->finish(stop_audio_);
        if (!stopped) { report_error(stopped.error()); }
    }
    elapsed_ = 0;
    movie_.reset();
    movie_frames_ = 0;
    movie_image_.clear();
    if (done())
    {
        return;
    }
    const auto& p = sequence_[page_];
    enter_music();
    if (p.illustration >= 0)
    {
        scroll_ = unsigned(illustrations_.at(p.illustration).size() / (640 * 4)) - 480;
    }
    if (p.movie)
    {
        movie_.emplace(Flic::from_disc(root_, 20 + p.movie, disc_));
        movie_->delay = std::max(10u, (movie_->delay * 10 / 48) * 10);
        if (movie_audio_)
        {
            auto begun = movie_audio_->begin(p.movie, 0);
            if (!begun) { report_error(begun.error()); }
        }
    }
}

auto OriginalEnding::scrolling() const -> bool { return std::find(illustration_pages_.begin(), illustration_pages_.end(), unsigned(page_)) != illustration_pages_.end(); }

auto OriginalEnding::string(size_t p) const -> std::vector<uint8_t>
{
    std::vector<uint8_t> result;
    while (text_.at(p))
    {
        result.push_back(text_.at(p++));
    }
    return result;
}

auto OriginalEnding::try_pcx(const std::filesystem::path& root, unsigned id, OriginalDisc disc) -> s2ne::Result<std::vector<uint8_t>>
{
    auto loaded = OriginalDiscResources(root).try_dat_entry(disc, id);
    if (!loaded) { return s2ne::Result<std::vector<uint8_t>>::failure(loaded.error()); }
    auto b = std::move(loaded).value();
    if (b.size() < 897 || b[0] != 10 || b[2] != 1 || b[3] != 8 || b[65] != 1 || word(b, 8) != 639 || word(b, 10) < 479)
    {
        return s2ne::Result<std::vector<uint8_t>>::failure(make_error("Unsupported native ending PCX"));
    }
    size_t colors = b.size() - 768;
    if (b.at(colors - 1) != 12)
    {
        return s2ne::Result<std::vector<uint8_t>>::failure(make_error("Missing ending palette"));
    }
    unsigned stride = word(b, 66), height = word(b, 10) + 1;
    if (stride < 640)
    {
        return s2ne::Result<std::vector<uint8_t>>::failure(make_error("Invalid ending PCX stride"));
    }
    auto image = texture_resources.load_image_scaled("ending", id, 640, int(height));
    if (!image) { return s2ne::Result<std::vector<uint8_t>>::failure(image.error()); }
    return s2ne::Result<std::vector<uint8_t>>::success(std::move(image).value().pixels);
}

auto OriginalEnding::pcx(const std::filesystem::path& root, unsigned id, OriginalDisc disc) -> std::vector<uint8_t>
{
    auto result = try_pcx(root, id, disc);
    return std::move(result).value();
}

auto OriginalEnding::paragraph(std::vector<uint8_t>& image, const std::vector<uint8_t>& bytes, int x, int y, int width) const -> s2ne::Result<bool>
{
    return font_.try_draw_original_wrapped(image, bytes, x, y, 175, 20, unsigned(width / 28), 28);
}

auto OriginalEnding::role_text(std::vector<uint8_t>& image, const std::vector<uint8_t>& text) const -> s2ne::Result<bool>
{
    size_t p = 0;
    for (unsigned row = 0; row < 30 && p < text.size(); ++row)
    {
        int compensation = 0;
        for (unsigned cell = 0, units = 1; units < 50 && p < text.size(); ++cell)
        {
            unsigned c = text[p++];
            if (!c || c == 10)
            {
                return s2ne::Result<bool>::success(true);
            }
            if (c == '*')
            {
                if (!cell)
                {
                    --cell;
                }
                else
                {
                    break;
                }
                continue;
            }
            std::vector<uint8_t> glyph{ uint8_t(std::max(c, 30u)) };
            if (c > 128)
            {
                if (p >= text.size()) { return s2ne::Result<bool>::failure(make_error("Truncated ending role text")); }
                glyph.push_back(text.at(p++));
                units += 2;
            }
            else
            {
                ++units;
            }
            int x = 120 + int(cell) * 16 - compensation;
            auto drawn = font_.try_draw_small(image, glyph, x, 230 + int(row) * 20, 175, true);
            if (!drawn) { return drawn; }
            if (c <= 128)
            {
                compensation += 8;
            }
        }
    }
    return s2ne::Result<bool>::success(true);
}

OriginalEnding::OriginalEnding(OriginalMenu font, const std::filesystem::path& root, const OriginalEvents& events, OriginalDisc disc) : font_(std::move(font)), events_(events), root_(root), disc_((events.stage == 61 || events.stage == 130 || events.stage == 131) ? OriginalDisc::two : disc) {}

auto OriginalEnding::initialize(const std::filesystem::path& root, const OriginalEvents& events, OriginalDisc disc) -> s2ne::Result<bool>
{
    disc = disc_;
    // 原40DDD5/40D6D6按后续关号为0进入41E9CF；结局入口不按关号或旗标白名单授权。
    auto campaign = OriginalCampaign::create(root);
    if (!campaign) { return s2ne::Result<bool>::failure(campaign.error()); }
    OriginalEvents source = events;
    auto stage_loaded = source.try_load_stage(events.stage);
    if (!stage_loaded) { return s2ne::Result<bool>::failure(stage_loaded.error()); }
    auto route = OriginalCampaign::try_completed_story_route(source, events);
    if (!route) { return s2ne::Result<bool>::failure(route.error()); }
    auto stage = campaign.value().try_stage(events.stage);
    if (!stage) { return s2ne::Result<bool>::failure(stage.error()); }
    unsigned selected = route.value() ? route.value() : stage.value().next;
    if (events.script_route && *events.script_route > 0) { selected = unsigned(*events.script_route); }
    if (selected != 0)
    {
        return s2ne::Result<bool>::failure(make_error("Native terminal outcome not authorized"));
    }
    auto pal_read_result = read_shared_resource(root / "SUPERSP.PAL");
    if (!pal_read_result)
    {
        return s2ne::Result<bool>::failure(pal_read_result.error());
    }
    pal_ = std::move(pal_read_result).value();
    auto cel_read_result = read_shared_resource(root / "SUPERSP.CEL");
    if (!cel_read_result)
    {
        return s2ne::Result<bool>::failure(cel_read_result.error());
    }
    cel_ = std::move(cel_read_result).value();
    auto prg_read_result = read_shared_resource(root / "SUPERSP.PRG");
    if (!prg_read_result)
    {
        return s2ne::Result<bool>::failure(prg_read_result.error());
    }
    prg_ = std::move(prg_read_result).value();
    auto bin_result = read_shared_resource(root / "SUPERSP.BIN");
    if (!bin_result)
    {
        return s2ne::Result<bool>::failure(bin_result.error());
    }
    auto bin = std::move(bin_result).value();
    size_t at = 848 + 16;
    ResourceReader palette(pal_);
    const unsigned raw_offset = palette.number(at, 4), raw_size = palette.number(at + 4, 4);
    if (!palette || raw_offset < 13 || raw_size < 13) { return s2ne::Result<bool>::failure(make_error("Invalid BIN3 ending directory")); }
    size_t offset = raw_offset - 13, size = raw_size - 13;
    if (offset > bin.size() || size > bin.size() - offset)
    {
        return s2ne::Result<bool>::failure(make_error("Invalid BIN3 ending text"));
    }
    text_.assign(bin.begin() + offset, bin.begin() + offset + size);
    for (unsigned i = 0; i < 8; ++i)
    {
        if (events.flags[71 + i])
        {
            variant_ = i;
            break;
        }
    }
    constexpr std::array<unsigned, 13> order{ 11, 1, 22, 2, 3, 4, 14, 15, 18, 32, 16, 17, 42 };
    auto append = [&](unsigned id)
    {
        if (std::any_of(events.units.begin(), events.units.end(), [&](auto& u)
                {
                    return u.id == id && !u.hostile();
                }))
        {
            roles_.push_back(id);
        }
    };
    for (auto id : order)
    {
        append(id);
    }
    for (unsigned id = 1; id <= 36; ++id)
    {
        if (std::find(order.begin(), order.end(), id) == order.end())
        {
            append(id);
        }
    }
    auto background_result = try_pcx(root, 26, disc);
    if (!background_result)
    {
        return s2ne::Result<bool>::failure(background_result.error());
    }
    background_ = std::move(background_result).value();
    auto intro_result = try_pcx(root, 40, disc);
    if (!intro_result)
    {
        return s2ne::Result<bool>::failure(intro_result.error());
    }
    intro_ = std::move(intro_result).value();
    sequence_.push_back(Page{ .text = 0 });
    sequence_.push_back(Page{ .movie = 4 });
    sequence_.push_back(Page{ .text = 1 });
    sequence_.push_back(Page{ .movie = 5 });
    for (size_t r = 0; r < roles_.size(); ++r)
    {
        auto it = std::find(order.begin(), order.end(), roles_[r]);
        if (it != order.end())
        {
            illustration_pages_.push_back(unsigned(sequence_.size()));
            auto illustration = try_pcx(root, 27 + unsigned(it - order.begin()), disc);
            if (!illustration)
            {
                return s2ne::Result<bool>::failure(illustration.error());
            }
            illustrations_.push_back(std::move(illustration).value());
            sequence_.push_back(Page{ .illustration = int(illustrations_.size() - 1) });
        }
        sequence_.push_back(Page{ .role = int(r) });
        sequence_.push_back(Page{ .role = int(r), .relationship = true });
    }
    sequence_.push_back(Page{ .summary = true });
    sequence_.push_back(Page{ .text = 2 });
    sequence_.push_back(Page{ .movie = 3 });
    enter();

    return s2ne::Result<bool>::success(true);
}

OriginalEnding::OriginalEnding(const std::filesystem::path& root, const OriginalEvents& events, OriginalDisc disc) : OriginalEnding(std::move(*create(root, events, disc).value()))
{
}

auto OriginalEnding::create(const std::filesystem::path& root, const OriginalEvents& events, OriginalDisc disc) -> s2ne::Result<std::unique_ptr<OriginalEnding>>
{
    auto font = OriginalMenu::create(root);
    if (!font) { return s2ne::Result<std::unique_ptr<OriginalEnding>>::failure(font.error()); }
    auto result = std::unique_ptr<OriginalEnding>(new OriginalEnding(std::move(font).value(), root, events, disc));
    auto ready = result->initialize(root, events, disc);
    if (!ready)
    {
        return s2ne::Result<std::unique_ptr<OriginalEnding>>::failure(ready.error());
    }
    return s2ne::Result<std::unique_ptr<OriginalEnding>>::success(std::move(result));
}

auto OriginalEnding::page() const -> size_t { return page_; }

auto OriginalEnding::pages() const -> size_t { return sequence_.size(); }

auto OriginalEnding::variant() const -> unsigned { return variant_; }

auto OriginalEnding::done() const -> bool { return page_ >= pages(); }

auto OriginalEnding::automatic() const -> bool { return !done() && (movie_.has_value() || scrolling()); }

auto OriginalEnding::movie_frames() const -> unsigned { return movie_frames_; }

auto OriginalEnding::set_effect_audio(const std::filesystem::path& root, OriginalAniAudio::Play play, OriginalAniAudio::Stop stop) -> void
{
    if (movie_audio_) { return; }
    auto loaded = OriginalAniAudio::create(root);
    if (!loaded)
    {
        report_error(loaded.error());
        return;
    }
    movie_audio_ = std::move(loaded).value();
    movie_audio_->play_music = music_audio_;
    movie_audio_->play_midi = midi_audio_;
    play_audio_ = std::move(play);
    stop_audio_ = std::move(stop);
    if (movie_)
    {
        auto begun = movie_audio_->begin(sequence_[page_].movie, 0);
        if (!begun) { report_error(begun.error()); }
    }
}

auto OriginalEnding::set_music_audio(OriginalAniAudio::Music play, OriginalAniAudio::Music midi) -> void
{
    bool initial = !music_audio_;
    music_audio_ = std::move(play);
    midi_audio_ = std::move(midi);
    if (movie_audio_)
    {
        movie_audio_->play_music = music_audio_;
        movie_audio_->play_midi = midi_audio_;
    }
    if (initial) { enter_music(); }
}

auto OriginalEnding::phase_description() const -> std::string
{
    if (done())
    {
        return "returned";
    }
    const auto& p = sequence_[page_];
    if (p.movie)
    {
        return "movie" + std::to_string(p.movie) + " frame=" + std::to_string(movie_frames_) + " frames=" + std::to_string(movie_ ? movie_->frames : 0) + " delay=" + std::to_string(movie_ ? movie_->delay : 0);
    }
    if (p.illustration >= 0)
    {
        return "41F011 scroll remaining=" + std::to_string(scroll_) + " rows=" + std::to_string(illustrations_[p.illustration].size() / (640 * 4)) + " step=3/10ms";
    }
    if (p.summary)
    {
        return "41EDD4 campaign statistics";
    }
    if (p.role >= 0)
    {
        return "41F167 role=" + std::to_string(roles_[p.role]) + (p.relationship ? " relationships" : " class/stat/text");
    }
    return "BIN3 text=" + std::to_string(p.text);
}

auto OriginalEnding::tick(unsigned milliseconds) -> void
{
    if (!automatic())
    {
        return;
    }
    elapsed_ += milliseconds;
    if (movie_)
    {
        if (elapsed_ >= movie_->delay)
        {
            elapsed_ = 0;
            if (!movie_->next(movie_image_))
            {
                ++page_;
                enter();
                return;
            }
            ++movie_frames_;
            if (movie_audio_ && play_audio_ && stop_audio_)
            {
                auto played = movie_audio_->advance(movie_frames_, play_audio_, stop_audio_);
                if (!played) { report_error(played.error()); }
            }
        }
    }
    else
    {
        while (scroll_ && elapsed_ >= 10)
        {
            elapsed_ -= 10;
            scroll_ = scroll_ > 3 ? scroll_ - 3 : 0;
        }
        if (!scroll_ && elapsed_ >= 500)
        {
            ++page_;
            enter();
        }
    }
}

auto OriginalEnding::advance() -> void
{
    if (!done() && !automatic())
    {
        ++page_;
        enter();
    }
}

auto OriginalEnding::try_draw() const -> s2ne::Result<std::vector<uint8_t>>
{
    auto image = background_;
    if (done())
    {
        return s2ne::Result<std::vector<uint8_t>>::success(image);
    }
    const auto& phase = sequence_[page_];
    if (phase.movie)
    {
        return s2ne::Result<std::vector<uint8_t>>::success(movie_image_.empty() ? std::vector<uint8_t>(640 * 480 * 4) : movie_image_);
    }
    if (phase.illustration >= 0)
    {
        const auto& source = illustrations_[phase.illustration];
        std::copy_n(source.begin() + size_t(scroll_) * 640 * 4, image.size(), image.begin());
        return s2ne::Result<std::vector<uint8_t>>::success(image);
    }
    unsigned count = word(text_, 0);
    size_t records = 4, variants = records + count * 6;
    if (phase.role < 0 && !phase.summary)
    {
        image = phase.text < 2 ? intro_ : background_;
        auto text = paragraph(image, string(dword(text_, records + phase.text * 6)), phase.text < 2 ? 25 : 60, phase.text == 1 ? 120 : 50, 590);
        if (!text) { return s2ne::Result<std::vector<uint8_t>>::failure(text.error()); }
        return s2ne::Result<std::vector<uint8_t>>::success(image);
    }
    if (phase.summary)
    {
        auto text = paragraph(image, font_.text_bytes(258), 70, 50, 300);
        if (!text) { return s2ne::Result<std::vector<uint8_t>>::failure(text.error()); }
        if (!events_.ending_statistics)
        {
            return s2ne::Result<std::vector<uint8_t>>::failure(make_error("Native ending campaign statistics unavailable"));
        }
        unsigned total = 0;
        for (auto id : roles_)
        {
            auto it = std::find_if(events_.units.begin(), events_.units.end(), [&](const auto& u)
                {
                    return !u.hostile() && u.id == id;
                });
            total += it->ending_kills;
        }
        const std::array<int, 6> y{ 50, 78, 106, 130, 158, 186 };
        for (size_t i = 0; i < 6; ++i)
        {
            auto number = std::to_string(i < 5 ? (*events_.ending_statistics)[i] : int(total));
            auto drawn = font_.try_draw_original_large(image, { number.begin(), number.end() }, 400, y[i], 175);
            if (!drawn) { return s2ne::Result<std::vector<uint8_t>>::failure(drawn.error()); }
        }
        return s2ne::Result<std::vector<uint8_t>>::success(image);
    }
    auto id = roles_.at(phase.role);
    const auto& unit = *std::find_if(events_.units.begin(), events_.units.end(), [&](auto& u)
        {
            return u.id == id && !u.hostile();
        });
    if (!phase.relationship)
    {
        auto class_result = try_class_image(image, unit.appearance);
        if (!class_result)
        {
            return s2ne::Result<std::vector<uint8_t>>::failure(class_result.error());
        }
        auto name = font_.try_draw_original_large(image, unit.name, 367, 42, 175);
        if (!name) { return s2ne::Result<std::vector<uint8_t>>::failure(name.error()); }
        auto title = unit.profession;
        auto level = " " + std::to_string(unit.level) + " ";
        title.insert(title.end(), level.begin(), level.end());
        auto suffix = font_.text_bytes(113);
        title.insert(title.end(), suffix.begin(), suffix.end());
        auto heading = font_.try_draw_original_large(image, title, 367, 72, 175);
        if (!heading) { return s2ne::Result<std::vector<uint8_t>>::failure(heading.error()); }
        auto number = [&](unsigned label, int value, int y)
        {
            auto text = font_.text_bytes(label);
            auto n = std::to_string(value);
            text.insert(text.end(), n.begin(), n.end());
            return font_.try_draw_original_large(image, text, 367, y, 175);
        };
        if (!unit.ending_combat_value)
        {
            return s2ne::Result<std::vector<uint8_t>>::failure(make_error("Native ending cached combat statistic unavailable"));
        }
        auto combat = number(28, *unit.ending_combat_value, 102);
        if (!combat) { return s2ne::Result<std::vector<uint8_t>>::failure(combat.error()); }
        auto kills = number(256, int(unit.ending_kills), 132);
        if (!kills) { return s2ne::Result<std::vector<uint8_t>>::failure(kills.error()); }
        auto losses = number(257, int(unit.ending_losses), 162);
        if (!losses) { return s2ne::Result<std::vector<uint8_t>>::failure(losses.error()); }
        size_t list = dword(text_, variants + variant_ * 8);
        unsigned n = word(text_, variants + variant_ * 8 + 4);
        for (unsigned i = 0; i < n; ++i)
        {
            size_t p = list + i * 8;
            if (word(text_, p) == id)
            {
                auto text = role_text(image, string(dword(text_, p + 2)));
                if (!text) { return s2ne::Result<std::vector<uint8_t>>::failure(text.error()); }
                break;
            }
        }
        if (unit.portrait)
        {
            size_t g = dword(pal_, 824) - 58, p = g + dword(cel_, g + 2 + (unit.portrait * 2 - 2) * 8);
            unsigned type = cel_.at(p++), w = word(cel_, p) + 1, h = word(cel_, p + 2) + 1;
            p += 4;
            for (unsigned y = 0; y < h; ++y)
            {
                for (unsigned x = 0; x < w;)
                {
                    unsigned op = type ? 0x81 : cel_.at(p++), n = op ? op & 127 : 1, c = 0;
                    if (!n || n > w - x)
                    {
                        return s2ne::Result<std::vector<uint8_t>>::failure(make_error("Invalid ending portrait"));
                    }
                    if (op && !(op & 128))
                    {
                        c = cel_.at(p++);
                    }
                    while (n--)
                    {
                        unsigned ink = op & 128 ? cel_.at(p++) : c;
                        int dx = 53 + int(x++), dy = 76 + int(y);
                        if (ink && dx >= 0 && dy >= 0 && dx < 640 && dy < 480)
                        {
                            size_t q = (size_t(dy) * 640 + dx) * 4;
                            for (unsigned k = 0; k < 3; ++k)
                            {
                                image[q + k] = uint8_t(unsigned(pal_.at(ink * 3 + k)) * 255 / 63);
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        auto title = unit.name;
        title.push_back(' ');
        auto label = font_.text_bytes(261);
        title.insert(title.end(), label.begin(), label.end());
        auto heading = font_.try_draw_original_large(image, title, 34, 220, 175);
        if (!heading) { return s2ne::Result<std::vector<uint8_t>>::failure(heading.error()); }
        unsigned row = 0;
        for (const auto& other : events_.units)
        {
            if (other.hostile() || other.id == id || other.id < 1 || other.id > 40)
            {
                continue;
            }
            int x = row % 2 ? 320 : 34, y = 250 + int(row / 2) * 26;
            auto name = other.name;
            std::string value = ":" + std::to_string(unit.relationships[other.id - 1]);
            name.insert(name.end(), value.begin(), value.end());
            auto drawn = font_.try_draw_original_large(image, name, x, y, 175);
            if (!drawn) { return s2ne::Result<std::vector<uint8_t>>::failure(drawn.error()); }
            if (++row >= 16)
            {
                break;
            }
        }
    }
    return s2ne::Result<std::vector<uint8_t>>::success(image);
}

auto OriginalEnding::draw() const -> std::vector<uint8_t>
{
    auto result = try_draw();
    return std::move(result).value();
}
}    //namespace s2ne
