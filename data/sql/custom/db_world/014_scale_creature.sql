-- rochenoire_scale_level_creature table data
-- Build the eligible creature set once so the area average and offsets use the same population
DROP TEMPORARY TABLE IF EXISTS temp_scalable_creatures;

CREATE TEMPORARY TABLE temp_scalable_creatures AS
SELECT c.guid, c.map, c.areaId, ct.entry, ct.name, ct.MinLevel, ct.MaxLevel
FROM creature c
JOIN creature_template ct ON c.id = ct.entry
WHERE ct.type IN (1, 2, 3, 4, 5, 6, 7, 9)
AND ct.minlevel != 1
AND c.npcflag = 0
AND ct.faction != 35 -- no friendly
AND (ct.type_flags & 0x00000002) = 0 -- Exclude CREATURE_TYPE_FLAG_VISIBLE_TO_GHOSTS
AND (ct.type_flags & 0x00000004) = 0 -- Exclude CREATURE_TYPE_FLAG_BOSS_MOB
AND (ct.type_flags & 0x00000080) = 0 -- Exclude CREATURE_TYPE_FLAG_INTERACT_WHILE_DEAD
AND (ct.type_flags & 0x00040000) = 0 -- Exclude CREATURE_TYPE_FLAG_ALLOW_INTERACTION_WHILE_IN_COMBAT
AND (ct.type_flags & 0x08000000) = 0 -- Exclude CREATURE_TYPE_FLAG_FORCE_GOSSIP
AND (ct.type_flags & 0x20000000) = 0 -- Exclude CREATURE_TYPE_FLAG_DO_NOT_TARGET_ON_INTERACTION
AND (ct.type_flags & 0x80000000) = 0 -- Exclude CREATURE_TYPE_FLAG_UNIT_IS_QUEST_BOSS
AND (ct.flags_extra & 0x00000002) = 0 -- Exclude CREATURE_FLAG_EXTRA_CIVILIAN
AND (ct.flags_extra & 0x00000004) = 0 -- Exclude CREATURE_FLAG_EXTRA_NO_PARRY
AND (ct.flags_extra & 0x00000008) = 0 -- Exclude CREATURE_FLAG_EXTRA_NO_PARRY_HASTEN
AND (ct.flags_extra & 0x00000010) = 0 -- Exclude CREATURE_FLAG_EXTRA_NO_BLOCK
AND (ct.flags_extra & 0x00000020) = 0 -- Exclude CREATURE_FLAG_EXTRA_NO_CRUSHING_BLOWS
AND (ct.flags_extra & 0x00000040) = 0 -- Exclude CREATURE_FLAG_EXTRA_NO_XP
AND (ct.flags_extra & 0x00000080) = 0 -- Exclude CREATURE_FLAG_EXTRA_TRIGGER
AND (ct.flags_extra & 0x00000400) = 0 -- Exclude CREATURE_FLAG_EXTRA_GHOST_VISIBILITY
AND (ct.flags_extra & 0x00800000) = 0 -- Exclude CREATURE_FLAG_EXTRA_NO_DODGE
;

-- Drop the temporary table if it exists
DROP TEMPORARY TABLE IF EXISTS temp_area_levels;

-- Create the temporary table with the area levels (AVG)
CREATE TEMPORARY TABLE temp_area_levels AS
SELECT map, areaId, ROUND(AVG(MinLevel)) AS areaLevel
FROM temp_scalable_creatures
GROUP BY map, areaId
LIMIT 999999;

-- Clear the rochenoire_scale_level_creature table
TRUNCATE TABLE rochenoire_scale_level_creature;

-- Insert the new data with the required filtering, including the name, entry, and levels in comment
REPLACE INTO rochenoire_scale_level_creature (guid, flvar, COMMENT)
SELECT 
    c.guid, 
    (c.MinLevel - t.areaLevel) AS flvar,
    -- Concatenate name, entry, MinLevel, and areaLevel with truncation if necessary
    CONCAT(
        LEFT(
            c.name, 
            255 - LENGTH(CONCAT(' (Entry: ', c.entry, ', MinLevel: ', c.MinLevel, ', AreaLevel: ', t.areaLevel, ')'))
        ),
        ' (Entry: ', c.entry, ', MinLevel: ', c.MinLevel, ', AreaLevel: ', t.areaLevel, ')'
    ) AS COMMENT
FROM temp_scalable_creatures c
JOIN temp_area_levels t ON c.map = t.map AND c.areaId = t.areaId
WHERE 
    -- Filter out flvar = 0
    (c.MinLevel - t.areaLevel) != 0
    -- Filter out flvar where its absolute value is less than the difference between MinLevel and MaxLevel
    AND ABS(c.MinLevel - t.areaLevel) >= (c.MaxLevel - c.MinLevel)
    -- The runtime loads flvar as int8
    AND (c.MinLevel - t.areaLevel) BETWEEN -128 AND 127
    -- Filter out cases where the sum of flvar and MinLevel is less than 0
    AND (c.MinLevel + (c.MinLevel - t.areaLevel)) >= 0;

-- Replace into rochenoire_scale_level_creature_template table
REPLACE INTO rochenoire_scale_level_creature_template (entry, flvar)
VALUES 
    (11713, -4), -- Blackwood Tracker
    (11714, -2), -- Marosh the Devious
    (6180, -2);  -- Defias Raider (Quest: Tome of Valor)
