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

/**
 * @file DeviceAppShell.cpp
 *
 * Shell commands for driving device-type-specific cluster attributes on the
 * NXP FreeRTOS all-devices app.  These commands mirror the functionality of
 * the posix named-pipe AppCommandDelegate but are exposed through the CHIP
 * shell instead of a JSON pipe.
 *
 * All commands are grouped under the "app" top-level shell command:
 *
 *   app occupancy <0|1>        -- set OccupancySensing::Occupancy
 *   app holdtime  <seconds>    -- set OccupancySensing::HoldTime
 *   app boolstate <0|1>        -- set BooleanState::StateValue
 *   app onoff     <0|1>        -- set OnOff::OnOff
 *
 * The correct sub-command set is available regardless of which device type is
 * running; invalid commands for the current device type print an error.
 */

#include "AllDevicesServer.h"

#ifdef ENABLE_CHIP_SHELL

#include <app/clusters/boolean-state-server/BooleanStateCluster.h>
#include <app/clusters/occupancy-sensor-server/OccupancySensingCluster.h>
#include <app/clusters/on-off-server/OnOffCluster.h>
#include <device/capabilities/on-off-load/OnOffLoad.h>
#include <device/types/boolean-state-sensor/BooleanStateSensor.h>
#include <device/types/occupancy-sensor/OccupancySensor.h>
#include <device/types/on-off-light/impl/LoggingOnOffLight.h>
#include <lib/shell/Engine.h>
#include <lib/shell/commands/Help.h>
#include <platform/CHIPDeviceLayer.h>

#include <stdlib.h>

using namespace chip::Shell;
using namespace chip::app::all_devices;

chip::Shell::Engine sShellAppSubCommands;

namespace {

// ---------------------------------------------------------------------------
// Occupancy
// ---------------------------------------------------------------------------

struct SetOccupancyArgs
{
    bool occupied;
};

void SetOccupancyWork(intptr_t arg)
{
    auto * args    = reinterpret_cast<SetOccupancyArgs *>(arg);
    bool occupied  = args->occupied;
    chip::Platform::Delete(args);

    auto * device = GetConstructedDevice();
    if (device == nullptr)
    {
        ChipLogError(Shell, "app occupancy: server not started");
        return;
    }
    auto * occupancyDevice = static_cast<chip::app::OccupancySensor *>(device);
    occupancyDevice->OccupancySensingCluster().SetOccupancy(occupied);
    ChipLogProgress(Shell, "app occupancy: set to %d", static_cast<int>(occupied));
}

static CHIP_ERROR HandleAppOccupancy(int argc, char ** argv)
{
    if (argc != 1)
    {
        ChipLogError(Shell, "Usage: app occupancy <0|1>");
        return CHIP_ERROR_INVALID_ARGUMENT;
    }
    const std::string & devType = GetDeviceType();
    if (devType != "occupancy-sensor")
    {
        ChipLogError(Shell, "app occupancy: not supported for device type '%s'", devType.c_str());
        return CHIP_ERROR_INCORRECT_STATE;
    }

    auto * args    = chip::Platform::New<SetOccupancyArgs>();
    args->occupied = (atoi(argv[0]) != 0);
    return chip::DeviceLayer::PlatformMgr().ScheduleWork(SetOccupancyWork, reinterpret_cast<intptr_t>(args));
}

// ---------------------------------------------------------------------------
// HoldTime
// ---------------------------------------------------------------------------

struct SetHoldTimeArgs
{
    uint16_t holdTime;
};

void SetHoldTimeWork(intptr_t arg)
{
    auto * args      = reinterpret_cast<SetHoldTimeArgs *>(arg);
    uint16_t holdTime = args->holdTime;
    chip::Platform::Delete(args);

    auto * device = GetConstructedDevice();
    if (device == nullptr)
    {
        ChipLogError(Shell, "app holdtime: server not started");
        return;
    }
    auto * occupancyDevice = static_cast<chip::app::OccupancySensor *>(device);
    occupancyDevice->OccupancySensingCluster().SetHoldTime(holdTime);
    ChipLogProgress(Shell, "app holdtime: set to %u", holdTime);
}

static CHIP_ERROR HandleAppHoldTime(int argc, char ** argv)
{
    if (argc != 1)
    {
        ChipLogError(Shell, "Usage: app holdtime <seconds>");
        return CHIP_ERROR_INVALID_ARGUMENT;
    }
    const std::string & devType = GetDeviceType();
    if (devType != "occupancy-sensor")
    {
        ChipLogError(Shell, "app holdtime: not supported for device type '%s'", devType.c_str());
        return CHIP_ERROR_INCORRECT_STATE;
    }

    long val = strtol(argv[0], nullptr, 10);
    if (val < 0 || val > 0xFFFF)
    {
        ChipLogError(Shell, "app holdtime: value out of range");
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    auto * args    = chip::Platform::New<SetHoldTimeArgs>();
    args->holdTime = static_cast<uint16_t>(val);
    return chip::DeviceLayer::PlatformMgr().ScheduleWork(SetHoldTimeWork, reinterpret_cast<intptr_t>(args));
}

// ---------------------------------------------------------------------------
// BooleanState
// ---------------------------------------------------------------------------

struct SetBoolStateArgs
{
    bool state;
};

void SetBoolStateWork(intptr_t arg)
{
    auto * args = reinterpret_cast<SetBoolStateArgs *>(arg);
    bool state  = args->state;
    chip::Platform::Delete(args);

    auto * device = GetConstructedDevice();
    if (device == nullptr)
    {
        ChipLogError(Shell, "app boolstate: server not started");
        return;
    }
    auto * boolDevice = static_cast<chip::app::BooleanStateSensor *>(device);
    boolDevice->BooleanState().SetStateValue(state);
    ChipLogProgress(Shell, "app boolstate: set to %d", static_cast<int>(state));
}

static CHIP_ERROR HandleAppBoolState(int argc, char ** argv)
{
    if (argc != 1)
    {
        ChipLogError(Shell, "Usage: app boolstate <0|1>");
        return CHIP_ERROR_INVALID_ARGUMENT;
    }
    const std::string & devType = GetDeviceType();
    if (devType != "contact-sensor" && devType != "water-leak-detector")
    {
        ChipLogError(Shell, "app boolstate: not supported for device type '%s'", devType.c_str());
        return CHIP_ERROR_INCORRECT_STATE;
    }

    auto * args = chip::Platform::New<SetBoolStateArgs>();
    args->state = (atoi(argv[0]) != 0);
    return chip::DeviceLayer::PlatformMgr().ScheduleWork(SetBoolStateWork, reinterpret_cast<intptr_t>(args));
}

// ---------------------------------------------------------------------------
// OnOff
// ---------------------------------------------------------------------------

struct SetOnOffArgs
{
    bool onOff;
};

void SetOnOffWork(intptr_t arg)
{
    auto * args = reinterpret_cast<SetOnOffArgs *>(arg);
    bool onOff  = args->onOff;
    chip::Platform::Delete(args);

    auto * device = GetConstructedDevice();
    if (device == nullptr)
    {
        ChipLogError(Shell, "app onoff: server not started");
        return;
    }
    auto * lightDevice = static_cast<chip::app::LoggingOnOffLight *>(device);
    CHIP_ERROR err     = lightDevice->OnOffCluster().SetOnOff(onOff);
    ChipLogProgress(Shell, "app onoff: set to %d: %" CHIP_ERROR_FORMAT, static_cast<int>(onOff), err.Format());
}

static CHIP_ERROR HandleAppOnOff(int argc, char ** argv)
{
    if (argc != 1)
    {
        ChipLogError(Shell, "Usage: app onoff <0|1>");
        return CHIP_ERROR_INVALID_ARGUMENT;
    }
    const std::string & devType = GetDeviceType();
    if (devType != "on-off-light" && devType != "on-off-plug-in-unit" && devType != "dimmable-light")
    {
        ChipLogError(Shell, "app onoff: not supported for device type '%s'", devType.c_str());
        return CHIP_ERROR_INCORRECT_STATE;
    }

    auto * args = chip::Platform::New<SetOnOffArgs>();
    args->onOff = (atoi(argv[0]) != 0);
    return chip::DeviceLayer::PlatformMgr().ScheduleWork(SetOnOffWork, reinterpret_cast<intptr_t>(args));
}

// ---------------------------------------------------------------------------
// Top-level dispatcher
// ---------------------------------------------------------------------------

static CHIP_ERROR PrintAppHelp(int argc, char ** argv)
{
    sShellAppSubCommands.ForEachCommand(PrintCommandHelp, nullptr);
    return CHIP_NO_ERROR;
}

static CHIP_ERROR HandleAppCmd(int argc, char ** argv)
{
    if (argc == 0)
    {
        return PrintAppHelp(argc, argv);
    }
    return sShellAppSubCommands.ExecCommand(argc, argv);
}

} // namespace

static const shell_command_t sAppSubCommands[] = {
    { HandleAppOccupancy, "occupancy", "Set occupancy: app occupancy <0|1>" },
    { HandleAppHoldTime,  "holdtime",  "Set hold time: app holdtime <seconds>" },
    { HandleAppBoolState, "boolstate", "Set boolean state: app boolstate <0|1>" },
    { HandleAppOnOff,     "onoff",     "Set on/off: app onoff <0|1>" },
};

static const shell_command_t sAppCommand = { HandleAppCmd, "app", "Device attribute control commands" };

void RegisterDeviceAppShellCommands()
{
    sShellAppSubCommands.RegisterCommands(sAppSubCommands, static_cast<size_t>(sizeof(sAppSubCommands) / sizeof(sAppSubCommands[0])));
    Engine::Root().RegisterCommands(&sAppCommand, 1);
}

#endif // ENABLE_CHIP_SHELL

