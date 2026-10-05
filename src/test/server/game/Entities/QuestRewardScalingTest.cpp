/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "IntegrationTestFixture.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace
{
class QuestRewardScalingTest : public IntegrationTestFixture
{
protected:
	void SetUp() override
	{
		IntegrationTestFixture::SetUp();
		ON_CALL(*GetWorldMock(), getIntConfig(CONFIG_MAX_PLAYER_LEVEL)).WillByDefault(::testing::Return(21));

		_player = CreateTestPlayer();
		_player->SetLevel(20);
		_player->SetUInt32Value(PLAYER_XP, 400);
		_player->SetUInt32Value(PLAYER_NEXT_LEVEL_XP, 1000);
	}

	TestPlayer* _player = nullptr;
};

TEST_F(QuestRewardScalingTest, RewardLevelRemainsCurrentBelowLevelUpThreshold)
{
	EXPECT_EQ(_player->CalculateQuestRewardLevel(599), 20);
}

TEST_F(QuestRewardScalingTest, RewardLevelIncludesLevelGainedFromQuestXP)
{
	EXPECT_EQ(_player->CalculateQuestRewardLevel(600), 21);
}
}
