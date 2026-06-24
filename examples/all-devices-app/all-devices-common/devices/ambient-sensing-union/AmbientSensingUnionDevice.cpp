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

#include "AmbientSensingUnionDevice.h"

namespace chip {
namespace app {

AmbientSensingUnionDevice::AmbientSensingUnionDevice(TimerDelegate & timerDelegate) :
    mTimerDelegate(timerDelegate)
{}

CHIP_ERROR AmbientSensingUnionDevice::Register(chip::EndpointId endpoint, CodeDrivenDataModelProvider & provider,
                                               EndpointId parentId)
{
    using namespace chip::app::Clusters;

    ReturnErrorOnFailure(mIdentifyCluster.Create(
        IdentifyCluster::Config(endpoint), provider, parentId));

    ReturnErrorOnFailure(mAmbientSensingUnionCluster.Create(
        AmbientSensingUnionCluster::Config(endpoint), provider, parentId));

    return CHIP_NO_ERROR;
}

void AmbientSensingUnionDevice::Unregister(CodeDrivenDataModelProvider & provider)
{
    if (mOccupancySensingCluster.IsConstructed())
    {
        mOccupancySensingCluster.Destroy(provider);
    }

    if (mAmbientSensingUnionCluster.IsConstructed())
    {
        mAmbientSensingUnionCluster.Destroy(provider);
    }

    if (mIdentifyCluster.IsConstructed())
    {
        mIdentifyCluster.Destroy(provider);
    }
}

Clusters::AmbientSensingUnionCluster & AmbientSensingUnionDevice::GetAmbientSensingUnionCluster()
{
    return mAmbientSensingUnionCluster.Get();
}

Clusters::OccupancySensingCluster & AmbientSensingUnionDevice::GetOccupancySensingCluster()
{
    return mOccupancySensingCluster.Get();
}

} // namespace app
} // namespace chip
