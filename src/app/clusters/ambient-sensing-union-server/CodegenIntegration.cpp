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

#include "CodegenIntegration.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/cluster-objects.h>
#include <app/clusters/ambient-sensing-union-server/AmbientSensingUnionCluster.h>
#include <app/static-cluster-config/AmbientSensingUnion.h>
#include <app/util/attribute-storage.h>
#include <app/util/endpoint-config-api.h>
#include <data-model-providers/codegen/ClusterIntegration.h>
#include <data-model-providers/codegen/CodegenDataModelProvider.h>
#include <lib/support/CodeUtils.h>
#include <platform/KeyValueStoreManager.h>

#include <cstdio>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::AmbientSensingUnion;

namespace {

constexpr size_t kAmbientSensingUnionFixedClusterCount = AmbientSensingUnion::StaticApplicationConfig::kFixedClusterConfig.size();
constexpr size_t kAmbientSensingUnionMaxClusterCount   = kAmbientSensingUnionFixedClusterCount + CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT;

LazyRegisteredServerCluster<AmbientSensingUnionCluster> gServers[kAmbientSensingUnionMaxClusterCount];

// Default contributor storage for each cluster instance
// Each instance gets its own storage: 32 Matter contributors + 8 non-Matter contributors
DefaultContributorStorage<32, 8> gContributorStorage[kAmbientSensingUnionMaxClusterCount];

/**
 * @brief KVS-backed persistence delegate for the UnionName attribute.
 *
 * Per spec, UnionName has Quality "N" (non-volatile) and must survive reboots.
 * Each endpoint gets its own key in the platform Key-Value Store:
 *   "g/asu/<endpointId_hex>/un"
 */
class KvsPersistenceDelegate : public AmbientSensingUnionPersistenceDelegate
{
public:
    void SetEndpointId(EndpointId endpointId) { mEndpointId = endpointId; }

    CHIP_ERROR LoadUnionName(char * buffer, size_t bufferSize, size_t & outLength) override
    {
        char key[kMaxKeyLength];
        BuildUnionNameKey(key, sizeof(key));

        size_t readSize = 0;
        CHIP_ERROR err  = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(key, buffer, bufferSize, &readSize);

        if (err == CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND)
        {
            return CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
        }
        ReturnErrorOnFailure(err);

        outLength = readSize;
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR SaveUnionName(const CharSpan & unionName) override
    {
        char key[kMaxKeyLength];
        BuildUnionNameKey(key, sizeof(key));

        if (unionName.empty())
        {
            // Delete the key when the name is cleared to reclaim storage
            CHIP_ERROR err = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Delete(key);
            // Treat "not found" as success — nothing to delete
            if (err == CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND)
            {
                return CHIP_NO_ERROR;
            }
            return err;
        }

        return chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Put(key, unionName.data(),
                                                                           unionName.size());
    }

private:
    // Key format: "g/asu/<hex_endpoint>/un"
    // "g"   = global namespace
    // "asu" = ambient sensing union
    // "un"  = union name
    // Max: "g/asu/FFFF/un" = 14 chars + null
    static constexpr size_t kMaxKeyLength = 32;

    void BuildUnionNameKey(char * buffer, size_t bufferSize) const
    {
        snprintf(buffer, bufferSize, "g/asu/%x/un", mEndpointId);
    }

    EndpointId mEndpointId = kInvalidEndpointId;
};

// One persistence delegate per cluster instance
KvsPersistenceDelegate gPersistenceDelegate[kAmbientSensingUnionMaxClusterCount];

class IntegrationDelegate : public CodegenClusterIntegration::Delegate
{
public:
    ServerClusterRegistration & CreateRegistration(EndpointId endpointId, unsigned clusterInstanceIndex,
                                                   uint32_t optionalAttributeBits, uint32_t featureMap) override
    {
        AmbientSensingUnionCluster::Config config(endpointId);

        // Initialize the persistence delegate for this endpoint
        gPersistenceDelegate[clusterInstanceIndex].SetEndpointId(endpointId);

        // Configure with default values
        // Per spec: UnionName is writable with max 128 chars, has Quality "N" (non-volatile)
        // Per spec: UnionHealth defaults to FullyFunctional
        // Per spec: UnionContributorList starts empty
        config.WithUnionName(CharSpan::fromCharString(""))
            .WithUnionHealth(UnionHealthEnum::kFullyFunctional)
            .WithContributorStorage(&gContributorStorage[clusterInstanceIndex])
            .WithPersistence(&gPersistenceDelegate[clusterInstanceIndex]);

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
        gContributorStorage[clusterInstanceIndex].ClearAllContributors();
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
            .fetchFeatureMap           = false,  // No features defined in spec
            .fetchOptionalAttributes   = false,  // All attributes are mandatory per spec
        },
        integrationDelegate);
}

void MatterAmbientSensingUnionClusterShutdownCallback(EndpointId endpointId, MatterClusterShutdownType shutdownType)
{
    IntegrationDelegate integrationDelegate;

    CodegenClusterIntegration::UnregisterServer(
        {
            .endpointId                = endpointId,
            .clusterId                 = AmbientSensingUnion::Id,
            .fixedClusterInstanceCount = kAmbientSensingUnionFixedClusterCount,
            .maxClusterInstanceCount   = kAmbientSensingUnionMaxClusterCount,
        },
        integrationDelegate, shutdownType);
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

