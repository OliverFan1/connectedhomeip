/*
 *
 *    Copyright (c) 2026 Project CHIP Authors
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

#include <app/clusters/ambient-sensing-union-server/AmbientSensingUnionCluster.h>
#include <data-model-providers/codedriven/CodeDrivenDataModelProvider.h>
#include <devices/ambient-sensing-union/AmbientSensingUnionDevice.h>
#include <lib/support/TimerDelegate.h>

namespace chip {
namespace app {

/**
 * @brief A basic implementation of an Ambient Sensing Union Device.
 *
 * Extends AmbientSensingUnionDevice with AmbientSensingUnionDelegate to log
 * union name and health changes. Mirrors the pattern of
 * LoggingOccupancySensorDevice relative to OccupancySensorDevice.
 */
class LoggingAmbientSensingUnionDevice
    : public AmbientSensingUnionDevice,
      public Clusters::AmbientSensingUnionDelegate
{
public:
    explicit LoggingAmbientSensingUnionDevice(TimerDelegate & timerDelegate);
    ~LoggingAmbientSensingUnionDevice() override = default;

    CHIP_ERROR Register(EndpointId endpoint, CodeDrivenDataModelProvider & provider,
                        EndpointId parentId = kInvalidEndpointId) override;
    void Unregister(CodeDrivenDataModelProvider & provider) override;

    // AmbientSensingUnionDelegate
    void OnUnionNameChanged(const chip::CharSpan & unionName) override;
    void OnUnionHealthChanged(
        Clusters::AmbientSensingUnion::UnionHealthEnum unionHealth) override;
};

} // namespace app
} // namespace chip

