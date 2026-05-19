/*
 *    Copyright (c) 2026 Project CHIP Authors
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

#include <algorithm>
#include <app/InteractionModelEngine.h>
#include <app/server-cluster/AttributeListBuilder.h>
#include <clusters/AmbientSensingUnion/Attributes.h>
#include <clusters/AmbientSensingUnion/Events.h>
#include <clusters/AmbientSensingUnion/Metadata.h>
#include <lib/support/CodeUtils.h>

namespace chip {
namespace app {
namespace Clusters {

using namespace AmbientSensingUnion;
using namespace AmbientSensingUnion::Attributes;

AmbientSensingUnionCluster::AmbientSensingUnionCluster(const Config & config) :
    DefaultServerCluster({ config.mEndpointId, AmbientSensingUnion::Id }),
    mDelegate(config.mDelegate),
    mContributorStorage(config.mContributorStorage),
    mPersistence(config.mPersistence),
    mUnionNameLength(0),
    mUnionHealth(config.mUnionHealth)
{
    mUnionNameBuffer[0] = '\0';

    if (!config.mUnionName.empty())
    {
        size_t len = std::min(config.mUnionName.size(), kMaxUnionNameLength);
        memcpy(mUnionNameBuffer, config.mUnionName.data(), len);
        mUnionNameBuffer[len] = '\0';
        mUnionNameLength      = len;
    }
}

CHIP_ERROR AmbientSensingUnionCluster::Startup(ServerClusterContext & context)
{
    ReturnErrorOnFailure(DefaultServerCluster::Startup(context));

    VerifyOrReturnError(mContributorStorage != nullptr, CHIP_ERROR_INVALID_ARGUMENT);

    if (mPersistence != nullptr)
    {
        CHIP_ERROR err = LoadPersistedAttributes();
        if (err != CHIP_NO_ERROR && err != CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND)
        {
            ChipLogError(Zcl, "AmbientSensingUnion: Failed to load persisted attributes: %" CHIP_ERROR_FORMAT, err.Format());
        }
    }

    // Recalculate union health based on current contributor state
    RecalculateUnionHealth();

    return CHIP_NO_ERROR;
}

void AmbientSensingUnionCluster::Shutdown(ClusterShutdownType shutdownType)
{
    DefaultServerCluster::Shutdown(shutdownType);
}

DataModel::ActionReturnStatus AmbientSensingUnionCluster::ReadAttribute(const DataModel::ReadAttributeRequest & request,
                                                                         AttributeValueEncoder & encoder)
{
    switch (request.path.mAttributeId)
    {
    case Attributes::FeatureMap::Id:
        // No features defined for this cluster in the spec
        return encoder.Encode(static_cast<uint32_t>(0));

    case Attributes::ClusterRevision::Id:
        return encoder.Encode(AmbientSensingUnion::kRevision);

    case Attributes::UnionName::Id:
        return encoder.Encode(GetUnionName());

    case Attributes::UnionHealth::Id:
        return encoder.Encode(mUnionHealth);

    case Attributes::UnionContributorList::Id:
        return EncodeContributorList(encoder);

    default:
        return Protocols::InteractionModel::Status::UnsupportedAttribute;
    }
}

DataModel::ActionReturnStatus AmbientSensingUnionCluster::WriteAttribute(const DataModel::WriteAttributeRequest & request,
                                                                          AttributeValueDecoder & decoder)
{
    switch (request.path.mAttributeId)
    {
    case UnionName::Id: {
        CharSpan newName;
        ReturnErrorOnFailure(decoder.Decode(newName));

        if (newName.size() > kMaxUnionNameLength)
        {
            return Protocols::InteractionModel::Status::ConstraintError;
        }

        CHIP_ERROR err = SetUnionName(newName);
        if (err != CHIP_NO_ERROR)
        {
            return Protocols::InteractionModel::Status::Failure;
        }
        return Protocols::InteractionModel::Status::Success;
    }

    default:
        return Protocols::InteractionModel::Status::UnsupportedWrite;
    }
}

CHIP_ERROR AmbientSensingUnionCluster::Attributes(const ConcreteClusterPath & path,
                                                   ReadOnlyBufferBuilder<DataModel::AttributeEntry> & builder)
{
    AttributeListBuilder listBuilder(builder);
    
    // No optional attributes for this cluster - all attributes are mandatory per spec
    return listBuilder.Append(Span(AmbientSensingUnion::Attributes::kMandatoryMetadata),
                              Span<const AttributeListBuilder::OptionalAttributeEntry>{});
}

CHIP_ERROR AmbientSensingUnionCluster::SetUnionName(const CharSpan & unionName)
{
    VerifyOrReturnError(unionName.size() <= kMaxUnionNameLength, CHIP_ERROR_INVALID_ARGUMENT);

    CharSpan currentName = GetUnionName();
    if (currentName.data_equal(unionName))
    {
        return CHIP_NO_ERROR;
    }

    mUnionNameLength = unionName.size();
    memcpy(mUnionNameBuffer, unionName.data(), mUnionNameLength);
    mUnionNameBuffer[mUnionNameLength] = '\0';

    NotifyAttributeChanged(UnionName::Id);

    if (mPersistence != nullptr)
    {
        CHIP_ERROR err = PersistUnionName();
        if (err != CHIP_NO_ERROR)
        {
            ChipLogError(Zcl, "AmbientSensingUnion: Failed to persist UnionName: %" CHIP_ERROR_FORMAT, err.Format());
        }
    }

    if (mDelegate != nullptr)
    {
        mDelegate->OnUnionNameChanged(GetUnionName());
    }

    return CHIP_NO_ERROR;
}

void AmbientSensingUnionCluster::SetUnionHealth(AmbientSensingUnion::UnionHealthEnum unionHealth)
{
    if (mUnionHealth == unionHealth)
    {
        return;
    }

    mUnionHealth = unionHealth;
    NotifyAttributeChanged(UnionHealth::Id);

    if (mDelegate != nullptr)
    {
        mDelegate->OnUnionHealthChanged(unionHealth);
    }
}

CharSpan AmbientSensingUnionCluster::GetUnionName() const
{
    return CharSpan(mUnionNameBuffer, mUnionNameLength);
}

AmbientSensingUnion::UnionHealthEnum AmbientSensingUnionCluster::GetUnionHealth() const
{
    return mUnionHealth;
}

size_t AmbientSensingUnionCluster::GetContributorCount() const
{
    if (mContributorStorage == nullptr)
    {
        return 0;
    }
    return mContributorStorage->GetTotalContributorCount();
}

CHIP_ERROR AmbientSensingUnionCluster::AddMatterContributor(NodeId nodeId, EndpointId endpointId,
                                                             AmbientSensingUnion::UnionContributorHealthEnum health)
{
    VerifyOrReturnError(mContributorStorage != nullptr, CHIP_ERROR_INCORRECT_STATE);

    ReturnErrorOnFailure(mContributorStorage->AddMatterContributor(nodeId, endpointId, health));

    NotifyAttributeChanged(UnionContributorList::Id);

    const MatterContributorEntry * entry = mContributorStorage->FindMatterContributor(nodeId, endpointId);
    if (entry == nullptr)
    {
        // This shouldn't happen since we just added it, but handle defensively
        ChipLogError(Zcl, "AmbientSensingUnion: Matter entry not found after add");
        RecalculateUnionHealth();
        return CHIP_NO_ERROR;
    }

    AmbientSensingUnion::Structs::UnionContributorStruct::Type contributor;
    entry->CopyTo(contributor);

    EmitUnionContributorAddedEvent(contributor);

    if (mDelegate != nullptr)
    {
        mDelegate->OnContributorAdded(contributor);
    }

    RecalculateUnionHealth();

    return CHIP_NO_ERROR;
}

CHIP_ERROR AmbientSensingUnionCluster::RemoveMatterContributor(NodeId nodeId, EndpointId endpointId)
{
    VerifyOrReturnError(mContributorStorage != nullptr, CHIP_ERROR_INCORRECT_STATE);

    const MatterContributorEntry * entry = mContributorStorage->FindMatterContributor(nodeId, endpointId);
    VerifyOrReturnError(entry != nullptr, CHIP_ERROR_NOT_FOUND);

    AmbientSensingUnion::Structs::UnionContributorStruct::Type contributor;
    entry->CopyTo(contributor);

    ReturnErrorOnFailure(mContributorStorage->RemoveMatterContributor(nodeId, endpointId));

    NotifyAttributeChanged(UnionContributorList::Id);

    EmitUnionContributorRemovedEvent(contributor);

    if (mDelegate != nullptr)
    {
        mDelegate->OnContributorRemoved(contributor);
    }

    RecalculateUnionHealth();

    return CHIP_NO_ERROR;
}

CHIP_ERROR AmbientSensingUnionCluster::UpdateMatterContributorHealth(NodeId nodeId, EndpointId endpointId,
                                                                      AmbientSensingUnion::UnionContributorHealthEnum health)
{
    VerifyOrReturnError(mContributorStorage != nullptr, CHIP_ERROR_INCORRECT_STATE);

    const MatterContributorEntry * entry = mContributorStorage->FindMatterContributor(nodeId, endpointId);
    VerifyOrReturnError(entry != nullptr, CHIP_ERROR_NOT_FOUND);

    if (entry->mHealth == health)
    {
        return CHIP_NO_ERROR;
    }

    ReturnErrorOnFailure(mContributorStorage->UpdateMatterContributorHealth(nodeId, endpointId, health));

    NotifyAttributeChanged(UnionContributorList::Id);

    entry = mContributorStorage->FindMatterContributor(nodeId, endpointId);
    if (entry == nullptr)
    {
        ChipLogError(Zcl, "AmbientSensingUnion: Matter entry not found after health update");
        RecalculateUnionHealth();
        return CHIP_NO_ERROR;
    }

    AmbientSensingUnion::Structs::UnionContributorStruct::Type contributor;
    entry->CopyTo(contributor);

    EmitUnionContributorHealthChangedEvent(contributor);

    if (mDelegate != nullptr)
    {
        mDelegate->OnContributorHealthChanged(contributor);
    }

    RecalculateUnionHealth();

    return CHIP_NO_ERROR;
}

CHIP_ERROR AmbientSensingUnionCluster::AddNonMatterContributor(const CharSpan & name,
                                                                AmbientSensingUnion::UnionContributorHealthEnum health)
{
    VerifyOrReturnError(mContributorStorage != nullptr, CHIP_ERROR_INCORRECT_STATE);

    // ContributorName is mandatory when NodeID is NULL
    VerifyOrReturnError(!name.empty(), CHIP_ERROR_INVALID_ARGUMENT);

    VerifyOrReturnError(name.size() <= NonMatterContributorEntry::kMaxNameLength, CHIP_ERROR_INVALID_ARGUMENT);

    ReturnErrorOnFailure(mContributorStorage->AddNonMatterContributor(name, health));

    NotifyAttributeChanged(UnionContributorList::Id);

    const NonMatterContributorEntry * entry = mContributorStorage->FindNonMatterContributor(name);
    if (entry == nullptr)
    {
        ChipLogError(Zcl, "AmbientSensingUnion: Non-Matter entry not found after add");
        RecalculateUnionHealth();
        return CHIP_NO_ERROR;
    }

    AmbientSensingUnion::Structs::UnionContributorStruct::Type contributor;
    entry->CopyTo(contributor);

    EmitUnionContributorAddedEvent(contributor);

    if (mDelegate != nullptr)
    {
        mDelegate->OnContributorAdded(contributor);
    }

    RecalculateUnionHealth();

    return CHIP_NO_ERROR;
}

CHIP_ERROR AmbientSensingUnionCluster::RemoveNonMatterContributor(const CharSpan & name)
{
    VerifyOrReturnError(mContributorStorage != nullptr, CHIP_ERROR_INCORRECT_STATE);

    const NonMatterContributorEntry * entry = mContributorStorage->FindNonMatterContributor(name);
    VerifyOrReturnError(entry != nullptr, CHIP_ERROR_NOT_FOUND);

    char localNameBuffer[NonMatterContributorEntry::kMaxNameLength + 1];
    CharSpan entryName = entry->GetName();
    size_t nameLength = std::min(entryName.size(), NonMatterContributorEntry::kMaxNameLength);
    memcpy(localNameBuffer, entryName.data(), nameLength);
    localNameBuffer[nameLength] = '\0';
    
    AmbientSensingUnion::UnionContributorHealthEnum entryHealth = entry->mHealth;

    ReturnErrorOnFailure(mContributorStorage->RemoveNonMatterContributor(name));

    NotifyAttributeChanged(UnionContributorList::Id);

    AmbientSensingUnion::Structs::UnionContributorStruct::Type contributor;
    contributor.contributorNodeID.SetNull();
    contributor.contributorEndpointID.SetNull();
    contributor.contributorName.SetValue(CharSpan(localNameBuffer, nameLength));
    contributor.contributorHealth = entryHealth;

    EmitUnionContributorRemovedEvent(contributor);

    if (mDelegate != nullptr)
    {
        mDelegate->OnContributorRemoved(contributor);
    }

    RecalculateUnionHealth();

    return CHIP_NO_ERROR;
}


CHIP_ERROR AmbientSensingUnionCluster::UpdateNonMatterContributorHealth(const CharSpan & name,
                                                                         AmbientSensingUnion::UnionContributorHealthEnum health)
{
    VerifyOrReturnError(mContributorStorage != nullptr, CHIP_ERROR_INCORRECT_STATE);

    const NonMatterContributorEntry * entry = mContributorStorage->FindNonMatterContributor(name);
    VerifyOrReturnError(entry != nullptr, CHIP_ERROR_NOT_FOUND);

    if (entry->mHealth == health)
    {
        return CHIP_NO_ERROR;
    }

    ReturnErrorOnFailure(mContributorStorage->UpdateNonMatterContributorHealth(name, health));

    NotifyAttributeChanged(UnionContributorList::Id);

    entry = mContributorStorage->FindNonMatterContributor(name);
    if (entry == nullptr)
    {
        ChipLogError(Zcl, "AmbientSensingUnion: Non-Matter entry not found after health update");
        RecalculateUnionHealth();
        return CHIP_NO_ERROR;
    }

    AmbientSensingUnion::Structs::UnionContributorStruct::Type contributor;
    entry->CopyTo(contributor);

    EmitUnionContributorHealthChangedEvent(contributor);

    if (mDelegate != nullptr)
    {
        mDelegate->OnContributorHealthChanged(contributor);
    }

    RecalculateUnionHealth();

    return CHIP_NO_ERROR;
}

void AmbientSensingUnionCluster::ClearAllContributors()
{
    if (mContributorStorage == nullptr)
    {
        return;
    }

    mContributorStorage->ClearAllContributors();

    NotifyAttributeChanged(UnionContributorList::Id);

    RecalculateUnionHealth();
}

void AmbientSensingUnionCluster::EmitUnionContributorAddedEvent(
    const AmbientSensingUnion::Structs::UnionContributorStruct::Type & contributor)
{
    if (mContext == nullptr)
    {
        return;
    }

    Events::UnionContributorAdded::Type event;
    event.addedContributor = contributor;

    // GenerateEvent returns std::optional<EventNumber>, ignore return value
    mContext->interactionContext.eventsGenerator.GenerateEvent(event, mPath.mEndpointId);
}

void AmbientSensingUnionCluster::EmitUnionContributorRemovedEvent(
    const AmbientSensingUnion::Structs::UnionContributorStruct::Type & contributor)
{
    if (mContext == nullptr)
    {
        return;
    }

    Events::UnionContributorRemoved::Type event;
    event.removedContributor = contributor;

    mContext->interactionContext.eventsGenerator.GenerateEvent(event, mPath.mEndpointId);
}

void AmbientSensingUnionCluster::EmitUnionContributorHealthChangedEvent(
    const AmbientSensingUnion::Structs::UnionContributorStruct::Type & contributor)
{
    if (mContext == nullptr)
    {
        return;
    }

    Events::UnionContributorHealthChanged::Type event;
    event.contributorHealth = contributor;

    mContext->interactionContext.eventsGenerator.GenerateEvent(event, mPath.mEndpointId);
}

CHIP_ERROR AmbientSensingUnionCluster::EncodeContributorList(AttributeValueEncoder & encoder)
{
    if (mContributorStorage == nullptr)
    {
        // Encode empty list
        return encoder.EncodeList([](const auto & listEncoder) { return CHIP_NO_ERROR; });
    }

    return encoder.EncodeList([this](const auto & listEncoder) {
        // Encode Matter contributors first
        CHIP_ERROR err = mContributorStorage->ForEachMatterContributor([&listEncoder](const MatterContributorEntry & entry) {
            AmbientSensingUnion::Structs::UnionContributorStruct::Type contributor;
            entry.CopyTo(contributor);

            CHIP_ERROR encodeErr = listEncoder.Encode(contributor);
            if (encodeErr != CHIP_NO_ERROR)
            {
                return Loop::Break;
            }
            return Loop::Continue;
        });

        ReturnErrorOnFailure(err);

        // Encode non-Matter contributors
        err = mContributorStorage->ForEachNonMatterContributor([&listEncoder](const NonMatterContributorEntry & entry) {
            AmbientSensingUnion::Structs::UnionContributorStruct::Type contributor;
            entry.CopyTo(contributor);

            CHIP_ERROR encodeErr = listEncoder.Encode(contributor);
            if (encodeErr != CHIP_NO_ERROR)
            {
                return Loop::Break;
            }
            return Loop::Continue;
        });

        return err;
    });
}

void AmbientSensingUnionCluster::RecalculateUnionHealth()
{
    if (mContributorStorage == nullptr)
    {
        SetUnionHealth(UnionHealthEnum::kNonFunctional);
        return;
    }

    size_t totalCount  = mContributorStorage->GetTotalContributorCount();
    size_t onlineCount = 0;

    // Count online Matter contributors
    CHIP_ERROR err = mContributorStorage->ForEachMatterContributor([&onlineCount](const MatterContributorEntry & entry) {
        if (entry.mHealth == UnionContributorHealthEnum::kUnionContributorOnline)
        {
            onlineCount++;
        }
        return Loop::Continue;
    });

    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(Zcl, "AmbientSensingUnion: Error iterating Matter contributors: %" CHIP_ERROR_FORMAT, err.Format());
    }

    // Count online non-Matter contributors
    err = mContributorStorage->ForEachNonMatterContributor([&onlineCount](const NonMatterContributorEntry & entry) {
        if (entry.mHealth == UnionContributorHealthEnum::kUnionContributorOnline)
        {
            onlineCount++;
        }
        return Loop::Continue;
    });

    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(Zcl, "AmbientSensingUnion: Error iterating non-Matter contributors: %" CHIP_ERROR_FORMAT, err.Format());
    }

    // Determine union health based on contributor health
    UnionHealthEnum newHealth;
    if (totalCount == 0)
    {
        // No contributors - non-functional
        newHealth = UnionHealthEnum::kNonFunctional;
    }
    else if (onlineCount == totalCount)
    {
        // All contributors online - fully functional
        newHealth = UnionHealthEnum::kFullyFunctional;
    }
    else if (onlineCount == 0)
    {
        // No contributors online - non-functional
        newHealth = UnionHealthEnum::kNonFunctional;
    }
    else
    {
        // Some contributors offline - limited/degraded
        newHealth = UnionHealthEnum::kLimitedDegraded;
    }

    SetUnionHealth(newHealth);
}

CHIP_ERROR AmbientSensingUnionCluster::LoadPersistedAttributes()
{
    if (mPersistence == nullptr)
    {
        return CHIP_ERROR_INCORRECT_STATE;
    }

    size_t loadedLength = 0;
    CHIP_ERROR err      = mPersistence->LoadUnionName(mUnionNameBuffer, kMaxUnionNameLength, loadedLength);

    if (err == CHIP_NO_ERROR)
    {
        mUnionNameLength               = loadedLength;
        mUnionNameBuffer[loadedLength] = '\0';
        ChipLogProgress(Zcl, "AmbientSensingUnion: Loaded persisted UnionName: %.*s", static_cast<int>(mUnionNameLength),
                        mUnionNameBuffer);
    }
    else if (err == CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND)
    {
        ChipLogProgress(Zcl, "AmbientSensingUnion: No persisted UnionName found, using default");
    }

    return err;
}

CHIP_ERROR AmbientSensingUnionCluster::PersistUnionName()
{
    if (mPersistence == nullptr)
    {
        return CHIP_ERROR_INCORRECT_STATE;
    }

    return mPersistence->SaveUnionName(GetUnionName());
}

} // namespace Clusters
} // namespace app
} // namespace chip
