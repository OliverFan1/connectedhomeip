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
    mFeatureMap(config.mFeatureMap), mHasUnionName(config.mHasUnionName), mUnionHealth(config.mUnionHealth)
{
    if (mHasUnionName && !config.mUnionName.empty())
    {    
        mUnionName.SetContent(config.mUnionName);
    }

    if (!config.mUnionContributorList.empty() && IsValidContributorListSize(config.mUnionContributorList.size()))
    {
        mUnionContributorList.assign(config.mUnionContributorList.begin(), config.mUnionContributorList.end());
    }
}

CHIP_ERROR AmbientSensingUnionCluster::Startup(ServerClusterContext & context)
{
    ReturnErrorOnFailure(DefaultServerCluster::Startup(context));

    if (mHasUnionName)
    {
        AttributePersistence persistence(context.attributeStorage);
        
        // Make a proper copy of the configured name before loading from persistence
        Storage::String<kMaxUnionNameSize> configuredUnionName;
        configuredUnionName.SetContent(mUnionName.Content());
        bool hasConfiguredValue = !configuredUnionName.Content().empty();
        
        // Try to load from persistence (this may overwrite mUnionName)
        bool loadedFromPersistence = persistence.LoadString(
            { mPath.mEndpointId, AmbientSensingUnion::Id, Attributes::UnionName::Id }, mUnionName);
        
        // If loading failed or resulted in empty name, and we had a configured value, restore it
        if ((!loadedFromPersistence || mUnionName.Content().empty()) && hasConfiguredValue)
        {
            mUnionName.SetContent(configuredUnionName.Content());
            // Store the configured value for future use
            CHIP_ERROR persistErr = persistence.StoreString({ mPath.mEndpointId, AmbientSensingUnion::Id, Attributes::UnionName::Id }, mUnionName);
            if (persistErr != CHIP_NO_ERROR)
            {
                ChipLogError(DataManagement, "Failed to persist UnionName: %" CHIP_ERROR_FORMAT, persistErr.Format());
            }
        }
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
    case Attributes::UnionContributorList::Id:
        if (!mFeatureMap.Has(Feature::kLeader))
        {
            return Protocols::InteractionModel::Status::UnsupportedAttribute;
        }
        return encoder.EncodeList([this](const auto & listEncoder) {
            for (const auto & contributor : mUnionContributorList)
            {
                ReturnErrorOnFailure(listEncoder.Encode(contributor));
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
        
        return SetUnionName(unionName);
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
        { mFeatureMap.Has(Feature::kLeader), Attributes::UnionContributorList::kMetadataEntry },
    };

    return listBuilder.Append(Span(AmbientSensingUnion::Attributes::kMandatoryMetadata), Span(optionalAttributes));
}

// Getter methods
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

Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type> AmbientSensingUnionCluster::GetUnionContributorList() const
{
    if (mUnionContributorList.empty())
    {
        return Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type>();
    }
    return Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type>(mUnionContributorList.data(), mUnionContributorList.size());
}

// Setter methods
DataModel::ActionReturnStatus AmbientSensingUnionCluster::SetUnionName(const CharSpan & unionName)
{
    VerifyOrReturnError(mHasUnionName, Protocols::InteractionModel::Status::UnsupportedAttribute);
    
    if (mUnionName.Content().data_equal(unionName))
    {
        return DataModel::ActionReturnStatus::FixedStatus::kWriteSuccessNoOp;
    }
    
    VerifyOrReturnError(unionName.size() <= kMaxUnionNameSize, Protocols::InteractionModel::Status::ConstraintError);
    VerifyOrReturnError(mUnionName.SetContent(unionName), Protocols::InteractionModel::Status::ConstraintError);
    NotifyAttributeChanged(Attributes::UnionName::Id);

    if (mDelegate)
    {
        mDelegate->OnUnionNameChanged(mPath.mEndpointId, mUnionName.Content());
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

bool AmbientSensingUnionCluster::IsValidUnionHealth(AmbientSensingUnion::UnionHealthEnum health) const
{
    return health == AmbientSensingUnion::UnionHealthEnum::kFullyFunctional ||
           health == AmbientSensingUnion::UnionHealthEnum::kLimitedDegraded ||
           health == AmbientSensingUnion::UnionHealthEnum::kNonFunctional;
}

DataModel::ActionReturnStatus AmbientSensingUnionCluster::SetUnionHealth(AmbientSensingUnion::UnionHealthEnum unionHealth)
{
    VerifyOrReturnError(mFeatureMap.Has(Feature::kLeader), Protocols::InteractionModel::Status::UnsupportedAttribute);
    VerifyOrReturnError(IsValidUnionHealth(unionHealth), Protocols::InteractionModel::Status::ConstraintError);
    
    if (mUnionHealth == unionHealth)
    {
        return DataModel::ActionReturnStatus::FixedStatus::kWriteSuccessNoOp;
    }
    
    mUnionHealth = unionHealth;
    NotifyAttributeChanged(Attributes::UnionHealth::Id);

    if (mDelegate)
    {
        mDelegate->OnUnionHealthChanged(mPath.mEndpointId, mUnionHealth);
    }
    
    return Protocols::InteractionModel::Status::Success;
}

DataModel::ActionReturnStatus AmbientSensingUnionCluster::SetUnionContributorList(const Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type> & contributorList)
{
    VerifyOrReturnError(mFeatureMap.Has(Feature::kLeader), Protocols::InteractionModel::Status::UnsupportedAttribute);
    VerifyOrReturnError(IsValidContributorListSize(contributorList.size()), 
                        Protocols::InteractionModel::Status::ConstraintError);
    
    // Check if the list is actually changing
    if (mUnionContributorList.size() == contributorList.size() && 
        std::equal(mUnionContributorList.begin(), mUnionContributorList.end(), contributorList.begin(),
                   [](const auto & a, const auto & b) {
                       return a.contributorNodeID == b.contributorNodeID &&
                              a.contributorEndpointID == b.contributorEndpointID &&
                              a.contributorHealth == b.contributorHealth;
                   }))
    {
        return DataModel::ActionReturnStatus::FixedStatus::kWriteSuccessNoOp;
    }
    
    mUnionContributorList.assign(contributorList.begin(), contributorList.end());
    
    NotifyAttributeChanged(Attributes::UnionContributorList::Id);

    if (mDelegate)
    {
        mDelegate->OnUnionContributorListChanged(mPath.mEndpointId, GetUnionContributorList());
    }
    
    // Generate contributor list change event
    CHIP_ERROR eventErr = GenerateUnionContributorListChangeEvent();
    if (eventErr != CHIP_NO_ERROR)
    {
        ChipLogError(DataManagement, "Failed to generate UnionContributorListChange event: %" CHIP_ERROR_FORMAT, eventErr.Format());
    }
    
    return Protocols::InteractionModel::Status::Success;
}

CHIP_ERROR AmbientSensingUnionCluster::GenerateUnionContributorListChangeEvent()
{
    VerifyOrReturnError(mFeatureMap.Has(Feature::kLeader), CHIP_ERROR_INCORRECT_STATE);

    if (mContext != nullptr)
    {
        Events::UnionContributorListChange::Type event;
        
        event.unionContributorList = DataModel::List<const AmbientSensingUnion::Structs::UnionMemberStruct::Type>(
            mUnionContributorList.data(), mUnionContributorList.size());
        
        mContext->interactionContext.eventsGenerator.GenerateEvent(event, mPath.mEndpointId);
    }

    return CHIP_NO_ERROR;
}

} // namespace chip::app::Clusters
