-- Rochenoire database setup.
--
-- Creates the custom world-database tables used by Rochenoire's creature,
-- zone, raid, loot, vendor, enchantment, and smart-loot systems. This file is
-- intended to run before the scripts that populate or recalculate these tables.

-- Raid
DROP TABLE IF EXISTS `rochenoire_scale_raid_creature_template`;
DROP TABLE IF EXISTS `rochenoire_scale_raid_creature_pool`;

-- Creatures
DROP TABLE IF EXISTS `rochenoire_scale_level_creature_template`;
DROP TABLE IF EXISTS `rochenoire_scale_level_creature`;

-- Loot
DROP TABLE IF EXISTS `rochenoire_scale_consumable_loot`;
DROP TABLE IF EXISTS `rochenoire_items_not_scaled_from_vendors`;
DROP TABLE IF EXISTS `rochenoire_scale_zone`;
DROP TABLE IF EXISTS `rochenoire_enchantment_family`;
DROP TABLE IF EXISTS `rochenoire_smart_loot_data`;

CREATE TABLE `rochenoire_scale_raid_creature_template` (
  `entry` MEDIUMINT(8) UNSIGNED NOT NULL COMMENT 'creature entry',
  `m_entry` MEDIUMINT(8) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'map entry',
  `nb_tank` SMALLINT(1) UNSIGNED NOT NULL DEFAULT '2' COMMENT 'nbr tank',
  `nb_pack` SMALLINT(2) UNSIGNED NOT NULL DEFAULT '1' COMMENT 'pack size',
  `ratio_hrht` FLOAT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'ratio HR_HT',
  `ratio_c1` FLOAT UNSIGNED NOT NULL DEFAULT '0.85' COMMENT 'ratio Boss C1',
  `ratio_c2` FLOAT UNSIGNED NOT NULL DEFAULT '0.85' COMMENT 'ratio Boss C2',
  `comment` VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`entry`,`m_entry`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

CREATE TABLE `rochenoire_scale_raid_creature_pool` (
  `guid` INT(10) UNSIGNED NOT NULL,
  `pool_id` INT(10) UNSIGNED DEFAULT NULL,
  `comment` VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`guid`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

CREATE TABLE `rochenoire_scale_level_creature_template` (
  `entry` MEDIUMINT(8) UNSIGNED NOT NULL COMMENT 'creature entry',
  `flvar` SMALLINT SIGNED NOT NULL DEFAULT '0' COMMENT 'force level variation',
  `comment` VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`entry`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

CREATE TABLE `rochenoire_scale_level_creature` (
  `guid` MEDIUMINT(8) UNSIGNED NOT NULL COMMENT 'creature guid',
  `flvar` SMALLINT SIGNED NOT NULL DEFAULT '0' COMMENT 'force level variation',
  `comment` VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`guid`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

CREATE TABLE `rochenoire_scale_consumable_loot` (
  `ItemId` MEDIUMINT(8) UNSIGNED NOT NULL,
  `RitemId` MEDIUMINT(8) UNSIGNED NOT NULL,
  `Plevel` MEDIUMINT(8) UNSIGNED NOT NULL,
  PRIMARY KEY (`ItemId`,`RitemId`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

CREATE TABLE `rochenoire_items_not_scaled_from_vendors` (
  `ItemId` MEDIUMINT(8) UNSIGNED NOT NULL,
  `comment` VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`ItemId`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

CREATE TABLE `rochenoire_scale_zone` (
  `areaName` VARCHAR(255) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL,
  `mapId` INT UNSIGNED NOT NULL,
  `areaId` INT UNSIGNED NOT NULL,
  `LevelRangeMin` INT DEFAULT NULL,
  `LevelRangeMax` INT DEFAULT NULL,
  `areaFlags` INT UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`areaId`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

CREATE TABLE `rochenoire_enchantment_family` (
  `ench` INT UNSIGNED NOT NULL,
  `propertyFamily` INT UNSIGNED NOT NULL,
  `suffixFamily` INT UNSIGNED NOT NULL,
  PRIMARY KEY (`ench`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

CREATE TABLE `rochenoire_smart_loot_data` (
  `table` VARCHAR(255) CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL,
  `Reference` INT UNSIGNED NOT NULL,
  `LootInfo` MEDIUMINT(8) SIGNED NOT NULL DEFAULT (-1),
  PRIMARY KEY (`table`, `Reference`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;
