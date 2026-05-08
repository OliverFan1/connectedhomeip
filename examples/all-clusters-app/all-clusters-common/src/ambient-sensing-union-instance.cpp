/*
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

#include "ambient-sensing-union-instance.h"

#include <app/clusters/ambient-sensing-union-server/AmbientSensingUnionCluster.h>
#include <app/clusters/ambient-sensing-union-server/CodegenIntegration.h>
#include <lib/support/CodeUtils.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::AmbientSensingUnion;

namespace {

// Endpoint where the cluster is instantiated
constexpr EndpointId kAmbientSensingUnionEndpoint = 1;

// Optional: Delegate implementation for application callbacks
class AllClustersAmbientSensingUnionDelegate : public AmbientSensingUnionDelegate
{
public:
    void OnUnionNameChanged(const CharSpan & unionName) override
    {
        ChipLogProgress(NotSpecified, "AmbientSensingUnion: Union name changed to '%.*s'",
                        static_cast<int>(unionName.size()), unionName.data());
    }

    void OnUnionHealthChanged(UnionHealthEnum unionHealth) override
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
        ChipLogProgress(NotSpecified, "AmbientSensingUnion: Union health changed to %s", healthStr);
    }

    void OnContributorAdded(const Structs::UnionContributorStruct::Type & contributor) override
    {
        if (!contributor.contributorNodeID.IsNull())
        {
            ChipLogProgress(NotSpecified, "AmbientSensingUnion: Matter contributor added - NodeId: 0x" ChipLogFormatX64
                            ", EndpointId: %u",
                            ChipLogValueX64(contributor.contributorNodeID.Value()),
                            contributor.contributorEndpointID.Value());
        }
        else if (contributor.contributorName.HasValue())
        {
            ChipLogProgress(NotSpecified, "AmbientSensingUnion: Non-Matter contributor added - Name: '%.*s'",
                            static_cast<int>(contributor.contributorName.Value().size()),
                            contributor.contributorName.Value().data());
        }
    }

    void OnContributorRemoved(const Structs::UnionContributorStruct::Type & contributor) override
    {
        if (!contributor.contributorNodeID.IsNull())
        {
            ChipLogProgress(NotSpecified, "AmbientSensingUnion: Matter contributor removed - NodeId: 0x" ChipLogFormatX64,
                            ChipLogValueX64(contributor.contributorNodeID.Value()));
        }
        else if (contributor.contributorName.HasValue())
        {
            ChipLogProgress(NotSpecified, "AmbientSensingUnion: Non-Matter contributor removed - Name: '%.*s'",
                            static_cast<int>(contributor.contributorName.Value().size()),
                            contributor.contributorName.Value().data());
        }
    }

    void OnContributorHealthChanged(const Structs::UnionContributorStruct::Type & contributor) override
    {
        const char * healthStr = (contributor.contributorHealth == UnionContributorHealthEum::kUnionContributorOnline)
            ? "Online"
            : "Offline";

        if (!contributor.contributorNodeID.IsNull())
        {
            ChipLogProgress(NotSpecified, "AmbientSensingUnion: Matter contributor health changed - NodeId: 0x" ChipLogFormatX64
                            " -> %s",
                            ChipLogValueX64(contributor.contributorNodeID.Value()), healthStr);
        }
        else if (contributor.contributorName.HasValue())
        {
            ChipLogProgress(NotSpecified, "AmbientSensingUnion: Non-Matter contributor health changed - Name: '%.*s' -> %s",
                            static_cast<int>(contributor.contributorName.Value().size()),
                            contributor.contributorName.Value().data(), healthStr);
        }
    }
};

AllClustersAmbientSensingUnionDelegate gDelegate;

} // namespace

namespace chip {
namespace app {
namespace Clusters {
namespace AmbientSensingUnion {

void SetupAmbientSensingUnionDelegate()
{
    AmbientSensingUnionCluster * cluster = FindClusterOnEndpoint(kAmbientSensingUnionEndpoint);
    if (cluster != nullptr)
    {
        cluster->SetDelegate(&gDelegate);
        ChipLogProgress(NotSpecified, "AmbientSensingUnion: Delegate attached to endpoint %u", kAmbientSensingUnionEndpoint);
    }
}

} // namespace AmbientSensingUnion
} // namespace Clusters
} // namespace app
} // namespace chip
