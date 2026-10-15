/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "InstancePackGenerator.h"
#include "DatabaseEnv.h"
#include "Field.h"
#include "Log.h"
#include "ModelIgnoreFlags.h"
#include "QueryResult.h"
#include "VMapMgr2.h"
#include "server/game/Grids/GridDefines.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <tuple>

namespace
{
char const* SpawnQueryTemplate = R"SQL(
SELECT
    c.`guid`, c.`{}`, c.`map`, c.`zoneId`, c.`areaId`, c.`spawnMask`, c.`phaseMask`,
    c.`position_x`, c.`position_y`, c.`position_z`, ct.`name`, ct.`rank`,
    COALESCE(cf.`leaderGUID`, 0), COALESCE(existing.`scalingGroupId`, 0),
    CASE
        WHEN c.`{}` = 0 THEN 'missing entry'
        WHEN ct.`type` NOT IN (1, 2, 3, 4, 5, 6, 7, 9) THEN 'unsafe creature type'
        WHEN ct.`minlevel` = 1 THEN 'level one'
        WHEN COALESCE(NULLIF(c.`npcflag`, 0), ct.`npcflag`) <> 0 THEN 'interactive NPC'
        WHEN (COALESCE(NULLIF(c.`unit_flags`, 0), ct.`unit_flags`) & 0x02000002) <> 0 THEN 'noncombat flags'
        WHEN ct.`faction` = 35 THEN 'friendly faction'
        WHEN ct.`rank` = 3 THEN 'world boss rank'
        WHEN (ct.`type_flags` & 0xA8040086) <> 0 THEN 'unsafe type flags'
        WHEN (ct.`flags_extra` & 0x0000A4C2) <> 0 THEN 'unsafe extra flags'
        WHEN COALESCE(c.`ScriptName`, '') <> '' OR COALESCE(ct.`ScriptName`, '') <> '' THEN 'scripted'
        WHEN ie.`creditEntry` IS NOT NULL THEN 'instance encounter credit'
        WHEN pc.`guid` IS NOT NULL THEN 'pool member'
        WHEN gec.`guid` IS NOT NULL THEN 'game event member'
        WHEN lr.`guid` IS NOT NULL OR reverseLr.`linkedGuid` IS NOT NULL THEN 'linked respawn'
        WHEN existing.`spawnId` IS NOT NULL THEN 'existing scaling group'
        ELSE ''
    END
FROM `creature` c
JOIN `creature_template` ct ON ct.`entry` = c.`{}`
JOIN (
    SELECT DISTINCT `mapId`
    FROM `rochenoire_scale_zone`
    WHERE `minimumPlayers` > 0 AND `maximumPlayers` >= `minimumPlayers`
) configuredMap ON configuredMap.`mapId` = c.`map`
LEFT JOIN `creature_formations` cf ON cf.`memberGUID` = c.`guid`
LEFT JOIN `instance_encounters` ie ON ie.`creditType` = 0 AND ie.`creditEntry` = c.`{}`
LEFT JOIN `pool_creature` pc ON pc.`guid` = c.`guid`
LEFT JOIN (SELECT DISTINCT `guid` FROM `game_event_creature`) gec ON gec.`guid` = c.`guid`
LEFT JOIN `linked_respawn` lr ON lr.`guid` = c.`guid`
LEFT JOIN `linked_respawn` reverseLr ON reverseLr.`linkedGuid` = c.`guid`
LEFT JOIN `rochenoire_scale_instance_spawn_group` existing ON existing.`spawnId` = c.`guid`
ORDER BY c.`map`, c.`guid`
)SQL";

float Distance(InstancePackSpawn const& left, InstancePackSpawn const& right)
{
    float const x = left.x - right.x;
    float const y = left.y - right.y;
    float const z = left.z - right.z;
    return std::sqrt(x * x + y * y + z * z);
}

float MaximumPairDistance(std::vector<InstancePackSpawn const*> const& members)
{
    float maximum = 0.0f;
    for (std::size_t left = 0; left < members.size(); ++left)
        for (std::size_t right = left + 1; right < members.size(); ++right)
            maximum = std::max(maximum, Distance(*members[left], *members[right]));
    return maximum;
}

InstancePackSpawn const* FindMedoid(std::vector<InstancePackSpawn const*> const& members)
{
    return *std::min_element(members.begin(), members.end(), [&members](InstancePackSpawn const* left, InstancePackSpawn const* right)
    {
        auto totalDistance = [&members](InstancePackSpawn const* candidate)
        {
            float total = 0.0f;
            for (InstancePackSpawn const* member : members)
                total += Distance(*candidate, *member);
            return total;
        };

        float const leftDistance = totalDistance(left);
        float const rightDistance = totalDistance(right);
        return leftDistance != rightDistance ? leftDistance < rightDistance : left->spawnId < right->spawnId;
    });
}

float MaximumCenterDistance(std::vector<InstancePackSpawn const*> const& members, InstancePackSpawn const& center)
{
    float maximum = 0.0f;
    for (InstancePackSpawn const* member : members)
        maximum = std::max(maximum, Distance(center, *member));
    return maximum;
}

float Confidence(std::vector<InstancePackSpawn const*> const& members, std::string const& source, float distanceLimit)
{
    float const compactness = std::max(0.0f, 1.0f - MaximumPairDistance(members) / std::max(distanceLimit * 2.0f, 0.01f));
    float const formationBonus = source == "formation" ? 0.25f : 0.0f;
    float const sizeBonus = float(std::min<std::size_t>(members.size(), 5)) * 0.03f;
    std::set<uint32> entries;
    for (InstancePackSpawn const* member : members)
        entries.insert(member->entry);
    float const diversityPenalty = entries.size() == members.size() ? 0.08f : 0.0f;
    return std::clamp(0.45f + compactness * 0.35f + formationBonus + sizeBonus - diversityPenalty, 0.0f, 1.0f);
}

using Context = std::tuple<uint32, uint32, uint32, uint32, uint32>;
constexpr float LineOfSightHeight = 1.5f;

std::string CsvString(std::string const& value)
{
    std::string escaped = value;
    std::size_t position = 0;
    while ((position = escaped.find('"', position)) != std::string::npos)
    {
        escaped.insert(position, 1, '"');
        position += 2;
    }
    return '"' + escaped + '"';
}

std::string SqlString(std::string const& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (char character : value)
    {
        if (character == '\\')
            escaped += "\\\\";
        else if (character == '\'')
            escaped += "''";
        else
            escaped += character;
    }
    return '\'' + escaped + '\'';
}
}

uint32 InstancePackCandidate::GetGroupId() const
{
    auto itr = std::min_element(members.begin(), members.end(), [](InstancePackSpawn const* left, InstancePackSpawn const* right)
    {
        return left->spawnId < right->spawnId;
    });
    return (*itr)->spawnId;
}

InstancePackGenerator::InstancePackGenerator(InstancePackGeneratorOptions options) : _options(std::move(options)) { }

bool InstancePackGenerator::Run()
{
    if (!LoadSpawns())
        return false;

    BuildCandidates();
    std::cout << "Found " << _candidates.size() << " candidate packs.\n";
    return WriteCsv() && WriteSql();
}

bool InstancePackGenerator::LoadSpawns()
{
    QueryResult id1Column = WorldDatabase.Query("SHOW COLUMNS FROM `creature` LIKE 'id1'");
    std::string const creatureEntryColumn = id1Column ? "id1" : "id";
    QueryResult result = WorldDatabase.Query(SpawnQueryTemplate, creatureEntryColumn, creatureEntryColumn, creatureEntryColumn, creatureEntryColumn);
    if (!result)
    {
        LOG_ERROR("instance_pack_generator", "The instance spawn extraction query returned no rows.");
        return false;
    }

    do
    {
        Field* fields = result->Fetch();
        InstancePackSpawn spawn;
        spawn.spawnId = fields[0].Get<uint32>();
        spawn.entry = fields[1].Get<uint32>();
        spawn.mapId = fields[2].Get<uint32>();
        if (_options.mapId && spawn.mapId != *_options.mapId)
            continue;
        spawn.zoneId = fields[3].Get<uint32>();
        spawn.areaId = fields[4].Get<uint32>();
        spawn.spawnMask = fields[5].Get<uint32>();
        spawn.phaseMask = fields[6].Get<uint32>();
        spawn.x = fields[7].Get<float>();
        spawn.y = fields[8].Get<float>();
        spawn.z = fields[9].Get<float>();
        spawn.name = fields[10].Get<std::string>();
        spawn.rank = fields[11].Get<uint8>();
        spawn.formationLeader = fields[12].Get<uint32>();
        spawn.existingGroup = fields[13].Get<uint32>();
        spawn.rejectionReason = fields[14].Get<std::string>();
        _spawns.push_back(std::move(spawn));
    } while (result->NextRow());

    std::cout << "Extracted " << _spawns.size() << " configured-instance spawns.\n";
    return !_spawns.empty();
}

void InstancePackGenerator::BuildCandidates()
{
    std::set<uint32> mapIds;
    for (InstancePackSpawn const& spawn : _spawns)
        mapIds.insert(spawn.mapId);

    for (uint32 mapId : mapIds)
    {
        std::size_t const mapSpawnCount = std::count_if(_spawns.begin(), _spawns.end(), [mapId](InstancePackSpawn const& spawn)
        {
            return spawn.mapId == mapId;
        });
        std::size_t const eligibleSpawnCount = std::count_if(_spawns.begin(), _spawns.end(), [mapId](InstancePackSpawn const& spawn)
        {
            return spawn.mapId == mapId && spawn.rejectionReason.empty();
        });
        std::cout << "Processing map " << mapId << ": " << mapSpawnCount << " spawns (" << eligibleSpawnCount
            << " eligible, " << mapSpawnCount - eligibleSpawnCount << " excluded).\n";

        std::size_t const candidatesBeforeMap = _candidates.size();
        std::size_t formationPackCount = 0;
        std::size_t spatialPackCount = 0;
        std::set<uint32> used;
        std::map<uint32, std::vector<InstancePackSpawn const*>> formations;
        for (InstancePackSpawn const& spawn : _spawns)
            if (spawn.mapId == mapId && spawn.rejectionReason.empty() && spawn.formationLeader)
                formations[spawn.formationLeader].push_back(&spawn);

        for (auto& [leader, members] : formations)
        {
            std::sort(members.begin(), members.end(), [](InstancePackSpawn const* left, InstancePackSpawn const* right)
            {
                auto priority = [](InstancePackSpawn const* member)
                {
                    return std::tuple(member->spawnId != member->formationLeader, -int32(member->rank), member->spawnId);
                };
                return priority(left) < priority(right);
            });

            bool const hasLeader = std::any_of(members.begin(), members.end(), [](InstancePackSpawn const* member)
            {
                return member->spawnId == member->formationLeader;
            });
            if (!hasLeader || members.size() < _options.minimumSize || members.size() > _options.maximumSize)
                continue;

            InstancePackSpawn const* center = FindMedoid(members);
            _candidates.push_back({ mapId, members, "formation", Confidence(members, "formation", _options.distance),
                MaximumPairDistance(members), center->spawnId, MaximumCenterDistance(members, *center) });
            ++formationPackCount;
            for (InstancePackSpawn const* member : members)
                used.insert(member->spawnId);
        }

        std::map<Context, std::vector<InstancePackSpawn const*>> contexts;
        for (InstancePackSpawn const& spawn : _spawns)
            if (spawn.mapId == mapId && spawn.rejectionReason.empty() && !spawn.formationLeader && !used.contains(spawn.spawnId))
                contexts[{ spawn.mapId, spawn.zoneId, spawn.areaId, spawn.spawnMask, spawn.phaseMask }].push_back(&spawn);

        for (auto& [context, remaining] : contexts)
        {
            while (remaining.size() >= _options.minimumSize)
            {
                InstancePackSpawn const* bestCenter = nullptr;
                std::vector<InstancePackSpawn const*> bestMembers;
                float bestTotalDistance = std::numeric_limits<float>::max();

                for (InstancePackSpawn const* center : remaining)
                {
                    std::vector<InstancePackSpawn const*> members;
                    for (InstancePackSpawn const* member : remaining)
                        if (std::abs(center->z - member->z) <= _options.verticalDistance && Distance(*center, *member) <= _options.distance && HasLineOfSight(*center, *member))
                            members.push_back(member);

                    std::sort(members.begin(), members.end(), [center](InstancePackSpawn const* left, InstancePackSpawn const* right)
                    {
                        float const leftDistance = Distance(*center, *left);
                        float const rightDistance = Distance(*center, *right);
                        return std::tuple(leftDistance, -int32(left->rank), left->spawnId) < std::tuple(rightDistance, -int32(right->rank), right->spawnId);
                    });
                    if (members.size() > _options.maximumSize)
                        members.resize(_options.maximumSize);

                    float totalDistance = 0.0f;
                    for (InstancePackSpawn const* member : members)
                        totalDistance += Distance(*center, *member);

                    if (members.size() > bestMembers.size() ||
                        (members.size() == bestMembers.size() && (totalDistance < bestTotalDistance ||
                        (totalDistance == bestTotalDistance && (!bestCenter || center->spawnId < bestCenter->spawnId)))))
                    {
                        bestCenter = center;
                        bestMembers = std::move(members);
                        bestTotalDistance = totalDistance;
                    }
                }

                if (bestMembers.size() < _options.minimumSize)
                    break;

                std::sort(bestMembers.begin(), bestMembers.end(), [](InstancePackSpawn const* left, InstancePackSpawn const* right)
                {
                    return std::tuple(-int32(left->rank), left->spawnId) < std::tuple(-int32(right->rank), right->spawnId);
                });
                _candidates.push_back({ mapId, bestMembers, "spatial", Confidence(bestMembers, "spatial", _options.distance),
                    MaximumPairDistance(bestMembers), bestCenter->spawnId, MaximumCenterDistance(bestMembers, *bestCenter) });
                ++spatialPackCount;

                std::set<uint32> selected;
                for (InstancePackSpawn const* member : bestMembers)
                    selected.insert(member->spawnId);
                std::erase_if(remaining, [&selected](InstancePackSpawn const* member) { return selected.contains(member->spawnId); });
            }
        }

        std::size_t packMemberCount = 0;
        for (auto itr = _candidates.begin() + candidatesBeforeMap; itr != _candidates.end(); ++itr)
            packMemberCount += itr->members.size();
        std::cout << "Completed map " << mapId << ": " << formationPackCount + spatialPackCount << " packs ("
            << formationPackCount << " formation, " << spatialPackCount << " spatial), " << packMemberCount << " grouped spawns.\n";
    }

    std::sort(_candidates.begin(), _candidates.end(), [](InstancePackCandidate const& left, InstancePackCandidate const& right)
    {
        return std::pair(left.mapId, left.GetGroupId()) < std::pair(right.mapId, right.GetGroupId());
    });
}

VMAP::StaticMapTree* InstancePackGenerator::GetVMap(uint32 mapId)
{
    if (auto itr = _vmaps.find(mapId); itr != _vmaps.end())
        return itr->second.get();

    std::filesystem::path const vmapPath = _options.dataPath / "vmaps";
    std::cout << "Loading VMAP data for map " << mapId << " from " << vmapPath.generic_string() << ".\n";
    auto tree = std::make_unique<VMAP::StaticMapTree>(mapId, vmapPath.generic_string());
    if (!tree->InitMap(VMAP::VMapMgr2::getMapFileName(mapId)))
    {
        LOG_ERROR("instance_pack_generator", "Unable to load VMAP tree for map {} from {}.", mapId, vmapPath.generic_string());
        _vmaps.emplace(mapId, nullptr);
        return nullptr;
    }

    VMAP::StaticMapTree* result = tree.get();
    _vmaps.emplace(mapId, std::move(tree));
    return result;
}

bool InstancePackGenerator::HasLineOfSight(InstancePackSpawn const& center, InstancePackSpawn const& member)
{
    if (center.spawnId == member.spawnId)
        return true;

    VMAP::StaticMapTree* tree = GetVMap(center.mapId);
    if (!tree)
        return false;

    GridCoord const centerGrid = Acore::ComputeGridCoord(center.x, center.y);
    GridCoord const memberGrid = Acore::ComputeGridCoord(member.x, member.y);
    if (!tree->LoadMapTile(centerGrid.x_coord, centerGrid.y_coord) || !tree->LoadMapTile(memberGrid.x_coord, memberGrid.y_coord))
        return false;

    G3D::Vector3 const start = VMAP::VMapMgr2::convertPositionToInternalRep(center.x, center.y, center.z + LineOfSightHeight);
    G3D::Vector3 const end = VMAP::VMapMgr2::convertPositionToInternalRep(member.x, member.y, member.z + LineOfSightHeight);
    return tree->isInLineOfSight(start, end, VMAP::ModelIgnoreFlags::Nothing);
}

bool InstancePackGenerator::WriteCsv() const
{
    std::ofstream output(_options.csvPath);
    if (!output)
    {
        LOG_ERROR("instance_pack_generator", "Unable to open CSV output {}.", _options.csvPath.generic_string());
        return false;
    }

    output << "mapId,candidateGroupId,source,confidence,packSize,maximumDistance,centerSpawnId,maximumCenterDistance,activationOrder,spawnId,entry,name,zoneId,areaId,spawnMask,phaseMask,x,y,z\n";
    output << std::fixed;
    for (InstancePackCandidate const& candidate : _candidates)
    {
        for (std::size_t activationOrder = 0; activationOrder < candidate.members.size(); ++activationOrder)
        {
            InstancePackSpawn const& member = *candidate.members[activationOrder];
            output << candidate.mapId << ',' << candidate.GetGroupId() << ',' << candidate.source << ','
                << std::setprecision(3) << candidate.confidence << ',' << candidate.members.size() << ','
                << std::setprecision(2) << candidate.maximumDistance << ',' << candidate.centerSpawnId << ','
                << candidate.maximumCenterDistance << ',' << activationOrder << ',' << member.spawnId << ','
                << member.entry << ',' << CsvString(member.name) << ',' << member.zoneId << ',' << member.areaId << ','
                << member.spawnMask << ',' << member.phaseMask << ',' << std::setprecision(3) << member.x << ',' << member.y << ',' << member.z << '\n';
        }
    }

    std::cout << "Wrote review report: " << _options.csvPath.generic_string() << '\n';
    return true;
}

bool InstancePackGenerator::WriteSql() const
{
    std::ofstream output(_options.sqlPath);
    if (!output)
    {
        LOG_ERROR("instance_pack_generator", "Unable to open SQL output {}.", _options.sqlPath.generic_string());
        return false;
    }

    output << "-- Review generated packs before executing this file.\n\n" << std::fixed;
    for (InstancePackCandidate const& candidate : _candidates)
    {
        output << "-- map=" << candidate.mapId << " group=" << candidate.GetGroupId() << " source=" << candidate.source
            << " confidence=" << std::setprecision(3) << candidate.confidence << " size=" << candidate.members.size()
            << " spread=" << std::setprecision(2) << candidate.maximumDistance << " center=" << candidate.centerSpawnId
            << " radius=" << candidate.maximumCenterDistance << '\n';
        output << "INSERT INTO `rochenoire_scale_instance_spawn_group`\n"
            << "    (`spawnId`, `mapId`, `scalingGroupId`, `activationOrder`, `comment`) VALUES\n";
        for (std::size_t activationOrder = 0; activationOrder < candidate.members.size(); ++activationOrder)
        {
            InstancePackSpawn const& member = *candidate.members[activationOrder];
            std::string const comment = "review:" + candidate.source + ':' + std::to_string(candidate.confidence) + ' ' + member.name;
            output << "    (" << member.spawnId << ", " << candidate.mapId << ", " << candidate.GetGroupId() << ", "
                << activationOrder << ", " << SqlString(comment) << ')'
                << (activationOrder + 1 == candidate.members.size() ? ";\n\n" : ",\n");
        }
    }

    std::cout << "Wrote executable SQL: " << _options.sqlPath.generic_string() << '\n';
    return true;
}
