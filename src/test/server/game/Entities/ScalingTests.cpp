/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "IntegrationTestFixture.h"
#include "ObjectMgr.h"
#include "SpellInfoTestHelper.h"
#include "SpellAuraEffects.h"
#include "Unit.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <array>
#include <iostream>
#include <optional>
#include <string_view>

namespace
{
constexpr uint8 TestLowLevel = 20;
constexpr uint8 TestMidLevel = 30;
constexpr uint8 TestHighLevel = 40;
constexpr uint32 TestNativeValue = 400;
constexpr uint32 TestAbsorbSpellId = 990001;
constexpr uint32 TestManaShieldSpellId = 990002;
constexpr uint32 TestAreaDotSpellId = 990003;

class ScalingTests : public IntegrationTestFixture
{
protected:
    void SetUp() override
    {
        IntegrationTestFixture::SetUp();
        std::cout << "\n[SCENARIO] " << ::testing::UnitTest::GetInstance()->current_test_info()->name() << '\n';
        ON_CALL(*GetWorldMock(), getIntConfig(CONFIG_MAX_PLAYER_LEVEL)).WillByDefault(::testing::Return(80));
        ON_CALL(*GetWorldMock(), getBoolConfig(CONFIG_BOOL_SCALE_PVP_HOSTILE)).WillByDefault(::testing::Return(true));
        ON_CALL(*GetWorldMock(), getBoolConfig(CONFIG_BOOL_SCALE_PVP_FRIENDLY)).WillByDefault(::testing::Return(true));
        SetCreatureStats(TestLowLevel, 1000, 500, 100.0f);
        SetCreatureStats(TestMidLevel, 2000, 1000, 200.0f);
        SetCreatureStats(TestHighLevel, 4000, 2000, 400.0f);
        sObjectMgr->SetPlayerClassLevelInfoForTest(CLASS_WARRIOR, TestLowLevel, 1000, 500);
        sObjectMgr->SetPlayerClassLevelInfoForTest(CLASS_WARRIOR, TestMidLevel, 2000, 1000);
        sObjectMgr->SetPlayerClassLevelInfoForTest(CLASS_WARRIOR, TestHighLevel, 4000, 2000);
    }

    void TearDown() override
    {
        for (uint8 level : { TestLowLevel, TestMidLevel, TestHighLevel })
            sObjectMgr->RemoveCreatureBaseStatsForTest(level, CLASS_WARRIOR);
        IntegrationTestFixture::TearDown();
    }

    void SetCreatureStats(uint8 level, uint32 health, uint32 mana, float armor)
    {
        CreatureBaseStats stats{};
        for (uint8 expansion = 0; expansion < MAX_EXPANSIONS; ++expansion)
            stats.BaseHealth[expansion] = health;
        stats.BaseMana = mana;
        stats.BaseArmor = armor;
        sObjectMgr->SetCreatureBaseStatsForTest(level, CLASS_WARRIOR, stats);
    }

    TestPlayer* CreatePlayer(ObjectGuid::LowType guid, uint8 level, uint32 faction = TEST_FACTION_HOSTILE_TO_MONSTERS)
    {
        TestPlayer* player = CreateTestPlayer(guid);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_WARRIOR);
        player->SetUInt32Value(UNIT_FIELD_FACTIONTEMPLATE, faction);
        player->SetLevel(level);
        return player;
    }

    TestCreature* CreateCreature(ObjectGuid::LowType guid, uint8 level)
    {
        TestCreature* creature = CreateTestCreature(guid, 99000 + guid, TEST_FACTION_HOSTILE_TO_ALL);
        creature->SetLevel(level);
        creature->SetArmor(TestNativeValue);
        creature->SetMaxPower(POWER_MANA, 2000);
        creature->SetPower(POWER_MANA, 1000);
        return creature;
    }

    static std::string_view SpellTypeName(SpellType type)
    {
        switch (type)
        {
            case SPELLTYPE_DAMAGE: return "damage";
            case SPELLTYPE_POWER: return "power";
            case SPELLTYPE_CHARSTAT: return "character stat";
            case SPELLTYPE_HEAL: return "healing";
            case SPELLTYPE_RESIST: return "resistance";
            case SPELLTYPE_AMORMAGICPEN: return "armor/magic penetration";
            default: return "unknown";
        }
    }

    static std::string_view UnitTypeName(Unit const* unit)
    {
        if (!unit)
            return "null";
        return unit->IsPlayer() ? "player" : unit->IsCreature() ? "creature" : "unit";
    }

    template <typename T>
    static void Report(std::string_view operation, T input, T expected, T actual)
    {
        std::cout << "  [RESULT] " << operation << ": input=" << input
                  << ", expected=" << expected << ", actual=" << actual << '\n';
    }

    float Scale(Unit* source, Unit* target, float value, SpellType type = SPELLTYPE_DAMAGE)
    {
        float result = sObjectMgr->ScaleDamage(source, target, value, type);
        std::cout << "  [SCALE] " << SpellTypeName(type) << ": " << UnitTypeName(source)
                  << " (level " << (source ? uint32(source->GetLevel()) : 0) << ") -> "
                  << UnitTypeName(target) << " (level " << (target ? uint32(target->GetLevel()) : 0)
                  << "), input=" << value << ", scaled=" << result << '\n';
        return result;
    }

    uint32 ExpectScaledDamage(Unit* attacker, Unit* defender, uint32 nativeDamage, uint32 expectedScaledDamage)
    {
        std::cout << "  [COMBATANTS] attacker=" << UnitTypeName(attacker)
                  << " level=" << uint32(attacker->GetLevel())
                  << ", defender=" << UnitTypeName(defender)
                  << " level=" << uint32(defender->GetLevel()) << '\n';
        float scaledDamage = Scale(attacker, defender, float(nativeDamage));
        Report("pre-mitigation scaled damage", float(nativeDamage), float(expectedScaledDamage), scaledDamage);
        EXPECT_FLOAT_EQ(scaledDamage, float(expectedScaledDamage));
        return uint32(std::lround(scaledDamage));
    }

    float RoundTrip(Unit* source, Unit* target, float value, SpellType type)
    {
        float scaled = Scale(source, target, value, type);
        float restored = sObjectMgr->ScaleDamageReverse(source, target, scaled, type);
        std::cout << "  [REVERSE] " << SpellTypeName(type) << ": stored=" << scaled
                  << ", restored=" << restored << ", original=" << value << '\n';
        return restored;
    }

    void ExpectPlayerToCreaturePath(std::string_view action, SpellType type = SPELLTYPE_DAMAGE)
    {
        TestPlayer* player = CreatePlayer(1, TestLowLevel);
        TestCreature* creature = CreateCreature(100, TestHighLevel);
        std::cout << "  [ACTION] " << action << " using " << SpellTypeName(type) << '\n';
        float scaled = Scale(player, creature, 100.0f, type);
        float restored = RoundTrip(player, creature, 100.0f, type);
        Report("scaled value", 100.0f, 400.0f, scaled);
        Report("round trip", 100.0f, 100.0f, restored);
        EXPECT_FLOAT_EQ(scaled, 400.0f);
        EXPECT_FLOAT_EQ(restored, 100.0f);
    }

    void ExpectCreatureToPlayerPath(std::string_view action, SpellType type = SPELLTYPE_DAMAGE)
    {
        TestCreature* creature = CreateCreature(100, TestHighLevel);
        TestPlayer* player = CreatePlayer(1, TestLowLevel);
        std::cout << "  [ACTION] " << action << " using " << SpellTypeName(type) << '\n';
        float scaled = Scale(creature, player, 100.0f, type);
        float restored = RoundTrip(creature, player, 100.0f, type);
        Report("scaled value", 100.0f, 25.0f, scaled);
        Report("round trip", 100.0f, 100.0f, restored);
        EXPECT_FLOAT_EQ(scaled, 25.0f);
        EXPECT_FLOAT_EQ(restored, 100.0f);
    }

    void ExpectControlledUnitUsesOwnerLevel(std::string_view summonType, ObjectGuid::LowType guid)
    {
        TestPlayer* owner = CreatePlayer(1, TestLowLevel);
        TestCreature* controlled = CreateCreature(guid, TestHighLevel);
        TestCreature* target = CreateCreature(guid + 1, TestHighLevel);
        controlled->SetOwnerGUID(owner->GetGUID());
        std::cout << "  [ACTION] " << summonType << " attack: owner level=" << uint32(owner->GetLevel())
                  << ", summon native level=" << uint32(controlled->GetLevel()) << ", input=100, expected=400\n";
        EXPECT_EQ(controlled->GetCharmerOrOwnerOrSelf(), owner);
        EXPECT_TRUE(sObjectMgr->UsesCreatureStorageScaling(controlled, target));
        float scaled = Scale(controlled, target, 100.0f);
        Report("controlled-unit damage", 100.0f, 400.0f, scaled);
        EXPECT_FLOAT_EQ(scaled, 400.0f);
    }

    std::unique_ptr<SpellInfo> BuildAbsorbSpell(uint32 id, AuraType auraType, int32 amount, float manaMultiplier = 0.0f,
        SpellSchoolMask schoolMask = SPELL_SCHOOL_MASK_FROST)
    {
        return SpellInfoBuilder()
            .WithId(id)
            .WithSchoolMask(schoolMask)
            .WithEffect(EFFECT_0, SPELL_EFFECT_APPLY_AURA, auraType)
            .WithEffectBasePoints(EFFECT_0, amount)
            .WithEffectMiscValue(EFFECT_0, schoolMask)
            .WithEffectValueMultiplier(EFFECT_0, manaMultiplier)
            .BuildUnique();
    }

    std::unique_ptr<SpellInfo> BuildAreaDotSpell(int32 tickDamage)
    {
        return SpellInfoBuilder()
            .WithId(TestAreaDotSpellId)
            .WithSchoolMask(SPELL_SCHOOL_MASK_SHADOW)
            .WithDmgClass(SPELL_DAMAGE_CLASS_MAGIC)
            .WithEffect(EFFECT_0, SPELL_EFFECT_APPLY_AURA, SPELL_AURA_PERIODIC_DAMAGE)
            .WithEffectBasePoints(EFFECT_0, tickDamage)
            .WithEffectImplicitTargets(EFFECT_0, TARGET_UNIT_CASTER, TARGET_UNIT_SRC_AREA_ENEMY)
            .BuildUnique();
    }

    DamageInfo ResolveAbsorb(Unit* attacker, Unit* victim, uint32 damage, SpellInfo const* spellInfo)
    {
        DamageInfo result(attacker, victim, damage, spellInfo, SPELL_SCHOOL_MASK_FROST, SPELL_DIRECT_DAMAGE);
        Unit::CalcAbsorbResist(result);
        std::cout << "  [ABSORB] incoming=" << damage << ", absorbed=" << result.GetAbsorb()
                  << ", remaining=" << result.GetDamage() << '\n';
        return result;
    }
};

TEST_F(ScalingTests, CreatureScaling_Level)
{
    TestPlayer* player = CreatePlayer(1, TestLowLevel);
    TestCreature* creature = CreateCreature(100, TestHighLevel);
    EXPECT_EQ(creature->getLevelForTarget(player), TestLowLevel);
    EXPECT_EQ(sObjectMgr->GetLevelScaled(player, creature), TestLowLevel);
}

TEST_F(ScalingTests, CreatureScaling_CritterRetainsNativeLevel)
{
    TestPlayer* player = CreatePlayer(1, TestLowLevel);
    TestCreature* creature = CreateCreature(100, TestHighLevel);
    CreatureTemplate* creatureTemplate = const_cast<CreatureTemplate*>(creature->GetCreatureTemplate());
    uint32 originalType = creatureTemplate->type;
    creatureTemplate->type = CREATURE_TYPE_CRITTER;

    EXPECT_FALSE(sObjectMgr->IsScalable(creature, player));
    EXPECT_EQ(creature->getLevelForTarget(player), TestHighLevel);
    EXPECT_EQ(sObjectMgr->GetLevelScaled(creature, player), TestHighLevel);

    creatureTemplate->type = originalType;
}

TEST_F(ScalingTests, CreatureScaling_WorldBossLevelIsClamped)
{
    TestPlayer* player = CreatePlayer(1, TestLowLevel);
    TestCreature* creature = CreateCreature(100, TestHighLevel);
    CreatureTemplate* creatureTemplate = const_cast<CreatureTemplate*>(creature->GetCreatureTemplate());
    uint32 originalTypeFlags = creatureTemplate->type_flags;
    creatureTemplate->type_flags |= CREATURE_TYPE_FLAG_BOSS_MOB;

    EXPECT_CALL(*GetWorldMock(), getIntConfig(CONFIG_WORLD_BOSS_LEVEL_DIFF))
        .WillOnce(::testing::Return(-100))
        .WillOnce(::testing::Return(300));
    EXPECT_CALL(*GetWorldMock(), getBoolConfig(CONFIG_BOOL_SCALE_PVE_ITEMLEVEL)).Times(0);

    EXPECT_EQ(creature->getLevelForTarget(player), 1);
    EXPECT_EQ(creature->getLevelForTarget(player), std::numeric_limits<uint8>::max());

    creatureTemplate->type_flags = originalTypeFlags;
}

TEST_F(ScalingTests, CreatureScaling_Health)
{
    TestPlayer* player = CreatePlayer(1, TestLowLevel);
    TestCreature* creature = CreateCreature(100, TestHighLevel);
    float ratio = sObjectMgr->GetCreatureBaseStatRatio(creature, TestLowLevel, SPELLTYPE_DAMAGE);
    uint32 health = creature->GetHealthForTarget(player);
    uint32 maxHealth = creature->GetMaxHealthForTarget(player);
    Report("health ratio", 1.0f, 0.25f, ratio);
    Report("visible health", creature->GetHealth(), 2500u, health);
    Report("visible max health", creature->GetMaxHealth(), 2500u, maxHealth);
    EXPECT_FLOAT_EQ(ratio, 0.25f);
    EXPECT_EQ(health, 2500u);
    EXPECT_EQ(maxHealth, 2500u);
}

TEST_F(ScalingTests, CreatureScaling_Power)
{
    TestPlayer* player = CreatePlayer(1, TestLowLevel);
    TestCreature* creature = CreateCreature(100, TestHighLevel);
    float ratio = sObjectMgr->GetCreatureBaseStatRatio(creature, TestLowLevel, SPELLTYPE_POWER);
    uint32 mana = creature->GetPowerForTarget(player, POWER_MANA);
    uint32 maxMana = creature->GetMaxPowerForTarget(player, POWER_MANA);
    Report("mana ratio", 1.0f, 0.25f, ratio);
    Report("visible mana", creature->GetPower(POWER_MANA), 250u, mana);
    Report("visible max mana", creature->GetMaxPower(POWER_MANA), 500u, maxMana);
    EXPECT_FLOAT_EQ(ratio, 0.25f);
    EXPECT_EQ(mana, 250u);
    EXPECT_EQ(maxMana, 500u);
}

TEST_F(ScalingTests, CreatureScaling_Armor)
{
    uint32 scaled = sObjectMgr->ScaleArmor(CreatePlayer(1, TestLowLevel), CreateCreature(100, TestHighLevel), TestNativeValue);
    Report("creature armor", TestNativeValue, 100u, scaled);
    EXPECT_EQ(scaled, 100u);
}

TEST_F(ScalingTests, CreatureScaling_Resistances)
{
    TestPlayer* player = CreatePlayer(1, TestLowLevel);
    TestCreature* creature = CreateCreature(100, TestHighLevel);
    creature->SetResistance(SPELL_SCHOOL_FIRE, 175);
    uint32 scaled = sObjectMgr->ScaleArmor(player, creature, creature->GetResistance(SPELL_SCHOOL_FIRE));
    Report("fire resistance projection", 175u, 43u, scaled);
    EXPECT_EQ(creature->GetResistance(SPELL_SCHOOL_FIRE), 175u);
    EXPECT_EQ(scaled, 43u);
}

TEST_F(ScalingTests, PvE_PlayerToCreature_Melee) { ExpectPlayerToCreaturePath("main-hand melee attack"); }
TEST_F(ScalingTests, PvE_PlayerToCreature_Ranged) { ExpectPlayerToCreaturePath("ranged auto attack"); }
TEST_F(ScalingTests, PvE_PlayerToCreature_Spell) { ExpectPlayerToCreaturePath("direct damage spell"); }
TEST_F(ScalingTests, PvE_PlayerToCreature_DoT) { ExpectPlayerToCreaturePath("periodic damage tick"); }
TEST_F(ScalingTests, PvE_PlayerToCreature_Proc) { ExpectPlayerToCreaturePath("triggered damage proc"); }
TEST_F(ScalingTests, PvE_PlayerToCreature_Pet) { ExpectControlledUnitUsesOwnerLevel("pet", 110); }
TEST_F(ScalingTests, PvE_CreatureToPlayer_Melee) { ExpectCreatureToPlayerPath("creature melee attack"); }
TEST_F(ScalingTests, PvE_CreatureToPlayer_Spell) { ExpectCreatureToPlayerPath("creature direct spell"); }
TEST_F(ScalingTests, PvE_CreatureToPlayer_DoT) { ExpectCreatureToPlayerPath("creature periodic damage tick"); }
TEST_F(ScalingTests, PvE_CreatureToPlayer_Proc) { ExpectCreatureToPlayerPath("creature triggered proc"); }

TEST_F(ScalingTests, EvP_PlayerToPlayer)
{
    EXPECT_EQ(sObjectMgr->GetLevelScaled(CreatePlayer(1, TestLowLevel), CreatePlayer(2, TestHighLevel, TEST_FACTION_HOSTILE_TO_ALL)), TestLowLevel);
}

TEST_F(ScalingTests, EvP_Damage)
{
    EXPECT_FLOAT_EQ(Scale(CreatePlayer(1, TestLowLevel), CreatePlayer(2, TestHighLevel, TEST_FACTION_HOSTILE_TO_ALL), 100.0f), 400.0f);
}

TEST_F(ScalingTests, EvP_Healing)
{
    EXPECT_FLOAT_EQ(RoundTrip(CreatePlayer(1, TestLowLevel), CreatePlayer(2, TestHighLevel), 100.0f, SPELLTYPE_HEAL), 100.0f);
}

TEST_F(ScalingTests, EvP_DoT)
{
    EXPECT_FLOAT_EQ(Scale(CreatePlayer(1, TestLowLevel), CreatePlayer(2, TestHighLevel, TEST_FACTION_HOSTILE_TO_ALL), 25.0f), 100.0f);
}

TEST_F(ScalingTests, AreaScaling_InheritsParentUnlessChildOverrides)
{
    constexpr uint32 ParentZoneId = 12;
    constexpr uint32 ChildAreaId = 34;
    std::optional<ZoneFlex> originalParent;
    std::optional<ZoneFlex> originalChild;

    if (ZoneFlex const* zoneFlex = sObjectMgr->GetZoneFlexForTest(ParentZoneId))
        originalParent = *zoneFlex;
    if (ZoneFlex const* zoneFlex = sObjectMgr->GetZoneFlexForTest(ChildAreaId))
        originalChild = *zoneFlex;

    ZoneFlex parent{ "Parent", ParentZoneId, 0, 10, 20, AREA_FLAG_LOWLEVEL };
    ZoneFlex child{ "Child", ChildAreaId, 0, 5, 10, 0 };
    sObjectMgr->SetZoneFlexForTest(parent);
    sObjectMgr->RemoveZoneFlexForTest(ChildAreaId);

    ZoneFlex const* inherited = sObjectMgr->GetAreaZoneFlex(ChildAreaId);
    EXPECT_EQ(inherited, sObjectMgr->GetZoneFlexForTest(ParentZoneId));

    sObjectMgr->SetZoneFlexForTest(child);
    EXPECT_EQ(sObjectMgr->GetAreaZoneFlex(ChildAreaId), sObjectMgr->GetZoneFlexForTest(ChildAreaId));

    if (originalParent)
        sObjectMgr->SetZoneFlexForTest(*originalParent);
    else
        sObjectMgr->RemoveZoneFlexForTest(ParentZoneId);
    if (originalChild)
        sObjectMgr->SetZoneFlexForTest(*originalChild);
    else
        sObjectMgr->RemoveZoneFlexForTest(ChildAreaId);
}

TEST_F(ScalingTests, EvP_Proc)
{
    TestPlayer* source = CreatePlayer(1, TestLowLevel);
    TestPlayer* target = CreatePlayer(2, TestHighLevel, TEST_FACTION_HOSTILE_TO_ALL);
    DamageInfo proc(source, target, uint32(Scale(source, target, 50.0f)), nullptr, SPELL_SCHOOL_MASK_FIRE, SPELL_DIRECT_DAMAGE);
    EXPECT_EQ(proc.GetDamage(), 200u);
}

TEST_F(ScalingTests, Healing_Direct)
{
    HealInfo heal(CreatePlayer(1, TestLowLevel), CreateCreature(100, TestHighLevel), 100, nullptr, SPELL_SCHOOL_MASK_HOLY);
    heal.ScaleValuesForTarget();
    EXPECT_EQ(heal.GetHeal(), 400u);
}

TEST_F(ScalingTests, Healing_Critical) { EXPECT_FLOAT_EQ(Scale(CreatePlayer(1, TestLowLevel), CreateCreature(100, TestHighLevel), 200.0f, SPELLTYPE_HEAL), 800.0f); }
TEST_F(ScalingTests, Healing_HoT) { ExpectPlayerToCreaturePath("periodic healing tick", SPELLTYPE_HEAL); }
TEST_F(ScalingTests, Healing_Proc) { ExpectPlayerToCreaturePath("triggered healing proc", SPELLTYPE_HEAL); }

TEST_F(ScalingTests, Healing_Leech)
{
    TestPlayer* player = CreatePlayer(1, TestLowLevel);
    TestCreature* creature = CreateCreature(100, TestHighLevel);
    float storedDamage = Scale(player, creature, 100.0f);
    EXPECT_FLOAT_EQ(sObjectMgr->ScaleDamageReverse(player, creature, storedDamage, SPELLTYPE_DAMAGE), 100.0f);
}

TEST_F(ScalingTests, Healing_Absorb)
{
    HealInfo heal(CreatePlayer(1, TestLowLevel), CreateCreature(100, TestHighLevel), 100, nullptr, SPELL_SCHOOL_MASK_HOLY);
    heal.ScaleValuesForTarget();
    heal.SetEffectiveHeal(400);
    heal.AbsorbHeal(125);
    EXPECT_EQ(heal.GetHeal(), 275u);
    EXPECT_EQ(heal.GetAbsorb(), 125u);
}

TEST_F(ScalingTests, Mitigation_Armor)
{
    EXPECT_LT(Unit::CalcArmorReducedDamage(CreatePlayer(1, TestLowLevel), CreateCreature(100, TestHighLevel), 1000, nullptr), 1000u);
}

TEST_F(ScalingTests, Mitigation_Resistance)
{
    DamageInfo damage(nullptr, nullptr, 400, nullptr, SPELL_SCHOOL_MASK_FIRE, SPELL_DIRECT_DAMAGE);
    damage.ResistDamage(100);
    EXPECT_EQ(damage.GetDamage(), 300u);
    EXPECT_EQ(damage.GetResist(), 100u);
}

TEST_F(ScalingTests, Mitigation_Block)
{
    DamageInfo damage(nullptr, nullptr, 400, nullptr, SPELL_SCHOOL_MASK_NORMAL, DIRECT_DAMAGE);
    damage.BlockDamage(75);
    EXPECT_EQ(damage.GetDamage(), 325u);
    EXPECT_EQ(damage.GetBlock(), 75u);
}

TEST_F(ScalingTests, Mitigation_Absorb)
{
    DamageInfo damage(nullptr, nullptr, 400, nullptr, SPELL_SCHOOL_MASK_FIRE, SPELL_DIRECT_DAMAGE);
    damage.AbsorbDamage(125);
    EXPECT_EQ(damage.GetDamage(), 275u);
    EXPECT_EQ(damage.GetAbsorb(), 125u);
}

TEST_F(ScalingTests, AbsorbShield_PartialDamageUsesRealAura)
{
    TestCreature* attacker = CreateCreature(100, TestHighLevel);
    TestPlayer* victim = CreatePlayer(1, TestLowLevel);
    std::unique_ptr<SpellInfo> shield = BuildAbsorbSpell(TestAbsorbSpellId, SPELL_AURA_SCHOOL_ABSORB, 20);
    Aura* aura = victim->AddAura(shield.get(), 1 << EFFECT_0, victim);
    ASSERT_NE(aura, nullptr);

    uint32 scaledDamage = ExpectScaledDamage(attacker, victim, 100, 25);
    DamageInfo result = ResolveAbsorb(attacker, victim, scaledDamage, shield.get());
    Report("partial absorb", scaledDamage, 20u, result.GetAbsorb());
    Report("damage after shield", scaledDamage, 5u, result.GetDamage());
    EXPECT_EQ(result.GetAbsorb(), 20u);
    EXPECT_EQ(result.GetDamage(), 5u);
    EXPECT_FALSE(victim->HasAura(TestAbsorbSpellId));
}

TEST_F(ScalingTests, AbsorbShield_FullDamageLeavesCapacity)
{
    TestCreature* attacker = CreateCreature(100, TestHighLevel);
    TestPlayer* victim = CreatePlayer(1, TestLowLevel);
    std::unique_ptr<SpellInfo> shield = BuildAbsorbSpell(TestAbsorbSpellId, SPELL_AURA_SCHOOL_ABSORB, 50);
    Aura* aura = victim->AddAura(shield.get(), 1 << EFFECT_0, victim);
    ASSERT_NE(aura, nullptr);

    uint32 scaledDamage = ExpectScaledDamage(attacker, victim, 100, 25);
    DamageInfo result = ResolveAbsorb(attacker, victim, scaledDamage, shield.get());
    Report("full absorb", scaledDamage, 25u, result.GetAbsorb());
    EXPECT_EQ(result.GetDamage(), 0u);
    EXPECT_EQ(result.GetAbsorb(), 25u);
    ASSERT_TRUE(victim->HasAura(TestAbsorbSpellId));
    EXPECT_EQ(aura->GetEffect(EFFECT_0)->GetAmount(), 25);
    victim->RemoveAurasDueToSpell(TestAbsorbSpellId);
}

TEST_F(ScalingTests, ManaShield_ConsumesManaAndPartiallyAbsorbs)
{
    TestCreature* attacker = CreateCreature(100, TestHighLevel);
    TestPlayer* victim = CreatePlayer(1, TestLowLevel);
    victim->SetMaxPower(POWER_MANA, 1000);
    victim->SetPower(POWER_MANA, 20);
    std::unique_ptr<SpellInfo> shield = BuildAbsorbSpell(TestManaShieldSpellId, SPELL_AURA_MANA_SHIELD, 50, 2.0f);
    ASSERT_NE(victim->AddAura(shield.get(), 1 << EFFECT_0, victim), nullptr);

    uint32 scaledDamage = ExpectScaledDamage(attacker, victim, 100, 25);
    DamageInfo result = ResolveAbsorb(attacker, victim, scaledDamage, shield.get());
    Report("mana shield absorbed", scaledDamage, 10u, result.GetAbsorb());
    Report("mana after shield", 20u, 0u, victim->GetPower(POWER_MANA));
    EXPECT_EQ(result.GetAbsorb(), 10u);
    EXPECT_EQ(result.GetDamage(), 15u);
    EXPECT_EQ(victim->GetPower(POWER_MANA), 0u);
    victim->RemoveAurasDueToSpell(TestManaShieldSpellId);
}

TEST_F(ScalingTests, AreaDamage_MultipleTargetsScaleIndependently)
{
    TestPlayer* caster = CreatePlayer(1, TestLowLevel);
    TestCreature* first = CreateCreature(100, TestHighLevel);
    TestCreature* second = CreateCreature(101, TestHighLevel);
    TestCreature* third = CreateCreature(102, TestHighLevel);
    std::array<TestCreature*, 3> targets = { first, second, third };

    for (TestCreature* target : targets)
    {
        uint32 scaledDamage = ExpectScaledDamage(caster, target, 100, 400);
        Report("AoE target damage", 100u, 400u, scaledDamage);
        EXPECT_EQ(scaledDamage, 400u);
    }
}

TEST_F(ScalingTests, AreaDot_AppliesAndTicksEveryTarget)
{
    TestPlayer* caster = CreatePlayer(1, TestLowLevel);
    TestCreature* first = CreateCreature(100, TestHighLevel);
    TestCreature* second = CreateCreature(101, TestHighLevel);
    std::unique_ptr<SpellInfo> areaDot = BuildAreaDotSpell(100);
    ASSERT_TRUE(areaDot->Effects[EFFECT_0].IsTargetingArea());

    for (TestCreature* target : { first, second })
    {
        uint32 expectedTickDamage = ExpectScaledDamage(caster, target, 100, 400);
        Aura* aura = caster->AddAura(areaDot.get(), 1 << EFFECT_0, target);
        ASSERT_NE(aura, nullptr);
        AuraEffect const* effect = aura->GetEffect(EFFECT_0);
        ASSERT_NE(effect, nullptr);
        uint32 healthBefore = target->GetHealth();
        effect->HandlePeriodicDamageAurasTick(target, caster);
        uint32 actualDamage = healthBefore - target->GetHealth();
        Report("AoE DoT target tick", 100u, expectedTickDamage, actualDamage);
        EXPECT_EQ(actualDamage, expectedTickDamage);
        target->RemoveAurasDueToSpell(TestAreaDotSpellId);
    }
}

TEST_F(ScalingTests, AreaDot_TargetAbsorbIsIndependent)
{
    TestPlayer* caster = CreatePlayer(1, TestLowLevel);
    TestCreature* shielded = CreateCreature(100, TestHighLevel);
    TestCreature* unshielded = CreateCreature(101, TestHighLevel);
    std::unique_ptr<SpellInfo> areaDot = BuildAreaDotSpell(100);
    std::unique_ptr<SpellInfo> shield = BuildAbsorbSpell(TestAbsorbSpellId, SPELL_AURA_SCHOOL_ABSORB, 150, 0.0f,
        SPELL_SCHOOL_MASK_SHADOW);
    ASSERT_NE(shielded->AddAura(shield.get(), 1 << EFFECT_0, shielded), nullptr);

    Aura* shieldedAura = caster->AddAura(areaDot.get(), 1 << EFFECT_0, shielded);
    Aura* unshieldedAura = caster->AddAura(areaDot.get(), 1 << EFFECT_0, unshielded);
    ASSERT_NE(shieldedAura, nullptr);
    ASSERT_NE(unshieldedAura, nullptr);
    uint32 shieldedScaledDamage = ExpectScaledDamage(caster, shielded, 100, 400);
    uint32 unshieldedScaledDamage = ExpectScaledDamage(caster, unshielded, 100, 400);
    uint32 shieldedBefore = shielded->GetHealth();
    uint32 unshieldedBefore = unshielded->GetHealth();
    shieldedAura->GetEffect(EFFECT_0)->HandlePeriodicDamageAurasTick(shielded, caster);
    unshieldedAura->GetEffect(EFFECT_0)->HandlePeriodicDamageAurasTick(unshielded, caster);
    uint32 shieldedDamage = shieldedBefore - shielded->GetHealth();
    uint32 unshieldedDamage = unshieldedBefore - unshielded->GetHealth();
    Report("shielded AoE DoT target", shieldedScaledDamage, 250u, shieldedDamage);
    Report("unshielded AoE DoT target", unshieldedScaledDamage, unshieldedScaledDamage, unshieldedDamage);
    EXPECT_EQ(shieldedDamage, shieldedScaledDamage - 150u);
    EXPECT_EQ(unshieldedDamage, unshieldedScaledDamage);
    shielded->RemoveAurasDueToSpell(TestAbsorbSpellId);
    shielded->RemoveAurasDueToSpell(TestAreaDotSpellId);
    unshielded->RemoveAurasDueToSpell(TestAreaDotSpellId);
}

TEST_F(ScalingTests, Mitigation_Modifiers)
{
    DamageInfo damage(nullptr, nullptr, 400, nullptr, SPELL_SCHOOL_MASK_FIRE, SPELL_DIRECT_DAMAGE);
    damage.ModifyDamage(-100);
    EXPECT_EQ(damage.GetDamage(), 300u);
    EXPECT_EQ(damage.GetUnmitigatedDamage(), 300u);
}

TEST_F(ScalingTests, PeriodicEffects_Snapshot)
{
    TestPlayer* player = CreatePlayer(1, TestLowLevel);
    TestCreature* creature = CreateCreature(100, TestHighLevel);
    float snapshot = Scale(player, creature, 100.0f);
    player->SetLevel(TestMidLevel);
    EXPECT_FLOAT_EQ(snapshot, 400.0f);
}

TEST_F(ScalingTests, PeriodicEffects_Dynamic)
{
    TestPlayer* player = CreatePlayer(1, TestLowLevel);
    TestCreature* creature = CreateCreature(100, TestHighLevel);
    EXPECT_FLOAT_EQ(Scale(player, creature, 100.0f), 400.0f);
    player->SetLevel(TestMidLevel);
    EXPECT_FLOAT_EQ(Scale(player, creature, 100.0f), 200.0f);
}

TEST_F(ScalingTests, PeriodicEffects_RescaleBeforeTick)
{
    TestPlayer* player = CreatePlayer(1, TestLowLevel);
    TestCreature* creature = CreateCreature(100, TestHighLevel);
    player->SetLevel(TestMidLevel);
    EXPECT_FLOAT_EQ(Scale(player, creature, 100.0f), 200.0f);
}

TEST_F(ScalingTests, PeriodicEffects_RescaleAfterTick)
{
    TestPlayer* player = CreatePlayer(1, TestLowLevel);
    TestCreature* creature = CreateCreature(100, TestHighLevel);
    float firstTick = Scale(player, creature, 100.0f);
    player->SetLevel(TestMidLevel);
    float secondTick = Scale(player, creature, 100.0f);
    EXPECT_FLOAT_EQ(firstTick, 400.0f);
    EXPECT_FLOAT_EQ(secondTick, 200.0f);
}

TEST_F(ScalingTests, Summons_Pets) { ExpectControlledUnitUsesOwnerLevel("pet", 120); }
TEST_F(ScalingTests, Summons_Guardians) { ExpectControlledUnitUsesOwnerLevel("guardian", 130); }
TEST_F(ScalingTests, Summons_Totems) { ExpectControlledUnitUsesOwnerLevel("totem", 140); }
TEST_F(ScalingTests, Summons_Vehicles) { ExpectControlledUnitUsesOwnerLevel("vehicle", 150); }

TEST(ScalingDamageInfoTests, AmalgamationIncludesBothDamageComponents)
{
    CalcDamageInfo damageInfo{};
    damageInfo.damages[0] = { SPELL_SCHOOL_MASK_NORMAL, 100, 10, 5 };
    damageInfo.damages[1] = { SPELL_SCHOOL_MASK_FIRE, 200, 20, 15 };
    damageInfo.cleanDamage = 30;
    damageInfo.attackType = BASE_ATTACK;
    damageInfo.TargetState = VICTIMSTATE_HIT;
    damageInfo.hitOutCome = MELEE_HIT_NORMAL;
    DamageInfo result(damageInfo);
    EXPECT_EQ(result.GetDamage(), 300u);
    EXPECT_EQ(result.GetAbsorb(), 30u);
    EXPECT_EQ(result.GetResist(), 20u);
    EXPECT_EQ(result.GetUnmitigatedDamage(), 410u);
}
} // namespace
