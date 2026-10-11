CREATE TABLE IF NOT EXISTS `rochenoire_scale_quest` (
  `QuestId` INT UNSIGNED NOT NULL,
  `SourceAreaId` INT UNSIGNED NOT NULL,
  `comment` VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`QuestId`)
) ENGINE=INNODB DEFAULT CHARSET=utf8;

REPLACE INTO `rochenoire_scale_quest` (`QuestId`, `SourceAreaId`, `comment`) VALUES
(373, 1581, 'The Unsent Letter - The Deadmines');

UPDATE `item_template`
SET `RequiredLevel` = 10
WHERE `entry` = 2874;
