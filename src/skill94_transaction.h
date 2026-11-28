#pragma once
#include "death_dispatch.h"
#include "skill94_resolution.h"
#include "skills.h"
namespace s2ne
{
// Single94 staging boundary. Unknown scripted death dispatch is rejected, never
// replaced by physical-combat settlement or a generic dead flag.
class Skill94Transaction
{
    Skill94Formula formula_;
    Skill94Resolution resolution_;
    DamageFormula damage_formula_;
    std::array<int, 256> config_{};

public:
    struct Context
    {
        Skill94Resolution::HitInput hit;
        Skill94Resolution::DamageContext damage;
        int elevation{};
        uint32_t seconds{};
        unsigned payment_state{};
        bool original_second_partition{}, target_runtime_known{}, synchronous_death_ready{}, computer{}, player_controlled{};
        const Events::Unit* partner{};
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
    static bool target_supported(const Events& events, const Events::Unit& target);
    using Award = Skills::AwardAdapter;
    Skill94Transaction() = default;
    static s2ne::Result<Skill94Transaction> create(const std::filesystem::path& root);
    explicit Skill94Transaction(const std::filesystem::path& root);
    struct AreaResult
    {
        std::vector<Result> targets;
        int raw_xp{}, proficiency{};
    };
    // 原46D431预估模式不抽签、不执行死亡／支付；复用实击公式与修正器。
    s2ne::Result<Skill94Resolution::Resolution> preview(const Skills::Record& record,
        const Events::Unit& actor, const Events::Unit& target, const Events& resources, Context ctx) const;
    s2ne::Result<AreaResult> execute_area(Skills& menu, Events::Unit& actor,
        std::vector<Events::Unit>& targets, const Events::Unit& center, const Events& resources,
        s2ne::Random& random, const std::vector<Context>& contexts, const Award& award,
        std::vector<uint8_t>& army_inventory, const PromotionTransaction& equipment, Events::Unit* partner = nullptr) const;
    // The caller must pass a requested original94 record and the actual map
    // triples. The function stages unit/RNG copies; adapters see staged data only.
    Result execute(Skills& menu, Events::Unit& actor, Events::Unit& target, const Events& resources,
        s2ne::Random& random, const Context& ctx, const Award& award,
        std::vector<uint8_t>* army_inventory = nullptr, const PromotionTransaction* equipment = nullptr, bool area_target = false) const;
};
}    //namespace s2ne
