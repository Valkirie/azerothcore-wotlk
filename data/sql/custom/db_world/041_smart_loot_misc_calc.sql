-- Rochenoire smart-loot reference classification.
--
-- The server loads `Reference` and `LootInfo` from `rochenoire_smart_loot_data`
-- for the requested loot table. When a creature loot row points to one of these
-- reference templates, LootMgr uses LootInfo as the item-quality input for the
-- smart-loot chance modifier.
--
-- This script classifies creature-loot references that contain weapon or armor
-- items. The item quality becomes LootInfo, so qualities 0 through 4 map to the
-- corresponding configured smart-loot quality rates. References containing
-- several matching items are written once per reference; GROUP BY removes the
-- duplicate matches because the lookup table has one row per table/reference.
--
-- Creature references are followed recursively so nested reference templates are
-- included in the quality classification.
-- The scoped delete and insert make the result safe to recalculate when item or
-- loot data changes.

-- creature_loot_template
-- Rebuild all creature-loot rows to keep the table idempotent and free of stale data.
DELETE FROM `rochenoire_smart_loot_data`
WHERE `table` = 'creature_loot_template';

-- One reference can resolve to mixed equipment qualities through nested references.
-- Keep one row per root creature-loot reference using the highest runtime-supported
-- equipment quality reachable from that root.
INSERT INTO `rochenoire_smart_loot_data` (`table`, `LootInfo`, `Reference`)
WITH RECURSIVE reachable_references (`RootReference`, `CurrentReference`) AS
(
    SELECT DISTINCT ABS(`creature_loot_template`.`Reference`), ABS(`creature_loot_template`.`Reference`)
    FROM `creature_loot_template`
    WHERE `creature_loot_template`.`Reference` <> 0

    UNION

    SELECT reachable_references.`RootReference`, ABS(`reference_loot_template`.`Reference`)
    FROM reachable_references
    INNER JOIN `reference_loot_template`
        ON `reference_loot_template`.`Entry` = reachable_references.`CurrentReference`
    WHERE `reference_loot_template`.`Reference` <> 0
)
SELECT 'creature_loot_template', MAX(`item_template`.`Quality`), reachable_references.`RootReference`
FROM reachable_references
INNER JOIN `reference_loot_template`
    ON `reference_loot_template`.`Entry` = reachable_references.`CurrentReference`
INNER JOIN `item_template`
    ON `item_template`.`Entry` = `reference_loot_template`.`Item`
WHERE (`item_template`.`class` = 2 OR `item_template`.`class` = 4)
  AND `item_template`.`Quality` BETWEEN 0 AND 4
GROUP BY reachable_references.`RootReference`;
