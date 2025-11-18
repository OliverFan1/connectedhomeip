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

#include <app/clusters/ambient-sensing-union-server/AmbientSensingUnionCluster.h>
#include <clusters/AmbientSensingUnion/Events.h>

#include <algorithm>
#include <app/InteractionModelEngine.h>
#include <app/persistence/AttributePersistence.h>
#include <app/server-cluster/AttributeListBuilder.h>
#include <clusters/AmbientSensingUnion/Attributes.h>
#include <clusters/AmbientSensingUnion/Metadata.h>
#include <cstring>
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>

namespace chip::app::Clusters {

using namespace AmbientSensingUnion::Attributes;
using namespace chip::app::Clusters::AmbientSensingUnion;

AmbientSensingUnionCluster::AmbientSensingUnionCluster(const Config & config) :
    DefaultServerCluster({ config.mEndpointId, AmbientSensingUnion::Id }), mDelegate(config.mDelegate),
    mFeatureMap(config.mFeatureMap), mUnionId(config.mUnionId), mSensorId(config.mSensorId), mHasUnionName(config.mHasUnionName),
    mUnionSensorList(), mUnionSensorHealth(), mZoneSensorList()
{
    // Initialize union name if provided
    if (mHasUnionName && !config.mUnionName.empty())
    {
        mUnionName.SetContent(config.mUnionName);
    }

    // Initialize sensor lists if provided
    if (!config.mUnionSensorList.empty() && config.mUnionSensorList.size() <= sizeof(mUnionSensorListBuffer))
    {
        memcpy(mUnionSensorListBuffer, config.mUnionSensorList.data(), config.mUnionSensorList.size());
        mUnionSensorList = Span<const uint8_t>(mUnionSensorListBuffer, config.mUnionSensorList.size());
    }

    if (!config.mZoneSensorList.empty() && config.mZoneSensorList.size() <= sizeof(mZoneSensorListBuffer))
    {
        memcpy(mZoneSensorListBuffer, config.mZoneSensorList.data(), config.mZoneSensorList.size());
        mZoneSensorList = Span<const uint8_t>(mZoneSensorListBuffer, config.mZoneSensorList.size());
    }
}

CHIP_ERROR AmbientSensingUnionCluster::Startup(ServerClusterContext & context)
{
    ReturnErrorOnFailure(DefaultServerCluster::Startup(context));

    if (mHasUnionName)
    {
        // Load persisted union name using the same pattern as BasicInformation
        AttributePersistence persistence(context.attributeStorage);
        (void) persistence.LoadString({ mPath.mEndpointId, AmbientSensingUnion::Id, Attributes::UnionName::Id }, mUnionName);
    }

    return CHIP_NO_ERROR;
}

DataModel::ActionReturnStatus AmbientSensingUnionCluster::ReadAttribute(const DataModel::ReadAttributeRequest & request,
                                                                        AttributeValueEncoder & encoder)
{
    switch (request.path.mAttributeId)
    {
    case Attributes::FeatureMap::Id:
        return encoder.Encode(mFeatureMap);
    case Attributes::ClusterRevision::Id:
        return encoder.Encode(AmbientSensingUnion::kRevision);
    case Attributes::UnionID::Id:
        return encoder.Encode(mUnionId);
    case Attributes::UnionName::Id:
        if (!mHasUnionName)
        {
            return Protocols::InteractionModel::Status::UnsupportedAttribute;
        }
        return encoder.Encode(mUnionName.Content());
    case Attributes::UnionHealth::Id:
        if (!mFeatureMap.Has(Feature::kLeader))
        {
            return Protocols::InteractionModel::Status::UnsupportedAttribute;
        }
        return encoder.Encode(mUnionHealth);
    case Attributes::UnionSensorList::Id:
        if (!mFeatureMap.Has(Feature::kLeader))
        {
            return Protocols::InteractionModel::Status::UnsupportedAttribute;
        }
        return encoder.EncodeList([this](const auto & listEncoder) {
            for (auto sensorId : mUnionSensorList)
            {
                ReturnErrorOnFailure(listEncoder.Encode(sensorId));
            }
            return CHIP_NO_ERROR;
        });
    case Attributes::UnionSensorHealth::Id:
        if (!mFeatureMap.Has(Feature::kLeader))
        {
            return Protocols::InteractionModel::Status::UnsupportedAttribute;
        }
        return encoder.EncodeList([this](const auto & listEncoder) {
            for (auto healthStatus : mUnionSensorHealth)
            {
                ReturnErrorOnFailure(listEncoder.Encode(healthStatus));
            }
            return CHIP_NO_ERROR;
        });
    case Attributes::SensorID::Id:
        return encoder.Encode(mSensorId);
    case Attributes::ZoneSensorList::Id:
        if (!mFeatureMap.Has(Feature::kLeader))
        {
            return Protocols::InteractionModel::Status::UnsupportedAttribute;
        }
        return encoder.EncodeList([this](const auto & listEncoder) {
            for (auto sensorId : mZoneSensorList)
            {
                ReturnErrorOnFailure(listEncoder.Encode(sensorId));
            }
            return CHIP_NO_ERROR;
        });
    default:
        return Protocols::InteractionModel::Status::UnsupportedAttribute;
    }
}

DataModel::ActionReturnStatus AmbientSensingUnionCluster::WriteAttribute(const DataModel::WriteAttributeRequest & request,
                                                                         AttributeValueDecoder & aDecoder)
{
    VerifyOrDie(request.path.mClusterId == AmbientSensingUnion::Id);

    switch (request.path.mAttributeId)
    {
    case Attributes::UnionName::Id: {
        if (!mHasUnionName)
        {
            return Protocols::InteractionModel::Status::UnsupportedAttribute;
        }
        
        CharSpan unionName;
        ReturnErrorOnFailure(aDecoder.Decode(unionName));
        
        // Let SetUnionName handle all validation and work
        return NotifyAttributeChangedIfSuccess(request.path.mAttributeId, SetUnionName(unionName));
    }
    default:
        return Protocols::InteractionModel::Status::UnsupportedWrite;
    }
}

CHIP_ERROR AmbientSensingUnionCluster::Attributes(const ConcreteClusterPath & clusterPath,
                                                  ReadOnlyBufferBuilder<DataModel::AttributeEntry> & builder)
{
    AttributeListBuilder listBuilder(builder);

    const AttributeListBuilder::OptionalAttributeEntry optionalAttributes[] = {
        { mHasUnionName, Attributes::UnionName::kMetadataEntry },
        { mFeatureMap.Has(Feature::kLeader), Attributes::UnionHealth::kMetadataEntry },
        { mFeatureMap.Has(Feature::kLeader), Attributes::UnionSensorList::kMetadataEntry },
        { mFeatureMap.Has(Feature::kLeader), Attributes::UnionSensorHealth::kMetadataEntry },
        { mFeatureMap.Has(Feature::kLeader), Attributes::ZoneSensorList::kMetadataEntry },
    };

    return listBuilder.Append(Span(AmbientSensingUnion::Attributes::kMandatoryMetadata), Span(optionalAttributes));
}

// Getter methods
uint64_t AmbientSensingUnionCluster::GetUnionId() const
{
    return mUnionId;
}

uint8_t AmbientSensingUnionCluster::GetSensorId() const
{
    return mSensorId;
}

bool AmbientSensingUnionCluster::HasUnionName() const
{
    return mHasUnionName;
}

CharSpan AmbientSensingUnionCluster::GetUnionName() const
{
    return mUnionName.Content();
}

bool AmbientSensingUnionCluster::HasLeaderFeature() const
{
    return mFeatureMap.Has(Feature::kLeader);
}

AmbientSensingUnion::UnionHealthEnum AmbientSensingUnionCluster::GetUnionHealth() const
{
    return mUnionHealth;
}

Span<const uint8_t> AmbientSensingUnionCluster::GetUnionSensorList() const
{
    return mUnionSensorList;
}

Span<const AmbientSensingUnion::UnionHealthEnum> AmbientSensingUnionCluster::GetUnionSensorHealth() const
{
    return mUnionSensorHealth;
}

Span<const uint8_t> AmbientSensingUnionCluster::GetZoneSensorList() const
{
    return mZoneSensorList;
}

// Setter methods - handle all work but NO notification (let caller handle notification)
DataModel::ActionReturnStatus AmbientSensingUnionCluster::SetUnionName(const CharSpan & unionName)
{
    VerifyOrReturnError(mHasUnionName, Protocols::InteractionModel::Status::UnsupportedAttribute);
    
    if (mUnionName.Content().data_equal(unionName))
    {
        return DataModel::ActionReturnStatus::FixedStatus::kWriteSuccessNoOp;
    }
    
    VerifyOrReturnError(unionName.size() <= 128, Protocols::InteractionModel::Status::ConstraintError);
    VerifyOrReturnError(mUnionName.SetContent(unionName), Protocols::InteractionModel::Status::ConstraintError);
    
    if (mDelegate)
    {
        mDelegate->OnUnionNameChanged(mUnionName.Content());
    }
    
    if (mContext != nullptr)
    {
        AttributePersistence persistence(mContext->attributeStorage);
        CHIP_ERROR persistErr = persistence.StoreString(
            { mPath.mEndpointId, AmbientSensingUnion::Id, Attributes::UnionName::Id }, mUnionName);
        if (persistErr != CHIP_NO_ERROR)
        {
            ChipLogError(DataManagement, "Failed to persist UnionName: %" CHIP_ERROR_FORMAT, persistErr.Format());
        }
    }
    
    return Protocols::InteractionModel::Status::Success;
}

DataModel::ActionReturnStatus AmbientSensingUnionCluster::SetUnionHealth(AmbientSensingUnion::UnionHealthEnum unionHealth)
{
    VerifyOrReturnError(mFeatureMap.Has(Feature::kLeader), Protocols::InteractionModel::Status::UnsupportedAttribute);
    
    if (mUnionHealth == unionHealth)
    {
        return DataModel::ActionReturnStatus::FixedStatus::kWriteSuccessNoOp;
    }
    
    mUnionHealth = unionHealth;
    
    if (mDelegate)
    {
        mDelegate->OnUnionHealthChanged(mUnionHealth);
    }
    
    return Protocols::InteractionModel::Status::Success;
}

DataModel::ActionReturnStatus AmbientSensingUnionCluster::SetUnionSensorList(const Span<const uint8_t> & sensorList)
{
    VerifyOrReturnError(mFeatureMap.Has(Feature::kLeader), Protocols::InteractionModel::Status::UnsupportedAttribute);
    VerifyOrReturnError(sensorList.size() <= sizeof(mUnionSensorListBuffer), 
                        Protocols::InteractionModel::Status::ConstraintError);
    
    // Check if the list is actually changing
    if (mUnionSensorList.size() == sensorList.size() && 
        memcmp(mUnionSensorList.data(), sensorList.data(), sensorList.size()) == 0)
    {
        return DataModel::ActionReturnStatus::FixedStatus::kWriteSuccessNoOp;
    }
    
    // Copy to our buffer
    memcpy(mUnionSensorListBuffer, sensorList.data(), sensorList.size());
    mUnionSensorList = Span<const uint8_t>(mUnionSensorListBuffer, sensorList.size());
    
    if (mDelegate)
    {
        mDelegate->OnUnionSensorListChanged(mUnionSensorList);
    }
    
    // Generate sensor list change event
    CHIP_ERROR eventErr = GenerateSensorListChangeEvent();
    if (eventErr != CHIP_NO_ERROR)
    {
        ChipLogError(DataManagement, "Failed to generate SensorListChange event: %" CHIP_ERROR_FORMAT, eventErr.Format());
    }
    
    return Protocols::InteractionModel::Status::Success;
}

DataModel::ActionReturnStatus AmbientSensingUnionCluster::SetUnionSensorHealth(const Span<const AmbientSensingUnion::UnionHealthEnum> & sensorHealth)
{
    VerifyOrReturnError(mFeatureMap.Has(Feature::kLeader), Protocols::InteractionModel::Status::UnsupportedAttribute);
    VerifyOrReturnError(sensorHealth.size() <= sizeof(mUnionSensorHealthBuffer) / sizeof(mUnionSensorHealthBuffer[0]), 
                        Protocols::InteractionModel::Status::ConstraintError);
    
    // Check if the health list is actually changing
    if (mUnionSensorHealth.size() == sensorHealth.size() && 
        memcmp(mUnionSensorHealth.data(), sensorHealth.data(), 
               sensorHealth.size() * sizeof(AmbientSensingUnion::UnionHealthEnum)) == 0)
    {
        return DataModel::ActionReturnStatus::FixedStatus::kWriteSuccessNoOp;
    }
    
    // Copy to our buffer
    memcpy(mUnionSensorHealthBuffer, sensorHealth.data(), 
           sensorHealth.size() * sizeof(AmbientSensingUnion::UnionHealthEnum));
    mUnionSensorHealth = Span<const AmbientSensingUnion::UnionHealthEnum>(mUnionSensorHealthBuffer, sensorHealth.size());
    
    if (mDelegate)
    {
        mDelegate->OnUnionSensorHealthChanged(mUnionSensorHealth);
    }
    
    return Protocols::InteractionModel::Status::Success;
}

DataModel::ActionReturnStatus AmbientSensingUnionCluster::SetZoneSensorList(const Span<const uint8_t> & zoneSensorList)
{
    VerifyOrReturnError(mFeatureMap.Has(Feature::kLeader), Protocols::InteractionModel::Status::UnsupportedAttribute);
    VerifyOrReturnError(zoneSensorList.size() <= sizeof(mZoneSensorListBuffer), 
                        Protocols::InteractionModel::Status::ConstraintError);
    
    // Check if the list is actually changing
    if (mZoneSensorList.size() == zoneSensorList.size() && 
        memcmp(mZoneSensorList.data(), zoneSensorList.data(), zoneSensorList.size()) == 0)
    {
        return DataModel::ActionReturnStatus::FixedStatus::kWriteSuccessNoOp;
    }
    
    // Copy to our buffer
    memcpy(mZoneSensorListBuffer, zoneSensorList.data(), zoneSensorList.size());
    mZoneSensorList = Span<const uint8_t>(mZoneSensorListBuffer, zoneSensorList.size());
    
    if (mDelegate)
    {
        mDelegate->OnZoneSensorListChanged(mZoneSensorList);
    }
    
    return Protocols::InteractionModel::Status::Success;
}

CHIP_ERROR AmbientSensingUnionCluster::GenerateSensorListChangeEvent()
{
    VerifyOrReturnError(mFeatureMap.Has(Feature::kLeader), CHIP_ERROR_INCORRECT_STATE);

    if (mContext != nullptr)
    {
        Events::SensorListChange::Type event;
        
        // Create the event with current union sensor list
        // Cast away const since event system will copy the data anyway
        event.unionSensorList = DataModel::List<uint8_t>(const_cast<uint8_t*>(mUnionSensorList.data()), mUnionSensorList.size());
        
        mContext->interactionContext.eventsGenerator.GenerateEvent(event, mPath.mEndpointId);
    }

    return CHIP_NO_ERROR;
}

} // namespace chip::app::Clusters
