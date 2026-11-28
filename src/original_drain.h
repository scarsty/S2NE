#pragma once
#include "original_combat_visual.h"
#include "original_death_dispatch.h"
#include "original_promotion_transaction.h"
#include "original_skill94_resolution.h"
namespace s2ne
{
// 466544 ->467625. No payment or reward: execute_pipeline owns those once.
class OriginalDrain
{
    OriginalSkill94Resolution resolution_;
    std::array<int, 256> config_{};

public:
    static void prepare_target(OriginalEvents::Unit& target);
    struct Packet
    {
        int chance{}, roll{}, damage{}, healing{}, snapshot{}, caster_before{}, caster_after_damage{};
        bool hit{}, killed{}, revived{}, reverse{}, reversed_damage{};
        unsigned consumed_item{};
    };
    static Result<OriginalDrain> create(const std::filesystem::path& root);
    Result<Packet> resolve(OriginalEvents::Unit& actor, OriginalEvents::Unit& target,
        const OriginalEvents& resources, const OriginalPromotionTransaction& equipment, std::vector<uint8_t>& inventory,
        s2ne::Random& random, OriginalSkill94Resolution::DamageContext context,
        unsigned skill, uint32_t seconds, bool synchronous_death_ready) const;
};
}    //namespace s2ne
