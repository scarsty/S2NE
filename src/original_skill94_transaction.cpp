#include "original_skill94_transaction.h"

namespace s2ne
{
auto OriginalSkill94Transaction::target_supported(const OriginalEvents& events, const OriginalEvents::Unit& target) -> bool
{
    (void)events;
    // 46618D needs target runtime for hit/status resolution. 41C02A is
    // only relevant after damage actually reduces the target to zero.
    // 446E53仅按80h/20h查找目标，10h/40h不是46618D的拒绝条件。
    return target.action_status_runtime.has_value() || target.status39.has_value();
}

auto OriginalSkill94Transaction::create(const std::filesystem::path& root) -> s2ne::Result<OriginalSkill94Transaction>
{
    OriginalSkill94Transaction result;
    auto formula = OriginalSkill94Formula::create(root);
    if (!formula)
    {
        return s2ne::Result<OriginalSkill94Transaction>::failure(formula.error());
    }
    auto resolution = OriginalSkill94Resolution::create(root);
    if (!resolution)
    {
        return s2ne::Result<OriginalSkill94Transaction>::failure(resolution.error());
    }
    auto resources = read_combat_resources(root);
    if (!resources)
    {
        return s2ne::Result<OriginalSkill94Transaction>::failure(resources.error());
    }
    result.formula_ = std::move(formula).value();
    auto damage_formula = NativeDamageFormula::create(root);
    if (!damage_formula) { return s2ne::Result<OriginalSkill94Transaction>::failure(damage_formula.error()); }
    result.damage_formula_ = std::move(damage_formula).value();
    result.resolution_ = std::move(resolution).value();
    auto& data = resources.value();
    for (unsigned i = 0; i < 256; ++i)
    {
        result.config_[i] = int(int8_t(data.bin[data.config + i]));
    }
    if (result.config_[0xE1] <= 0)
    {
        return s2ne::Result<OriginalSkill94Transaction>::failure(make_error("Invalid94 reward divisor"));
    }
    return s2ne::Result<OriginalSkill94Transaction>::success(std::move(result));
}

OriginalSkill94Transaction::OriginalSkill94Transaction(const std::filesystem::path& root)
{
    auto result = create(root);
    if (!result)
    {
        fatal(result.error());
    }
    *this = std::move(result).value();
}

auto OriginalSkill94Transaction::preview(const OriginalSkills::Record& record,
    const OriginalEvents::Unit& actor, const OriginalEvents::Unit& target, const OriginalEvents& resources, Context ctx) const -> s2ne::Result<OriginalSkill94Resolution::Resolution>
{
    if (record.effect() == 3)
    {
        auto modifiers = ctx.damage;
        modifiers.effect = 41;
        modifiers.terrain_flags = 2;
        modifiers.target_immunity2f = target.runtime_role_selector;
        s2ne::Random unused(1);
        auto result = resolution_.try_resolve_drain(actor.st, target.st, target.hp, target.hp, modifiers, unused, 0, record.word(23), false);
        if (result) { result.value().damage = result.value().chance ? target.hp : 0; }
        return result;
    }
    NativeDamageFormula::Input input;
    int attack = 0, defense = 0, count = 0, actorCount = 0;
    if (record.raw[33] & 1)
    {
        attack += actor.st + (resources.equipment_special(actor, 18) ? actor.st / 5 : 0);
        defense += target.st + (resources.equipment_special(target, 18) ? target.st / 5 : 0);
        ++count;
        ++actorCount;
        if (ctx.partner)
        {
            attack += ctx.partner->st + (resources.equipment_special(*ctx.partner, 18) ? ctx.partner->st / 5 : 0);
            ++actorCount;
        }
    }
    if (record.raw[33] & 2)
    {
        attack += actor.fencing;
        defense += target.fencing;
        ++count;
        ++actorCount;
        if (ctx.partner)
        {
            attack += ctx.partner->fencing;
            ++actorCount;
        }
    }
    if (!count || !actorCount) { return s2ne::Result<OriginalSkill94Resolution::Resolution>::failure(make_error("46E858 AI damage source unavailable")); }
    input.actor_value = std::max(1, attack / actorCount);
    input.target_value = std::max(1, defense / count);
    input.proficiency = actor.learned_proficiency[record.group];
    if (ctx.partner) { input.proficiency = ((input.proficiency / 10 + ctx.partner->learned_proficiency[record.group] / 10) / 2) * 10; }
    input.target_defense = target.defense;
    input.target_attribute = target.kind_attribute + (resources.equipment_special(target, 64) ? 10 : 0);
    input.actor_hp = actor.hp;
    input.target_hp = target.hp;
    input.actor_combat_value = actor.ending_combat_value.value_or(resources.combat_value(actor));
    auto raw = damage_formula_.raw(record.id, input);
    if (record.effect() == 41 || record.effect() == 42)
    {
        input.actor_value = std::max(1, actor.st + (resources.equipment_special(actor, 18) ? actor.st / 5 : 0));
        input.target_value = std::max(1, target.st + (resources.equipment_special(target, 18) ? target.st / 5 : 0));
        raw = NativeDamageFormula::evaluate(record.raw[20], record.word(21), input, config_);
    }
    if (!raw) { return s2ne::Result<OriginalSkill94Resolution::Resolution>::failure(raw.error()); }
    auto& modifiers = ctx.damage;
    modifiers.target_direction_guard = target.abilities & 0x2000;
    modifiers.damage_mask = record.word(27);
    modifiers.original_group = record.raw[0];
    modifiers.formula_flags = record.raw[33];
    modifiers.bypass_modifiers = record.raw[29] & 64;
    modifiers.terrain_flags = record.raw[29];
    modifiers.target_movement_class = resources.skill_target_class(target, modifiers.target_status3c);
    modifiers.target_immunity2f = target.runtime_role_selector;
    modifiers.target_magic_guard = target.abilities & 0x10;
    modifiers.actor_spell_boost = actor.abilities & 0x400000;
    modifiers.target_spell_armor = target.abilities & 0x2000000;
    modifiers.target_equipment11 = resources.equipment_special(target, 11);
    modifiers.effect = record.effect();
    const unsigned status3e = target.action_status_runtime ? target.action_status_runtime->at(0x3E) : target.status3e;
    s2ne::Random unused(1);
    if (record.effect() == 41 || record.effect() == 42)
    {
        modifiers.terrain_flags = 0;
        modifiers.formula_flags = 1;
        return resolution_.try_resolve_drain(actor.st, target.st, raw.value(), target.hp, modifiers, unused, 0, record.word(23), false);
    }
    const unsigned hit_source = record.raw[33] & 7u;
    return resolution_.try_resolve(ctx.hit, raw.value(), target.hp, ctx.elevation, modifiers, unused, 0,
        record.word(21), (modifiers.target_status39 & 2) || (modifiers.target_status3d & 0x20) || (status3e & 2),
        record.raw[33] & 128, hit_source, false);
}

auto OriginalSkill94Transaction::execute_area(OriginalSkills& menu, OriginalEvents::Unit& actor,
    std::vector<OriginalEvents::Unit>& targets, const OriginalEvents::Unit& center, const OriginalEvents& resources,
    s2ne::Random& random, const std::vector<Context>& contexts, const Award& award,
    std::vector<uint8_t>& army_inventory, const OriginalPromotionTransaction& equipment, OriginalEvents::Unit* partner) const -> s2ne::Result<AreaResult>
{
    auto fail = []
    {
        return s2ne::Result<AreaResult>::failure(make_error("Original466082 area transaction rejected"));
    };
    if (!menu.damage_supported() || menu.page() != OriginalSkills::Page::requested || targets.empty() || targets.size() != contexts.size() || !award || !menu.sleep2_center_in_range(actor, center)) { return fail(); }
    auto a = actor;
    const bool cooperative = menu.selected().raw[32] & 7;
    if (cooperative && (!partner || !menu.request() || partner->id != menu.request()->partner_id || partner->appearance != menu.request()->partner_appearance || !menu.cooperative_pair(actor, *partner) || menu.resource_failure(*partner, menu.selected()) != OriginalSkills::Failure::none)) { return fail(); }
    auto p = partner ? *partner : actor;
    auto staged = targets;
    auto stream = random;
    auto inventory = army_inventory;
    AreaResult out;
    int successes = 0;
    for (size_t i = 0; i < staged.size(); ++i)
    {
        auto& target = staged[i];
        if (!menu.sleep2_area(center, target) || !menu.status_area_target(actor, target)) { return fail(); }
        auto single = menu;
        auto unit = a;
        auto context = contexts[i];
        context.payment_state = 2;
        context.partner = cooperative ? &p : nullptr;
        auto packet = execute(single, unit, target, resources, stream, context, [](auto&, auto&, int, int, unsigned)
            {
                return true;
            },
            &inventory, &equipment, true);
        if (!packet.applied) { return s2ne::Result<AreaResult>::failure(make_error("Original46618D target failure " + std::to_string(int(packet.failure)) + " target=" + std::to_string(target.id) + " flags=" + std::to_string(target.status_flags))); }
        a.ending_kills = unit.ending_kills;
        out.raw_xp += packet.raw_xp;
        successes += packet.damage != 0;
        out.targets.push_back(packet);
    }
    out.proficiency = (6 + successes + int(uint16_t(a.ending_kills - actor.ending_kills)) - int(contexts.front().damage.difficulty)) * 10 / 3 + 10;
    if (cooperative)
    {
        for (const auto& target : staged)
        {
            if (target.id == p.id && target.appearance == p.appearance) { p = target; }
        }
    }
    if (cooperative)
    {
        out.raw_xp = std::max(1, out.raw_xp / 2);
        if (!award(p, stream, out.raw_xp, out.proficiency, menu.selected().group)) { return fail(); }
    }
    if (!award(a, stream, out.raw_xp, out.proficiency, menu.selected().group)) { return fail(); }
    OriginalSkills::ExecutionContext payment;
    payment.skill_payment_state = contexts.front().payment_state;
    menu.pay_staged(a, payment, actor.abilities);
    if (cooperative) { menu.pay_staged(p, payment, partner->abilities); }
    if (cooperative)
    {
        for (auto& target : staged)
        {
            if (target.id == p.id && target.appearance == p.appearance) { target = p; }
        }
    }
    actor = std::move(a);
    targets = std::move(staged);
    random = stream;
    army_inventory = std::move(inventory);
    if (cooperative) { *partner = std::move(p); }
    menu.cancel();
    return s2ne::Result<AreaResult>::success(std::move(out));
}

auto OriginalSkill94Transaction::execute(OriginalSkills& menu, OriginalEvents::Unit& actor, OriginalEvents::Unit& target, const OriginalEvents& resources,
    s2ne::Random& random, const Context& ctx, const Award& award,
    std::vector<uint8_t>* army_inventory, const OriginalPromotionTransaction* equipment, bool area_target) const -> Result
{
    Result out;
    if (menu.page() != OriginalSkills::Page::requested || !menu.request())
    {
        return out;
    }
    const auto request = *menu.request();
    const auto record = menu.selected();
    const bool family = area_target && menu.damage_supported();
    const bool led_ally = ctx.player_controlled && actor.behavior == 1 && !actor.hostile();
    if ((!family && !menu.sword94_supported()) || ((record.raw[32] & 7) && !ctx.partner) || request.unit_id != actor.id || request.appearance != actor.appearance || request.group != record.group || request.bit != record.bit || request.second != ctx.original_second_partition || !award || !menu.source_authorized(actor, record.group, record.bit) || (!actor.controllable() && !ctx.computer && !led_ally) || !(actor.status_flags & 0x80) || (actor.status_flags & 0x20) || actor.hp <= 0 || target.hp <= 0 || (record.raw[31] == 2 && actor.hostile() == target.hostile()) || (actor.id == target.id && actor.appearance == target.appearance) || !(target.status_flags & 0x80) || (target.status_flags & 0x20) || ctx.damage.difficulty < 1 || ctx.damage.difficulty > 5 || (!ctx.damage.friendly_army && !ctx.computer) || ctx.damage.conditions_mode || menu.resource_failure(actor, record) != OriginalSkills::Failure::none)
    {
        return out;
    }
    int dx = 2 * (target.x - actor.x) + int(target.y & 1) - int(actor.y & 1), dy = target.y - actor.y;
    unsigned status3d = actor.action_status_runtime ? actor.action_status_runtime->at(0x3D) : actor.status3d;
    if ((!family && std::max(std::abs(dx), std::abs(dy)) > 3) || ((status3d & 0x40) && !request.second))
    {
        return out;
    }
    if (!ctx.target_runtime_known || !actor.ending_combat_value)
    {
        out.failure = Failure::unknown_runtime;
        return out;
    }
    if (ctx.hit.actor_proficiency != actor.learned_proficiency[record.group] || ctx.hit.target_proficiency != target.learned_proficiency[record.group] || ctx.hit.actor_fencing != actor.fencing || ctx.hit.target_fencing != target.fencing || ctx.hit.target_tactics != target.tactics)
    {
        return out;
    }
    if (!target_supported(resources, target))
    {
        out.failure = Failure::scripted_death;
        return out;
    }
    auto a = actor, t = target;
    auto rng = random;
    auto inventory = army_inventory ? *army_inventory : std::vector<uint8_t>{};
    if (army_inventory && (!equipment || inventory.size() != equipment->item_count()))
    {
        return out;
    }
    auto raw = formula_.try_raw_damage(a, t);
    if (family)
    {
        NativeDamageFormula::Input input;
        int actor_sum = 0, target_sum = 0, actor_sources = 0, target_sources = 0;
        if (record.raw[33] & 1)
        {
            actor_sum = a.st + (resources.equipment_special(a, 18) ? a.st / 5 : 0);
            target_sum = t.st + (resources.equipment_special(t, 18) ? t.st / 5 : 0);
            ++actor_sources;
            ++target_sources;
            if (ctx.partner)
            {
                actor_sum += ctx.partner->st + (resources.equipment_special(*ctx.partner, 18) ? ctx.partner->st / 5 : 0);
                ++actor_sources;
            }
        }
        if (record.raw[33] & 2)
        {
            actor_sum += a.fencing;
            target_sum += t.fencing;
            ++actor_sources;
            ++target_sources;
            if (ctx.partner)
            {
                actor_sum += ctx.partner->fencing;
                ++actor_sources;
            }
        }
        if (!actor_sources || !target_sources) { return out; }
        input.actor_value = std::max(1, actor_sum / actor_sources);
        input.target_value = std::max(1, target_sum / target_sources);
        input.proficiency = a.learned_proficiency[record.group];
        if (ctx.partner) { input.proficiency = ((a.learned_proficiency[record.group] / 10 + ctx.partner->learned_proficiency[record.group] / 10) / 2) * 10; }
        input.target_defense = t.defense;
        input.target_attribute = t.kind_attribute + (resources.equipment_special(t, 64) ? 10 : 0);
        input.actor_hp = a.hp;
        input.target_hp = t.hp;
        input.actor_combat_value = *a.ending_combat_value;
        raw = damage_formula_.raw(record.id, input);
    }
    if (!raw)
    {
        report_error(raw.error());
        return out;
    }
    const unsigned status39 = t.action_status_runtime ? t.action_status_runtime->at(0x39) : t.status39.value_or(0);
    const unsigned target3d = t.action_status_runtime ? t.action_status_runtime->at(0x3D) : t.status3d;
    const unsigned status3e = t.action_status_runtime ? t.action_status_runtime->at(0x3E) : t.status3e;
    auto modifiers = ctx.damage;
    modifiers.target_direction_guard = t.abilities & 0x2000;
    if (family)
    {
        modifiers.damage_mask = record.word(27);
        modifiers.original_group = record.raw[0];
        modifiers.formula_flags = record.raw[33];
        modifiers.bypass_modifiers = record.raw[29] & 64;
        modifiers.terrain_flags = record.raw[29];
        modifiers.target_movement_class = resources.skill_target_class(t, ctx.damage.target_status3c);
        if (!t.action_status_runtime)
        {
            out.failure = Failure::unknown_runtime;
            return out;
        }
        modifiers.target_immunity2f = t.action_status_runtime->at(0x2F);
        modifiers.target_magic_guard = t.abilities & 0x10;
        modifiers.actor_spell_boost = a.abilities & 0x400000;
        modifiers.target_spell_armor = t.abilities & 0x2000000;
        modifiers.target_equipment11 = resources.equipment_special(t, 11);
    }
    const unsigned hit_source = record.raw[33] & 7u;
    auto resolved = resolution_.try_resolve(ctx.hit, raw.value(), t.hp, ctx.elevation, modifiers, rng, ctx.seconds, family ? record.word(21) : 50, (status39 & 2) || (target3d & 0x20) || (status3e & 2), (record.raw[33] & 128) != 0, hit_source);
    if (!resolved)
    {
        report_error(resolved.error());
        return out;
    }
    auto packet = resolved.value();
    out.chance = packet.chance;
    out.roll = packet.roll;
    out.hit = packet.hit;
    out.damage = packet.hit ? packet.damage : 0;
    // 468BAC always captures+132, even on MISS; effect1 has no drain branch.
    out.hp_snapshot = t.hp;
    if (family && actor.hostile() == target.hostile() && t.action_status_runtime && t.action_status_runtime->at(0x0A) == 2 && t.action_status_runtime->at(0x0B) == 0)
    {
        t.ai_role10 = 1;
        auto& runtime = *t.action_status_runtime;
        runtime[0x0A] = 1;
        runtime[0x0B] = runtime[0x0C] = runtime[0x0D] = 0;
    }
    t.hp -= out.damage;
    if (!t.hp)
    {
        if (resources.equipment_special(t, 25))
        {
            out.hp_snapshot = 0;
            t.hp = t.max_hp;
            out.revived = true;
        }
        else if (resources.equipment_special(t, 28))
        {
            if (!army_inventory || !equipment)
            {
                out.failure = Failure::resurrection_consumption;
                return out;
            }
            auto slot = equipment->first_consumable_special(t, 28);
            if (!slot)
            {
                out.failure = Failure::resurrection_consumption;
                return out;
            }
            out.hp_snapshot = 0;
            t.hp = t.max_hp;
            out.revived = true;
            out.consumed_item = t.equipment[*slot];
            OriginalPromotionTransaction::State state{ t, inventory, OriginalRandom({ rng.seed_state(), rng.draw_count() }) };
            equipment->remove_equipment(state, *slot, true);
            t = std::move(state.unit);
            inventory = std::move(state.inventory);
        }
        else
        {
            // Scripted deaths require the synchronous41C02A continuation.
            // Departure requires its own presentation; summoned slots release on publication.
            auto dispatch = OriginalDeathDispatch::inspect(resources, t.id);
            if (!army_inventory || !equipment || (!dispatch.settled() && !ctx.synchronous_death_ready) || (t.status_flags & 0x40) || ((t.status_flags & 0x10) && !resources.army_status))
            {
                out.failure = Failure::scripted_death;
                return out;
            }
            out.killed = true;
            t.ending_losses = uint16_t(t.ending_losses + 1);
            t.status_flags |= 0x20;
            a.ending_kills = uint16_t(a.ending_kills + 1);
            // 468FDF mode0/friendly army collects initial_status WORDs, not
            // equipped items.458B03 merges the byte buffer with cap99.
            std::vector<uint8_t> loot(inventory.size());
            for (auto id : t.initial_status)
            {
                if (id)
                {
                    if (id > loot.size())
                    {
                        out.failure = Failure::scripted_death;
                        return out;
                    }
                    ++loot[id - 1];
                }
            }
            for (size_t i = 0; i < inventory.size(); ++i)
            {
                if (loot[i])
                {
                    inventory[i] = uint8_t(std::min(99u, unsigned(inventory[i]) + unsigned(loot[i])));
                }
            }
        }
    }
    else if (resources.equipment_special(t, 29) && rng.percent(ctx.seconds) < 30)
    {
        // Original only draws this extra roll for living targets, including MISS.
        out.reverse_heal = true;
        out.hp_snapshot = t.hp;
        out.healed = std::min(out.damage * 2, t.max_hp - t.hp);
        t.hp += out.healed;
    }
    // 469101 mode1 uses the last468BAC+132 HP snapshot, other stats live.
    t.ending_combat_value = out.hp_snapshot;
    if (!out.killed)
    {
        int av = resources.combat_value(a, 1), tv = resources.combat_value(t, 1);
        out.raw_xp = std::max(1, out.damage * config_[0xE6] * tv / (tv + av) / config_[0xE1]);
    }
    // 468FDF passes arg4=0 to469101. With HP0,46913D increments
    // the XP accumulator once and returns BEFORE value/kill-bonus arithmetic.
    if (out.killed)
    {
        out.raw_xp = 1;
    }
    // 466082: successful targets + kill-count delta; resurrection is not a kill.
    out.proficiency = (6 + int(out.damage != 0) + int(out.killed) - int(ctx.damage.difficulty)) * 10 / 3 + 10;
    if (!award(a, rng, out.raw_xp, out.proficiency, record.group))
    {
        out.failure = Failure::award;
        return out;
    }
    a.action_points -= config_[0xEF] + ((a.status34 & 8) ? config_[0xF3] : 0);
    a.status34 |= 8;
    a.carry_points = std::min(a.carry_points, a.action_points);
    if (ctx.payment_state != 2)
    {
        a.st -= record.word(13);
    }
    actor = std::move(a);
    target = std::move(t);
    random = rng;
    if (army_inventory)
    {
        *army_inventory = std::move(inventory);
    }
    menu.cancel();
    out.applied = true;
    out.failure = Failure::none;
    return out;
}
}    //namespace s2ne
