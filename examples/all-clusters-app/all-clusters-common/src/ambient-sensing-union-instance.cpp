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
void LeaderSensorDelegate::OnUnionNameChanged(const CharSpan & unionName)
{
    ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Leader: Union name changed to: %.*s", 
                   static_cast<int>(unionName.size()), unionName.data());
}

void LeaderSensorDelegate::OnUnionHealthChanged(UnionHealthEnum unionHealth)
{
    const char * healthStr = (unionHealth == UnionHealthEnum::kFullyFunctional) ? "Fully Functional" :
                            (unionHealth == UnionHealthEnum::kLimitedDegraded) ? "Limited/Degraded" : "Non-Functional";
    ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Leader: Union health changed to: %s", healthStr);
}

void LeaderSensorDelegate::OnUnionSensorListChanged(const Span<const uint8_t> & sensorList)
{
    ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Leader: Sensor list changed, now has %u sensors:", 
                   static_cast<unsigned>(sensorList.size()));
    for (size_t i = 0; i < sensorList.size(); i++)
    {
        ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Sensor ID: %u", sensorList[i]);
    }
}

void LeaderSensorDelegate::OnUnionSensorHealthChanged(const Span<const UnionHealthEnum> & sensorHealth)
{
    ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Leader: Sensor health status updated for %u sensors", 
                   static_cast<unsigned>(sensorHealth.size()));
}

void LeaderSensorDelegate::OnZoneSensorListChanged(const Span<const uint8_t> & zoneSensorList)
{
    ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Leader: Zone sensor list changed, %u sensors in zone", 
                   static_cast<unsigned>(zoneSensorList.size()));
}

void FollowerSensorDelegate::OnUnionNameChanged(const CharSpan & unionName)
{
    ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Follower: Received union name update: %.*s", 
                   static_cast<int>(unionName.size()), unionName.data());
}

// Application implementation
CHIP_ERROR AmbientSensingUnionInstance::Initialize()
{
    ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Initializing Ambient Sensing Union Instance");

    // Initialize leader sensor on endpoint 1
    ReturnErrorOnFailure(InitializeLeaderSensor());

    ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Ambient Sensing Union instance initialized");
    return CHIP_NO_ERROR;
}

void AmbientSensingUnionInstance::RunDemo()
{
    ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Starting Ambient Sensing Union demo...");

    // Simulate adding sensors to the union
    SimulateUnionGrowth();

    // Simulate health status changes  
    SimulateHealthChanges();

    // Simulate zone management
    SimulateZoneManagement();

    ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Demo complete!");
}

CHIP_ERROR AmbientSensingUnionInstance::InitializeLeaderSensor()
{
    // Find cluster on endpoint 1
    auto * leaderCluster = FindClusterOnEndpoint(1);
    VerifyOrReturnError(leaderCluster != nullptr, CHIP_ERROR_NOT_FOUND);

    ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Found Ambient Sensing Union cluster on endpoint 1");

    // Set up initial configuration
    uint8_t initialSensorList[] = { 100 }; // Leader itself
    
    auto nameStatus = leaderCluster->SetUnionName("AllClustersApp-Demo"_span);
    if (!nameStatus.IsSuccess())
    {
        ChipLogError(AppServer, "Ambient Sensing Union cluster - Failed to set union name");
        return CHIP_ERROR_INTERNAL;
    }
    
    auto healthStatus = leaderCluster->SetUnionHealth(UnionHealthEnum::kFullyFunctional);
    if (!healthStatus.IsSuccess())
    {
        ChipLogError(AppServer, "Ambient Sensing Union cluster - Failed to set union health");
        return CHIP_ERROR_INTERNAL;
    }
    
    auto listStatus = leaderCluster->SetUnionSensorList(Span<const uint8_t>(initialSensorList, 1));
    if (!listStatus.IsSuccess())
    {
        ChipLogError(AppServer, "Ambient Sensing Union cluster - Failed to set union sensor list");
        return CHIP_ERROR_INTERNAL;
    }

    ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Leader sensor configured:");
    ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Union ID: %" PRIu64, leaderCluster->GetUnionId());
    ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Sensor ID: %u", leaderCluster->GetSensorId());

    return CHIP_NO_ERROR;
}

void AmbientSensingUnionInstance::SimulateUnionGrowth()
{
    auto * cluster = FindClusterOnEndpoint(1);
    if (cluster == nullptr) 
    {
        ChipLogError(AppServer, "Ambient Sensing Union cluster - No Ambient Sensing Union cluster found on endpoint 1");
        return;
    }

    ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Simulating union growth...");

    // Add more sensors to the union
    uint8_t expandedSensorList[] = { 100, 101, 102, 103 };
    auto status = cluster->SetUnionSensorList(Span<const uint8_t>(expandedSensorList, 4));
    if (status.IsSuccess())
    {
        ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Expanded union to 4 sensors");
    }
    else
    {
        ChipLogError(AppServer, "Ambient Sensing Union cluster - Failed to expand union sensor list");
    }

    // Set health status for all sensors
    UnionHealthEnum healthList[] = { 
        UnionHealthEnum::kFullyFunctional,
        UnionHealthEnum::kFullyFunctional,
        UnionHealthEnum::kFullyFunctional,
        UnionHealthEnum::kFullyFunctional
    };
    status = cluster->SetUnionSensorHealth(Span<const UnionHealthEnum>(healthList, 4));
    if (!status.IsSuccess())
    {
        ChipLogError(AppServer, "Ambient Sensing Union cluster - Failed to set union sensor health");
    }
}

void AmbientSensingUnionInstance::SimulateHealthChanges()
{
    auto * cluster = FindClusterOnEndpoint(1);
    if (cluster == nullptr) 
    {
        ChipLogError(AppServer, "Ambient Sensing Union cluster - No Ambient Sensing Union cluster found on endpoint 1");
        return;
    }

    ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Simulating health changes...");

    // Simulate one sensor going degraded
    UnionHealthEnum healthList[] = { 
        UnionHealthEnum::kFullyFunctional,
        UnionHealthEnum::kLimitedDegraded,  // Sensor 101 degraded
        UnionHealthEnum::kFullyFunctional,
        UnionHealthEnum::kFullyFunctional
    };
    
    auto status = cluster->SetUnionSensorHealth(Span<const UnionHealthEnum>(healthList, 4));
    if (status.IsSuccess())
    {
        auto unionHealthStatus = cluster->SetUnionHealth(UnionHealthEnum::kLimitedDegraded);
        if (unionHealthStatus.IsSuccess())
        {
            ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Union health degraded due to sensor 101");
        }
        else
        {
            ChipLogError(AppServer, "Ambient Sensing Union cluster - Failed to set union health status");
        }
    }
    else
    {
        ChipLogError(AppServer, "Ambient Sensing Union cluster - Failed to set sensor health status");
    }
}

void AmbientSensingUnionInstance::SimulateZoneManagement()
{
    auto * cluster = FindClusterOnEndpoint(1);
    if (cluster == nullptr) 
    {
        ChipLogError(AppServer, "Ambient Sensing Union cluster - No Ambient Sensing Union cluster found on endpoint 1");
        return;
    }

    ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Simulating zone management...");

    // Create a zone with some sensors
    uint8_t zoneList[] = { 101, 102 };
    auto status = cluster->SetZoneSensorList(Span<const uint8_t>(zoneList, 2));
    if (status.IsSuccess())
    {
        ChipLogProgress(AppServer, "Ambient Sensing Union cluster - Created zone with sensors 101, 102");
    }
    else
    {
        ChipLogError(AppServer, " Failed to create zone sensor list");
    }
}

// Global instance
AmbientSensingUnionInstance gAmbientSensingUnionInstance;


CHIP_ERROR AmbientSensingUnionInstance::SetUnionName(const CharSpan & unionName)
{
    auto * cluster = FindClusterOnEndpoint(1);
    VerifyOrReturnError(cluster != nullptr, CHIP_ERROR_NOT_FOUND);
    
    auto status = cluster->SetUnionName(unionName);
    return status.IsSuccess() ? CHIP_NO_ERROR : CHIP_ERROR_INTERNAL;
}

CHIP_ERROR AmbientSensingUnionInstance::SetUnionHealth(uint8_t healthValue)
{
    auto * cluster = FindClusterOnEndpoint(1);
    VerifyOrReturnError(cluster != nullptr, CHIP_ERROR_NOT_FOUND);
    
    auto health = static_cast<AmbientSensingUnion::UnionHealthEnum>(healthValue);
    auto status = cluster->SetUnionHealth(health);
    return status.IsSuccess() ? CHIP_NO_ERROR : CHIP_ERROR_INTERNAL;
}

CHIP_ERROR AmbientSensingUnionInstance::SetUnionSensorList(const Span<const uint8_t> & sensorList)
{
    auto * cluster = FindClusterOnEndpoint(1);
    VerifyOrReturnError(cluster != nullptr, CHIP_ERROR_NOT_FOUND);
    
    auto status = cluster->SetUnionSensorList(sensorList);
    return status.IsSuccess() ? CHIP_NO_ERROR : CHIP_ERROR_INTERNAL;
}