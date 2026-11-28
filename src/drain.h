#pragma once
#include "combat_visual.h"
#include "death_dispatch.h"
#include "promotion_transaction.h"
#include "skill94_resolution.h"
namespace s2ne
{
// 466544 ->467625. No payment or reward: execute_pipeline owns those once.
class Drain
{
    Skill94Resolution resolution_;
    std::array<int, 256> config_{};

public:
    static void prepare_target(Events::Unit& target);
    struct Packet
    {
        int chance{}, roll{}, damage{}, healing{}, snapshot{}, caster_before{}, caster_after_damage{};
        bool hit{}, killed{}, revived{}, reverse{}, reversed_damage{};
        unsigned consumed_item{};
    };
    static Result<Drain> create(const std::filesystem::path& root);
    Result<Packet> resolve(Events::Unit& actor, Events::Unit& target,
        const Events& resources, const PromotionTransaction& equipment, std::vector<uint8_t>& inventory,
        s2ne::Random& random, Skill94Resolution::DamageContext context,
        unsigned skill, uint32_t seconds, bool synchronous_death_ready) const;
};
}    //namespace s2ne
