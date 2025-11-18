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

#include <app/persistence/String.h>
#include <app/server-cluster/DefaultServerCluster.h>
#include <app/server-cluster/OptionalAttributeSet.h>
#include <clusters/AmbientSensingUnion/AttributeIds.h>
#include <clusters/AmbientSensingUnion/ClusterId.h>
#include <clusters/AmbientSensingUnion/Enums.h>
#include <lib/core/DataModelTypes.h>
#include <lib/support/Span.h>

namespace chip {
namespace app {
namespace Clusters {

class AmbientSensingUnionDelegate
{
public:
    virtual ~AmbientSensingUnionDelegate() = default;

    virtual void OnUnionNameChanged(const CharSpan & unionName) {}
    virtual void OnUnionHealthChanged(AmbientSensingUnion::UnionHealthEnum unionHealth) {}
    virtual void OnUnionSensorListChanged(const Span<const uint8_t> & sensorList) {}
    virtual void OnUnionSensorHealthChanged(const Span<const AmbientSensingUnion::UnionHealthEnum> & sensorHealth) {}
    virtual void OnZoneSensorListChanged(const Span<const uint8_t> & zoneSensorList) {}
};

class AmbientSensingUnionCluster : public DefaultServerCluster
{
public:
    struct Config
    {
        Config(EndpointId endpointId) : mEndpointId(endpointId) {}

        Config & WithFeatures(AmbientSensingUnion::Feature featureMap)
        {
            mFeatureMap = featureMap;
            return *this;
        }

        Config & WithUnionId(uint64_t unionId)
        {
            mUnionId = unionId;
            return *this;
        }

        Config & WithSensorId(uint8_t sensorId)
        {
            mSensorId = sensorId;
            return *this;
        }

        Config & WithDelegate(AmbientSensingUnionDelegate * delegate)
        {
            mDelegate = delegate;
            return *this;
        }

        Config & WithUnionName(const CharSpan & unionName)
        {
            mHasUnionName = true;
            mUnionName = unionName;
            return *this;
        }

        Config & WithUnionSensorList(const Span<const uint8_t> & sensorList)
        {
            mUnionSensorList = sensorList;
            return *this;
        }

        Config & WithZoneSensorList(const Span<const uint8_t> & zoneSensorList)
        {
            mZoneSensorList = zoneSensorList;
            return *this;
        }

        EndpointId mEndpointId;
        BitMask<AmbientSensingUnion::Feature> mFeatureMap = 0;
        uint64_t mUnionId = 0;
        uint8_t mSensorId = 0;
        bool mHasUnionName = false;
        CharSpan mUnionName;
        Span<const uint8_t> mUnionSensorList;
        Span<const uint8_t> mZoneSensorList;
        AmbientSensingUnionDelegate * mDelegate = nullptr;
    };

    AmbientSensingUnionCluster(const Config & config);

    // Server cluster implementation
    CHIP_ERROR Startup(ServerClusterContext & context) override;
    DataModel::ActionReturnStatus ReadAttribute(const DataModel::ReadAttributeRequest & request,
                                                AttributeValueEncoder & encoder) override;
    DataModel::ActionReturnStatus WriteAttribute(const DataModel::WriteAttributeRequest & request,
                                                 AttributeValueDecoder & decoder) override;
    CHIP_ERROR Attributes(const ConcreteClusterPath & path, ReadOnlyBufferBuilder<DataModel::AttributeEntry> & builder) override;

    // Getter methods
    uint64_t GetUnionId() const;
    uint8_t GetSensorId() const;
    bool HasUnionName() const;
    CharSpan GetUnionName() const;
    bool HasLeaderFeature() const;
    AmbientSensingUnion::UnionHealthEnum GetUnionHealth() const;
    Span<const uint8_t> GetUnionSensorList() const;
    Span<const AmbientSensingUnion::UnionHealthEnum> GetUnionSensorHealth() const;
    Span<const uint8_t> GetZoneSensorList() const;

    // Setter methods - handle validation, setting, delegate calls, persistence (NO notification)
    DataModel::ActionReturnStatus SetUnionName(const CharSpan & unionName);
    DataModel::ActionReturnStatus SetUnionHealth(AmbientSensingUnion::UnionHealthEnum unionHealth);
    DataModel::ActionReturnStatus SetUnionSensorList(const Span<const uint8_t> & sensorList);
    DataModel::ActionReturnStatus SetUnionSensorHealth(const Span<const AmbientSensingUnion::UnionHealthEnum> & sensorHealth);
    DataModel::ActionReturnStatus SetZoneSensorList(const Span<const uint8_t> & zoneSensorList);

    // Event generation
    CHIP_ERROR GenerateSensorListChangeEvent();

private:
    static constexpr size_t kMaxUnionSensorListSize = 128;
    static constexpr size_t kMaxZoneSensorListSize = 128;
    static constexpr size_t kMaxUnionSensorHealthSize = 128;

    AmbientSensingUnionDelegate * mDelegate;
    BitMask<AmbientSensingUnion::Feature> mFeatureMap;
    uint64_t mUnionId;
    uint8_t mSensorId;
    bool mHasUnionName;

    Storage::String<128> mUnionName;
    AmbientSensingUnion::UnionHealthEnum mUnionHealth = AmbientSensingUnion::UnionHealthEnum::kFullyFunctional;

    // Arrays with their backing storage
    uint8_t mUnionSensorListBuffer[kMaxUnionSensorListSize];
    Span<const uint8_t> mUnionSensorList;

    AmbientSensingUnion::UnionHealthEnum mUnionSensorHealthBuffer[kMaxUnionSensorHealthSize];
    Span<const AmbientSensingUnion::UnionHealthEnum> mUnionSensorHealth;

    uint8_t mZoneSensorListBuffer[kMaxZoneSensorListSize];
    Span<const uint8_t> mZoneSensorList;
};

} // namespace Clusters
} // namespace app
} // namespace chip
