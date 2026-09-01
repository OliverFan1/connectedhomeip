/*
 *
 *    Copyright (c) 2026 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "AllDevicesServer.h"

#ifdef ENABLE_CHIP_SHELL

#include <device-factory/DeviceFactory.h>
#include <lib/shell/Engine.h>
#include <lib/shell/commands/Help.h>
#include <platform/CHIPDeviceLayer.h>

#include <string>

using namespace chip::Shell;

// Declared before the anonymous namespace so it is visible to functions within it.
chip::Shell::Engine sShellDevTypeSubCommands;


namespace {

std::string gRequestedDeviceType;
bool gStartRequested = false;

void StartAllDevicesServerWork(intptr_t)
{
    CHIP_ERROR err = chip::app::all_devices::StartAllDevicesServer(gRequestedDeviceType);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(AppServer, "Failed to start all-devices server: %" CHIP_ERROR_FORMAT, err.Format());
        gStartRequested = false;
    }
}

static CHIP_ERROR PrintDevTypeHelp(int argc, char ** argv)
{
    sShellDevTypeSubCommands.ForEachCommand(PrintCommandHelp, nullptr);
    return CHIP_NO_ERROR;
}

static CHIP_ERROR HandleDevTypeSet(int argc, char ** argv)
{
    if (argc != 1)
    {
        ChipLogError(Shell, "Usage: devtype set <device-type>");
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    if (chip::app::all_devices::IsServerStarted())
    {
        ChipLogError(Shell, "Server already started. Factory reset required to change device type.");
        return CHIP_ERROR_INCORRECT_STATE;
    }

    if (gStartRequested)
    {
        ChipLogError(Shell, "Server start is already scheduled.");
        return CHIP_ERROR_INCORRECT_STATE;
    }

    const std::string deviceType(argv[0]);
    auto & factory = chip::app::DeviceFactory::GetInstance();

    if (!factory.IsValidDevice(deviceType))
    {
        ChipLogError(Shell, "Invalid device type: %s", deviceType.c_str());
        ChipLogError(Shell, "Supported device types:");
        for (const auto & item : factory.SupportedDeviceTypes())
        {
            ChipLogError(Shell, "  %s", item.c_str());
        }
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    gRequestedDeviceType = deviceType;
    gStartRequested      = true;

    CHIP_ERROR err = chip::DeviceLayer::PlatformMgr().ScheduleWork(StartAllDevicesServerWork, 0);
    if (err != CHIP_NO_ERROR)
    {
        gStartRequested = false;
        ChipLogError(Shell, "Failed to schedule server start: %" CHIP_ERROR_FORMAT, err.Format());
        return err;
    }

    ChipLogProgress(Shell, "Starting device type: %s", deviceType.c_str());
    return CHIP_NO_ERROR;
}

static CHIP_ERROR HandleDevTypeCmd(int argc, char ** argv)
{
    if (argc == 0)
    {
        return PrintDevTypeHelp(argc, argv);
    }
    return sShellDevTypeSubCommands.ExecCommand(argc, argv);
}

} // namespace

static const shell_command_t sDevTypeSetCommand  = { HandleDevTypeSet, "set",
                                                     "Set device type: devtype set <device-type>" };

static const shell_command_t sDevTypeCommand = { HandleDevTypeCmd, "devtype", "All-devices device type commands" };

void RegisterDevTypeCommands()
{
    sShellDevTypeSubCommands.RegisterCommands(&sDevTypeSetCommand, 1);
    Engine::Root().RegisterCommands(&sDevTypeCommand, 1);
}

#endif // ENABLE_CHIP_SHELL
