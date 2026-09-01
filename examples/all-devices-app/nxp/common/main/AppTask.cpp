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

#include "AppTask.h"

#ifdef ENABLE_CHIP_SHELL
// Forward-declare the registration function defined in DeviceTypeShell.cpp.
void RegisterDevTypeCommands();
#endif

using namespace chip;

void AllDevicesApp::AppTask::PreInitMatterStack()
{
    ChipLogProgress(DeviceLayer, "Welcome to NXP All-Devices App");
}

void AllDevicesApp::AppTask::AppMatter_RegisterCustomCliCommands()
{
#ifdef ENABLE_CHIP_SHELL
    RegisterDevTypeCommands();
#endif

}

AllDevicesApp::AppTask & AllDevicesApp::AppTask::GetDefaultInstance()
{
    static AllDevicesApp::AppTask sAppTask;
    return sAppTask;
}

chip::NXP::App::AppTaskBase & chip::NXP::App::GetAppTask()
{
    return AllDevicesApp::AppTask::GetDefaultInstance();
}
