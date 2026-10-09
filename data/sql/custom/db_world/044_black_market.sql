-- Black Market item rescaling representatives

DELETE FROM `creature` WHERE `id` = 500000;
DELETE FROM `creature_template_model` WHERE `CreatureID` = 500000;
DELETE FROM `creature_template` WHERE `entry` = 500000;

INSERT INTO `creature_template` (`entry`, `name`, `subname`, `minlevel`, `maxlevel`, `faction`, `npcflag`, `unit_flags`, `unit_class`, `type`, `ScriptName`, `VerifiedBuild`) VALUES
(500000, 'Black Market Representative', 'Item Rescaling', 80, 80, 35, 1, 2, 1, 7, 'npc_black_market', 0);

INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`) VALUES
(500000, 0, 7355, 1, 1, 0);

INSERT INTO `creature` (`guid`, `id`, `map`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `curhealth`, `MovementType`, `VerifiedBuild`, `Comment`) VALUES
(9000000, 500000, 1,   1, 1,   6721.65, -4663.43, 721.004, 2.40214, 300, 1, 0, 0, 'Black Market Representative - Everlook'),
(9000001, 500000, 1,   1, 1,  -7188.17, -3796.79,   9.453, 1.08760, 300, 1, 0, 0, 'Black Market Representative - Gadgetzan'),
(9000002, 500000, 0,   1, 1, -14374.50,   397.87,   6.627, 1.46358, 300, 1, 0, 0, 'Black Market Representative - Booty Bay'),
(9000003, 500000, 1,   1, 1,  -1058.38, -3666.63,  23.918, 2.86234, 300, 1, 0, 0, 'Black Market Representative - Ratchet'),
(9000004, 500000, 530, 1, 1,  -1894.00,  5155.00, -40.232, 0.80000, 300, 1, 0, 0, 'Black Market Representative - Shattrath City'),
(9000005, 500000, 571, 1, 1,   5793.00,   558.44, 650.719, 0.00000, 300, 1, 0, 0, 'Black Market Representative - Dalaran');

DELETE FROM `npc_text` WHERE `ID` IN (600000, 600001, 600002);
INSERT INTO `npc_text` (`ID`, `text0_0`, `Probability0`) VALUES
(600000, 'Looking for an upgrade, $N? Pick a piece of carried equipment and I will show you what I can find.', 1),
(600001, 'Choose the level you want. Better merchandise costs more Contraband Marks.', 1),
(600002, 'Here\'s what I can do for ya. It might take some time for me to find it based on its rarity.$B$BWhatcha think? Do we have a deal?', 1);

DELETE FROM `npc_text_locale` WHERE `ID` IN (600000, 600001, 600002) AND `Locale` = 'frFR';
INSERT INTO `npc_text_locale` (`ID`, `Locale`, `Text0_0`) VALUES
(600000, 'frFR', 'Vous cherchez une amélioration, $N ? Choisissez une pièce d\'équipement dans votre inventaire et je vous montrerai ce que je peux trouver.'),
(600001, 'frFR', 'Choisissez le niveau souhaité. Les meilleures marchandises coûtent davantage de Marques de contrebande.'),
(600002, 'frFR', 'Voici ce que je peux faire pour vous. Cela peut me prendre du temps de le trouver selon sa rareté.$B$BQu\'en pensez-vous ? Marché conclu ?');

DELETE FROM `acore_string` WHERE `entry` IN (11039, 11040, 11041, 11042, 11043, 11044, 11045, 11046, 11047, 11050, 11051, 11052, 11053, 11054, 11055, 11056, 11057, 11058, 11059, 11060, 11061, 11062, 11063, 11064, 11065, 11066, 11067, 11068, 11069, 11070, 11100, 11101, 11102);
INSERT INTO `acore_string` (`entry`, `content_default`, `locale_frFR`) VALUES
(11039, 'Black Market Delivery', 'Livraison du marché noir'),
(11040, 'This package contains {} and is meant to be delivered to {} only. If you are not the original receiver, please return it to the closest Black Market representative.', 'Ce paquet contient {} et est destiné à être livré à {}. Si vous n\'êtes pas le destinataire original, veuillez le retourner au représentant du marché noir le plus proche.'),
(11041, 'Armor, Miscellaneous', 'Armure, Divers'),
(11042, 'Armor, Cloth', 'Armure, Tissu'),
(11043, 'Armor, Leather', 'Armure, Cuir'),
(11044, 'Armor, Mail', 'Armure, Mailles'),
(11045, 'Armor, Plate', 'Armure, Plaques'),
(11046, 'Armor, Buckler', 'Armure, Boucliers'),
(11047, 'Armor, Shield', 'Armure, Boucliers'),
(11050, 'Weapon, Axe 1H', 'Armes, Haches à une main'),
(11051, 'Weapon, Axe 2H', 'Armes, Haches à deux mains'),
(11052, 'Weapon, Bow', 'Armes, Arcs'),
(11053, 'Weapon, Gun', 'Armes, Armes à feu'),
(11054, 'Weapon, Mace 1H', 'Armes, Masses à une main'),
(11055, 'Weapon, Mace 2H', 'Armes, Masses à deux mains'),
(11056, 'Weapon, Polearm', 'Armes, Armes d\'hast'),
(11057, 'Weapon, Sword 1H', 'Armes, Épées à une main'),
(11058, 'Weapon, Sword 2H', 'Armes, Épées à deux mains'),
(11059, 'Weapon, Obsolete', 'Armes, Obsolète'),
(11060, 'Weapon, Staff', 'Armes, Bâtons'),
(11061, 'Weapon, Exotic', 'Armes, Exotique'),
(11062, 'Weapon, Exotic', 'Armes, Exotique'),
(11063, 'Weapon, Fist Weapon', 'Armes, Armes de pugilat'),
(11064, 'Weapon, Miscellaneous', 'Armes, Divers'),
(11065, 'Weapon, Dagger', 'Armes, Dagues'),
(11066, 'Weapon, Thrown', 'Armes, Armes de jet'),
(11067, 'Weapon, Spear', 'Armes, Lances'),
(11068, 'Weapon, Crossbow', 'Armes, Arbalètes'),
(11069, 'Weapon, Wand', 'Armes, Baguettes'),
(11070, 'Weapon, Fishing Pole', 'Armes, Cannes à pêche'),
(11100, 'Yes. (cost: {} Contraband Marks).', 'Oui. (coût : {} Marques de contrebande).'),
(11101, 'No.', 'Non.'),
(11102, 'Your item will be replaced by {}.', 'Votre objet sera remplacé par {}.');

DELETE FROM `creature_text` WHERE `CreatureID` = 500000;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextId`, `TextRange`, `comment`) VALUES
(500000, 0, 0, 'You don\'t have enough inventory space, $N!', 12, 0, 100, 1, 0, 0, 0, 0, 'Black Market - inventory full'),
(500000, 1, 0, 'What a scammer, $N! You don\'t have enough Contraband Marks!', 14, 0, 100, 5, 0, 5960, 0, 0, 'Black Market - not enough currency'),
(500000, 2, 0, 'You\'re wasting my time...', 12, 0, 100, 14, 0, 5960, 0, 0, 'Black Market - cancelled deal'),
(500000, 3, 0, 'Time is money, friend. That\'s all I ever hear. How about moving this mailbox closer if time is so important?', 12, 0, 25, 1, 0, 0, 0, 0, 'Black Market - mailbox call'),
(500000, 3, 1, 'Go there... Do this...', 12, 0, 25, 1, 0, 0, 0, 0, 'Black Market - mailbox call'),
(500000, 3, 2, 'Why don\'t you just go get your own items instead of bothering me?', 12, 0, 25, 1, 0, 0, 0, 0, 'Black Market - mailbox call'),
(500000, 3, 3, 'Maybe I could talk one of those gnomes into doing this walking for me.', 12, 0, 25, 1, 0, 0, 0, 0, 'Black Market - mailbox call'),
(500000, 4, 0, 'I\'m not gettin\' paid enough for this.', 12, 0, 25, 1, 0, 0, 0, 0, 'Black Market - posting mail'),
(500000, 4, 1, 'Used to be a chief engineer back in Kezan, you know. Now look at me...', 12, 0, 25, 1, 0, 0, 0, 0, 'Black Market - posting mail'),
(500000, 4, 2, 'Ah! Wait, I didn\'t mean to mail those! Those pictures are definitely going to land in Booty Bay now...', 12, 0, 25, 1, 0, 0, 0, 0, 'Black Market - posting mail'),
(500000, 4, 3, 'I hear those gnomes have machines that can fly now... I\'d kill to get my hands on one.', 12, 0, 25, 1, 0, 0, 0, 0, 'Black Market - posting mail'),
(500000, 5, 0, 'That\'s it? I got mouths to feed, $N! Find me something where I can turn a profit at least.', 12, 0, 25, 1, 0, 0, 0, 0, 'Black Market - accepted deal'),
(500000, 5, 1, 'I\'ll mail it to ya as soon as I\'ve found it. Don\'t worry about the shipping cost, it\'s included.', 12, 0, 25, 1, 0, 0, 0, 0, 'Black Market - accepted deal'),
(500000, 5, 2, 'Don\'t forget, I got the best deals anywhere, even on the hard-to-find stuff!', 12, 0, 25, 1, 0, 0, 0, 0, 'Black Market - accepted deal'),
(500000, 5, 3, 'Some of this stuff might be hard to find... Don\'t worry though, I\'m on it!', 12, 0, 25, 1, 0, 0, 0, 0, 'Black Market - accepted deal'),
(500000, 6, 0, 'Upgrades! Get your upgrades here!', 12, 0, 50, 1, 0, 0, 0, 0, 'Black Market - advertisement'),
(500000, 6, 1, 'Got some old gear you\'re thinking about selling? Give me a minute and I can make it worth saving!', 12, 0, 50, 1, 0, 0, 0, 0, 'Black Market - advertisement'),
(500000, 7, 0, 'Hey $N, stop by the black market...', 15, 0, 50, 1, 0, 0, 0, 0, 'Black Market - whispered advertisement'),
(500000, 7, 1, 'Make that rusty gear of yours shine like a new Goblin Drag Car!', 15, 0, 50, 1, 0, 0, 0, 0, 'Black Market - whispered advertisement');

DELETE FROM `creature_text_locale` WHERE `CreatureID` = 500000 AND `Locale` = 'frFR';
INSERT INTO `creature_text_locale` (`CreatureID`, `GroupID`, `ID`, `Locale`, `Text`) VALUES
(500000, 0, 0, 'frFR', 'Vous n\'avez pas assez de place dans votre inventaire, $N !'),
(500000, 1, 0, 'frFR', 'Quel arnaqueur, $N ! Vous n\'avez pas assez de Marques de contrebande !'),
(500000, 2, 0, 'frFR', 'Je perds mon temps...'),
(500000, 3, 0, 'frFR', 'Le temps, c\'est de l\'argent. Que diriez-vous alors de me rapprocher cette foutue boîte aux lettres ?'),
(500000, 3, 1, 'frFR', 'Va là-bas... Fais ça...'),
(500000, 3, 2, 'frFR', 'Pourquoi n\'allez-vous pas chercher vos propres objets au lieu de m\'embêter ?'),
(500000, 3, 3, 'frFR', 'Peut-être que je pourrais convaincre un de ces gnomes de faire cette course pour moi.'),
(500000, 4, 0, 'frFR', 'Je ne suis pas assez payé pour ça...'),
(500000, 4, 1, 'frFR', 'J\'étais ingénieur en chef à Kezan, vous savez. Maintenant, regardez-moi...'),
(500000, 4, 2, 'frFR', 'Ah ! Attendez, je ne voulais pas les poster ! Ces photos vont finir à Baie-du-Butin maintenant...'),
(500000, 4, 3, 'frFR', 'J\'ai entendu dire que ces fichus gnomes avaient des machines volantes... Je tuerais pour en avoir une.'),
(500000, 5, 0, 'frFR', 'C\'est tout ? J\'ai des bouches à nourrir, $N ! Trouvez-moi quelque chose qui me rapporte un peu.'),
(500000, 5, 1, 'frFR', 'Je vous l\'enverrai dès que je l\'aurai trouvé. Les frais de livraison sont inclus.'),
(500000, 5, 2, 'frFR', 'N\'oubliez pas que j\'ai les meilleures offres, même sur les objets difficiles à trouver !'),
(500000, 5, 3, 'frFR', 'Certaines de ces choses seront difficiles à trouver... Mais ne vous inquiétez pas, je m\'en occupe !'),
(500000, 6, 0, 'frFR', 'Améliorations ! Demandez vos améliorations ici !'),
(500000, 6, 1, 'frFR', 'Vous avez du vieux matériel à vendre ? Donnez-moi une minute et je vous aiderai à le sauver !'),
(500000, 7, 0, 'frFR', 'Hé $N, arrêtez-vous au marché noir...'),
(500000, 7, 1, 'frFR', 'Faites briller votre équipement rouillé comme un Chariot gobelin flambant neuf !');

DELETE FROM `creature_template_locale` WHERE `entry` = 500000 AND `locale` = 'frFR';
INSERT INTO `creature_template_locale` (`entry`, `locale`, `Name`, `Title`, `VerifiedBuild`) VALUES
(500000, 'frFR', 'Représentant du marché noir', 'Redimensionnement d\'objets', 0);

DELETE FROM `item_template_locale` WHERE `ID` = 500001;
DELETE FROM `item_template` WHERE `entry` = 500001;
INSERT INTO `item_template` (`entry`, `class`, `subclass`, `SoundOverrideSubclass`, `name`, `displayid`, `Quality`, `Flags`, `BuyCount`, `InventoryType`, `AllowableClass`, `AllowableRace`, `RequiredLevel`, `maxcount`, `stackable`, `bonding`, `description`, `Material`, `BagFamily`, `VerifiedBuild`) VALUES
(500001, 10, 0, -1, 'Contraband Mark', 51567, 1, 0, 0, 0, -1, -1, 0, 0, 2147483647, 1, 'Stamped by an unknown cartel and accepted by black market representatives throughout Azeroth.', -1, 0, 0);

INSERT INTO `item_template_locale` (`ID`, `locale`, `Name`, `Description`, `VerifiedBuild`) VALUES
(500001, 'frFR', 'Marque de contrebande', 'Frappée par un cartel inconnu et acceptée par les représentants du marché noir à travers Azeroth.', 0);

-- Contraband Marks are the black-market currency and drop from every loot-bearing
-- elite, rare elite, world boss, and rare creature. Shared loot tables use the
-- highest reward assigned to any creature using that table.
DELETE `loot`
FROM `creature_loot_template` AS `loot`
INNER JOIN (
    SELECT DISTINCT `lootid`
    FROM `creature_template`
    WHERE `rank` IN (1, 2, 3, 4) AND `lootid` <> 0
) AS `ranked_loot` ON `ranked_loot`.`lootid` = `loot`.`Entry`
WHERE `loot`.`Item` IN (29434, 500001);

INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`)
SELECT `lootid`, 500001, 0, 100, 0, 1, 0, `reward`, `reward`, 'Ranked creature - Contraband Mark'
FROM (
    SELECT `lootid`, MAX(CASE `rank`
        WHEN 1 THEN 1 -- Elite
        WHEN 2 THEN 2 -- Rare elite
        WHEN 3 THEN 4 -- World boss
        WHEN 4 THEN 2 -- Rare
    END) AS `reward`
    FROM `creature_template`
    WHERE `rank` IN (1, 2, 3, 4) AND `lootid` <> 0
    GROUP BY `lootid`
) AS `ranked_loot`;
