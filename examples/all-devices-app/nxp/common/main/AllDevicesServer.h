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

#pragma once

#include <app/server/Server.h>
#include <device/api/Interface.h>
#include <lib/core/CHIPError.h>

#include <string>

namespace chip {
namespace app {
namespace all_devices {

/**
 * Returns true if the all-devices server has been started (i.e. a device type
 * has been selected and Server::Init has been called).
 */
bool IsServerStarted();

/**
 * Returns a pointer to the constructed device, or nullptr if the server has
 * not been started yet.
 */
DeviceInterface * GetConstructedDevice();

/**
 * Returns the currently active device type string, or an empty string if the
 * server has not been started yet.
 */
const std::string & GetDeviceType();

/**
 * Initializes the all-devices server infrastructure.
 *
 * If a device type was previously persisted in storage, the server is started
 * immediately. Otherwise the server waits for a call to StartAllDevicesServer.
 *
 * Must be called from the Matter event loop (e.g. via PlatformMgr().ScheduleWork).
 */
CHIP_ERROR InitAllDevicesServer(CommonCaseDeviceServerInitParams & initParams);

/**
 * Starts the all-devices server with the given device type string and persists
 * the selection so the same device type is restored on reboot.
 *
 * Must be called from the Matter event loop.
 */
CHIP_ERROR StartAllDevicesServer(const std::string & deviceType);

} // namespace all_devices
} // namespace app
} // namespace chip
