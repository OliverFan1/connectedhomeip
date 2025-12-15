/*
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

#include "ambient-sensing-union-instance.h"
#include <app/clusters/ambient-sensing-union-server/CodegenIntegration.h>
#include <lib/support/logging/CHIPLogging.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::AmbientSensingUnion;

// Delegate implementations
void LeaderSensorDelegate::OnUnionNameChanged(EndpointId endpointId, const CharSpan & unionName)
{
    ChipLogProgress(AppServer, "ASU - Leader EP %u: Union name changed to: %.*s", 
                   endpointId, static_cast<int>(unionName.size()), unionName.data());
}

void LeaderSensorDelegate::OnUnionHealthChanged(EndpointId endpointId, UnionHealthEnum unionHealth)
{
    const char * healthStr = (unionHealth == UnionHealthEnum::kFullyFunctional) ? "Fully Functional" :
                            (unionHealth == UnionHealthEnum::kLimitedDegraded) ? "Limited/Degraded" : "Non-Functional";
    ChipLogProgress(AppServer, "ASU - Leader EP %u: Union health changed to: %s", 
                   endpointId, healthStr);
}

void LeaderSensorDelegate::OnUnionContributorListChanged(EndpointId endpointId, const Span<const Structs::UnionMemberStruct::Type> & contributorList)
{
    ChipLogProgress(AppServer, "ASU - Leader EP %u: Contributor list changed, now has %u contributors:", 
                   endpointId, static_cast<unsigned>(contributorList.size()));
    for (size_t i = 0; i < contributorList.size(); i++)
    {
        ChipLogProgress(AppServer, "ASU - EP %u Contributor: NodeID=0x%016llX, EndpointID=%u, Health=0x%02X", 
                       endpointId, 
                       static_cast<unsigned long long>(contributorList[i].contributorNodeID),
                       contributorList[i].contributorEndpointID,
                       contributorList[i].contributorHealth);
    }
}

void MemberSensorDelegate::OnUnionNameChanged(EndpointId endpointId, const CharSpan & unionName)
{
    ChipLogProgress(AppServer, "ASU - Member EP %u: Received union name update: %.*s", 
                   endpointId, static_cast<int>(unionName.size()), unionName.data());
}

// Application implementation
CHIP_ERROR AmbientSensingUnionInstance::Initialize()
{
    ChipLogProgress(AppServer, "ASU - Initializing Ambient Sensing Union Instance");

    // Initialize all leader sensors across all endpoints
    ReturnErrorOnFailure(InitializeAllLeaderSensors());

    ChipLogProgress(AppServer, "ASU - Ambient Sensing Union instance initialized");
    return CHIP_NO_ERROR;
}

CHIP_ERROR AmbientSensingUnionInstance::InitializeAllLeaderSensors()
{
    CHIP_ERROR overallResult = CHIP_NO_ERROR;
    bool foundAtLeastOneLeader = false;

    // Iterate through all possible endpoints
    for (EndpointId endpointId = 0; endpointId <= CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT; endpointId++)
    {
        auto * cluster = FindClusterOnEndpoint(endpointId);
        if (cluster == nullptr)
        {
            continue; // No cluster on this endpoint
        }

        // Check if this cluster has Leader feature
        if (!cluster->HasLeaderFeature())
        {
            ChipLogProgress(AppServer, "ASU - Endpoint %u has cluster but no Leader feature", endpointId);
            continue;
        }

        // Initialize this leader sensor
        CHIP_ERROR result = InitializeLeaderSensor(endpointId);
        if (result == CHIP_NO_ERROR)
        {
            foundAtLeastOneLeader = true;
            ChipLogProgress(AppServer, "ASU - Successfully initialized leader on endpoint %u", endpointId);
        }
        else
        {
            ChipLogError(AppServer, "ASU - Failed to initialize leader on endpoint %u: %" CHIP_ERROR_FORMAT, 
                        endpointId, result.Format());
            if (overallResult == CHIP_NO_ERROR)
            {
                overallResult = result;
            }
        }
    }

    if (!foundAtLeastOneLeader)
    {
        ChipLogError(AppServer, "ASU - No leader sensors found on any endpoint");
        return CHIP_ERROR_NOT_FOUND;
    }

    return overallResult;
}

CHIP_ERROR AmbientSensingUnionInstance::InitializeLeaderSensor(EndpointId endpointId)
{
    // Find cluster on the specified endpoint
    auto * leaderCluster = FindClusterOnEndpoint(endpointId);
    VerifyOrReturnError(leaderCluster != nullptr, CHIP_ERROR_NOT_FOUND);
    VerifyOrReturnError(leaderCluster->HasLeaderFeature(), CHIP_ERROR_INCORRECT_STATE);

    ChipLogProgress(AppServer, "ASU - Found Ambient Sensing Union leader cluster on endpoint %u", endpointId);

    // Set up initial configuration - create endpoint-specific union name
    char unionNameBuffer[32];
    snprintf(unionNameBuffer, sizeof(unionNameBuffer), "AmbientSensingUnion-EP%u", endpointId);
    
    auto nameStatus = leaderCluster->SetUnionName(CharSpan(unionNameBuffer, strlen(unionNameBuffer)));
    if (!nameStatus.IsSuccess())
    {
        ChipLogError(AppServer, "ASU - Failed to set union name on endpoint %u", endpointId);
        return CHIP_ERROR_INTERNAL;
    }
    
    auto healthStatus = leaderCluster->SetUnionHealth(UnionHealthEnum::kFullyFunctional);
    if (!healthStatus.IsSuccess())
    {
        ChipLogError(AppServer, "ASU - Failed to set union health on endpoint %u", endpointId);
        return CHIP_ERROR_INTERNAL;
    }
    
    // Initialize with a single contributor (the leader itself)
    // Use endpoint-specific node ID for demonstration
    uint64_t nodeId = 0x1000000000000000ULL + endpointId;
    Structs::UnionMemberStruct::Type initialContributor = {
        .contributorNodeID = nodeId,
        .contributorEndpointID = endpointId,
        .contributorHealth = 0x01 // MatterContributorOnline
    };
    
    auto listStatus = leaderCluster->SetUnionContributorList(Span<const Structs::UnionMemberStruct::Type>(&initialContributor, 1));
    if (!listStatus.IsSuccess())
    {
        ChipLogError(AppServer, "ASU - Failed to set union contributor list on endpoint %u", endpointId);
        return CHIP_ERROR_INTERNAL;
    }

    ChipLogProgress(AppServer, "ASU - Leader sensor configured on endpoint %u:", endpointId);
    ChipLogProgress(AppServer, "ASU - Union Name: %s", unionNameBuffer);

    return CHIP_NO_ERROR;
}

void AmbientSensingUnionInstance::RunDemo()
{
    ChipLogProgress(AppServer, "ASU - Starting Ambient Sensing Union demo...");

    // Run demo on all leader endpoints
    RunDemoOnAllLeaderEndpoints();

    ChipLogProgress(AppServer, "ASU - Demo complete!");
}

void AmbientSensingUnionInstance::RunDemoOnAllLeaderEndpoints()
{
    // Iterate through all possible endpoints and run demo on leaders
    for (EndpointId endpointId = 0; endpointId <= CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT; endpointId++)
    {
        auto * cluster = FindClusterOnEndpoint(endpointId);
        if (cluster == nullptr || !cluster->HasLeaderFeature())
        {
            continue;
        }

        ChipLogProgress(AppServer, "ASU - Running demo on endpoint %u", endpointId);

        // Simulate operations on this endpoint
        SimulateUnionGrowth(endpointId);
        SimulateHealthChanges(endpointId);
    }
}

void AmbientSensingUnionInstance::SimulateUnionGrowth(EndpointId endpointId)
{
    auto * cluster = FindClusterOnEndpoint(endpointId);
    if (cluster == nullptr || !cluster->HasLeaderFeature()) 
    {
        ChipLogError(AppServer, "ASU - No leader cluster found on endpoint %u", endpointId);
        return;
    }

    ChipLogProgress(AppServer, "ASU - Simulating union growth on endpoint %u...", endpointId);

    // Add more contributors to the union - use endpoint-specific node IDs
    uint64_t baseNodeId = 0x1000000000000000ULL + endpointId;
    Structs::UnionMemberStruct::Type expandedContributorList[] = {
        { .contributorNodeID = baseNodeId, .contributorEndpointID = endpointId, .contributorHealth = 0x01 },
        { .contributorNodeID = baseNodeId + 0x100, .contributorEndpointID = static_cast<uint16_t>(endpointId + 1), .contributorHealth = 0x01 },
        { .contributorNodeID = baseNodeId + 0x200, .contributorEndpointID = static_cast<uint16_t>(endpointId + 2), .contributorHealth = 0x04 },
        { .contributorNodeID = baseNodeId + 0x300, .contributorEndpointID = static_cast<uint16_t>(endpointId + 3), .contributorHealth = 0x01 }
    };
    
    auto status = cluster->SetUnionContributorList(Span<const Structs::UnionMemberStruct::Type>(expandedContributorList, 4));
    if (status.IsSuccess())
    {
        ChipLogProgress(AppServer, "ASU - Expanded union to 4 contributors on endpoint %u", endpointId);
    }
    else
    {
        ChipLogError(AppServer, "ASU - Failed to expand union contributor list on endpoint %u", endpointId);
    }
}

void AmbientSensingUnionInstance::SimulateHealthChanges(EndpointId endpointId)
{
    auto * cluster = FindClusterOnEndpoint(endpointId);
    if (cluster == nullptr || !cluster->HasLeaderFeature()) 
    {
        ChipLogError(AppServer, "ASU - No leader cluster found on endpoint %u", endpointId);
        return;
    }

    ChipLogProgress(AppServer, "ASU - Simulating health changes on endpoint %u...", endpointId);

    // Simulate one contributor going offline
    uint64_t baseNodeId = 0x1000000000000000ULL + endpointId;
    Structs::UnionMemberStruct::Type contributorListWithDegradedHealth[] = {
        { .contributorNodeID = baseNodeId, .contributorEndpointID = endpointId, .contributorHealth = 0x01 },
        { .contributorNodeID = baseNodeId + 0x100, .contributorEndpointID = static_cast<uint16_t>(endpointId + 1), .contributorHealth = 0x02 }, // Offline
        { .contributorNodeID = baseNodeId + 0x200, .contributorEndpointID = static_cast<uint16_t>(endpointId + 2), .contributorHealth = 0x04 },
        { .contributorNodeID = baseNodeId + 0x300, .contributorEndpointID = static_cast<uint16_t>(endpointId + 3), .contributorHealth = 0x01 }
    };
    
    auto status = cluster->SetUnionContributorList(Span<const Structs::UnionMemberStruct::Type>(contributorListWithDegradedHealth, 4));
    if (status.IsSuccess())
    {
        auto unionHealthStatus = cluster->SetUnionHealth(UnionHealthEnum::kLimitedDegraded);
        if (unionHealthStatus.IsSuccess())
        {
            ChipLogProgress(AppServer, "ASU - Union health degraded on endpoint %u due to contributor offline", endpointId);
        }
        else
        {
            ChipLogError(AppServer, "ASU - Failed to set union health status on endpoint %u", endpointId);
        }
    }
    else
    {
        ChipLogError(AppServer, "ASU - Failed to set contributor health status on endpoint %u", endpointId);
    }
}

// Global instance
AmbientSensingUnionInstance gAmbientSensingUnionInstance;

CHIP_ERROR AmbientSensingUnionInstance::SetUnionName(EndpointId endpointId, const CharSpan & unionName)
{
    auto * cluster = FindClusterOnEndpoint(endpointId);
    VerifyOrReturnError(cluster != nullptr, CHIP_ERROR_NOT_FOUND);
    
    auto status = cluster->SetUnionName(unionName);
    if (!status.IsSuccess())
    {
        ChipLogError(AppServer, "ASU - Failed to set union name on endpoint %u", endpointId);
        return CHIP_ERROR_INTERNAL;
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR AmbientSensingUnionInstance::SetUnionHealth(EndpointId endpointId, uint8_t healthValue)
{
    auto * cluster = FindClusterOnEndpoint(endpointId);
    VerifyOrReturnError(cluster != nullptr, CHIP_ERROR_NOT_FOUND);
    
    auto health = static_cast<AmbientSensingUnion::UnionHealthEnum>(healthValue);
    auto status = cluster->SetUnionHealth(health);
    if (!status.IsSuccess())
    {
        ChipLogError(AppServer, "ASU - Failed to set union health on endpoint %u", endpointId);
        return CHIP_ERROR_INTERNAL;
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR AmbientSensingUnionInstance::SetUnionContributorList(EndpointId endpointId, const Span<const Structs::UnionMemberStruct::Type> & contributorList)
{
    VerifyOrReturnError(!contributorList.empty(), CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(contributorList.size() <= MAX_UNION_CONTRIBUTOR_LIST_SIZE, CHIP_ERROR_INVALID_ARGUMENT);

    auto * cluster = FindClusterOnEndpoint(endpointId);
    VerifyOrReturnError(cluster != nullptr, CHIP_ERROR_NOT_FOUND);
    VerifyOrReturnError(cluster->HasLeaderFeature(), CHIP_ERROR_INCORRECT_STATE);
    
    auto status = cluster->SetUnionContributorList(contributorList);
    if (!status.IsSuccess())
    {
        ChipLogError(AppServer, "ASU - Failed to set union contributor list on endpoint %u", endpointId);
        return CHIP_ERROR_INTERNAL;
    }
    return CHIP_NO_ERROR;
}
