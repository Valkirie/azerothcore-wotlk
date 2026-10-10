/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 */

#include "DBCStores.h"
#include "Group.h"
#include "IntegrationTestFixture.h"
#include "ItemEnchantmentMgr.h"
#include "LootMgr.h"
#include "ObjectMgr.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <array>
#include <iostream>
#include <optional>

namespace
{
constexpr uint8 LowLevel = 20;
constexpr uint8 HighLevel = 40;

constexpr uint32 PlainItemId = 100;
constexpr uint32 PropertyItemId = 200;
constexpr uint32 SuffixItemId = 300;

constexpr int32 BasePropertyTemplate = 8100;
constexpr int32 LowPropertyTemplate = 8120;
constexpr int32 HighPropertyTemplate = 8140;
constexpr uint32 BasePropertyId = 6100;
constexpr uint32 LowPropertyId = 6120;
constexpr uint32 HighPropertyId = 6140;
constexpr uint32 PropertyFamily = 77;

constexpr int32 BaseSuffixTemplate = 9100;
constexpr int32 LowSuffixTemplate = 9120;
constexpr int32 HighSuffixTemplate = 9140;
constexpr uint32 BaseSuffixId = 7100;
constexpr uint32 LowSuffixId = 7120;
constexpr uint32 HighSuffixId = 7140;
constexpr uint32 SuffixFamily = 88;

constexpr uint32 ScaledItemId(uint32 itemId, uint8 level)
{
	return MIN_ENTRY_SCALE + itemId * MAX_REQUIREDLEVEL + level;
}

class LootScalingTest : public IntegrationTestFixture
{
protected:
	void SetUp() override
	{
		IntegrationTestFixture::SetUp();
		ON_CALL(*GetWorldMock(), getIntConfig(CONFIG_MAX_PLAYER_LEVEL)).WillByDefault(::testing::Return(80));

		AddItemVariants(PlainItemId, 0, 0);
		AddItemVariants(PropertyItemId, BasePropertyTemplate, 0, LowPropertyTemplate, HighPropertyTemplate);
		AddItemVariants(SuffixItemId, 0, BaseSuffixTemplate, 0, 0, LowSuffixTemplate, HighSuffixTemplate);

		AddProperty(BasePropertyTemplate, BasePropertyId);
		AddProperty(LowPropertyTemplate, LowPropertyId);
		AddProperty(HighPropertyTemplate, HighPropertyId);
		AddSuffix(BaseSuffixTemplate, BaseSuffixId);
		AddSuffix(LowSuffixTemplate, LowSuffixId);
		AddSuffix(HighSuffixTemplate, HighSuffixId);

		AddRandomPropertyEntry(BasePropertyId);
		AddRandomPropertyEntry(LowPropertyId);
		AddRandomPropertyEntry(HighPropertyId);
		AddRandomSuffixEntry(BaseSuffixId);
		AddRandomSuffixEntry(LowSuffixId);
		AddRandomSuffixEntry(HighSuffixId);
		AddRandomPropertyPoints(10, 100);
		AddRandomPropertyPoints(LowLevel, 200);
		AddRandomPropertyPoints(HighLevel, 400);
	}

	void TearDown() override
	{
		for (uint32 itemId : _itemIds)
			sObjectMgr->RemoveItemTemplateForTest(itemId);

		RemoveRandomEnchantmentForTest(BasePropertyTemplate, BasePropertyId);
		RemoveRandomEnchantmentForTest(LowPropertyTemplate, LowPropertyId);
		RemoveRandomEnchantmentForTest(HighPropertyTemplate, HighPropertyId);
		RemoveRandomEnchantmentForTest(BaseSuffixTemplate, BaseSuffixId);
		RemoveRandomEnchantmentForTest(LowSuffixTemplate, LowSuffixId);
		RemoveRandomEnchantmentForTest(HighSuffixTemplate, HighSuffixId);

		for (uint32 id : { BasePropertyId, LowPropertyId, HighPropertyId })
			sItemRandomPropertiesStore.SetEntry(id, nullptr);
		for (uint32 id : { BaseSuffixId, LowSuffixId, HighSuffixId })
			sItemRandomSuffixStore.SetEntry(id, nullptr);
		for (uint32 level : { 10u, uint32(LowLevel), uint32(HighLevel) })
			sRandomPropertiesPointsStore.SetEntry(level, nullptr);

		IntegrationTestFixture::TearDown();
	}

	TestPlayer* CreatePlayer(ObjectGuid::LowType guid, uint8 level)
	{
		TestPlayer* player = CreateTestPlayer(guid);
		player->SetLevel(level);
		return player;
	}

	static LootItem CreateLootItem(uint32 itemId)
	{
		LootStoreItem storeItem(itemId, 0, 100.0f, false, 1, 0, 1, 1);
		return LootItem(storeItem);
	}

	static void ReportSolo(char const* itemType, uint8 playerLevel, uint32 baseItemId, LootItem const& baseItem, LootItem const& scaledItem)
	{
		std::cout << "\n[LOOT SCALING][SOLO] type=" << itemType
			<< " playerLevel=" << uint32(playerLevel)
			<< " baseItemId=" << baseItemId
			<< " scaledItemId=" << scaledItem.itemid
			<< " baseRandomPropertyId=" << baseItem.randomPropertyId
			<< " scaledRandomPropertyId=" << scaledItem.randomPropertyId
			<< " propertyFamily=" << scaledItem.randomPropertyFamily
			<< " suffixFamily=" << scaledItem.randomSuffixFamily
			<< " baseSuffixFactor=" << baseItem.randomSuffix
			<< " scaledSuffixFactor=" << scaledItem.randomSuffix << '\n';
	}

	static void ReportGroup(char const* itemType, TestPlayer const* player, uint32 baseItemId, LootItem const& lootItem,
		Roll::ItemInfo const& itemInfo, Roll::ItemInfo const& cachedItemInfo)
	{
		std::cout << "\n[LOOT SCALING][GROUP] type=" << itemType
			<< " playerGuid=" << player->GetGUID().ToString()
			<< " playerLevel=" << uint32(player->GetLevel())
			<< " baseItemId=" << baseItemId
			<< " scaledItemId=" << itemInfo.itemId
			<< " baseRandomPropertyId=" << lootItem.randomPropertyId
			<< " scaledRandomPropertyId=" << itemInfo.randomPropertyId
			<< " propertyFamily=" << lootItem.randomPropertyFamily
			<< " suffixFamily=" << lootItem.randomSuffixFamily
			<< " baseSuffixFactor=" << lootItem.randomSuffix
			<< " scaledSuffixFactor=" << itemInfo.randomSuffix
			<< " cacheStable=" << std::boolalpha << (&itemInfo == &cachedItemInfo) << '\n';
	}

private:
	void AddItemVariants(uint32 baseItemId, int32 baseProperty, int32 baseSuffix,
		int32 lowProperty = 0, int32 highProperty = 0, int32 lowSuffix = 0, int32 highSuffix = 0)
	{
		AddItem(baseItemId, 10, baseProperty, baseSuffix);
		AddItem(ScaledItemId(baseItemId, LowLevel), LowLevel, lowProperty, lowSuffix);
		AddItem(ScaledItemId(baseItemId, HighLevel), HighLevel, highProperty, highSuffix);
	}

	void AddItem(uint32 itemId, uint32 itemLevel, int32 randomProperty, int32 randomSuffix)
	{
		ItemTemplate item{};
		item.ItemId = itemId;
		item.Class = ITEM_CLASS_ARMOR;
		item.SubClass = ITEM_SUBCLASS_ARMOR_CLOTH;
		item.Quality = ITEM_QUALITY_UNCOMMON;
		item.InventoryType = INVTYPE_HEAD;
		item.ItemLevel = itemLevel;
		item.RequiredLevel = itemLevel;
		item.Stackable = 1;
		item.RandomProperty = randomProperty;
		item.RandomSuffix = randomSuffix;
		sObjectMgr->SetItemTemplateForTest(item);
		_itemIds.push_back(itemId);
	}

	static void AddProperty(int32 entry, uint32 enchantmentId)
	{
		AddRandomEnchantmentForTest(entry, enchantmentId, PropertyFamily, 0);
	}

	static void AddSuffix(int32 entry, uint32 enchantmentId)
	{
		AddRandomEnchantmentForTest(entry, enchantmentId, 0, SuffixFamily);
	}

	static void AddRandomPropertyEntry(uint32 id)
	{
		auto* entry = new ItemRandomPropertiesEntry{};
		entry->ID = id;
		sItemRandomPropertiesStore.SetEntry(id, entry);
	}

	static void AddRandomSuffixEntry(uint32 id)
	{
		auto* entry = new ItemRandomSuffixEntry{};
		entry->ID = id;
		sItemRandomSuffixStore.SetEntry(id, entry);
	}

	static void AddRandomPropertyPoints(uint32 itemLevel, uint32 points)
	{
		auto* entry = new RandomPropertiesPointsEntry{};
		entry->itemLevel = itemLevel;
		entry->UncommonPropertiesPoints[0] = points;
		sRandomPropertiesPointsStore.SetEntry(itemLevel, entry);
	}

	std::vector<uint32> _itemIds;
};

TEST_F(LootScalingTest, SoloPlainItemScalesToPlayerLevel)
{
	TestPlayer* player = CreatePlayer(1, LowLevel);
	LootItem item = CreateLootItem(PlainItemId);
	LootItem baseItem = item;

	item.ScaleForPlayer(LowLevel, player);
	ReportSolo("plain", LowLevel, PlainItemId, baseItem, item);

	EXPECT_EQ(item.itemid, ScaledItemId(PlainItemId, LowLevel));
	EXPECT_EQ(item.randomPropertyId, 0);
	EXPECT_EQ(item.randomSuffix, 0u);
}

TEST_F(LootScalingTest, QuestRewardScalesToLevelReachedByRewardXP)
{
	ON_CALL(*GetWorldMock(), getIntConfig(CONFIG_MAX_PLAYER_LEVEL)).WillByDefault(::testing::Return(HighLevel));
	TestPlayer* player = CreatePlayer(1, HighLevel - 1);
	player->SetUInt32Value(PLAYER_XP, 900);
	player->SetUInt32Value(PLAYER_NEXT_LEVEL_XP, 1000);

	uint8 rewardLevel = player->CalculateQuestRewardLevel(100);

	EXPECT_EQ(rewardLevel, HighLevel);
	EXPECT_EQ(LootStore::LoadScaledLoot(PlainItemId, player, rewardLevel), ScaledItemId(PlainItemId, HighLevel));
}

TEST_F(LootScalingTest, PlayerAwareScalingKeepsBaseItemInLowLevelArea)
{
	std::optional<ZoneFlex> originalLocation;
	if (ZoneFlex const* location = sObjectMgr->GetZoneFlexForTest(0))
		originalLocation = *location;

	sObjectMgr->SetZoneFlexForTest({ "Low Level", 0, 0, 1, 10, AREA_FLAG_LOWLEVEL });
	TestPlayer* player = CreatePlayer(1, 5);

	EXPECT_EQ(LootStore::LoadScaledLoot(PlainItemId, player), PlainItemId);

	if (originalLocation)
		sObjectMgr->SetZoneFlexForTest(*originalLocation);
	else
		sObjectMgr->RemoveZoneFlexForTest(0);
}

TEST_F(LootScalingTest, ForcedLevelIsClampedToAreaBounds)
{
	std::optional<ZoneFlex> originalLocation;
	if (ZoneFlex const* location = sObjectMgr->GetZoneFlexForTest(0))
		originalLocation = *location;

	sObjectMgr->SetZoneFlexForTest({ "Scalable", 0, 0, 10, LowLevel, 0 });
	TestPlayer* player = CreatePlayer(1, LowLevel);

	EXPECT_EQ(LootStore::LoadScaledLoot(PlainItemId, player, LowLevel), ScaledItemId(PlainItemId, LowLevel));
    EXPECT_EQ(LootStore::LoadScaledLoot(PlainItemId, player, HighLevel), ScaledItemId(PlainItemId, LowLevel));

	if (originalLocation)
		sObjectMgr->SetZoneFlexForTest(*originalLocation);
	else
		sObjectMgr->RemoveZoneFlexForTest(0);
}

TEST_F(LootScalingTest, ExplicitLevelBypassesAreaBounds)
{
	std::optional<ZoneFlex> originalLocation;
	if (ZoneFlex const* location = sObjectMgr->GetZoneFlexForTest(0))
		originalLocation = *location;

	sObjectMgr->SetZoneFlexForTest({ "Scalable", 0, 0, 10, LowLevel, 0 });
	TestPlayer* player = CreatePlayer(1, HighLevel);

	EXPECT_EQ(LootStore::LoadScaledLootAtLevel(PlainItemId, player, HighLevel), ScaledItemId(PlainItemId, HighLevel));

	if (originalLocation)
		sObjectMgr->SetZoneFlexForTest(*originalLocation);
	else
		sObjectMgr->RemoveZoneFlexForTest(0);
}

TEST_F(LootScalingTest, SoloRandomPropertyScalesWithinGeneratedFamily)
{
	TestPlayer* player = CreatePlayer(1, LowLevel);
	LootItem item = CreateLootItem(PropertyItemId);
	LootItem baseItem = item;
	ASSERT_EQ(item.randomPropertyId, int32(BasePropertyId));
	ASSERT_EQ(item.randomPropertyFamily, PropertyFamily);

	item.ScaleForPlayer(LowLevel, player);
	ReportSolo("random-property", LowLevel, PropertyItemId, baseItem, item);

	EXPECT_EQ(item.itemid, ScaledItemId(PropertyItemId, LowLevel));
	EXPECT_EQ(item.randomPropertyId, int32(LowPropertyId));
	EXPECT_EQ(item.randomPropertyFamily, PropertyFamily);
	EXPECT_EQ(item.randomSuffixFamily, 0u);
}

TEST_F(LootScalingTest, SoloRandomSuffixScalesWithinGeneratedFamily)
{
	TestPlayer* player = CreatePlayer(1, LowLevel);
	LootItem item = CreateLootItem(SuffixItemId);
	LootItem baseItem = item;
	ASSERT_EQ(item.randomPropertyId, -int32(BaseSuffixId));
	ASSERT_EQ(item.randomSuffixFamily, SuffixFamily);

	item.ScaleForPlayer(LowLevel, player);
	ReportSolo("random-suffix", LowLevel, SuffixItemId, baseItem, item);

	EXPECT_EQ(item.itemid, ScaledItemId(SuffixItemId, LowLevel));
	EXPECT_EQ(item.randomPropertyId, -int32(LowSuffixId));
	EXPECT_EQ(item.randomSuffix, 200u);
	EXPECT_EQ(item.randomPropertyFamily, 0u);
	EXPECT_EQ(item.randomSuffixFamily, SuffixFamily);
}

TEST_F(LootScalingTest, GroupPlainItemResolvesAndCachesEachPlayersLevel)
{
	TestPlayer* lowPlayer = CreatePlayer(1, LowLevel);
	TestPlayer* highPlayer = CreatePlayer(2, HighLevel);
	Loot loot;
	loot.items.push_back(CreateLootItem(PlainItemId));
	Roll roll(ObjectGuid::Create<HighGuid::Item>(1), loot.items.front());
	roll.setLoot(&loot);

	Roll::ItemInfo const& lowInfo = roll.GetItemInfoForPlayer(lowPlayer);
	Roll::ItemInfo const& highInfo = roll.GetItemInfoForPlayer(highPlayer);
	ReportGroup("plain", lowPlayer, PlainItemId, loot.items.front(), lowInfo, roll.GetItemInfoForPlayer(lowPlayer));
	ReportGroup("plain", highPlayer, PlainItemId, loot.items.front(), highInfo, roll.GetItemInfoForPlayer(highPlayer));

	EXPECT_EQ(lowInfo.itemId, ScaledItemId(PlainItemId, LowLevel));
	EXPECT_EQ(highInfo.itemId, ScaledItemId(PlainItemId, HighLevel));
	EXPECT_EQ(lowInfo.randomPropertyId, 0);
	EXPECT_EQ(highInfo.randomPropertyId, 0);
	EXPECT_EQ(&roll.GetItemInfoForPlayer(lowPlayer), &lowInfo);
	EXPECT_EQ(&roll.GetItemInfoForPlayer(highPlayer), &highInfo);
}

TEST_F(LootScalingTest, GroupRandomPropertyVariantsShareFamilyAndRemainStable)
{
	TestPlayer* lowPlayer = CreatePlayer(1, LowLevel);
	TestPlayer* highPlayer = CreatePlayer(2, HighLevel);
	Loot loot;
	loot.items.push_back(CreateLootItem(PropertyItemId));
	ASSERT_EQ(loot.items.front().randomPropertyFamily, PropertyFamily);
	Roll roll(ObjectGuid::Create<HighGuid::Item>(1), loot.items.front());
	roll.setLoot(&loot);

	Roll::ItemInfo const& lowInfo = roll.GetItemInfoForPlayer(lowPlayer);
	Roll::ItemInfo const& highInfo = roll.GetItemInfoForPlayer(highPlayer);
	ReportGroup("random-property", lowPlayer, PropertyItemId, loot.items.front(), lowInfo, roll.GetItemInfoForPlayer(lowPlayer));
	ReportGroup("random-property", highPlayer, PropertyItemId, loot.items.front(), highInfo, roll.GetItemInfoForPlayer(highPlayer));

	EXPECT_EQ(lowInfo.itemId, ScaledItemId(PropertyItemId, LowLevel));
	EXPECT_EQ(highInfo.itemId, ScaledItemId(PropertyItemId, HighLevel));
	EXPECT_EQ(lowInfo.randomPropertyId, int32(LowPropertyId));
	EXPECT_EQ(highInfo.randomPropertyId, int32(HighPropertyId));
	EXPECT_EQ(loot.items.front().randomPropertyFamily, PropertyFamily);
	EXPECT_EQ(roll.GetItemInfoForPlayer(lowPlayer).randomPropertyId, lowInfo.randomPropertyId);
	EXPECT_EQ(roll.GetItemInfoForPlayer(highPlayer).randomPropertyId, highInfo.randomPropertyId);
}

TEST_F(LootScalingTest, GroupRandomSuffixVariantsShareFamilyAndRemainStable)
{
	TestPlayer* lowPlayer = CreatePlayer(1, LowLevel);
	TestPlayer* highPlayer = CreatePlayer(2, HighLevel);
	Loot loot;
	loot.items.push_back(CreateLootItem(SuffixItemId));
	ASSERT_EQ(loot.items.front().randomSuffixFamily, SuffixFamily);
	Roll roll(ObjectGuid::Create<HighGuid::Item>(1), loot.items.front());
	roll.setLoot(&loot);

	Roll::ItemInfo const& lowInfo = roll.GetItemInfoForPlayer(lowPlayer);
	Roll::ItemInfo const& highInfo = roll.GetItemInfoForPlayer(highPlayer);
	ReportGroup("random-suffix", lowPlayer, SuffixItemId, loot.items.front(), lowInfo, roll.GetItemInfoForPlayer(lowPlayer));
	ReportGroup("random-suffix", highPlayer, SuffixItemId, loot.items.front(), highInfo, roll.GetItemInfoForPlayer(highPlayer));

	EXPECT_EQ(lowInfo.itemId, ScaledItemId(SuffixItemId, LowLevel));
	EXPECT_EQ(highInfo.itemId, ScaledItemId(SuffixItemId, HighLevel));
	EXPECT_EQ(lowInfo.randomPropertyId, -int32(LowSuffixId));
	EXPECT_EQ(highInfo.randomPropertyId, -int32(HighSuffixId));
	EXPECT_EQ(lowInfo.randomSuffix, 200u);
	EXPECT_EQ(highInfo.randomSuffix, 400u);
	EXPECT_EQ(loot.items.front().randomSuffixFamily, SuffixFamily);
	EXPECT_EQ(roll.GetItemInfoForPlayer(lowPlayer).randomPropertyId, lowInfo.randomPropertyId);
	EXPECT_EQ(roll.GetItemInfoForPlayer(highPlayer).randomPropertyId, highInfo.randomPropertyId);
}
}
