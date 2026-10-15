-- Rochenoire database setup.
--
-- Creates the custom world-database tables used by Rochenoire's creature,
-- zone, raid, loot, vendor, enchantment, and smart-loot systems. This file is
-- intended to run before the scripts that populate or recalculate these tables.

-- Raid
/*
DROP TABLE IF EXISTS `rochenoire_scale_raid_creature_template`;
DROP TABLE IF EXISTS `rochenoire_scale_raid_creature_pool`;
DROP TABLE IF EXISTS `rochenoire_scale_instance_creature`;
DROP TABLE IF EXISTS `rochenoire_scale_instance_spawn_group`;

-- Creatures
DROP TABLE IF EXISTS `rochenoire_scale_level_creature_template`;
DROP TABLE IF EXISTS `rochenoire_scale_level_creature`;

-- Loot
DROP TABLE IF EXISTS `rochenoire_scale_consumable_loot`;
DROP TABLE IF EXISTS `rochenoire_items_not_scaled_from_vendors`;
DROP TABLE IF EXISTS `rochenoire_scale_zone`;
DROP TABLE IF EXISTS `rochenoire_scale_quest`;
DROP TABLE IF EXISTS `rochenoire_enchantment_family`;
DROP TABLE IF EXISTS `rochenoire_smart_loot_data`;
*/

CREATE TABLE IF NOT EXISTS `rochenoire_scale_instance_creature` (
  `creatureEntry` MEDIUMINT UNSIGNED NOT NULL,
  `mapId` SMALLINT UNSIGNED NOT NULL,
  `requiredTankCountOverride` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `raidDamageWeight` FLOAT UNSIGNED NOT NULL DEFAULT '0',
  `minimumCreatureDamageMultiplier` FLOAT UNSIGNED NOT NULL DEFAULT '0.8',
  `minimumRetainedCreatureHealthMultiplier` FLOAT UNSIGNED NOT NULL DEFAULT '0.7',
  `comment` VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`creatureEntry`,`mapId`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `rochenoire_scale_instance_spawn_group` (
  `spawnId` INT UNSIGNED NOT NULL,
  `mapId` SMALLINT UNSIGNED NOT NULL,
  `scalingGroupId` INT UNSIGNED NOT NULL,
  `activationOrder` SMALLINT UNSIGNED NOT NULL DEFAULT '0',
  `comment` VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`spawnId`),
  KEY `idx_map_group_order` (`mapId`,`scalingGroupId`,`activationOrder`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `rochenoire_scale_level_creature_template` (
  `entry` MEDIUMINT(8) UNSIGNED NOT NULL COMMENT 'creature entry',
  `flvar` SMALLINT SIGNED NOT NULL DEFAULT '0' COMMENT 'force level variation',
  `comment` VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`entry`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `rochenoire_scale_level_creature` (
  `guid` MEDIUMINT(8) UNSIGNED NOT NULL COMMENT 'creature guid',
  `flvar` SMALLINT SIGNED NOT NULL DEFAULT '0' COMMENT 'force level variation',
  `comment` VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`guid`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `rochenoire_scale_consumable_loot` (
  `ItemId` MEDIUMINT(8) UNSIGNED NOT NULL,
  `RitemId` MEDIUMINT(8) UNSIGNED NOT NULL,
  `Plevel` MEDIUMINT(8) UNSIGNED NOT NULL,
  PRIMARY KEY (`ItemId`,`RitemId`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `rochenoire_items_not_scaled_from_vendors` (
  `ItemId` MEDIUMINT(8) UNSIGNED NOT NULL,
  `comment` VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`ItemId`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `rochenoire_scale_zone` (
  `areaName` VARCHAR(255) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL,
  `mapId` INT UNSIGNED NOT NULL,
  `areaId` INT UNSIGNED NOT NULL,
  `LevelRangeMin` INT DEFAULT NULL,
  `LevelRangeMax` INT DEFAULT NULL,
  `areaFlags` INT UNSIGNED NOT NULL DEFAULT '0',
  `minimumPlayers` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `maximumPlayers` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  `requiredTankCount` TINYINT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`areaId`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `rochenoire_scale_quest` (
  `QuestId` INT UNSIGNED NOT NULL,
  `SourceAreaId` INT UNSIGNED NOT NULL,
  `comment` VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`QuestId`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `rochenoire_enchantment_family` (
  `ench` INT UNSIGNED NOT NULL,
  `propertyFamily` INT UNSIGNED NOT NULL,
  `suffixFamily` INT UNSIGNED NOT NULL,
  PRIMARY KEY (`ench`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `rochenoire_smart_loot_data` (
  `table` VARCHAR(255) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL,
  `Reference` INT UNSIGNED NOT NULL,
  `LootInfo` MEDIUMINT(8) SIGNED NOT NULL DEFAULT (-1),
  PRIMARY KEY (`table`, `Reference`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;
