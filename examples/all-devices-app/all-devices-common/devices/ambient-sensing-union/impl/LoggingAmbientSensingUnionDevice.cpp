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

#include "LoggingAmbientSensingUnionDevice.h"

#include <lib/support/logging/CHIPLogging.h>

using namespace chip::app::Clusters::AmbientSensingUnion;

namespace chip {
namespace app {

LoggingAmbientSensingUnionDevice::LoggingAmbientSensingUnionDevice(TimerDelegate & timerDelegate) :
    AmbientSensingUnionDevice(timerDelegate)
{}

CHIP_ERROR LoggingAmbientSensingUnionDevice::Register(EndpointId endpoint, CodeDrivenDataModelProvider & provider,
                                                      EndpointId parentId)
{
    // Let the base class create and register all clusters.
    ReturnErrorOnFailure(AmbientSensingUnionDevice::Register(endpoint, provider, parentId));

    // Attach this object as the delegate now that the cluster exists.
    GetAmbientSensingUnionCluster().SetDelegate(this);

    return CHIP_NO_ERROR;
}

void LoggingAmbientSensingUnionDevice::Unregister(CodeDrivenDataModelProvider & provider)
{
    // Detach delegate before the base class destroys the cluster to avoid a
    // dangling pointer.
    if (mAmbientSensingUnionCluster.IsConstructed())
    {
        GetAmbientSensingUnionCluster().SetDelegate(nullptr);
    }

    AmbientSensingUnionDevice::Unregister(provider);
}

void LoggingAmbientSensingUnionDevice::OnUnionNameChanged(const chip::CharSpan & unionName)
{
    ChipLogProgress(AppServer, "AmbientSensingUnion: Union name changed to '%.*s'",
                    static_cast<int>(unionName.size()), unionName.data());
}

void LoggingAmbientSensingUnionDevice::OnUnionHealthChanged(UnionHealthEnum unionHealth)
{
    const char * healthStr = "Unknown";
    switch (unionHealth)
    {
    case UnionHealthEnum::kFullyFunctional:
        healthStr = "FullyFunctional";
        break;
    case UnionHealthEnum::kLimitedDegraded:
        healthStr = "LimitedDegraded";
        break;
    case UnionHealthEnum::kNonFunctional:
        healthStr = "NonFunctional";
        break;
    default:
        break;
    }
    ChipLogProgress(AppServer, "AmbientSensingUnion: Union health changed to %s", healthStr);
}

} // namespace app
} // namespace chip

