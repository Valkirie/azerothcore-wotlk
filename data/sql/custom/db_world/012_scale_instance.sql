-- Rochenoire instance population scaling data.
--
-- Automatically creates conservative spawn-scaling groups from existing creature
-- formations. A formation is accepted only when every member is ordinary combat
-- trash on a configured instance map. Existing manually curated rows are never
-- replaced, and any formation containing one is left entirely to manual tuning.
--
-- rochenoire_scale_instance_creature intentionally remains override-only. All
-- persistent instance creatures already use the default scaling policy, so rows
-- should be added there only for encounter-specific tuning.

DELETE FROM `rochenoire_scale_instance_spawn_group`
WHERE `comment` LIKE 'auto:formation:%';

DROP TEMPORARY TABLE IF EXISTS `temp_instance_formation_members`;

CREATE TEMPORARY TABLE `temp_instance_formation_members` AS
SELECT
    c.`guid` AS `spawnId`,
    c.`map` AS `mapId`,
    cf.`leaderGUID`,
    c.`id` AS `creatureEntry`,
    ct.`name` AS `creatureName`,
    ct.`rank`,
    CASE
        WHEN c.`guid` = cf.`leaderGUID` THEN 0
        WHEN ct.`rank` > 0 THEN 1
        ELSE 2
    END AS `retentionPriority`,
    CASE
        WHEN c.`id` = 0 THEN 1
        WHEN ct.`type` NOT IN (1, 2, 3, 4, 5, 6, 7, 9) THEN 1
        WHEN ct.`minlevel` = 1 THEN 1
        WHEN COALESCE(NULLIF(c.`npcflag`, 0), ct.`npcflag`) <> 0 THEN 1
        WHEN (COALESCE(NULLIF(c.`unit_flags`, 0), ct.`unit_flags`) & 0x02000002) <> 0 THEN 1
        WHEN ct.`faction` = 35 THEN 1
        WHEN ct.`rank` = 3 THEN 1
        WHEN (ct.`type_flags` & 0xA8040086) <> 0 THEN 1
        WHEN (ct.`flags_extra` & 0x0000A4C2) <> 0 THEN 1
        WHEN COALESCE(c.`ScriptName`, '') <> '' THEN 1
        WHEN COALESCE(ct.`ScriptName`, '') <> '' THEN 1
        WHEN EXISTS (
            SELECT 1
            FROM `instance_encounters` ie
            WHERE ie.`creditType` = 0
              AND ie.`creditEntry` = c.`id`
        ) THEN 1
        WHEN EXISTS (
            SELECT 1
            FROM `pool_creature` pc
            WHERE pc.`guid` = c.`guid`
        ) THEN 1
        WHEN EXISTS (
            SELECT 1
            FROM `game_event_creature` gec
            WHERE gec.`guid` = c.`guid`
        ) THEN 1
        WHEN EXISTS (
            SELECT 1
            FROM `linked_respawn` lr
            WHERE lr.`guid` = c.`guid`
               OR lr.`linkedGuid` = c.`guid`
        ) THEN 1
        WHEN EXISTS (
            SELECT 1
            FROM `rochenoire_scale_instance_spawn_group` existing
            WHERE existing.`spawnId` = c.`guid`
        ) THEN 1
        ELSE 0
    END AS `unsafe`
FROM `creature_formations` cf
JOIN `creature` c
    ON c.`guid` = cf.`memberGUID`
JOIN `creature_template` ct
    ON ct.`entry` = c.`id`
JOIN (
    SELECT DISTINCT `mapId`
    FROM `rochenoire_scale_zone`
    WHERE `minimumPlayers` > 0
      AND `maximumPlayers` >= `minimumPlayers`
) configuredMap
    ON configuredMap.`mapId` = c.`map`;

ALTER TABLE `temp_instance_formation_members`
    ADD PRIMARY KEY (`spawnId`),
    ADD KEY `idx_formation` (`mapId`, `leaderGUID`);

DROP TEMPORARY TABLE IF EXISTS `temp_safe_instance_formations`;

CREATE TEMPORARY TABLE `temp_safe_instance_formations` AS
SELECT
    `mapId`,
    `leaderGUID`
FROM `temp_instance_formation_members`
GROUP BY `mapId`, `leaderGUID`
HAVING COUNT(*) >= 2
   AND SUM(`spawnId` = `leaderGUID`) = 1
   AND SUM(`unsafe`) = 0;

ALTER TABLE `temp_safe_instance_formations`
    ADD PRIMARY KEY (`mapId`, `leaderGUID`);

INSERT INTO `rochenoire_scale_instance_spawn_group`
    (`spawnId`, `mapId`, `scalingGroupId`, `activationOrder`, `comment`)
SELECT
    ranked.`spawnId`,
    ranked.`mapId`,
    ranked.`leaderGUID`,
    ranked.`activationOrder`,
    CONCAT('auto:formation:', ranked.`leaderGUID`, ' ', ranked.`creatureName`)
FROM (
    SELECT
        member.`spawnId`,
        member.`mapId`,
        member.`leaderGUID`,
        member.`creatureName`,
        ROW_NUMBER() OVER (
            PARTITION BY member.`mapId`, member.`leaderGUID`
            ORDER BY member.`retentionPriority`, member.`rank` DESC, member.`spawnId`
        ) - 1 AS `activationOrder`
    FROM `temp_instance_formation_members` member
    JOIN `temp_safe_instance_formations` safeFormation
        ON safeFormation.`mapId` = member.`mapId`
       AND safeFormation.`leaderGUID` = member.`leaderGUID`
) ranked
ORDER BY ranked.`mapId`, ranked.`leaderGUID`, ranked.`activationOrder`;

DROP TEMPORARY TABLE IF EXISTS `temp_safe_instance_formations`;
DROP TEMPORARY TABLE IF EXISTS `temp_instance_formation_members`;
