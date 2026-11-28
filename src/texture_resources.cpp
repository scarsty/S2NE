#include "texture_resources.h"

namespace s2ne
{
TextureResources::TargetRestore::TargetRestore(SDL_Renderer* renderer) : renderer_(renderer), target_(SDL_GetRenderTarget(renderer)) {}

auto TextureResources::TargetRestore::release() -> void { active_ = false; }

TextureResources::TargetRestore::~TargetRestore()
{
    if (active_ && !SDL_SetRenderTarget(renderer_, target_)) { std::println(stderr, "Texture render target restore failed: {}", SDL_GetError()); }
}

auto TextureResources::copy_surface_pixels(const SDL_Surface& surface, std::span<uint8_t> pixels) -> void
{
    for (int y = 0; y < surface.h; ++y) { std::memcpy(pixels.data() + size_t(y) * surface.w * 4, static_cast<const uint8_t*>(surface.pixels) + y * surface.pitch, size_t(surface.w) * 4); }
}

auto TextureResources::decode_image(const std::filesystem::path& path) -> Result<Image>
{
    auto loaded = load_png(path);
    if (!loaded) { return Result<Image>::failure(loaded.error()); }
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> source(loaded.value(), SDL_DestroySurface);
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> rgba(SDL_ConvertSurface(source.get(), SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface);
    if (!rgba) { return Result<Image>::failure(make_error(path.string() + ": " + SDL_GetError())); }
    std::vector<uint8_t> pixels(size_t(rgba->w) * rgba->h * 4);
    copy_surface_pixels(*rgba, pixels);
    return Result<Image>::success(Image{ SharedBytes(std::move(pixels)), rgba->w, rgba->h });
}

auto TextureResources::read_resource(const std::string& relative) const -> Result<std::vector<char>>
{
    const auto file = root_ / std::filesystem::path(relative);
    std::ifstream input(file, std::ios::binary);
    if (!input) { return Result<std::vector<char>>::failure(make_error("Cannot read " + file.string())); }
    return Result<std::vector<char>>::success({ std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() });
}

auto TextureResources::load_png(const std::filesystem::path& path) -> Result<SDL_Surface*>
{
    if (path.parent_path().filename() == "portraits" && !std::filesystem::exists(path))
    {
        const auto stem = path.stem().string();
        unsigned id{};
        const auto parsed = std::from_chars(stem.data(), stem.data() + stem.size(), id);
        if (parsed.ec == std::errc{} && parsed.ptr == stem.data() + stem.size() && (id & 1))
        {
            const auto large = path.parent_path() / (std::to_string(id - 1) + ".png");
            std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> source(SDL_LoadPNG(large.string().c_str()), SDL_DestroySurface);
            if (!source) { return Result<SDL_Surface*>::failure(make_error(large.string() + ": " + SDL_GetError())); }
            auto* scaled = SDL_ScaleSurface(source.get(), 92, 111, SDL_SCALEMODE_LINEAR);
            if (!scaled) { return Result<SDL_Surface*>::failure(make_error(path.string() + ": " + SDL_GetError())); }
            return Result<SDL_Surface*>::success(scaled);
        }
    }
    auto* surface = SDL_LoadPNG(path.string().c_str());
    if (!surface) { return Result<SDL_Surface*>::failure(make_error(path.string() + ": " + SDL_GetError())); }
    return Result<SDL_Surface*>::success(surface);
}

auto TextureResources::load_image(const std::string& group, unsigned id) -> Result<Image>
{
    if (!active()) { return Result<Image>::failure(make_error("Texture resources are not active")); }
    const auto relative = group + "/" + std::to_string(id) + ".png";
    if (auto cached = images_.find(relative); cached != images_.end()) { return Result<Image>::success(cached->second); }
    auto loaded = decode_image(root_ / std::filesystem::path(relative));
    if (!loaded) { return Result<Image>::failure(loaded.error()); }
    auto result = std::move(loaded).value();
    images_.emplace(relative, result);
    return Result<Image>::success(std::move(result));
}

auto TextureResources::load_image_scaled(const std::string& group, unsigned id, int width, int height) -> Result<Image>
{
    auto loaded = load_image(group, id);
    return scale_image(std::move(loaded), width, height);
}

auto TextureResources::load_file_scaled(const std::filesystem::path& path, int width, int height) -> Result<Image>
{
    return scale_image(decode_image(path), width, height);
}

auto TextureResources::scale_image(Result<Image> loaded, int width, int height) -> Result<Image>
{
    if (!loaded) { return loaded; }
    if (loaded.value().width == width && loaded.value().height == height) { return loaded; }
    const auto& source = loaded.value();
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(SDL_CreateSurface(source.width, source.height, SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface);
    if (!surface) { return Result<Image>::failure(make_error(SDL_GetError())); }
    for (int y = 0; y < source.height; ++y) { std::memcpy(static_cast<uint8_t*>(surface->pixels) + y * surface->pitch, source.pixels.data() + size_t(y) * source.width * 4, size_t(source.width) * 4); }
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> scaled(SDL_ScaleSurface(surface.get(), width, height, SDL_SCALEMODE_LINEAR), SDL_DestroySurface);
    if (!scaled) { return Result<Image>::failure(make_error(SDL_GetError())); }
    std::vector<uint8_t> pixels(size_t(width) * height * 4);
    copy_surface_pixels(*scaled, pixels);
    return Result<Image>::success(Image{ SharedBytes(std::move(pixels)), width, height });
}

auto TextureResources::draw_small_text(std::vector<uint8_t>& image, std::string_view text, int left, int top, SDL_Color color) -> Result<bool>
{
    if (!small_font_) { return Result<bool>::failure(make_error("SDL_ttf small font is not initialized")); }
    return draw_ttf_text(image, text, left, top, color, small_font_.get(), 16.0f);
}

auto TextureResources::resolve_frame(const std::string& group, unsigned frame) -> Result<std::string>
{
    std::string filename = std::to_string(frame);
    if (group == "ui" || group == "ground" || group == "objects" || group == "scenery")
    {
        if (!cycles_.contains(group))
        {
            auto frame_data = read_resource(group + "/frames.txt");
            if (!frame_data) { return Result<std::string>::failure(frame_data.error()); }
            std::istringstream input(std::string(frame_data.value().begin(), frame_data.value().end()));
            std::unordered_map<unsigned, std::vector<unsigned>> cycles;
            std::string line;
            while (std::getline(input, line))
            {
                std::istringstream row(line);
                unsigned id{}, delay{};
                char colon{};
                row >> std::ws;
                if (row.peek() < '0' || row.peek() > '9') { return Result<std::string>::failure(make_error("Invalid animation identifier " + group + "/frames.txt")); }
                if (!(row >> id >> colon) || colon != ':') { return Result<std::string>::failure(make_error("Invalid animation table " + group + "/frames.txt")); }
                if (cycles.contains(id)) { return Result<std::string>::failure(make_error("Duplicate animation " + group + "/frames.txt")); }
                for (;;)
                {
                    row >> std::ws;
                    if (row.peek() < '0' || row.peek() > '9') { return Result<std::string>::failure(make_error("Invalid animation delay " + group + "/frames.txt")); }
                    if (!(row >> delay) || !delay) { return Result<std::string>::failure(make_error("Invalid animation delay " + group + "/frames.txt")); }
                    cycles[id].push_back(delay);
                    row >> std::ws;
                    if (row.eof()) { break; }
                    char comma{};
                    if (!(row >> comma) || comma != ',') { return Result<std::string>::failure(make_error("Invalid animation separator " + group + "/frames.txt")); }
                }
                if (cycles[id].empty()) { return Result<std::string>::failure(make_error("Empty animation " + group + "/frames.txt")); }
            }
            if (input.bad()) { return Result<std::string>::failure(make_error("Cannot finish reading " + group + "/frames.txt")); }
            cycles_.emplace(group, std::move(cycles));
        }
        if (auto cycle = cycles_.at(group).find(frame); cycle != cycles_.at(group).end())
        {
            const auto& delays = cycle->second;
            uint64_t total = 0;
            for (const auto delay : delays) { total += delay; }
            uint64_t position = elapsed_ % total;
            unsigned selected = 0;
            for (; selected + 1 < delays.size() && position >= delays[selected]; ++selected) { position -= delays[selected]; }
            filename += "_" + std::to_string(selected);
        }
    }
    return Result<std::string>::success(group + "/" + filename);
}

auto TextureResources::load(const std::string& group, unsigned frame) -> Result<SDL_Texture*>
{
    if (!renderer_) { return Result<SDL_Texture*>::failure(make_error("Texture renderer not bound")); }
    auto resolved = resolve_frame(group, frame);
    if (!resolved) { return Result<SDL_Texture*>::failure(resolved.error()); }
    const auto key = std::move(resolved).value();
    if (auto found = textures_.find(key); found != textures_.end()) { return Result<SDL_Texture*>::success(found->second.get()); }
    auto loaded = load_png(root_ / (key + ".png"));
    if (!loaded) { return Result<SDL_Texture*>::failure(loaded.error()); }
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(loaded.value(), SDL_DestroySurface);
    std::shared_ptr<SDL_Texture> texture(SDL_CreateTextureFromSurface(renderer_, surface.get()), SDL_DestroyTexture);
    if (!texture || !SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND) || !SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST)) { return Result<SDL_Texture*>::failure(make_error(SDL_GetError())); }
    auto* result = texture.get();
    textures_.emplace(key, std::move(texture));
    if (!logged_.contains(group))
    {
        std::println(stderr, "Load texture group: {} (PNG, renderer-owned cache)", group);
        logged_[group] = true;
    }
    return Result<SDL_Texture*>::success(result);
}

auto TextureResources::readback(std::vector<uint8_t>& image) -> Result<bool>
{
    const int width = framebuffer_width(image);
    if (!width) { return Result<bool>::failure(make_error("Invalid texture readback framebuffer")); }
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> pixels(SDL_RenderReadPixels(renderer_, nullptr), SDL_DestroySurface);
    if (!pixels) { return Result<bool>::failure(make_error(SDL_GetError())); }
    if (pixels->format != SDL_PIXELFORMAT_RGBA32)
    {
        pixels.reset(SDL_ConvertSurface(pixels.get(), SDL_PIXELFORMAT_RGBA32));
        if (!pixels) { return Result<bool>::failure(make_error(SDL_GetError())); }
    }
    if (pixels->w != width || pixels->h != framebuffer_height(image)) { return Result<bool>::failure(make_error("Invalid texture readback extent")); }
    copy_surface_pixels(*pixels, image);
    return Result<bool>::success(true);
}

auto TextureResources::prepare_canvas(std::span<const uint8_t> image) -> Result<bool>
{
    const int width = framebuffer_width(image);
    const int height = framebuffer_height(image);
    if (!renderer_ || !width) { return Result<bool>::failure(make_error("Invalid texture canvas framebuffer")); }
    float old_width{}, old_height{};
    if (target_ && (!SDL_GetTextureSize(target_.get(), &old_width, &old_height) || int(old_width) != width || int(old_height) != height))
    {
        if (batching_) { return Result<bool>::failure(make_error("Texture batch framebuffer changed")); }
        target_.reset();
        background_.reset();
    }
    if (batching_) { return Result<bool>::success(true); }
    if (!target_) { target_.reset(SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, width, height)); }
    if (!background_) { background_.reset(SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, width, height)); }
    if (!target_ || !background_) { return Result<bool>::failure(make_error(SDL_GetError())); }
    if (!SDL_SetTextureBlendMode(background_.get(), SDL_BLENDMODE_NONE) || !SDL_SetTextureBlendMode(target_.get(), SDL_BLENDMODE_NONE)
        || !SDL_UpdateTexture(background_.get(), nullptr, image.data(), width * 4) || !SDL_SetRenderTarget(renderer_, target_.get())
        || !SDL_RenderTexture(renderer_, background_.get(), nullptr, nullptr)) { return Result<bool>::failure(make_error(SDL_GetError())); }
    return Result<bool>::success(true);
}

auto TextureResources::initialize_small_font(const std::filesystem::path& font_file) -> Result<bool>
{
    auto loaded = load_font(font_file);
    if (!loaded) { return Result<bool>::failure(loaded.error()); }
    small_font_ = std::move(loaded).value();
    return Result<bool>::success(true);
}

auto TextureResources::load_font(const std::filesystem::path& path) -> Result<std::shared_ptr<TTF_Font>>
{
    if (!renderer_) { return Result<std::shared_ptr<TTF_Font>>::failure(make_error("Texture renderer not bound")); }
    std::error_code error;
    auto canonical = std::filesystem::weakly_canonical(path, error);
    if (error) { return Result<std::shared_ptr<TTF_Font>>::failure(make_error(error.message())); }
#ifdef _WIN32
    auto native = canonical.native();
    for (auto& character : native) { character = wchar_t(std::towlower(character)); }
    canonical = std::filesystem::path(native);
#endif
    const auto key = canonical.string();
    if (auto found = fonts_.find(key); found != fonts_.end()) { return Result<std::shared_ptr<TTF_Font>>::success(found->second); }
    if (!ttf_owned_)
    {
        if (!TTF_Init()) { return Result<std::shared_ptr<TTF_Font>>::failure(make_error(SDL_GetError())); }
        ttf_owned_ = true;
    }
    std::shared_ptr<TTF_Font> font(TTF_OpenFont(path.string().c_str(), 24.0f), TTF_CloseFont);
    if (!font) { return Result<std::shared_ptr<TTF_Font>>::failure(make_error(path.string() + ": " + SDL_GetError())); }
    fonts_.emplace(key, font);
    return Result<std::shared_ptr<TTF_Font>>::success(std::move(font));
}

auto TextureResources::set_elapsed(uint64_t elapsed) -> void { elapsed_ = elapsed; }

auto TextureResources::elapsed() const -> uint64_t { return elapsed_; }

auto TextureResources::begin_canvas(std::span<const uint8_t> image) -> Result<bool>
{
    if (!renderer_ || batching_ || !framebuffer_width(image)) { return Result<bool>::failure(make_error("Invalid texture canvas begin")); }
    batch_previous_ = SDL_GetRenderTarget(renderer_);
    TargetRestore restore(renderer_);
    auto prepared = prepare_canvas(image);
    if (!prepared) { return prepared; }
    batching_ = true;
    restore.release();
    return Result<bool>::success(true);
}

auto TextureResources::end_canvas(std::vector<uint8_t>& image) -> Result<bool>
{
    if (!batching_) { return Result<bool>::failure(make_error("Texture canvas not active")); }
    auto result = readback(image);
    batching_ = false;
    if (!SDL_SetRenderTarget(renderer_, batch_previous_)) { return Result<bool>::failure(make_error(SDL_GetError())); }
    return result;
}

auto TextureResources::draw_cached(std::vector<uint8_t>& image, SDL_Texture* source, int left, int top) -> Result<bool>
{
    if (!renderer_ || !source || !framebuffer_width(image)) { return Result<bool>::failure(make_error("Invalid cached texture draw")); }
    TargetRestore restore(renderer_);
    auto prepared = prepare_canvas(image);
    if (!prepared) { return prepared; }
    float width{}, height{};
    if (!SDL_GetTextureSize(source, &width, &height)) { return Result<bool>::failure(make_error(SDL_GetError())); }
    SDL_FRect rectangle{ float(left), float(top), width, height };
    if (!SDL_RenderTexture(renderer_, source, nullptr, &rectangle)) { return Result<bool>::failure(make_error(SDL_GetError())); }
    if (batching_) { return Result<bool>::success(true); }
    return readback(image);
}

auto TextureResources::draw_glyph(std::vector<uint8_t>& image, std::span<const uint8_t> bits, unsigned width, unsigned height, int left, int top, std::span<const uint8_t> palette, unsigned color, std::vector<uint8_t>* indexed) -> Result<bool>
{
    const int stride = framebuffer_width(image);
    if (!stride || (indexed && indexed->size() != image.size() / 4)) { return Result<bool>::failure(make_error("Invalid glyph framebuffer")); }
    if (!renderer_ || !width || width % 8 || bits.size() != size_t(width / 8) * height || palette.size() < 768 || color > 255) { return Result<bool>::failure(make_error("Invalid original glyph texture")); }
    std::string key = "glyph/" + std::to_string(width) + "/" + std::to_string(height) + "/";
    key.append(reinterpret_cast<const char*>(bits.data()), bits.size());
    if (!textures_.contains(key))
    {
        std::vector<uint8_t> rgba(size_t(width) * height * 4, 255);
        for (unsigned y = 0; y < height; ++y)
        {
            for (unsigned x = 0; x < width; ++x) { rgba[(size_t(y) * width + x) * 4 + 3] = (bits[size_t(y) * (width / 8) + x / 8] & (128 >> (x % 8))) ? 255 : 0; }
        }
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(SDL_CreateSurfaceFrom(int(width), int(height), SDL_PIXELFORMAT_RGBA32, rgba.data(), int(width) * 4), SDL_DestroySurface);
        if (!surface) { return Result<bool>::failure(make_error(SDL_GetError())); }
        std::shared_ptr<SDL_Texture> texture(SDL_CreateTextureFromSurface(renderer_, surface.get()), SDL_DestroyTexture);
        if (!texture || !SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND)) { return Result<bool>::failure(make_error(SDL_GetError())); }
        textures_.emplace(key, std::move(texture));
        if (!logged_.contains("fonts"))
        {
            std::println(stderr, "Load texture group: fonts (original glyphs, renderer-owned cache)");
            logged_["fonts"] = true;
        }
    }
    auto* glyph = textures_.at(key).get();
    if (!SDL_SetTextureColorMod(glyph, uint8_t(unsigned(palette[color * 3]) * 255 / 63), uint8_t(unsigned(palette[color * 3 + 1]) * 255 / 63), uint8_t(unsigned(palette[color * 3 + 2]) * 255 / 63))) { return Result<bool>::failure(make_error(SDL_GetError())); }
    auto result = Result<bool>::success(true);
    if (batching_) { result = draw_cached(image, glyph, left, top); }
    else
    {
        for (unsigned y = 0; y < height; ++y)
        {
            for (unsigned x = 0; x < width; ++x)
            {
                const int dx = left + int(x), dy = top + int(y);
                if (dx < 0 || dx >= stride || dy < 0 || dy >= framebuffer_height(image) || !(bits[size_t(y) * (width / 8) + x / 8] & (128 >> (x % 8)))) { continue; }
                const size_t p = (size_t(dy) * stride + dx) * 4;
                for (unsigned k = 0; k < 3; ++k) { image[p + k] = uint8_t(unsigned(palette[color * 3 + k]) * 255 / 63); }
                image[p + 3] = 255;
            }
        }
    }
    if (!result || !indexed) { return result; }
    for (unsigned y = 0; y < height; ++y)
    {
        for (unsigned x = 0; x < width; ++x)
        {
            const int dx = left + int(x), dy = top + int(y);
            if (dx >= 0 && dx < stride && dy >= 0 && dy < framebuffer_height(image) && (bits[size_t(y) * (width / 8) + x / 8] & (128 >> (x % 8)))) { indexed->at(size_t(dy) * stride + dx) = uint8_t(color); }
        }
    }
    return result;
}

auto TextureResources::tint(std::vector<uint8_t>& image, int left, int top, int width, int height, uint8_t red, uint8_t green, uint8_t blue, unsigned amount) -> Result<bool>
{
    if (amount > 100) { return Result<bool>::failure(make_error("Invalid texture tint amount")); }
    auto begun = begin_canvas(image);
    if (!begun) { return begun; }
    SDL_FRect rectangle{ float(left), float(top), float(width), float(height) };
    const bool drawn = SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND) && SDL_SetRenderDrawColor(renderer_, red, green, blue, uint8_t(amount * 255 / 100)) && SDL_RenderFillRect(renderer_, &rectangle);
    const std::string error = drawn ? "" : SDL_GetError();
    auto ended = end_canvas(image);
    if (!drawn) { return Result<bool>::failure(make_error(error)); }
    return ended;
}

auto TextureResources::clear() -> void
{
    textures_.clear();
    text_textures_.clear();
    text_surfaces_.clear();
    images_.clear();
    groups_.clear();
    available_ui_.clear();
    target_.reset();
    background_.reset();
    presentation_.reset();
    large_font_.reset();
    small_font_.reset();
    fonts_.clear();
    if (ttf_owned_) { TTF_Quit(); }
    ttf_owned_ = false;
    renderer_ = nullptr;
    logged_.clear();
    sequences_.clear();
    cycles_.clear();
    palette_indices_.clear();
    batching_ = false;
    batch_previous_ = nullptr;
    map_available_ = false;
    root_.clear();
}

auto TextureResources::configure(const std::filesystem::path& working) -> void
{
    clear();
    const auto path = working / "texture";
    if (!std::filesystem::exists(path)) { return; }
    if (!std::filesystem::is_directory(path)) { throw std::runtime_error("texture is not a directory"); }
    root_ = path;
    map_available_ = true;
    for (const auto* group : { "ground", "objects", "scenery" })
    {
        auto available = group_available(group);
        if (!available) { throw std::runtime_error(available.error().message); }
        map_available_ = map_available_ && available.value();
    }
    if (!map_available_) { std::println(stderr, "External map groups incomplete; retaining original map renderer"); }
    std::println(stderr, "Texture resource directory: {}", root_.string());
}

auto TextureResources::active() const -> bool { return !root_.empty(); }

auto TextureResources::group_available(const std::string& group) -> Result<bool>
{
    if (!active()) { return Result<bool>::success(false); }
    if (auto found = groups_.find(group); found != groups_.end()) { return Result<bool>::success(found->second); }
    std::error_code error;
    const auto path = root_ / group;
    const bool present = std::filesystem::exists(path, error);
    if (error) { return Result<bool>::failure(make_error(path.string() + ": " + error.message())); }
    if (present && !std::filesystem::is_directory(path, error)) { return Result<bool>::failure(make_error("Invalid texture group directory: " + path.string())); }
    if (error) { return Result<bool>::failure(make_error(path.string() + ": " + error.message())); }
    groups_.emplace(group, present);
    return Result<bool>::success(present);
}

auto TextureResources::map_available() const -> bool { return active() && map_available_; }

auto TextureResources::bind(SDL_Renderer* renderer) -> void { renderer_ = renderer; }

auto TextureResources::initialize_font(const std::filesystem::path& font_file) -> Result<bool>
{
    auto loaded = load_font(font_file);
    if (!loaded) { return Result<bool>::failure(loaded.error()); }
    large_font_ = std::move(loaded).value();
    return Result<bool>::success(true);
}

auto TextureResources::font_ready() const -> bool { return large_font_ != nullptr; }

auto TextureResources::small_font_ready() const -> bool { return small_font_ != nullptr; }

auto TextureResources::draw_large_text(std::vector<uint8_t>& image, std::string_view text, int left, int top, SDL_Color color) -> Result<bool>
{
    if (!large_font_) { return Result<bool>::failure(make_error("SDL_ttf font is not initialized")); }
    return draw_ttf_text(image, text, left, top, color, large_font_.get(), 24.0f);
}

auto TextureResources::draw_ttf_text(std::vector<uint8_t>& image, std::string_view text, int left, int top, SDL_Color color, TTF_Font* font, float size) -> Result<bool>
{
    const int stride = framebuffer_width(image);
    if (!font || !stride) { return Result<bool>::failure(make_error("Invalid SDL_ttf draw target")); }
    std::string display(text);
    for (auto position = display.find("\xE2\x80\xA7"); position != std::string::npos; position = display.find("\xE2\x80\xA7", position + 2))
    {
        display.replace(position, 3, "\xC2\xB7");
    }
    text = display;
    const auto key = std::to_string(reinterpret_cast<uintptr_t>(font)) + "/" + std::to_string(size) + "/" + std::string(text);
    auto cached = text_textures_.find(key);
    if (cached == text_textures_.end())
    {
        if (!TTF_SetFontSize(font, size)) { return Result<bool>::failure(make_error(SDL_GetError())); }
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(TTF_RenderText_Blended(font, std::string(text).c_str(), text.size(), SDL_Color{ 255, 255, 255, 255 }), SDL_DestroySurface);
        if (!surface) { return Result<bool>::failure(make_error(SDL_GetError())); }
        std::shared_ptr<SDL_Texture> texture(SDL_CreateTextureFromSurface(renderer_, surface.get()), SDL_DestroyTexture);
        if (!texture || !SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND)) { return Result<bool>::failure(make_error(SDL_GetError())); }
        text_surfaces_.emplace(key, std::shared_ptr<SDL_Surface>(surface.release(), SDL_DestroySurface));
        cached = text_textures_.emplace(key, std::move(texture)).first;
    }
    if (!batching_)
    {
        auto* surface = text_surfaces_.at(key).get();
        if (!SDL_SetSurfaceColorMod(surface, color.r, color.g, color.b) || !SDL_SetSurfaceAlphaMod(surface, color.a) || !SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND)) { return Result<bool>::failure(make_error(SDL_GetError())); }
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> destination(SDL_CreateSurfaceFrom(stride, framebuffer_height(image), SDL_PIXELFORMAT_RGBA32, image.data(), stride * 4), SDL_DestroySurface);
        if (!destination) { return Result<bool>::failure(make_error(SDL_GetError())); }
        SDL_Rect rectangle{ left, top, surface->w, surface->h };
        if (!SDL_BlitSurface(surface, nullptr, destination.get(), &rectangle)) { return Result<bool>::failure(make_error(SDL_GetError())); }
        return Result<bool>::success(true);
    }
    auto* texture = cached->second.get();
    if (!SDL_SetTextureColorMod(texture, color.r, color.g, color.b) || !SDL_SetTextureAlphaMod(texture, color.a)) { return Result<bool>::failure(make_error(SDL_GetError())); }
    return draw_cached(image, texture, left, top);
}

auto TextureResources::draw_group(std::vector<uint8_t>& image, const std::string& group, unsigned id, int left, int top,
    std::span<const uint8_t> palette, std::vector<uint8_t>* indexed) -> Result<bool>
{
    if (!renderer_) { return Result<bool>::failure(make_error("Texture renderer not bound")); }
    const int stride = framebuffer_width(image);
    if (!stride || (indexed && (palette.size() < 768 || indexed->size() != image.size() / 4))) { return Result<bool>::failure(make_error("Invalid PNG palette/framebuffer")); }
    auto loaded = load(group, id);
    if (!loaded) { return Result<bool>::failure(loaded.error()); }
    auto* texture = loaded.value();
    TargetRestore restore(renderer_);
    auto prepared = prepare_canvas(image);
    if (!prepared) { return prepared; }
    float width{}, height{};
    if (!SDL_GetTextureSize(texture, &width, &height)) { return Result<bool>::failure(make_error(SDL_GetError())); }
    SDL_FRect rectangle{ float(left), float(top), width, height };
    if (!SDL_RenderTexture(renderer_, texture, nullptr, &rectangle)) { return Result<bool>::failure(make_error(SDL_GetError())); }
    if (batching_) { return Result<bool>::success(true); }
    auto read = readback(image);
    if (!read) { return read; }
    if (!indexed) { return Result<bool>::success(true); }
    if (palette_indices_.empty() || !std::equal(indexed_palette_.begin(), indexed_palette_.end(), palette.begin()))
    {
        std::copy_n(palette.begin(), 768, indexed_palette_.begin());
        palette_indices_.clear();
        for (unsigned c = 0; c < 256; ++c)
        {
            uint32_t rgb = 0;
            for (unsigned k = 0; k < 3; ++k) { rgb |= (unsigned(palette[c * 3 + k]) * 255 / 63) << (k * 8); }
            palette_indices_.emplace(rgb, uint8_t(c));
        }
    }
    for (int y = std::max(0, top); y < std::min(framebuffer_height(image), top + int(height)); ++y)
    {
        for (int x = std::max(0, left); x < std::min(stride, left + int(width)); ++x)
        {
            const size_t position = size_t(y) * stride + x;
            const auto* pixel = image.data() + position * 4;
            if (indexed)
            {
                uint32_t rgb = pixel[0] | uint32_t(pixel[1]) << 8 | uint32_t(pixel[2]) << 16;
                auto found = palette_indices_.find(rgb);
                if (found == palette_indices_.end()) { return Result<bool>::failure(make_error("PNG UI color cannot enter legacy indexed effect")); }
                indexed->at(position) = found->second;
            }
        }
    }
    return Result<bool>::success(true);
}

auto TextureResources::draw_unit(std::vector<uint8_t>& image, unsigned appearance, unsigned action, unsigned direction, uint64_t elapsed, int left, int top, unsigned width, unsigned height, std::span<const uint8_t> mask) -> Result<bool>
{
    if (action * 4 + direction >= 30 || mask.size() != size_t(width) * height) { return Result<bool>::failure(make_error("Invalid external unit sequence/mask")); }
    if (!sequences_.contains(appearance))
    {
        const auto relative = "units/" + std::to_string(appearance) + "/frames.txt";
        auto description = read_resource(relative);
        if (!description) { return Result<bool>::failure(description.error()); }
        std::istringstream input(std::string(description.value().begin(), description.value().end()));
        std::array<std::pair<unsigned, unsigned>, 30> sequences{};
        std::string line;
        while (std::getline(input, line))
        {
            std::istringstream row(line);
            unsigned a{}, d{}, first{}, count{};
            char comma{}, colon{}, second_comma{};
            if (!(row >> a >> comma >> d >> colon >> first >> second_comma >> count) || comma != ',' || colon != ':' || second_comma != ',' || a >= 8 || d >= 4 || a * 4 + d >= sequences.size() || sequences[a * 4 + d].second) { return Result<bool>::failure(make_error("Invalid unit frames: " + relative)); }
            row >> std::ws;
            if (!row.eof()) { return Result<bool>::failure(make_error("Unexpected unit frame data: " + relative)); }
            sequences[a * 4 + d] = { first, count };
        }
        if (input.bad()) { return Result<bool>::failure(make_error("Cannot finish reading " + relative)); }
        sequences_.emplace(appearance, sequences);
    }
    const auto [first, count] = sequences_.at(appearance)[action * 4 + direction];
    if (!count) { return Result<bool>::failure(make_error("Missing external unit sequence")); }
    return draw_masked(image, "units/" + std::to_string(appearance), first + unsigned((elapsed / 190) % count), left, top, width, height, mask);
}

auto TextureResources::draw_masked(std::vector<uint8_t>& image, const std::string& group, unsigned frame, int left, int top, unsigned width, unsigned height, std::span<const uint8_t> mask) -> Result<bool>
{
    if (!width || !height || !framebuffer_width(image) || mask.size() != size_t(width) * height) { return Result<bool>::failure(make_error("Invalid external texture mask")); }
    auto texture = load(group, frame);
    if (!texture) { return Result<bool>::failure(texture.error()); }
    TargetRestore restore(renderer_);
    auto prepared = prepare_canvas(image);
    if (!prepared) { return prepared; }
    float texture_width{}, texture_height{};
    if (!SDL_GetTextureSize(texture.value(), &texture_width, &texture_height)) { return Result<bool>::failure(make_error(SDL_GetError())); }
    for (unsigned y = 0; y < height; ++y)
    {
        for (unsigned x = 0; x < width;)
        {
            if (!mask[size_t(y) * width + x])
            {
                ++x;
                continue;
            }
            unsigned start = x++;
            while (x < width && mask[size_t(y) * width + x]) { ++x; }
            SDL_FRect source{ float(start) * texture_width / width, float(y) * texture_height / height, float(x - start) * texture_width / width, texture_height / height };
            SDL_FRect destination{ float(left) + start, float(top) + y, float(x - start), 1 };
            if (!SDL_RenderTexture(renderer_, texture.value(), &source, &destination)) { return Result<bool>::failure(make_error(SDL_GetError())); }
        }
    }
    if (batching_) { return Result<bool>::success(true); }
    return readback(image);
}

auto TextureResources::draw_ui(std::vector<uint8_t>& image, unsigned id, int left, int top,
    std::span<const uint8_t> palette, std::vector<uint8_t>* indexed) -> Result<bool>
{
    return draw_group(image, "ui", id, left, top, palette, indexed);
}

auto TextureResources::has_ui(unsigned id) -> Result<bool>
{
    auto present = group_available("ui");
    if (!present || !present.value()) { return present; }
    auto resolved = resolve_frame("ui", id);
    if (!resolved) { return Result<bool>::failure(resolved.error()); }
    if (auto found = available_ui_.find(resolved.value()); found != available_ui_.end()) { return Result<bool>::success(found->second); }
    std::error_code error;
    bool exists = std::filesystem::exists(root_ / (resolved.value() + ".png"), error);
    if (error) { return Result<bool>::failure(make_error(error.message())); }
    if (!exists && cycles_.at("ui").contains(id)) { return Result<bool>::failure(make_error("Missing declared animation frame: " + resolved.value())); }
    available_ui_.emplace(resolved.value(), exists);
    return Result<bool>::success(exists);
}

auto TextureResources::draw_map_sprite(std::vector<uint8_t>& image, const std::string& group, unsigned id, int left, int top) -> Result<bool>
{
    return draw_group(image, group, id, left, top, {}, nullptr);
}

auto TextureResources::upload_frame(std::span<const uint8_t> image) -> Result<SDL_Texture*>
{
    if (!renderer_ || !framebuffer_width(image)) { return Result<SDL_Texture*>::failure(make_error("Invalid external frame upload")); }
    const int width = framebuffer_width(image), height = framebuffer_height(image);
    float previous_width{}, previous_height{};
    if (presentation_ && (!SDL_GetTextureSize(presentation_.get(), &previous_width, &previous_height) || int(previous_width) != width || int(previous_height) != height)) { presentation_.reset(); }
    if (!presentation_)
    {
        presentation_.reset(SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, width, height));
        if (!presentation_ || !SDL_SetTextureScaleMode(presentation_.get(), SDL_SCALEMODE_LINEAR)) { return Result<SDL_Texture*>::failure(make_error(SDL_GetError())); }
    }
    if (!SDL_UpdateTexture(presentation_.get(), nullptr, image.data(), width * 4))
    {
        return Result<SDL_Texture*>::failure(make_error(SDL_GetError()));
    }
    return Result<SDL_Texture*>::success(presentation_.get());
}
}    //namespace s2ne
