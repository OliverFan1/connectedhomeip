/*
 *
 *    Copyright (c) 2025 Project CHIP Authors
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

#include "CodegenIntegration.h"
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/cluster-objects.h>
#include <app/clusters/ambient-sensing-union-server/AmbientSensingUnionCluster.h>
#include <app/static-cluster-config/AmbientSensingUnion.h>
#include <app/util/attribute-storage.h>
#include <data-model-providers/codegen/ClusterIntegration.h>
#include <data-model-providers/codegen/CodegenDataModelProvider.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::AmbientSensingUnion;
using namespace chip::app::Clusters::AmbientSensingUnion::Attributes;

namespace {

constexpr size_t kAmbientSensingUnionFixedClusterCount = AmbientSensingUnion::StaticApplicationConfig::kFixedClusterConfig.size();
constexpr size_t kAmbientSensingUnionMaxClusterCount = kAmbientSensingUnionFixedClusterCount + CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT;

LazyRegisteredServerCluster<AmbientSensingUnionCluster> gServers[kAmbientSensingUnionMaxClusterCount];

class IntegrationDelegate : public CodegenClusterIntegration::Delegate
{
public:
    ServerClusterRegistration & CreateRegistration(EndpointId endpointId, unsigned clusterInstanceIndex,
                                                   uint32_t optionalAttributeBits, uint32_t featureMap) override
    {
        // Generate default IDs based on endpoint and instance
        uint64_t defaultUnionId = (static_cast<uint64_t>(endpointId) << 32) | clusterInstanceIndex;
        uint8_t defaultSensorId = static_cast<uint8_t>(clusterInstanceIndex);

        AmbientSensingUnionCluster::Config config(endpointId);
        config.WithFeatures(static_cast<Feature>(featureMap))
              .WithUnionId(defaultUnionId)
              .WithSensorId(defaultSensorId);

        // Enable UnionName if it's marked as optional
        if (optionalAttributeBits & (1 << Attributes::UnionName::Id))
        {
            config.WithUnionName("AmbientSensingUnion"_span);
        }

        gServers[clusterInstanceIndex].Create(config);
        return gServers[clusterInstanceIndex].Registration();
    }

    ServerClusterInterface * FindRegistration(unsigned clusterInstanceIndex) override
    {
        VerifyOrReturnValue(gServers[clusterInstanceIndex].IsConstructed(), nullptr);
        return &gServers[clusterInstanceIndex].Cluster();
    }

    void ReleaseRegistration(unsigned clusterInstanceIndex) override 
    { 
        gServers[clusterInstanceIndex].Destroy(); 
    }
};

} // namespace

void MatterAmbientSensingUnionClusterInitCallback(EndpointId endpointId)
{
    IntegrationDelegate integrationDelegate;

    CodegenClusterIntegration::RegisterServer(
        {
            .endpointId                = endpointId,
            .clusterId                 = AmbientSensingUnion::Id,
            .fixedClusterInstanceCount = kAmbientSensingUnionFixedClusterCount,
            .maxClusterInstanceCount   = kAmbientSensingUnionMaxClusterCount,
            .fetchFeatureMap           = true,
            .fetchOptionalAttributes   = true,
        },
        integrationDelegate);
}

void MatterAmbientSensingUnionClusterShutdownCallback(EndpointId endpointId)
{
    IntegrationDelegate integrationDelegate;

    CodegenClusterIntegration::UnregisterServer(
        {
            .endpointId                = endpointId,
            .clusterId                 = AmbientSensingUnion::Id,
            .fixedClusterInstanceCount = kAmbientSensingUnionFixedClusterCount,
            .maxClusterInstanceCount   = kAmbientSensingUnionMaxClusterCount,
        },
        integrationDelegate);
}

namespace chip::app::Clusters::AmbientSensingUnion {

AmbientSensingUnionCluster * FindClusterOnEndpoint(EndpointId endpointId)
{
    IntegrationDelegate integrationDelegate;

    ServerClusterInterface * ambientSensingUnion = CodegenClusterIntegration::FindClusterOnEndpoint(
        {
            .endpointId                = endpointId,
            .clusterId                 = AmbientSensingUnion::Id,
            .fixedClusterInstanceCount = kAmbientSensingUnionFixedClusterCount,
            .maxClusterInstanceCount   = kAmbientSensingUnionMaxClusterCount,
        },
        integrationDelegate);

    return static_cast<AmbientSensingUnionCluster *>(ambientSensingUnion);
}

} // namespace chip::app::Clusters::AmbientSensingUnion

// Legacy PluginServer callback stubs
void MatterAmbientSensingUnionPluginServerInitCallback() {}
void MatterAmbientSensingUnionPluginServerShutdownCallback() {}
