#!/usr/bin/env python3
"""Generate reviewable Rochenoire instance spawn-group candidates.

The utility reads configured instance creature spawns from a MySQL/MariaDB world
Database, conservatively clusters likely trash packs, and writes CSV and SQL
candidate files. The generated SQL contains executable inserts but the utility
never executes them or modifies the database itself.

Examples:
    python tools/generate_instance_spawn_groups.py
    python tools/generate_instance_spawn_groups.py --worldserver-conf=worldserver.conf
    python tools/generate_instance_spawn_groups.py --host 127.0.0.1 --user acore \
        --database acore_world --distance 10

By default, the utility searches common build output paths for worldserver.conf
and reads MySQLExecutable and WorldDatabaseInfo. The database password is passed
to the MySQL child process through its environment, never on its command line.
"""

from __future__ import annotations

import argparse
import csv
import math
import os
import pathlib
import shutil
import subprocess
import sys
from collections import defaultdict
from dataclasses import dataclass
from typing import Iterable


EXTRACTION_QUERY = r"""
SELECT
    c.guid,
    c.id,
    c.map,
    c.zoneId,
    c.areaId,
    c.spawnMask,
    c.phaseMask,
    c.position_x,
    c.position_y,
    c.position_z,
    REPLACE(REPLACE(ct.name, '\t', ' '), '\n', ' '),
    ct.rank,
    COALESCE(cf.leaderGUID, 0),
    COALESCE(existing.scalingGroupId, 0),
    CASE
        WHEN c.id = 0 THEN 'missing entry'
        WHEN ct.type NOT IN (1, 2, 3, 4, 5, 6, 7, 9) THEN 'unsafe creature type'
        WHEN ct.minlevel = 1 THEN 'level one'
        WHEN COALESCE(NULLIF(c.npcflag, 0), ct.npcflag) <> 0 THEN 'interactive NPC'
        WHEN (COALESCE(NULLIF(c.unit_flags, 0), ct.unit_flags) & 0x02000002) <> 0 THEN 'noncombat flags'
        WHEN ct.faction = 35 THEN 'friendly faction'
        WHEN ct.rank = 3 THEN 'world boss rank'
        WHEN (ct.type_flags & 0xA8040086) <> 0 THEN 'unsafe type flags'
        WHEN (ct.flags_extra & 0x0000A4C2) <> 0 THEN 'unsafe extra flags'
        WHEN COALESCE(c.ScriptName, '') <> '' OR COALESCE(ct.ScriptName, '') <> '' THEN 'scripted'
        WHEN ie.creditEntry IS NOT NULL THEN 'instance encounter credit'
        WHEN pc.guid IS NOT NULL THEN 'pool member'
        WHEN gec.guid IS NOT NULL THEN 'game event member'
        WHEN lr.guid IS NOT NULL OR reverseLr.linkedGuid IS NOT NULL THEN 'linked respawn'
        WHEN existing.spawnId IS NOT NULL THEN 'existing scaling group'
        ELSE ''
    END
FROM creature c
JOIN creature_template ct ON ct.entry = c.id
JOIN (
    SELECT DISTINCT mapId
    FROM rochenoire_scale_zone
    WHERE minimumPlayers > 0
      AND maximumPlayers >= minimumPlayers
) configuredMap ON configuredMap.mapId = c.map
LEFT JOIN creature_formations cf ON cf.memberGUID = c.guid
LEFT JOIN instance_encounters ie ON ie.creditType = 0 AND ie.creditEntry = c.id
LEFT JOIN pool_creature pc ON pc.guid = c.guid
LEFT JOIN (SELECT DISTINCT guid FROM game_event_creature) gec ON gec.guid = c.guid
LEFT JOIN linked_respawn lr ON lr.guid = c.guid
LEFT JOIN linked_respawn reverseLr ON reverseLr.linkedGuid = c.guid
LEFT JOIN rochenoire_scale_instance_spawn_group existing ON existing.spawnId = c.guid
ORDER BY c.map, c.guid
""".strip()


@dataclass(frozen=True)
class Spawn:
    spawn_id: int
    entry: int
    map_id: int
    zone_id: int
    area_id: int
    spawn_mask: int
    phase_mask: int
    x: float
    y: float
    z: float
    name: str
    rank: int
    formation_leader: int
    existing_group: int
    rejection_reason: str

    @property
    def context(self) -> tuple[int, int, int, int, int]:
        return self.map_id, self.zone_id, self.area_id, self.spawn_mask, self.phase_mask


@dataclass(frozen=True)
class CandidatePack:
    map_id: int
    members: tuple[Spawn, ...]
    source: str
    confidence: float
    maximum_distance: float
    center_spawn_id: int
    maximum_center_distance: float

    @property
    def group_id(self) -> int:
        return min(member.spawn_id for member in self.members)


@dataclass(frozen=True)
class DatabaseConfig:
    mysql: str
    database: str
    host: str | None = None
    port: int | None = None
    user: str | None = None
    password: str | None = None
    defaults_extra_file: str | None = None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate reviewable spatial candidates for rochenoire_scale_instance_spawn_group."
    )
    parser.add_argument("--worldserver-conf", type=pathlib.Path, help="Path to worldserver.conf")
    parser.add_argument("--database", default=os.environ.get("AC_WORLD_DATABASE"))
    parser.add_argument("--mysql", help="MySQL/MariaDB client executable")
    parser.add_argument("--defaults-extra-file", help="MySQL option file containing credentials")
    parser.add_argument("--host", help="Database host; omitted when supplied by the option file")
    parser.add_argument("--port", type=int, help="Database port")
    parser.add_argument("--user", help="Database user; use MYSQL_PWD or an option file for its password")
    parser.add_argument("--distance", type=float, default=12.0, help="Maximum 3D distance from the spatial pack center")
    parser.add_argument("--vertical-distance", type=float, default=5.0, help="Maximum vertical separation")
    parser.add_argument("--min-size", type=int, default=2, help="Minimum candidate pack size")
    parser.add_argument("--max-size", type=int, default=8, help="Maximum candidate pack size")
    parser.add_argument("--csv", type=pathlib.Path, default=pathlib.Path("instance_spawn_group_candidates.csv"))
    parser.add_argument("--sql", type=pathlib.Path, default=pathlib.Path("instance_spawn_group_candidates.sql"))
    return parser.parse_args()


def unquote_config_value(value: str) -> str:
    value = value.strip()
    if len(value) >= 2 and value[0] == value[-1] == '"':
        return value[1:-1]
    return value


def read_worldserver_config(path: pathlib.Path) -> dict[str, str]:
    settings: dict[str, str] = {}
    with path.open(encoding="utf-8-sig") as config_file:
        for raw_line in config_file:
            line = raw_line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, value = line.split("=", 1)
            settings[key.strip()] = unquote_config_value(value)
    return settings


def discover_worldserver_config() -> pathlib.Path | None:
    repository = pathlib.Path(__file__).resolve().parents[1]
    candidates = [
        repository / "worldserver.conf",
        repository / "bin" / "configs" / "worldserver.conf",
        repository / "out" / "bin" / "Debug" / "configs" / "worldserver.conf",
        repository / "out" / "bin" / "RelWithDebInfo" / "configs" / "worldserver.conf",
        repository / "out" / "bin" / "Release" / "configs" / "worldserver.conf",
    ]
    return next((candidate for candidate in candidates if candidate.is_file()), None)


def resolve_database_config(args: argparse.Namespace) -> DatabaseConfig:
    config_path = args.worldserver_conf or discover_worldserver_config()
    settings = read_worldserver_config(config_path) if config_path else {}

    connection = settings.get("WorldDatabaseInfo", "").split(";", 4)
    if connection and len(connection) != 5:
        raise RuntimeError(f"Invalid WorldDatabaseInfo in {config_path}")

    host, port, user, password, database = connection if len(connection) == 5 else (None, None, None, None, None)
    mysql = args.mysql or settings.get("MySQLExecutable") or "mysql"
    resolved_database = args.database or database or "acore_world"
    resolved_port = args.port or (int(port) if port else None)

    return DatabaseConfig(
        mysql=mysql,
        database=resolved_database,
        host=args.host or host,
        port=resolved_port,
        user=args.user or user,
        password=password,
        defaults_extra_file=args.defaults_extra_file,
    )


def find_medoid(members: Iterable[Spawn]) -> Spawn:
    member_list = list(members)
    if not member_list:
        raise ValueError("Cannot find the center of an empty pack")
    return min(
        member_list,
        key=lambda candidate: (
            sum(distance(candidate, member) for member in member_list),
            candidate.spawn_id,
        ),
    )


def maximum_center_distance(members: Iterable[Spawn], center: Spawn) -> float:
    return max((distance(center, member) for member in members), default=0.0)


def mysql_command(config: DatabaseConfig) -> list[str]:
    executable = shutil.which(config.mysql)
    if not executable:
        raise RuntimeError(f"MySQL client not found: {config.mysql}")

    command = [executable]
    if config.defaults_extra_file:
        command.append(f"--defaults-extra-file={config.defaults_extra_file}")
    if config.host:
        command.extend(["--host", config.host])
    if config.port:
        command.extend(["--port", str(config.port)])
    if config.user:
        command.extend(["--user", config.user])
    command.extend(["--batch", "--raw", "--skip-column-names", config.database])
    return command


def extract_spawns(args: argparse.Namespace) -> list[Spawn]:
    config = resolve_database_config(args)
    environment = os.environ.copy()
    if config.password and not config.defaults_extra_file:
        environment["MYSQL_PWD"] = config.password
    process = subprocess.run(
        mysql_command(config),
        input=EXTRACTION_QUERY,
        text=True,
        capture_output=True,
        check=False,
        env=environment,
    )
    if process.returncode:
        raise RuntimeError(process.stderr.strip() or "MySQL extraction failed")

    spawns: list[Spawn] = []
    for line_number, line in enumerate(process.stdout.splitlines(), 1):
        columns = line.split("\t")
        if len(columns) != 15:
            raise RuntimeError(f"Unexpected MySQL output at line {line_number}: {len(columns)} columns")
        spawns.append(
            Spawn(
                spawn_id=int(columns[0]),
                entry=int(columns[1]),
                map_id=int(columns[2]),
                zone_id=int(columns[3]),
                area_id=int(columns[4]),
                spawn_mask=int(columns[5]),
                phase_mask=int(columns[6]),
                x=float(columns[7]),
                y=float(columns[8]),
                z=float(columns[9]),
                name=columns[10],
                rank=int(columns[11]),
                formation_leader=int(columns[12]),
                existing_group=int(columns[13]),
                rejection_reason=columns[14],
            )
        )
    return spawns


def distance(left: Spawn, right: Spawn) -> float:
    return math.dist((left.x, left.y, left.z), (right.x, right.y, right.z))


def maximum_pair_distance(members: Iterable[Spawn]) -> float:
    member_list = list(members)
    return max(
        (distance(left, right) for index, left in enumerate(member_list) for right in member_list[index + 1 :]),
        default=0.0,
    )


def pack_confidence(members: tuple[Spawn, ...], source: str, distance_limit: float) -> float:
    spread = maximum_pair_distance(members)
    compactness = max(0.0, 1.0 - spread / max(distance_limit * 2.0, 0.01))
    formation_bonus = 0.25 if source == "formation" else 0.0
    size_bonus = min(len(members), 5) * 0.03
    entry_diversity_penalty = 0.08 if len({member.entry for member in members}) == len(members) else 0.0
    return max(0.0, min(1.0, 0.45 + compactness * 0.35 + formation_bonus + size_bonus - entry_diversity_penalty))


def connected_components(spawns: list[Spawn], distance_limit: float, vertical_limit: float) -> list[list[Spawn]]:
    neighbors: dict[int, list[int]] = defaultdict(list)
    cell_size = distance_limit
    cells: dict[tuple[int, int, int], list[int]] = defaultdict(list)

    for index, spawn in enumerate(spawns):
        cell = (math.floor(spawn.x / cell_size), math.floor(spawn.y / cell_size), math.floor(spawn.z / cell_size))
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                for dz in (-1, 0, 1):
                    for other_index in cells.get((cell[0] + dx, cell[1] + dy, cell[2] + dz), ()):
                        other = spawns[other_index]
                        if abs(spawn.z - other.z) <= vertical_limit and distance(spawn, other) <= distance_limit:
                            neighbors[index].append(other_index)
                            neighbors[other_index].append(index)
        cells[cell].append(index)

    components: list[list[Spawn]] = []
    visited: set[int] = set()
    for index in range(len(spawns)):
        if index in visited:
            continue
        stack = [index]
        visited.add(index)
        component: list[Spawn] = []
        while stack:
            current = stack.pop()
            component.append(spawns[current])
            for neighbor in neighbors.get(current, ()):
                if neighbor not in visited:
                    visited.add(neighbor)
                    stack.append(neighbor)
        components.append(component)
    return components


def medoid_radius_clusters(
    spawns: list[Spawn], distance_limit: float, vertical_limit: float, min_size: int, max_size: int
) -> list[tuple[Spawn, tuple[Spawn, ...]]]:
    remaining = list(spawns)
    clusters: list[tuple[Spawn, tuple[Spawn, ...]]] = []

    while len(remaining) >= min_size:
        center_candidates: list[tuple[int, float, int, Spawn, list[Spawn]]] = []
        for center in remaining:
            members = [
                member
                for member in remaining
                if abs(center.z - member.z) <= vertical_limit and distance(center, member) <= distance_limit
            ]
            members.sort(key=lambda member: (distance(center, member), -member.rank, member.spawn_id))
            selected = members[:max_size]
            center_candidates.append(
                (-len(selected), sum(distance(center, member) for member in selected), center.spawn_id, center, selected)
            )

        _, _, _, center, selected = min(center_candidates)
        if len(selected) < min_size:
            break

        ordered = tuple(sorted(selected, key=lambda member: (-member.rank, member.spawn_id)))
        clusters.append((center, ordered))
        selected_ids = {member.spawn_id for member in selected}
        remaining = [member for member in remaining if member.spawn_id not in selected_ids]

    return clusters


def build_candidates(spawns: list[Spawn], args: argparse.Namespace) -> list[CandidatePack]:
    eligible = [spawn for spawn in spawns if not spawn.rejection_reason]
    used: set[int] = set()
    candidates: list[CandidatePack] = []

    formations: dict[tuple[int, int], list[Spawn]] = defaultdict(list)
    for spawn in eligible:
        if spawn.formation_leader:
            formations[(spawn.map_id, spawn.formation_leader)].append(spawn)
    for members in formations.values():
        ordered = tuple(sorted(members, key=lambda member: (member.spawn_id != member.formation_leader, -member.rank, member.spawn_id)))
        if args.min_size <= len(ordered) <= args.max_size and any(member.spawn_id == member.formation_leader for member in ordered):
            center = find_medoid(ordered)
            candidates.append(
                CandidatePack(
                    map_id=ordered[0].map_id,
                    members=ordered,
                    source="formation",
                    confidence=pack_confidence(ordered, "formation", args.distance),
                    maximum_distance=maximum_pair_distance(ordered),
                    center_spawn_id=center.spawn_id,
                    maximum_center_distance=maximum_center_distance(ordered, center),
                )
            )
            used.update(member.spawn_id for member in ordered)

    contexts: dict[tuple[int, int, int, int, int], list[Spawn]] = defaultdict(list)
    for spawn in eligible:
        if spawn.spawn_id not in used and not spawn.formation_leader:
            contexts[spawn.context].append(spawn)

    for context_spawns in contexts.values():
        for component in connected_components(context_spawns, args.distance, args.vertical_distance):
            for center, ordered in medoid_radius_clusters(
                component, args.distance, args.vertical_distance, args.min_size, args.max_size
            ):
                candidates.append(
                    CandidatePack(
                        map_id=ordered[0].map_id,
                        members=ordered,
                        source="spatial",
                        confidence=pack_confidence(ordered, "spatial", args.distance),
                        maximum_distance=maximum_pair_distance(ordered),
                        center_spawn_id=center.spawn_id,
                        maximum_center_distance=maximum_center_distance(ordered, center),
                    )
                )

    return sorted(candidates, key=lambda candidate: (candidate.map_id, candidate.group_id))


def sql_string(value: str) -> str:
    return "'" + value.replace("\\", "\\\\").replace("'", "''") + "'"


def write_csv(path: pathlib.Path, candidates: list[CandidatePack]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output)
        writer.writerow(
            [
                "mapId",
                "candidateGroupId",
                "source",
                "confidence",
                "packSize",
                "maximumDistance",
                "centerSpawnId",
                "maximumCenterDistance",
                "activationOrder",
                "spawnId",
                "entry",
                "name",
                "zoneId",
                "areaId",
                "spawnMask",
                "phaseMask",
                "x",
                "y",
                "z",
            ]
        )
        for candidate in candidates:
            for activation_order, member in enumerate(candidate.members):
                writer.writerow(
                    [
                        candidate.map_id,
                        candidate.group_id,
                        candidate.source,
                        f"{candidate.confidence:.3f}",
                        len(candidate.members),
                        f"{candidate.maximum_distance:.2f}",
                        candidate.center_spawn_id,
                        f"{candidate.maximum_center_distance:.2f}",
                        activation_order,
                        member.spawn_id,
                        member.entry,
                        member.name,
                        member.zone_id,
                        member.area_id,
                        member.spawn_mask,
                        member.phase_mask,
                        f"{member.x:.3f}",
                        f"{member.y:.3f}",
                        f"{member.z:.3f}",
                    ]
                )


def write_sql(path: pathlib.Path, candidates: list[CandidatePack]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="\n") as output:
        output.write("-- Review generated packs before executing this file.\n\n")
        for candidate in candidates:
            output.write(
                f"-- map={candidate.map_id} group={candidate.group_id} source={candidate.source} "
                f"confidence={candidate.confidence:.3f} size={len(candidate.members)} "
                f"spread={candidate.maximum_distance:.2f} center={candidate.center_spawn_id} "
                f"radius={candidate.maximum_center_distance:.2f}\n"
            )
            output.write("INSERT INTO `rochenoire_scale_instance_spawn_group`\n")
            output.write("    (`spawnId`, `mapId`, `scalingGroupId`, `activationOrder`, `comment`) VALUES\n")
            rows = []
            for activation_order, member in enumerate(candidate.members):
                comment = f"auto:{candidate.source}:{candidate.confidence:.3f} {member.name}"
                rows.append(
                    f"    ({member.spawn_id}, {candidate.map_id}, {candidate.group_id}, "
                    f"{activation_order}, {sql_string(comment)})"
                )
            output.write(",\n".join(rows))
            output.write(";\n\n")


def main() -> int:
    args = parse_args()
    if args.distance <= 0 or args.vertical_distance < 0:
        raise ValueError("Distance limits must be positive")
    if args.min_size < 2 or args.max_size < args.min_size:
        raise ValueError("Pack size limits are invalid")

    spawns = extract_spawns(args)
    candidates = build_candidates(spawns, args)
    write_csv(args.csv, candidates)
    write_sql(args.sql, candidates)
    print(f"Extracted {len(spawns)} configured-instance spawns and found {len(candidates)} candidate packs.")
    print(f"Wrote review report: {args.csv}")
    print(f"Wrote commented SQL: {args.sql}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1)
