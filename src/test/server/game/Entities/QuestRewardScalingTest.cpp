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
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <optional>

namespace
{
class QuestRewardScalingTest : public IntegrationTestFixture
{
protected:
    static constexpr uint32 QuestId = 0xFFFFFFFE;
    static constexpr uint32 QuestZoneId = 65000;
    static constexpr uint32 QuestSourceZoneId = 65001;

    void SetUp() override
    {
        IntegrationTestFixture::SetUp();
        ON_CALL(*GetWorldMock(), getIntConfig(CONFIG_MAX_PLAYER_LEVEL)).WillByDefault(::testing::Return(21));
        if (ZoneFlex const* zoneFlex = sObjectMgr->GetZoneFlexForTest(QuestZoneId))
            _originalZoneFlex = *zoneFlex;
        if (ZoneFlex const* zoneFlex = sObjectMgr->GetZoneFlexForTest(QuestSourceZoneId))
            _originalSourceZoneFlex = *zoneFlex;
        sObjectMgr->SetZoneFlexForTest({ "Quest Zone", QuestZoneId, 0, 10, 20, 0 });
        sObjectMgr->SetZoneFlexForTest({ "Quest Source Zone", QuestSourceZoneId, 0, 12, 18, 0 });

        _player = CreateTestPlayer();
        _player->SetLevel(20);
        _player->SetUInt32Value(PLAYER_XP, 400);
        _player->SetUInt32Value(PLAYER_NEXT_LEVEL_XP, 1000);
    }

    void TearDown() override
    {
        sObjectMgr->RemoveQuestZoneFlexOverrideForTest(QuestId);
        if (_originalZoneFlex)
            sObjectMgr->SetZoneFlexForTest(*_originalZoneFlex);
        else
            sObjectMgr->RemoveZoneFlexForTest(QuestZoneId);
        if (_originalSourceZoneFlex)
            sObjectMgr->SetZoneFlexForTest(*_originalSourceZoneFlex);
        else
            sObjectMgr->RemoveZoneFlexForTest(QuestSourceZoneId);

        IntegrationTestFixture::TearDown();
    }

    TestPlayer* _player = nullptr;
    std::optional<ZoneFlex> _originalZoneFlex;
    std::optional<ZoneFlex> _originalSourceZoneFlex;
};

TEST_F(QuestRewardScalingTest, RewardLevelRemainsCurrentBelowLevelUpThreshold)
{
    EXPECT_EQ(_player->CalculateQuestRewardLevel(599), 20);
}

TEST_F(QuestRewardScalingTest, RewardLevelIncludesLevelGainedFromQuestXP)
{
    EXPECT_EQ(_player->CalculateQuestRewardLevel(600), 21);
}

TEST_F(QuestRewardScalingTest, OriginatingZoneClampsQuestLevel)
{
    EXPECT_EQ(sObjectMgr->GetQuestLevelScaled(QuestZoneId, 5), 10);
    EXPECT_EQ(sObjectMgr->GetQuestLevelScaled(QuestZoneId, 15), 15);
    EXPECT_EQ(sObjectMgr->GetQuestLevelScaled(QuestZoneId, 25), 20);
}

TEST_F(QuestRewardScalingTest, QuestWithoutScalableOriginKeepsPlayerLevel)
{
    EXPECT_EQ(sObjectMgr->GetQuestLevelScaled(-1, 25), 25);
    EXPECT_EQ(sObjectMgr->GetQuestLevelScaled(QuestZoneId + 1, 25), 25);

    sObjectMgr->SetZoneFlexForTest({ "Disabled Quest Zone", QuestZoneId, 0, 10, 20, AREA_FLAG_LOWLEVEL });
    EXPECT_EQ(sObjectMgr->GetQuestLevelScaled(QuestZoneId, 25), 25);
}

TEST_F(QuestRewardScalingTest, QuestSourceOverrideTakesPrecedenceOverZoneOrSort)
{
    sObjectMgr->SetQuestZoneFlexOverrideForTest(QuestId, QuestSourceZoneId);

    EXPECT_EQ(sObjectMgr->GetQuestZoneFlex(QuestId, QuestZoneId), sObjectMgr->GetZoneFlexForTest(QuestSourceZoneId));
    EXPECT_EQ(sObjectMgr->GetQuestLevelScaled(QuestId, QuestZoneId, 5), 12);
    EXPECT_EQ(sObjectMgr->GetQuestLevelScaled(QuestId, QuestZoneId, 25), 18);
}

TEST_F(QuestRewardScalingTest, QuestWithoutSourceOverrideFallsBackToZoneOrSort)
{
    EXPECT_EQ(sObjectMgr->GetQuestZoneFlex(QuestId, QuestZoneId), sObjectMgr->GetZoneFlexForTest(QuestZoneId));
    EXPECT_EQ(sObjectMgr->GetQuestLevelScaled(QuestId, QuestZoneId, 5), 10);
}

TEST_F(QuestRewardScalingTest, DisabledQuestSourceOverrideDoesNotFallBackToDestination)
{
    sObjectMgr->SetZoneFlexForTest({ "Disabled Quest Source Zone", QuestSourceZoneId, 0, 12, 18, AREA_FLAG_LOWLEVEL });
    sObjectMgr->SetQuestZoneFlexOverrideForTest(QuestId, QuestSourceZoneId);

    EXPECT_EQ(sObjectMgr->GetQuestZoneFlex(QuestId, QuestZoneId), nullptr);
    EXPECT_EQ(sObjectMgr->GetQuestLevelScaled(QuestId, QuestZoneId, 15), 15);
}
}
