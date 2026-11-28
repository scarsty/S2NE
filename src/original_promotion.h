#pragma once
#include "original_events.h"
#include "original_menu.h"
#include "original_random.h"
#include "original_settings.h"
#include "original_sprite.h"
#include <cmath>
#include <functional>
#include <optional>

namespace s2ne
{
// 4409B7 / 45954D / 459FCF. Commit is deliberately a host transaction:
// changing only Unit::appearance would omit 45B5C4's equipment/RNG/save effects.
class OriginalPromotion
{
public:
    inline static bool protagonist_class_tree = false;
    static constexpr unsigned eligibility_address = 0x4409B7, selection_address = 0x45954D,
                              confirmation_address = 0x459FCF, commit_address = 0x45B5C4;
    struct Candidate
    {
        unsigned kind{};
        std::vector<uint8_t> name;
        unsigned required_item{}, ability_mask{}, training_mask{}, tier{};
    };
    struct Request
    {
        unsigned unit_id{}, source_kind{}, target_kind{};
        int training_group = -1;
        bool battlefield = true;
    };
    // The callback must revalidate the live unit and atomically apply the FULL
    // original transaction to live events AND the backing original save record.
    using Commit = std::function<bool(const Request&)>;
    struct Preview
    {
        std::array<int, 10> current{}, promoted{}, reference{};
    };
    using PreviewSource = std::function<s2ne::Result<Preview>(unsigned)>;
    enum class Page
    {
        selection,
        confirmation,
        closed
    };
    struct Result
    {
        bool changed = false;
        unsigned consumed_mask = 0;
        bool commit_blocked = false;
    };

private:
    SharedBytes kinds_, bin_, pal_, cel_, prg_;
    OriginalMenu font_;
    size_t branches_{}, records_{}, growth_{};
    unsigned count_{};
    OriginalEvents::Unit unit_;
    std::vector<Candidate> choices_;
    Commit commit_;
    PreviewSource preview_source_;
    std::optional<Preview> preview_;
    Page page_ = Page::closed;
    size_t selected_{};
    std::optional<uint64_t> animation_tick_;
    unsigned animation_frame_{};
    int training_ = -1, confirmation_hover_ = 11, return_code_ = 0;
    bool battlefield_ = true;
    unsigned word(size_t p) const;
    unsigned dword(size_t p) const;
    s2ne::Result<size_t> try_record(unsigned kind) const;
    size_t record(unsigned kind) const;
    s2ne::Result<std::pair<int, int>> confirmation_origin() const;

public:
    OriginalPromotion() = default;
    static s2ne::Result<OriginalPromotion> create(const std::filesystem::path& root);
    explicit OriginalPromotion(const std::filesystem::path& root);
    using BoundedRandom = std::function<unsigned(unsigned)>;
    // Exact 427A9B arithmetic with caller-supplied original 4732ED outcomes.
    // This does not pretend the Windows CRT rand() is the original runtime RNG.
    static s2ne::Result<int> try_growth_formula(const std::array<int, 6>& words, int level, const BoundedRandom& random);
    static int growth_formula(const std::array<int, 6>& words, int level, const BoundedRandom& random);
    s2ne::Result<std::array<int, 9>> try_growth(unsigned kind, int level, const BoundedRandom& random, bool maximize = false) const;
    s2ne::Result<std::array<int, 9>> try_growth(unsigned kind, int level, OriginalRandom& random, const OriginalRandom::TimeSource& time = {}, bool maximize = false) const;
    std::array<int, 9> growth(unsigned kind, int level, const BoundedRandom& random) const;
    std::array<int, 9> growth(unsigned kind, int level, OriginalRandom& random, const OriginalRandom::TimeSource& time = {}) const;
    std::array<int, 9> baseline(unsigned kind) const;
    unsigned class_count() const;
    void set_preview_source(PreviewSource source);
    s2ne::Result<Preview> try_preview(unsigned target, OriginalRandom& random, const OriginalRandom::TimeSource& time) const;
    Candidate candidate(unsigned kind) const;
    unsigned experience_threshold(unsigned kind) const;
    unsigned innate_ability(unsigned kind) const;
    // 4409B7 mutates EXP only for a terminal class, after level/mode gates.
    bool eligible(OriginalEvents::Unit& unit, unsigned runtime_threshold, int game_mode = 0) const;
    bool target_allowed(const OriginalEvents::Unit& unit, unsigned kind) const;
    std::vector<Candidate> candidates(const OriginalEvents::Unit& unit) const;
    bool open(OriginalEvents::Unit unit, unsigned runtime_threshold, Commit commit = {}, bool battlefield = true, int game_mode = 0);
    Page page() const;
    const std::vector<Candidate>& choices() const;
    size_t selected_index() const;
    int training_group() const;
    int return_code() const;
    const OriginalEvents::Unit& unit() const;
    const Candidate& selected() const;
    bool tick(uint64_t milliseconds);
    std::optional<Request> request() const;
    void choose(size_t index);
    void inspect();
    bool train(int group);
    void cancel();
    bool confirm();
    s2ne::Result<std::vector<uint8_t>> try_draw(const std::vector<uint8_t>& background) const;
    std::vector<uint8_t> draw(const std::vector<uint8_t>& background) const;
    Result event(const OriginalSettings::Event& e);
};
}    //namespace s2ne
