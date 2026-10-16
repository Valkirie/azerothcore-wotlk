/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef INSTANCE_PACK_GENERATOR_H
#define INSTANCE_PACK_GENERATOR_H

#include "Define.h"
#include "MapTree.h"
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct InstancePackSpawn
{
    uint32 spawnId = 0;
    uint32 entry = 0;
    uint32 mapId = 0;
    uint32 zoneId = 0;
    uint32 areaId = 0;
    uint32 spawnMask = 0;
    uint32 phaseMask = 0;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    std::string name;
    uint8 rank = 0;
    uint32 formationLeader = 0;
    uint32 existingGroup = 0;
    std::string rejectionReason;
};

struct InstancePackCandidate
{
    uint32 mapId = 0;
    std::vector<InstancePackSpawn const*> members;
    std::string source;
    float confidence = 0.0f;
    float maximumDistance = 0.0f;
    uint32 centerSpawnId = 0;
    float maximumCenterDistance = 0.0f;

    uint32 GetGroupId() const;
};

struct InstancePackGeneratorOptions
{
    std::filesystem::path dataPath;
    std::filesystem::path csvPath = "instance_spawn_group_candidates.csv";
    std::filesystem::path sqlPath = "instance_spawn_group_candidates.sql";
    float distance = 12.0f;
    float verticalDistance = 5.0f;
    uint32 minimumSize = 2;
    uint32 maximumSize = 8;
    std::optional<uint32> mapId;
};

class InstancePackGenerator
{
public:
    explicit InstancePackGenerator(InstancePackGeneratorOptions options);
    bool Run();

private:
    bool LoadSpawns();
    void BuildCandidates();
    VMAP::StaticMapTree* GetVMap(uint32 mapId);
    bool HasLineOfSight(InstancePackSpawn const& center, InstancePackSpawn const& member);
    bool WriteCsv() const;
    bool WriteSql() const;

    InstancePackGeneratorOptions _options;
    std::vector<InstancePackSpawn> _spawns;
    std::vector<InstancePackCandidate> _candidates;
    std::map<uint32, std::unique_ptr<VMAP::StaticMapTree>> _vmaps;
};

#endif
