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

#include <devices/Types.h>
#include <lib/support/logging/CHIPLogging.h>

using namespace chip::app::Clusters;

namespace chip {
namespace app {

AmbientSensingUnionDevice::AmbientSensingUnionDevice(TimerDelegate & timerDelegate) :
    SingleEndpointDevice(Span<const DataModel::DeviceTypeEntry>(&Device::Type::kAmbientSensingUnion, 1)),
    mTimerDelegate(timerDelegate)
{}

CHIP_ERROR AmbientSensingUnionDevice::Register(chip::EndpointId endpoint, CodeDrivenDataModelProvider & provider,
                                               EndpointId parentId)
{
    ReturnErrorOnFailure(SingleEndpointRegistration(endpoint, provider, parentId));

    mIdentifyCluster.Create(IdentifyCluster::Config(endpoint, mTimerDelegate));
    ReturnErrorOnFailure(provider.AddCluster(mIdentifyCluster.Registration()));

    mAmbientSensingUnionCluster.Create(AmbientSensingUnionCluster::Config(endpoint));
    ReturnErrorOnFailure(provider.AddCluster(mAmbientSensingUnionCluster.Registration()));

    return provider.AddEndpoint(mEndpointRegistration);
}

void AmbientSensingUnionDevice::Unregister(CodeDrivenDataModelProvider & provider)
{
    SingleEndpointUnregistration(provider);

    if (mAmbientSensingUnionCluster.IsConstructed())
    {
        LogErrorOnFailure(provider.RemoveCluster(&mAmbientSensingUnionCluster.Cluster()));
        mAmbientSensingUnionCluster.Destroy();
    }

    if (mIdentifyCluster.IsConstructed())
    {
        LogErrorOnFailure(provider.RemoveCluster(&mIdentifyCluster.Cluster()));
        mIdentifyCluster.Destroy();
    }
}

Clusters::AmbientSensingUnionCluster & AmbientSensingUnionDevice::GetAmbientSensingUnionCluster()
{
    VerifyOrDie(mAmbientSensingUnionCluster.IsConstructed());
    return mAmbientSensingUnionCluster.Cluster();
}

} // namespace app
} // namespace chip
