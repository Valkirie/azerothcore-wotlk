-- Rochenoire smart-loot reference classification.
--
-- The server loads `Reference` and `LootInfo` from `rochenoire_smart_loot_data`
-- for the requested loot table. When a loot row points to one of these
-- reference templates, LootMgr uses LootInfo as the item-quality input for the
-- smart-loot chance modifier.
--
-- This script classifies references from every outer loot table that contain
-- weapon or armor items. The item quality becomes LootInfo, so qualities 0
-- through 4 map to the corresponding configured smart-loot quality rates.
-- References containing several matching items are written once per table and
-- reference; GROUP BY removes duplicate matches.
--
-- References are followed recursively so nested reference templates are included
-- in the quality classification for every supported outer loot table.
-- The scoped delete and insert make the result safe to recalculate when item or
-- loot data changes.

DELETE FROM `rochenoire_smart_loot_data`
WHERE `table` IN (
    'creature_loot_template',
    'disenchant_loot_template',
    'fishing_loot_template',
    'gameobject_loot_template',
    'item_loot_template',
    'mail_loot_template',
    'milling_loot_template',
    'pickpocketing_loot_template',
    'player_loot_template',
    'prospecting_loot_template',
    'reference_loot_template',
    'skinning_loot_template',
    'spell_loot_template'
);

INSERT INTO `rochenoire_smart_loot_data` (`table`, `LootInfo`, `Reference`)
WITH RECURSIVE reachable_references (`LootTable`, `RootReference`, `CurrentReference`) AS
(
    SELECT sources.`LootTable`, ABS(sources.`Reference`), ABS(sources.`Reference`)
    FROM
    (
        SELECT 'creature_loot_template' AS `LootTable`, `Reference` FROM `creature_loot_template`
        UNION ALL
        SELECT 'disenchant_loot_template', `Reference` FROM `disenchant_loot_template`
        UNION ALL
        SELECT 'fishing_loot_template', `Reference` FROM `fishing_loot_template`
        UNION ALL
        SELECT 'gameobject_loot_template', `Reference` FROM `gameobject_loot_template`
        UNION ALL
        SELECT 'item_loot_template', `Reference` FROM `item_loot_template`
        UNION ALL
        SELECT 'mail_loot_template', `Reference` FROM `mail_loot_template`
        UNION ALL
        SELECT 'milling_loot_template', `Reference` FROM `milling_loot_template`
        UNION ALL
        SELECT 'pickpocketing_loot_template', `Reference` FROM `pickpocketing_loot_template`
        UNION ALL
        SELECT 'player_loot_template', `Reference` FROM `player_loot_template`
        UNION ALL
        SELECT 'prospecting_loot_template', `Reference` FROM `prospecting_loot_template`
        UNION ALL
        SELECT 'reference_loot_template', `Reference` FROM `reference_loot_template`
        UNION ALL
        SELECT 'skinning_loot_template', `Reference` FROM `skinning_loot_template`
        UNION ALL
        SELECT 'spell_loot_template', `Reference` FROM `spell_loot_template`
    ) sources
    WHERE sources.`Reference` <> 0

    UNION

    SELECT reachable_references.`LootTable`, reachable_references.`RootReference`, ABS(`reference_loot_template`.`Reference`)
    FROM reachable_references
    INNER JOIN `reference_loot_template`
        ON `reference_loot_template`.`Entry` = reachable_references.`CurrentReference`
    WHERE `reference_loot_template`.`Reference` <> 0
)
SELECT reachable_references.`LootTable`, MAX(`item_template`.`Quality`), reachable_references.`RootReference`
FROM reachable_references
INNER JOIN `reference_loot_template`
    ON `reference_loot_template`.`Entry` = reachable_references.`CurrentReference`
INNER JOIN `item_template`
    ON `item_template`.`Entry` = `reference_loot_template`.`Item`
WHERE (`item_template`.`class` = 2 OR `item_template`.`class` = 4)
  AND `item_template`.`Quality` BETWEEN 0 AND 4
GROUP BY reachable_references.`LootTable`, reachable_references.`RootReference`;
