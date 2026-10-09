-- rochenoire_scale_level_creature table data
-- Build the eligible creature set once so the area median and offsets use the same population
DROP TEMPORARY TABLE IF EXISTS temp_creature_event_groups;

CREATE TEMPORARY TABLE temp_creature_event_groups AS
SELECT guid, GROUP_CONCAT(eventEntry ORDER BY eventEntry SEPARATOR ',') AS eventGroup
FROM game_event_creature
GROUP BY guid;

ALTER TABLE temp_creature_event_groups ADD PRIMARY KEY (guid);

DROP TEMPORARY TABLE IF EXISTS temp_scalable_creatures;

CREATE TEMPORARY TABLE temp_scalable_creatures AS
SELECT
    c.guid,
    c.map,
    c.areaId,
    c.phaseMask,
    COALESCE(ceg.eventGroup, '') AS eventGroup,
    ct.entry,
    ct.name,
    ct.MinLevel,
    ct.MaxLevel
FROM creature c
JOIN creature_template ct ON c.id = ct.entry
LEFT JOIN temp_creature_event_groups ceg ON ceg.guid = c.guid
LEFT JOIN rochenoire_scale_zone areaScale ON areaScale.areaId = c.areaId
LEFT JOIN rochenoire_scale_zone zoneScale ON zoneScale.areaId = c.zoneId
WHERE ct.type IN (1, 2, 3, 4, 5, 6, 7, 9)
AND ct.minlevel != 1
AND c.areaId != 0
AND (COALESCE(areaScale.areaFlags, zoneScale.areaFlags, 0) & 0x00100300) = 0 -- Exclude AREA_FLAG_LOWLEVEL, AREA_FLAG_CAPITAL, and AREA_FLAG_CITY
AND COALESCE(NULLIF(c.npcflag, 0), ct.npcflag) = 0
AND (COALESCE(NULLIF(c.unit_flags, 0), ct.unit_flags) & 0x02000002) = 0 -- Exclude UNIT_FLAG_NOT_SELECTABLE and UNIT_FLAG_NON_ATTACKABLE
AND ct.faction != 35 -- no friendly
AND (ct.type_flags & 0x00000002) = 0 -- Exclude CREATURE_TYPE_FLAG_VISIBLE_TO_GHOSTS
AND (ct.type_flags & 0x00000004) = 0 -- Exclude CREATURE_TYPE_FLAG_BOSS_MOB
AND (ct.type_flags & 0x00000080) = 0 -- Exclude CREATURE_TYPE_FLAG_INTERACT_WHILE_DEAD
AND (ct.type_flags & 0x00040000) = 0 -- Exclude CREATURE_TYPE_FLAG_ALLOW_INTERACTION_WHILE_IN_COMBAT
AND (ct.type_flags & 0x08000000) = 0 -- Exclude CREATURE_TYPE_FLAG_FORCE_GOSSIP
AND (ct.type_flags & 0x20000000) = 0 -- Exclude CREATURE_TYPE_FLAG_DO_NOT_TARGET_ON_INTERACTION
AND (ct.type_flags & 0x80000000) = 0 -- Exclude CREATURE_TYPE_FLAG_UNIT_IS_QUEST_BOSS
AND (ct.flags_extra & 0x00000002) = 0 -- Exclude CREATURE_FLAG_EXTRA_CIVILIAN
AND (ct.flags_extra & 0x00000040) = 0 -- Exclude CREATURE_FLAG_EXTRA_NO_XP
AND (ct.flags_extra & 0x00000080) = 0 -- Exclude CREATURE_FLAG_EXTRA_TRIGGER
AND (ct.flags_extra & 0x00000400) = 0 -- Exclude CREATURE_FLAG_EXTRA_GHOST_VISIBILITY
AND (ct.flags_extra & 0x00002000) = 0 -- Exclude CREATURE_FLAG_EXTRA_CANNOT_ENTER_COMBAT
AND (ct.flags_extra & 0x00008000) = 0 -- Exclude CREATURE_FLAG_EXTRA_GUARD
;

-- Rank each spawn within its area, phase, and event context so common creatures retain their natural weight
DROP TEMPORARY TABLE IF EXISTS temp_ranked_area_levels;

CREATE TEMPORARY TABLE temp_ranked_area_levels AS
SELECT
    map,
    areaId,
    phaseMask,
    eventGroup,
    MinLevel,
    ROW_NUMBER() OVER (
        PARTITION BY map, areaId, phaseMask, eventGroup
        ORDER BY MinLevel
    ) AS levelRank,
    COUNT(*) OVER (
        PARTITION BY map, areaId, phaseMask, eventGroup
    ) AS areaPopulation
FROM temp_scalable_creatures;

-- Drop the temporary table if it exists
DROP TEMPORARY TABLE IF EXISTS temp_area_levels;

-- Use the middle spawn level (or the rounded mean of both middle levels) as the contextual area level
CREATE TEMPORARY TABLE temp_area_levels AS
SELECT map, areaId, phaseMask, eventGroup, ROUND(AVG(MinLevel)) AS areaLevel
FROM temp_ranked_area_levels
WHERE levelRank IN (
    FLOOR((areaPopulation + 1) / 2),
    FLOOR((areaPopulation + 2) / 2)
)
GROUP BY map, areaId, phaseMask, eventGroup;

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
JOIN temp_area_levels t
    ON c.map = t.map
    AND c.areaId = t.areaId
    AND c.phaseMask = t.phaseMask
    AND c.eventGroup = t.eventGroup
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
