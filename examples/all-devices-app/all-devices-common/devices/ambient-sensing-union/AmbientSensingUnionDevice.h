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
#include <app/clusters/identify-server/IdentifyCluster.h>
#include <app/clusters/occupancy-sensor-server/OccupancySensingCluster.h>
#include <data-model-providers/codedriven/CodeDrivenDataModelProvider.h>
#include <devices/interface/SingleEndpointDevice.h>
#include <lib/support/TimerDelegate.h>

namespace chip::app {

/**
 * @brief Base device class for Ambient Sensing Union (device type 0x0151).
 *
 * Handles cluster instantiation and lifetime only. Delegate logic is
 * intentionally absent — subclasses (e.g. LoggingAmbientSensingUnionDevice)
 * implement and attach the delegate, following the same pattern as
 * OccupancySensorDevice / LoggingOccupancySensorDevice.
 *
 * Mandatory clusters (code-driven available):
 *   - Identify
 *   - Ambient Sensing Union
 *
 * Optional clusters (code-driven available):
 *   - Occupancy Sensing
 *
 * Missing (not yet code-driven in SDK):
 *   - Service Area (mandatory per spec)
 *   - Ambient Context Sensing (optional)
 *   - Boolean State Configuration (optional)
 */
class AmbientSensingUnionDevice : public SingleEndpointDevice
{
public:
    explicit AmbientSensingUnionDevice(TimerDelegate & timerDelegate);
    ~AmbientSensingUnionDevice() override = default;

    CHIP_ERROR Register(chip::EndpointId endpoint, CodeDrivenDataModelProvider & provider,
                        EndpointId parentId = kInvalidEndpointId) override;
    void Unregister(CodeDrivenDataModelProvider & provider) override;

    ::chip::app::Clusters::AmbientSensingUnionCluster & GetAmbientSensingUnionCluster();
    ::chip::app::Clusters::OccupancySensingCluster & GetOccupancySensingCluster();

protected:
    TimerDelegate & mTimerDelegate;
    LazyRegisteredServerCluster<::chip::app::Clusters::IdentifyCluster> mIdentifyCluster;
    LazyRegisteredServerCluster<::chip::app::Clusters::AmbientSensingUnionCluster> mAmbientSensingUnionCluster;
    LazyRegisteredServerCluster<::chip::app::Clusters::OccupancySensingCluster> mOccupancySensingCluster;
};

} // namespace chip::app
