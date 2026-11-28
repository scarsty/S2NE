#include "item_use.h"

namespace s2ne
{
auto ItemUse::offset(unsigned id) const -> size_t { return 2 + size_t(id - 1) * 38; }

ItemUse::ItemUse(const std::filesystem::path& root) : equipment_(root), root_(root)
{
    auto bytes = read_resource(root / "ITEMSP.BIN");
    if (!bytes)
    {
        report_error(bytes.error());
        return;
    }
    items_ = std::move(bytes).value();
}

auto ItemUse::choices(const Save::State& state, size_t role) const -> std::vector<unsigned>
{
    std::vector<unsigned> out;
    if (items_.empty() || role >= state.groups[0].units.size()) { return out; }
    // 44FD8B在关间模式不枚举装备，只枚举军团消耗品；战场才有装备施法。
    for (unsigned slot = 0; state.globals_4f7cd0[4] == 0 && slot < 6; ++slot)
    {
        unsigned id = word(state.groups[0].units[role], 0xCB + slot * 2);
        if (id && items_.at(offset(id) + 15)) { out.push_back(id); }
    }
    for (unsigned id = 1; id <= state.groups[0].inventory.size(); ++id)
    {
        size_t p = offset(id);
        if (state.groups[0].inventory[id - 1] && items_.at(p + 15) && items_.at(p + 14) == 12) { out.push_back(id); }
    }
    return out;
}

auto ItemUse::eligible(const Save::State& state, size_t role, unsigned id) const -> bool
{
    return id && role < state.groups[0].units.size() && equipment_.eligible(state.groups[0].units[role], id);
}

auto ItemUse::usable_in_inn(unsigned id) const -> bool { return id && id <= word(items_, 0) && items_.at(offset(id) + 15) == 1; }

auto ItemUse::spell(unsigned id) const -> unsigned { return id && id <= word(items_, 0) && items_.at(offset(id) + 15) == 2 ? items_.at(offset(id) + 25) : 0; }

auto ItemUse::targeted(unsigned id) const -> bool { return id && id <= word(items_, 0) && items_.at(offset(id) + 14) == 12; }

auto ItemUse::effect(unsigned id) const -> unsigned { return items_.at(offset(id) + 25); }

auto ItemUse::feedback_amount(const Save::State& before, const Save::State& after, size_t target, unsigned id,
    s2ne::Random random, uint32_t seconds) const -> int
{
    const auto& a = *target_record(before, target);
    const auto& b = *target_record(after, target);
    switch (effect(id))
    {
    case 4:
    case 9:
    case 20:
    case 53:
    case 43:
        return random.bounded(6 - int(before.globals_4f7cd0.at(28)), seconds) + 1;
    case 5:
    case 33: return std::max(0, sw(b, 0x124) - sw(a, 0x124));
    case 6:
    case 10:
    case 34: return std::max(0, sw(b, 0x126) - sw(a, 0x126));
    case 60: return std::max(0, sw(b, 0x128) - sw(a, 0x128));
    case 35: return std::max({ 0, sw(b, 0x124) - sw(a, 0x124), sw(b, 0x126) - sw(a, 0x126), sw(b, 0x128) - sw(a, 0x128) });
    default: return 0;
    }
}

auto ItemUse::target_record(const Save::State& state, size_t target) -> const Save::UnitRecord*
{
    for (const auto& group : state.groups)
    {
        if (target < group.units.size()) { return &group.units[target]; }
        target -= group.units.size();
    }
    return nullptr;
}

auto ItemUse::target_allowed(const Save::State& state, size_t actor, size_t target, unsigned id) const -> bool
{
    const auto* record = target_record(state, target);
    if (!id || id > word(items_, 0) || actor >= state.groups[0].units.size() || !record) { return false; }
    const auto& a = state.groups[0].units[actor];
    const auto& t = *record;
    if (!targeted(id) || state.globals_4f7cd0[4] != 0) { return actor == target && equipment_.eligible(t, id); }
    // 446E53按当前完整军表顺序查80；死亡20只有同时带2的残留对象可选。
    if (!(t[0x33] & 0x80) || ((t[0x33] & 0x20) && !(t[0x33] & 2)) || !equipment_.eligible(t, id)) { return false; }
    // 4068C5读取DWORD0D的最高字节，即BYTE10h射程，而非WORD11h效果量。
    int ax = sw(a, 0x23), ay = sw(a, 0x27), tx = sw(t, 0x23), ty = sw(t, 0x27);
    return std::max(std::abs(2 * (tx - ax) + (ty & 1) - (ay & 1)), std::abs(ty - ay)) <= items_.at(offset(id) + 16)
        && (t[0x33] & 0x80);
}

auto ItemUse::try_apply(Save::State& state, size_t role, unsigned id, s2ne::Random& random, uint32_t seconds,
    std::optional<size_t> target) const -> Result<bool>
{
    if (items_.size() < 2 || !id || id > word(items_, 0) || role >= state.groups[0].units.size() || state.groups[0].inventory.size() != word(items_, 0))
    {
        return Result<bool>::failure(make_error("Invalid original45024C item use"));
    }
    const auto& recipient = state.groups[0].units[role];
    if ((recipient.at(0x33) & 0x70) || sw(recipient, 8) == 1) { return Result<bool>::success(false); }
    const auto available = choices(state, role);
    const size_t recipient_index = target.value_or(role);
    if (std::find(available.begin(), available.end(), id) == available.end() || !target_allowed(state, role, recipient_index, id)) { return Result<bool>::success(false); }
    size_t p = offset(id);
    if (items_[p + 15] == 2) { return Result<bool>::success(false); }
    auto staged = state;
    auto rng = random;
    size_t recipient_group = 0, recipient_role = recipient_index;
    while (recipient_group < staged.groups.size() && recipient_role >= staged.groups[recipient_group].units.size())
    {
        recipient_role -= staged.groups[recipient_group].units.size();
        ++recipient_group;
    }
    auto& r = staged.groups[recipient_group].units[recipient_role];
    const unsigned effect = items_[p + 25];
    const int amount = sw(items_, p + 17), duration = rng.bounded(6 - int(state.globals_4f7cd0.at(28)), seconds) + 1;
    auto recalculate = [&]()
    {
        return s2ne::try_recalculate_status_stats(r);
    };
    auto heal = [&](size_t maximum, size_t current, int value)
    {
        add(r, current, std::min(value, sw(r, maximum) - sw(r, current)));
    };
    auto grow = [&](size_t baseline, size_t maximum, size_t current, int cap, int value)
    {
        int delta = std::max(0, std::min(value, cap - sw(r, baseline)));
        add(r, maximum, delta);
        if (current) { add(r, current, delta); }
    };
    Result<bool> applied = Result<bool>::success(true);
    switch (effect)
    {
    case 0: break;
    case 4: applied = s2ne::try_apply_timed_status(r, 8, duration); break;
    case 9: applied = s2ne::try_apply_timed_status(r, 0x100000, duration); break;
    case 20: applied = s2ne::try_apply_timed_status(r, 2, duration); break;
    case 53: applied = s2ne::try_apply_timed_status(r, 16, duration); break;
    case 5:
        if (!(r[0x3E] & 1)) { put(r, 0x124, sw(r, 0xB7)); }
        break;
    case 6:
    case 10:
        if (!(r[0x3E] & 1)) { heal(0xB9, 0x126, effect == 6 ? 60 : 30); }
        break;
    case 27:
    case 32: break;
    case 33:
        if (!(r[0x3E] & 1)) { heal(0xB7, 0x124, amount); }
        break;
    case 34:
        if (!(r[0x3E] & 1)) { heal(0xB9, 0x126, amount); }
        break;
    case 35:
        if (!(r[0x3E] & 1))
        {
            put(r, 0x38, word(r, 0x38) & 0xB974);
            for (unsigned i = 0; i < 4; ++i) { r[0x3C + i] &= uint8_t(0xFEF00FFFu >> (8 * i)); }
            applied = recalculate();
            if (!applied) { return applied; }
            int delta = amount;
            if (sw(r, 0xB7) - sw(r, 0x124) < amount) { delta = sw(r, 0xB7) - sw(r, 0x124); }
            add(r, 0x124, delta);
            if (sw(r, 0xB9) - sw(r, 0x126) < amount)
            {
                if (sw(r, 0xB9) - sw(r, 0x126) > delta) { delta = sw(r, 0xB9) - sw(r, 0x126); }
                put(r, 0x126, sw(r, 0xB9));
            }
            else
            {
                add(r, 0x126, amount);
                delta = amount;
            }
            if (sw(r, 0xBB) - sw(r, 0x128) < amount)
            {
                if (sw(r, 0xBB) - sw(r, 0x128) > delta) { delta = sw(r, 0xBB) - sw(r, 0x128); }
                put(r, 0x128, sw(r, 0xBB));
            }
            else
            {
                add(r, 0x128, amount);
            }
        }
        break;
    case 36:
        r[0x39] &= 0xFB;
        applied = recalculate();
        break;
    case 37:
        if (r[0x39] & 2)
        {
            r[0x39] ^= 2;
            applied = recalculate();
        }
        break;
    case 38:
    case 39:
        put(r, 0x38, word(r, 0x38) & (effect == 38 ? 0xBB74 : 0xB974));
        for (unsigned i = 0; i < 4; ++i) { r[0x3C + i] &= uint8_t(0xFEF00FFFu >> (8 * i)); }
        applied = recalculate();
        break;
    case 40: put(r, 0xF1, std::min(15000, int(word(r, 0xF1)) + amount * 10)); break;
    case 43: applied = s2ne::try_apply_stat134_status(r, sw(r, 0x134) / 2, duration); break;
    case 44: grow(0xC3, 0xC3, 0x134, 999, amount); break;
    case 45: grow(0xC5, 0xC5, 0x136, 999, amount); break;
    case 46: grow(0xC1, 0xC1, 0, 999, amount); break;
    case 47: grow(0xBD, 0xBD, 0x12A, 999, amount); break;
    case 48: grow(0xC9, 0xC9, 0x138, 999, amount); break;
    case 49: grow(0xC7, 0xC7, 0, 999, amount); break;
    case 50:
        if (!(r[0x3E] & 1)) { grow(0xB7, 0xB7, 0x124, 5000, amount); }
        break;
    case 51:
        if (!(r[0x3E] & 1)) { grow(0xB9, 0xB9, 0x126, 999, amount); }
        break;
    case 52:
    {
        int delta = std::max(0, std::min(amount, 110 - sw(r, 0xBF)));
        add(r, 0xBF, delta);
        add(r, 0x12C, delta);
        add(r, 0x12E, delta);
        break;
    }
    case 61:
        grow(0xB7, 0xB7, 0x124, 5000, 100);
        {
            auto shift = [&](size_t base, size_t maximum, size_t current, size_t currentBase, int target)
            {
                int delta = (target - sw(r, base)) / 3;
                add(r, maximum, delta);
                if (sw(r, maximum) < 0) { put(r, maximum, 1); }
                else if (sw(r, maximum) > 999) { put(r, maximum, 999); }
                if (current)
                {
                    add(r, current, delta);
                    if (sw(r, currentBase) < 0) { put(r, current, 1); }
                    else if (sw(r, currentBase) > 999) { put(r, current, 999); }
                }
            };
            shift(0xC3, 0xC3, 0x134, 0x134, 250);
            if (sw(r, 0xB9) > 0) { shift(0xB9, 0xB9, 0x126, 0x126, 130); }
            if (sw(r, 0xBB) > 0) { shift(0xBB, 0xBB, 0x128, 0x128, 130); }
            shift(0xBD, 0xBD, 0x12A, 0x12A, 130);
            shift(0xC5, 0xC5, 0x136, 0x136, 130);
            shift(0xC9, 0xC9, 0x138, 0x138, 130);
            shift(0xC7, 0xC7, 0, 0, 130);
            shift(0xC1, 0xC1, 0, 0, 130);
        }
        break;
    case 54: r[0x30] |= 1; break;
    case 59: r[0x3D] &= 0x7F; break;
    case 60:
        if (!(r[0x3E] & 1)) { heal(0xBB, 0x128, amount); }
        break;
    case 7:
    case 8:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 28:
    case 29:
    case 30:
    case 31:
    case 41:
    case 42:
    case 55:
    case 56:
    case 57:
    case 58:
    case 62:
    case 63:
    case 64: break;
    case 67:
    {
        auto restored = SaveBridge::try_restore(root_, staged);
        if (!restored) { return Result<bool>::failure(restored.error()); }
        auto value = restored.value().events.try_combat_value(restored.value().events.units.at(recipient_index));
        if (!value) { return Result<bool>::failure(value.error()); }
        put(r, 0x113, std::min(999u, unsigned(uint16_t(word(r, 0x113) + (value.value() < 3000 ? 999 : 100)))));
        break;
    }
    case 65:
    case 66:
    {
        auto restored = SaveBridge::try_restore(root_, staged);
        if (!restored) { return Result<bool>::failure(restored.error()); }
        auto transaction = PromotionTransaction::create(root_);
        if (!transaction) { return Result<bool>::failure(transaction.error()); }
        auto actor = restored.value().events.units.at(recipient_index);
        if (!transaction.value().learn_range(actor, unsigned(amount), effect == 66 ? 1u : unsigned(items_[p + 19]), unsigned(items_[p + 19])))
        {
            return Result<bool>::failure(make_error("Original451DDA item learning range rejected"));
        }
        for (size_t i = 0; i < actor.learned.size(); ++i)
        {
            put(r, 0xFB + i * 2, int(actor.learned[i]));
            put(r, 0xE3 + i * 2, int(actor.learned_proficiency[i]));
        }
        break;
    }
    default: return Result<bool>::failure(make_error("Original450550 item effect requires complete handler: " + std::to_string(effect)));
    }
    if (!applied) { return applied; }
    if (items_[p + 14] == 12) { --staged.groups[0].inventory[id - 1]; }
    state = std::move(staged);
    random = rng;
    return Result<bool>::success(true);
}

auto ItemUse::try_prepare(const Save::State& before, size_t role, unsigned id,
    s2ne::Random& random, uint32_t seconds, std::optional<size_t> target) const -> Result<Save::State>
{
    auto staged = before;
    auto rng = random;
    auto applied = try_apply(staged, role, id, rng, seconds, target);
    if (!applied) { return Result<Save::State>::failure(applied.error()); }
    if (!applied.value()) { return Result<Save::State>::failure(make_error("Original45024C item source or recipient rejected")); }
    auto restored = SaveBridge::try_restore(root_, staged);
    if (!restored) { return Result<Save::State>::failure(restored.error()); }
    auto captured = SaveBridge::try_capture(restored.value().events, restored.value().turn, restored.value().camera, &staged);
    if (!captured) { return captured; }
    auto encoded = Save::try_encode(captured.value(), staged.groups[0].inventory.size());
    if (!encoded) { return Result<Save::State>::failure(encoded.error()); }
    random = rng;
    return captured;
}
}    //namespace s2ne
