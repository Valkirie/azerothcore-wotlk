/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "Config.h"
#include "DatabaseEnv.h"
#include "DatabaseLoader.h"
#include "InstancePackGenerator.h"
#include "Log.h"
#include "MySQLThreading.h"
#include <boost/program_options.hpp>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
using namespace boost::program_options;

namespace
{
bool StartDatabase()
{
    MySQL::Library_Init();
    DatabaseLoader loader("instance_pack_generator", DatabaseLoader::DATABASE_NONE);
    loader.AddDatabase(WorldDatabase, "World");
    return loader.Load();
}

void StopDatabase()
{
    WorldDatabase.Close();
    MySQL::Library_End();
}
}

int main(int argc, char** argv)
{
    fs::path configFile = fs::path(sConfigMgr->GetConfigPath() + "worldserver.conf");
    InstancePackGeneratorOptions options;

    options_description arguments("Allowed options");
    arguments.add_options()
        ("help,h", "print usage message")
        ("config,c", value<fs::path>(&configFile)->default_value(configFile), "worldserver configuration file")
        ("data-path", value<fs::path>(&options.dataPath), "directory containing the vmaps directory; defaults to DataDir")
        ("distance", value<float>(&options.distance)->default_value(options.distance), "maximum 3D distance from the spatial pack center")
        ("vertical-distance", value<float>(&options.verticalDistance)->default_value(options.verticalDistance), "maximum vertical separation")
        ("min-size", value<uint32>(&options.minimumSize)->default_value(options.minimumSize), "minimum candidate pack size")
        ("max-size", value<uint32>(&options.maximumSize)->default_value(options.maximumSize), "maximum candidate pack size")
        ("map", value<uint32>(), "limit generation to one map ID")
        ("csv", value<fs::path>(&options.csvPath), "CSV output path; defaults beside the executable")
        ("sql", value<fs::path>(&options.sqlPath), "SQL output path; defaults beside the executable");

    variables_map variables;
    try
    {
        store(command_line_parser(argc, argv).options(arguments).run(), variables);
        notify(variables);
    }
    catch (std::exception const& exception)
    {
        std::cerr << exception.what() << '\n' << arguments << '\n';
        return 1;
    }

    if (variables.count("help"))
    {
        std::cout << arguments << '\n';
        return 0;
    }

    if (variables.count("map"))
        options.mapId = variables["map"].as<uint32>();

    fs::path const executableDirectory = fs::absolute(argv[0]).parent_path();
    if (!variables.count("csv"))
        options.csvPath = executableDirectory / options.csvPath;
    if (!variables.count("sql"))
        options.sqlPath = executableDirectory / options.sqlPath;

    sConfigMgr->Configure(configFile.generic_string(), std::vector<std::string>(argv, argv + argc));
    if (!sConfigMgr->LoadAppConfigs())
        return 1;

    sLog->Initialize();

    if (options.dataPath.empty())
        options.dataPath = sConfigMgr->GetOption<std::string>("DataDir", ".");
    if (options.dataPath.is_relative())
    {
        fs::path const executableDataPath = executableDirectory / options.dataPath;
        fs::path const configDataPath = configFile.parent_path().parent_path() / options.dataPath;
        options.dataPath = fs::exists(executableDataPath) ? executableDataPath : configDataPath;
    }

    if (options.distance <= 0.0f || options.verticalDistance < 0.0f || options.minimumSize < 2 || options.maximumSize < options.minimumSize)
    {
        LOG_ERROR("instance_pack_generator", "Invalid distance or pack-size limits.");
        return 1;
    }

    if (!StartDatabase())
        return 1;

    bool const success = InstancePackGenerator(std::move(options)).Run();
    StopDatabase();
    return success ? 0 : 1;
}
