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

#pragma once

#include <app/clusters/ambient-sensing-union-server/AmbientSensingUnionCluster.h>
#include <lib/core/CHIPError.h>

// Remove global using namespace directives from header
// Use fully qualified names instead

class LeaderSensorDelegate : public chip::app::Clusters::AmbientSensingUnionDelegate
{
public:
    void OnUnionNameChanged(const chip::CharSpan & unionName) override;
    void OnUnionHealthChanged(chip::app::Clusters::AmbientSensingUnion::UnionHealthEnum unionHealth) override;
    void OnUnionSensorListChanged(const chip::Span<const uint8_t> & sensorList) override;
    void OnUnionSensorHealthChanged(const chip::Span<const chip::app::Clusters::AmbientSensingUnion::UnionHealthEnum> & sensorHealth) override;
    void OnZoneSensorListChanged(const chip::Span<const uint8_t> & zoneSensorList) override;
};

class FollowerSensorDelegate : public chip::app::Clusters::AmbientSensingUnionDelegate
{
public:
    void OnUnionNameChanged(const chip::CharSpan & unionName) override;
};

class AmbientSensingUnionInstance
{
public:
    CHIP_ERROR Initialize();
    void RunDemo();

    CHIP_ERROR SetUnionName(const chip::CharSpan & unionName);
    CHIP_ERROR SetUnionHealth(uint8_t healthValue);
    CHIP_ERROR SetUnionSensorList(const chip::Span<const uint8_t> & sensorList);

private:
    CHIP_ERROR InitializeLeaderSensor();
    chip::app::Clusters::AmbientSensingUnionCluster * FindClusterOnAnyEndpoint();
    
    void SimulateUnionGrowth();
    void SimulateHealthChanges();
    void SimulateZoneManagement();

    LeaderSensorDelegate mLeaderDelegate;
    FollowerSensorDelegate mFollowerDelegate;
};

extern AmbientSensingUnionInstance gAmbientSensingUnionInstance;
