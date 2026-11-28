#pragma once
#include "original_death_dispatch.h"
#include "original_skill94_resolution.h"
#include "original_skills.h"
namespace s2ne
{
// Single94 staging boundary. Unknown scripted death dispatch is rejected, never
// replaced by physical-combat settlement or a generic dead flag.
class OriginalSkill94Transaction
{
    OriginalSkill94Formula formula_;
    OriginalSkill94Resolution resolution_;
    NativeDamageFormula damage_formula_;
    std::array<int, 256> config_{};

public:
    struct Context
    {
        OriginalSkill94Resolution::HitInput hit;
        OriginalSkill94Resolution::DamageContext damage;
        int elevation{};
        uint32_t seconds{};
        unsigned payment_state{};
        bool original_second_partition{}, target_runtime_known{}, synchronous_death_ready{}, computer{}, player_controlled{};
        const OriginalEvents::Unit* partner{};
    };
    enum class Failure
    {
        none,
        gate,
        unknown_runtime,
        scripted_death,
        resurrection_consumption,
        award
    };
    struct Result
    {
        bool applied{}, hit{}, revived{}, reverse_heal{};
        Failure failure{ Failure::gate };
        int chance{}, roll{}, damage{}, healed{}, raw_xp{}, proficiency{}, hp_snapshot{};
        bool killed{};
        unsigned consumed_item{};
    };
    static bool target_supported(const OriginalEvents& events, const OriginalEvents::Unit& target);
    using Award = OriginalSkills::AwardAdapter;
    OriginalSkill94Transaction() = default;
    static s2ne::Result<OriginalSkill94Transaction> create(const std::filesystem::path& root);
    explicit OriginalSkill94Transaction(const std::filesystem::path& root);
    struct AreaResult
    {
        std::vector<Result> targets;
        int raw_xp{}, proficiency{};
    };
    // 原46D431预估模式不抽签、不执行死亡／支付；复用实击公式与修正器。
    s2ne::Result<OriginalSkill94Resolution::Resolution> preview(const OriginalSkills::Record& record,
        const OriginalEvents::Unit& actor, const OriginalEvents::Unit& target, const OriginalEvents& resources, Context ctx) const;
    s2ne::Result<AreaResult> execute_area(OriginalSkills& menu, OriginalEvents::Unit& actor,
        std::vector<OriginalEvents::Unit>& targets, const OriginalEvents::Unit& center, const OriginalEvents& resources,
        s2ne::Random& random, const std::vector<Context>& contexts, const Award& award,
        std::vector<uint8_t>& army_inventory, const OriginalPromotionTransaction& equipment, OriginalEvents::Unit* partner = nullptr) const;
    // The caller must pass a requested original94 record and the actual map
    // triples. The function stages unit/RNG copies; adapters see staged data only.
    Result execute(OriginalSkills& menu, OriginalEvents::Unit& actor, OriginalEvents::Unit& target, const OriginalEvents& resources,
        s2ne::Random& random, const Context& ctx, const Award& award,
        std::vector<uint8_t>* army_inventory = nullptr, const OriginalPromotionTransaction* equipment = nullptr, bool area_target = false) const;
};
}    //namespace s2ne
