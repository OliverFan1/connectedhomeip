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
#include <clusters/AmbientSensingUnion/Structs.h>
#include <lib/core/CHIPError.h>

class LeaderSensorDelegate : public chip::app::Clusters::AmbientSensingUnionDelegate
{
public:
    void OnUnionNameChanged(chip::EndpointId endpointId, const chip::CharSpan & unionName) override;
    void OnUnionHealthChanged(chip::EndpointId endpointId, chip::app::Clusters::AmbientSensingUnion::UnionHealthEnum unionHealth) override;
    void OnUnionContributorListChanged(chip::EndpointId endpointId, const chip::Span<const chip::app::Clusters::AmbientSensingUnion::Structs::UnionMemberStruct::Type> & contributorList) override;
};

class AmbientSensingUnionInstance
{
public:
    CHIP_ERROR Initialize();
    void RunDemo();

private:
    static constexpr uint8_t MAX_UNION_CONTRIBUTOR_LIST_SIZE = 128;

    CHIP_ERROR InitializeAllLeaderSensors();
    CHIP_ERROR InitializeLeaderSensor(chip::EndpointId endpointId);
    
    void RunDemoOnAllLeaderEndpoints();
    void SimulateUnionGrowth(chip::EndpointId endpointId);
    void SimulateHealthChanges(chip::EndpointId endpointId);

    LeaderSensorDelegate mLeaderDelegate;
};

extern AmbientSensingUnionInstance gAmbientSensingUnionInstance;
