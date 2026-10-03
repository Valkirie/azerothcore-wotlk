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
#include "Unit.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <iostream>
#include <string_view>

namespace
{
constexpr uint8 TestLowLevel = 20;
constexpr uint8 TestMidLevel = 30;
constexpr uint8 TestHighLevel = 40;
constexpr uint32 TestNativeValue = 400;

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
};

TEST_F(ScalingTests, CreatureScaling_Level)
{
    TestPlayer* player = CreatePlayer(1, TestLowLevel);
    TestCreature* creature = CreateCreature(100, TestHighLevel);
    EXPECT_EQ(creature->getLevelForTarget(player), TestLowLevel);
    EXPECT_EQ(sObjectMgr->GetLevelScaled(player, creature), TestLowLevel);
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
