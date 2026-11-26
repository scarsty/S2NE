#pragma once
#include "original_combat_visual.hpp"
#include "original_death_dispatch.hpp"
#include "original_promotion_transaction.hpp"
#include "original_skill94_resolution.hpp"
namespace s2ne
{
// 466544 ->467625. No payment or reward: execute_pipeline owns those once.
class OriginalDrain
{
    OriginalSkill94Resolution resolution_;
    std::array<int, 256> config_{};

public:
    static void prepare_target(OriginalEvents::Unit& target)
    {
        auto& runtime = *target.action_status_runtime;
        if ((unsigned(runtime[8]) | unsigned(runtime[9]) << 8) != 2) { return; }
        target.ai_role10 = 1;
        runtime[0xA] = 1;
        runtime[0xB] = runtime[0xC] = runtime[0xD] = 0;
    }
    struct Packet
    {
        int chance{}, roll{}, damage{}, healing{}, snapshot{}, caster_before{}, caster_after_damage{};
        bool hit{}, killed{}, revived{}, reverse{}, reversed_damage{};
        unsigned consumed_item{};
    };
    static Result<OriginalDrain> create(const std::filesystem::path& root)
    {
        auto resolution = OriginalSkill94Resolution::create(root);
        if (!resolution) { return Result<OriginalDrain>::failure(resolution.error()); }
        auto resources = read_combat_resources(root);
        if (!resources) { return Result<OriginalDrain>::failure(resources.error()); }
        OriginalDrain result;
        result.resolution_ = std::move(resolution).value();
        for (unsigned i = 0; i < 256; ++i) { result.config_[i] = int(int8_t(resources.value().bin.at(resources.value().config + i))); }
        return Result<OriginalDrain>::success(std::move(result));
    }
    Result<Packet> resolve(OriginalEvents::Unit& actor, OriginalEvents::Unit& target,
        const OriginalEvents& resources, const OriginalPromotionTransaction& equipment, std::vector<uint8_t>& inventory,
        original_combat::Random& random, OriginalSkill94Resolution::DamageContext context,
        unsigned skill, uint32_t seconds, bool synchronous_death_ready) const
    {
        if ((skill != 18 && skill != 40) || !actor.action_status_runtime || !target.action_status_runtime
            || target.hp <= 0 || inventory.size() != equipment.item_count())
        {
            return Result<Packet>::failure(make_error("467625 drain runtime/inventory unavailable"));
        }
        auto a = actor, t = target;
        auto stream = random;
        auto items = inventory;
        Packet out;
        out.caster_before = a.hp;
        context.effect = skill == 18 ? 42 : 41;
        context.original_group = skill == 18 ? 2 : 4;
        context.formula_flags = 1;
        context.damage_mask = skill == 18 ? 2 : 0;
        context.terrain_flags = 0;
        context.target_status39 = t.action_status_runtime->at(0x39);
        context.target_status3c = t.action_status_runtime->at(0x3C);
        context.target_status3d = t.action_status_runtime->at(0x3D);
        context.target_status3f = t.action_status_runtime->at(0x3F);
        context.target_magic_guard = t.abilities & 0x10;
        context.actor_spell_boost = a.abilities & 0x400000;
        context.target_spell_armor = t.abilities & 0x2000000;
        context.target_special13 = resources.equipment_special(t, 13);
        context.target_equipment11 = resources.equipment_special(t, 11);
        const auto masks = resources.skill_class_masks(t);
        context.profession_resistance = masks[0];
        context.profession_weakness = masks[1];
        auto& runtime = *t.action_status_runtime;
        prepare_target(t);
        NativeDamageFormula::Input input;
        input.actor_value = std::max(1, a.st + (resources.equipment_special(a, 18) ? a.st / 5 : 0));
        input.target_value = std::max(1, t.st + (resources.equipment_special(t, 18) ? t.st / 5 : 0));
        input.actor_hp = a.hp;
        auto raw = NativeDamageFormula::evaluate(skill == 18 ? 9 : 23, skill == 18 ? 75 : 1, input, config_);
        if (!raw) { return Result<Packet>::failure(raw.error()); }
        auto resolved = resolution_.try_resolve_drain(a.st, t.st, raw.value(), t.hp, context, stream, seconds, skill == 18 ? 70 : 60);
        if (!resolved) { return Result<Packet>::failure(resolved.error()); }
        out.chance = resolved.value().chance;
        out.roll = resolved.value().roll;
        out.hit = resolved.value().hit;
        out.damage = out.hit ? resolved.value().damage : 0;
        out.snapshot = t.hp;
        if (out.hit)
        {
            runtime[0x3E] &= uint8_t(~2u);
            t.status3e = runtime[0x3E];
            if (context.effect == 42 && (runtime[0x3D] & 0x80) && stream.percent(seconds) <= 70)
            {
                runtime[0x3D] &= 0x7F;
                t.status3d = runtime[0x3D];
            }
        }
        if (out.damage > 0)
        {
            out.reversed_damage = (context.target_status3c & 8) != 0;
            original_combat::Feedback feedback;
            feedback.begin(0, 1, t.hp, out.damage, false, false);
            while (feedback.remaining)
            {
                feedback.consume_with_caster(context.effect, a.hp, a.max_hp, out.reversed_damage);
                ++feedback.step;
            }
            t.hp = feedback.hp_after;
            out.caster_after_damage = a.hp;
            if (!t.hp)
            {
                if (resources.equipment_special(t, 25)) { out.revived = true; }
                else if (resources.equipment_special(t, 28))
                {
                    auto slot = equipment.first_consumable_special(t, 28);
                    if (!slot) { return Result<Packet>::failure(make_error("467625 resurrection consumption unavailable")); }
                    out.consumed_item = t.equipment[*slot];
                    OriginalPromotionTransaction::State state{ t, items, OriginalRandom({ stream.seed_state(), stream.draw_count() }) };
                    equipment.remove_equipment(state, *slot, true);
                    t = std::move(state.unit);
                    items = std::move(state.inventory);
                    out.revived = true;
                }
                else if (context.difficulty == 2 && !t.hostile()) { out.revived = true; }
                else
                {
                    if ((t.status_flags & 0x40) || ((t.status_flags & 0x10) && !resources.army_status) || !synchronous_death_ready || !OriginalDeathDispatch::continuation(resources, t.id).supported)
                    {
                        return Result<Packet>::failure(make_error("467625 drain death continuation unavailable"));
                    }
                    out.killed = true;
                    t.status_flags |= 0x20;
                    t.ending_losses = uint16_t(t.ending_losses + 1);
                    a.ending_kills = uint16_t(a.ending_kills + 1);
                    for (auto item : t.initial_status)
                    {
                        if (!item) { continue; }
                        if (item > items.size()) { return Result<Packet>::failure(make_error("468FDF drain loot index invalid")); }
                        items[item - 1] = uint8_t(std::min(99u, unsigned(items[item - 1]) + 1));
                    }
                }
                if (out.revived)
                {
                    out.snapshot = 0;
                    out.healing = t.max_hp;
                    t.hp = t.max_hp;
                }
            }
            else if (resources.equipment_special(t, 29) && stream.percent(seconds) < 30)
            {
                out.reverse = true;
                out.snapshot = t.hp;
                out.healing = std::min(out.damage * 2, t.max_hp - t.hp);
                t.hp += out.healing;
            }
        }
        else
        {
            out.caster_after_damage = a.hp;
        }
        t.ending_combat_value = out.snapshot;
        auto put = [](auto& bytes, size_t p, int value)
        {
            bytes[p] = uint8_t(value);
            bytes[p + 1] = uint8_t(unsigned(value) >> 8);
        };
        put(*a.action_status_runtime, 0x124, a.hp);
        put(*t.action_status_runtime, 0x124, t.hp);
        put(*t.action_status_runtime, 0x132, out.snapshot);
        t.action_status_runtime->at(0x33) = uint8_t(t.status_flags);
        actor = std::move(a);
        target = std::move(t);
        inventory = std::move(items);
        random = stream;
        return Result<Packet>::success(out);
    }
};
}    //namespace s2ne
