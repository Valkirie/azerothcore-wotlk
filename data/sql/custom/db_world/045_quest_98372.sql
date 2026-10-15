--
-- Night Watch content from WoW Forever
-- Quest 98372: An Unfortunate End
--

DELETE FROM `quest_template` WHERE `ID` = 98372;
INSERT INTO `quest_template` (`ID`, `QuestType`, `QuestLevel`, `MinLevel`, `QuestSortID`, `AllowableRaces`, `Flags`, `QuestInfoID`, `RewardItem1`, `RewardAmount1`, `LogTitle`, `QuestDescription`, `QuestCompletionLog`, `VerifiedBuild`)
VALUES (98372, 2, 60, 1, 10, 0, 0, 0, 69999, 1, 'An Unfortunate End', 'The Night Watchman lying before you is covered in deep scratches and wide slashes. It is obvious that he was mauled by a large beast.$B$BYou notice that he still clutches an extinguished torch in his hand. You lean over and pry it from his stiff fingers. It may prove useful during your travels in Duskwood and beyond, to reveal what the darkness conceals.', 'You take the extinguished torch from the Night Watchman.', NULL);

DELETE FROM `quest_template_locale` WHERE `ID` = 98372 AND `locale` = 'frFR';
INSERT INTO `quest_template_locale` (`ID`, `locale`, `Title`, `Details`, `CompletedText`, `VerifiedBuild`)
VALUES (98372, 'frFR', 'Une triste fin', 'Le guetteur de nuit qui gît devant vous est couvert de profondes griffures et de larges entailles. Il est évident qu’il a été mutilé par une bête de grande taille.$B$BVous remarquez qu’il serre encore une torche éteinte dans sa main. Vous vous penchez et l’arrachez à ses doigts raidis. Elle pourrait vous être utile au cours de vos voyages dans le bois de la Pénombre et au-delà, pour révéler ce que les ténèbres dissimulent.', 'Vous prenez la torche éteinte au guetteur de nuit.', NULL);

DELETE FROM `creature_queststarter` WHERE `id` = 271866 AND `quest` = 98372;
INSERT INTO `creature_queststarter` (`id`, `quest`) VALUES (271866, 98372);

DELETE FROM `creature_questender` WHERE `id` = 271866 AND `quest` = 98372;
INSERT INTO `creature_questender` (`id`, `quest`) VALUES (271866, 98372);

DELETE FROM `item_template` WHERE `entry` = 69999;
DROP TEMPORARY TABLE IF EXISTS `ac_custom_item_template`;
CREATE TEMPORARY TABLE `ac_custom_item_template` LIKE `item_template`;
DELETE FROM `ac_custom_item_template`;
INSERT INTO `ac_custom_item_template` SELECT * FROM `item_template` WHERE `entry` = 6654;
UPDATE `ac_custom_item_template` SET `entry` = 69999, `name` = 'Night Watchman''s Torch', `Quality` = 2, `maxcount` = 1, `spellid_1` = 0, `spelltrigger_1` = 0, `spellcharges_1` = 0, `spellppmRate_1` = 0, `spellcooldown_1` = -1, `spellcategory_1` = 0, `spellcategorycooldown_1` = -1, `spellid_2` = 0, `spelltrigger_2` = 0, `spellcharges_2` = 0, `spellppmRate_2` = 0, `spellcooldown_2` = -1, `spellcategory_2` = 0, `spellcategorycooldown_2` = -1, `spellid_3` = 0, `spelltrigger_3` = 0, `spellcharges_3` = 0, `spellppmRate_3` = 0, `spellcooldown_3` = -1, `spellcategory_3` = 0, `spellcategorycooldown_3` = -1, `spellid_4` = 0, `spelltrigger_4` = 0, `spellcharges_4` = 0, `spellppmRate_4` = 0, `spellcooldown_4` = -1, `spellcategory_4` = 0, `spellcategorycooldown_4` = -1, `spellid_5` = 0, `spelltrigger_5` = 0, `spellcharges_5` = 0, `spellppmRate_5` = 0, `spellcooldown_5` = -1, `spellcategory_5` = 0, `spellcategorycooldown_5` = -1;
DELETE FROM `item_template` WHERE `entry` = 69999;
INSERT INTO `item_template` SELECT * FROM `ac_custom_item_template`;
DROP TEMPORARY TABLE `ac_custom_item_template`;

DELETE FROM `creature_template` WHERE `entry` = 271866;
DROP TEMPORARY TABLE IF EXISTS `ac_custom_creature_template`;
CREATE TEMPORARY TABLE `ac_custom_creature_template` LIKE `creature_template`;
DELETE FROM `ac_custom_creature_template`;
INSERT INTO `ac_custom_creature_template` SELECT * FROM `creature_template` WHERE `entry` = 10038;
UPDATE `ac_custom_creature_template` SET `entry` = 271866, `name` = 'Night Watch Guard', `subname` = 'The Night Watch', `minlevel` = 25, `maxlevel` = 25, `faction` = 35, `npcflag` = 2, `unit_flags` = 131074, `dynamicflags` = 1, `MovementType` = 0;
DELETE FROM `creature_template` WHERE `entry` = 271866;
INSERT INTO `creature_template` SELECT * FROM `ac_custom_creature_template`;
DROP TEMPORARY TABLE `ac_custom_creature_template`;

DELETE FROM `creature_template_locale` WHERE `entry` = 271866 AND `locale` = 'frFR';
INSERT INTO `creature_template_locale` (`entry`, `locale`, `Name`, `Title`, `VerifiedBuild`)
VALUES (271866, 'frFR', 'Garde des Veilleurs', 'Les Veilleurs', NULL);

DELETE FROM `creature_template_addon` WHERE `entry` = 271866;
INSERT INTO `creature_template_addon` (`entry`, `path_id`, `mount`, `bytes1`, `bytes2`, `emote`, `visibilityDistanceType`, `auras`)
VALUES (271866, 0, 0, 7, 1, 0, 0, '');

DELETE FROM `creature_template_model` WHERE `CreatureID` = 271866;
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`)
SELECT 271866, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`
FROM `creature_template_model`
WHERE `CreatureID` = 10038;

DELETE FROM `creature_equip_template` WHERE `CreatureID` = 271866 AND `ID` = 1;
INSERT INTO `creature_equip_template` (`CreatureID`, `ID`, `ItemID1`, `ItemID2`, `ItemID3`, `VerifiedBuild`)
VALUES (271866, 1, 6654, 0, 0, NULL);

DELETE FROM `item_template_locale` WHERE `ID` = 69999 AND `locale` = 'frFR';
INSERT INTO `item_template_locale` (`ID`, `locale`, `Name`, `Description`, `VerifiedBuild`)
VALUES (69999, 'frFR', 'Torche du guetteur de nuit', '', NULL);

DELETE FROM `creature` WHERE `id` = 271866 AND `guid` IN (9000010, 9000011, 9000012, 9000013, 9000014, 9000015);
INSERT INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `equipment_id`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`, `Comment`)
VALUES
(9000010, 271866, 0, 10, 0, 1, 1, 1, -10695.866, 368.933, 38.02967, 0, 900, 0, 2, 131074, 1, 'Night Watch quest corpse - Wowhead ZoneX/ZoneY 17.2, 54.4'),
(9000011, 271866, 0, 10, 0, 1, 1, 1, -10314.266, 196.133, 34.840576, 0, 900, 0, 2, 131074, 1, 'Night Watch quest corpse - Wowhead ZoneX/ZoneY 23.6, 33.2'),
(9000012, 271866, 0, 10, 0, 1, 1, 1, -10789.466, -451.867, 65.02236, 0, 900, 0, 2, 131074, 1, 'Night Watch quest corpse - Wowhead ZoneX/ZoneY 47.6, 59.6'),
(9000013, 271866, 0, 10, 0, 1, 1, 1, -11138.666, -397.867, 46.21033, 0, 900, 0, 2, 131074, 1, 'Night Watch quest corpse - Wowhead ZoneX/ZoneY 45.6, 79.0'),
(9000014, 271866, 0, 10, 0, 1, 1, 1, -10969.466, -1024.267, 73.351006, 0, 900, 0, 2, 131074, 1, 'Night Watch quest corpse - Wowhead ZoneX/ZoneY 68.8, 69.6'),
(9000015, 271866, 0, 10, 0, 1, 1, 1, -10325.066, -948.667, 42.133858, 0, 900, 0, 2, 131074, 1, 'Night Watch quest corpse - Wowhead ZoneX/ZoneY 66.0, 33.8');
