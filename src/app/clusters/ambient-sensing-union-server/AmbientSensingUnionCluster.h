/*
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
#pragma once

#include <app-common/zap-generated/cluster-objects.h>
#include <app/server-cluster/DefaultServerCluster.h>
#include <app/server-cluster/ServerClusterContext.h>
#include <lib/core/CHIPError.h>
#include <lib/core/DataModelTypes.h>
#include <lib/core/Optional.h>
#include <lib/support/Pool.h>

#include <cstdint>
#include <cstring>

namespace chip {
namespace app {
namespace Clusters {

/**
 * @brief Delegate interface for the Ambient Sensing Union cluster.
 *
 * Applications should implement this interface to receive notifications
 * about changes in the ambient sensing union state.
 */
class AmbientSensingUnionDelegate
{
public:
    virtual ~AmbientSensingUnionDelegate() = default;

    /**
     * @brief Called when the union name changes.
     * @param unionName The new union name.
     */
    virtual void OnUnionNameChanged(const CharSpan & unionName) = 0;

    /**
     * @brief Called when the union health changes.
     * @param unionHealth The new union health state.
     */
    virtual void OnUnionHealthChanged(AmbientSensingUnion::UnionHealthEnum unionHealth) = 0;

    /**
     * @brief Called when a contributor is added to the union.
     * @param contributor The contributor that was added.
     */
    virtual void OnContributorAdded(const AmbientSensingUnion::Structs::UnionContributorStruct::Type & contributor) = 0;

    /**
     * @brief Called when a contributor is removed from the union.
     * @param contributor The contributor that was removed.
     */
    virtual void OnContributorRemoved(const AmbientSensingUnion::Structs::UnionContributorStruct::Type & contributor) = 0;

    /**
     * @brief Called when a contributor's health changes.
     * @param contributor The contributor whose health changed.
     */
    virtual void OnContributorHealthChanged(const AmbientSensingUnion::Structs::UnionContributorStruct::Type & contributor) = 0;
};

/**
 * @brief Persistence delegate for non-volatile attributes.
 *
 */
class AmbientSensingUnionPersistenceDelegate
{
public:
    virtual ~AmbientSensingUnionPersistenceDelegate() = default;

    /**
     * @brief Load the persisted union name.
     * @param buffer Buffer to store the name.
     * @param bufferSize Size of the buffer.
     * @param outLength Output: actual length of the name.
     * @return CHIP_NO_ERROR on success, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND if not found.
     */
    virtual CHIP_ERROR LoadUnionName(char * buffer, size_t bufferSize, size_t & outLength) = 0;

    /**
     * @brief Persist the union name.
     * @param unionName The name to persist.
     * @return CHIP_NO_ERROR on success.
     */
    virtual CHIP_ERROR SaveUnionName(const CharSpan & unionName) = 0;
};

/**
 * @brief Storage for Matter contributors (NodeID is not null).
 *
 */
struct MatterContributorEntry
{
    NodeId mNodeId       = kUndefinedNodeId;
    EndpointId mEndpointId = kInvalidEndpointId;
    AmbientSensingUnion::UnionContributorHealthEum mHealth =
        AmbientSensingUnion::UnionContributorHealthEum::kUnionContributorOffline;

    bool IsValid() const { return mNodeId != kUndefinedNodeId; }
    void Clear() { mNodeId = kUndefinedNodeId; }

    void CopyTo(AmbientSensingUnion::Structs::UnionContributorStruct::Type & dest) const
    {
        dest.contributorNodeID.SetNonNull(mNodeId);
        dest.contributorEndpointID.SetNonNull(mEndpointId);
        // ContributorName is optional for Matter contributors, leave unset
        dest.contributorHealth = mHealth;
    }
};
static_assert(sizeof(MatterContributorEntry) <= 16, "MatterContributorEntry should be compact");

/**
 * @brief Storage for non-Matter contributors (NodeID is null).
 *
 */
struct NonMatterContributorEntry
{
    static constexpr size_t kMaxNameLength = 128;

    char mName[kMaxNameLength + 1] = {0};
    size_t mNameLength             = 0;
    AmbientSensingUnion::UnionContributorHealthEum mHealth =
        AmbientSensingUnion::UnionContributorHealthEum::kUnionContributorOffline;

    bool IsValid() const { return mNameLength > 0; }
    void Clear() { mNameLength = 0; }

    void SetName(const CharSpan & name)
    {
        mNameLength = std::min(name.size(), kMaxNameLength);
        memcpy(mName, name.data(), mNameLength);
        mName[mNameLength] = '\0';
    }

    CharSpan GetName() const { return CharSpan(mName, mNameLength); }

    void CopyTo(AmbientSensingUnion::Structs::UnionContributorStruct::Type & dest) const
    {
        dest.contributorNodeID.SetNull();
        dest.contributorEndpointID.SetNull();
        dest.contributorName.SetValue(GetName());
        dest.contributorHealth = mHealth;
    }
};

/**
 * @brief Storage provider interface for contributor list.
 */
class ContributorStorageDelegate
{
public:
    virtual ~ContributorStorageDelegate() = default;

    // Matter contributor operations
    virtual CHIP_ERROR AddMatterContributor(NodeId nodeId, EndpointId endpointId,
                                            AmbientSensingUnion::UnionContributorHealthEum health) = 0;
    virtual CHIP_ERROR RemoveMatterContributor(NodeId nodeId, EndpointId endpointId)               = 0;
    virtual CHIP_ERROR UpdateMatterContributorHealth(NodeId nodeId, EndpointId endpointId,
                                                     AmbientSensingUnion::UnionContributorHealthEum health) = 0;
    virtual const MatterContributorEntry * FindMatterContributor(NodeId nodeId, EndpointId endpointId) const = 0;

    // Non-Matter contributor operations
    virtual CHIP_ERROR AddNonMatterContributor(const CharSpan & name,
                                               AmbientSensingUnion::UnionContributorHealthEum health) = 0;
    virtual CHIP_ERROR RemoveNonMatterContributor(const CharSpan & name)                              = 0;
    virtual CHIP_ERROR UpdateNonMatterContributorHealth(const CharSpan & name,
                                                        AmbientSensingUnion::UnionContributorHealthEum health) = 0;
    virtual const NonMatterContributorEntry * FindNonMatterContributor(const CharSpan & name) const = 0;

    // Common operations
    virtual void ClearAllContributors()          = 0;
    virtual size_t GetMatterContributorCount() const    = 0;
    virtual size_t GetNonMatterContributorCount() const = 0;
    size_t GetTotalContributorCount() const { return GetMatterContributorCount() + GetNonMatterContributorCount(); }

    // Iteration support for attribute encoding
    virtual CHIP_ERROR ForEachMatterContributor(
        std::function<Loop(const MatterContributorEntry &)> callback) const = 0;
    virtual CHIP_ERROR ForEachNonMatterContributor(
        std::function<Loop(const NonMatterContributorEntry &)> callback) const = 0;
};

/**
 * @brief Default in-memory contributor storage with separate pools for efficiency.
 *
 * @tparam kMaxMatterContributors Maximum number of Matter contributors (common case).
 * @tparam kMaxNonMatterContributors Maximum number of non-Matter contributors (rare case).
 */
template <size_t kMaxMatterContributors = 32, size_t kMaxNonMatterContributors = 8>
class DefaultContributorStorage : public ContributorStorageDelegate
{
public:
    static_assert(kMaxMatterContributors + kMaxNonMatterContributors <= 128,
                  "Total contributors cannot exceed spec limit of 128");

    DefaultContributorStorage()  = default;
    ~DefaultContributorStorage() override = default;

    // Matter contributor operations
    CHIP_ERROR AddMatterContributor(NodeId nodeId, EndpointId endpointId,
                                    AmbientSensingUnion::UnionContributorHealthEum health) override
    {
        if (FindMatterContributor(nodeId, endpointId) != nullptr)
        {
            return CHIP_ERROR_DUPLICATE_KEY_ID;
        }

        MatterContributorEntry * entry = mMatterPool.CreateObject();
        if (entry == nullptr)
        {
            return CHIP_ERROR_NO_MEMORY;
        }

        entry->mNodeId     = nodeId;
        entry->mEndpointId = endpointId;
        entry->mHealth     = health;
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR RemoveMatterContributor(NodeId nodeId, EndpointId endpointId) override
    {
        MatterContributorEntry * entry = FindMatterContributorMutable(nodeId, endpointId);
        if (entry == nullptr)
        {
            return CHIP_ERROR_NOT_FOUND;
        }
        mMatterPool.ReleaseObject(entry);
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR UpdateMatterContributorHealth(NodeId nodeId, EndpointId endpointId,
                                             AmbientSensingUnion::UnionContributorHealthEum health) override
    {
        MatterContributorEntry * entry = FindMatterContributorMutable(nodeId, endpointId);
        if (entry == nullptr)
        {
            return CHIP_ERROR_NOT_FOUND;
        }
        entry->mHealth = health;
        return CHIP_NO_ERROR;
    }

    const MatterContributorEntry * FindMatterContributor(NodeId nodeId, EndpointId endpointId) const override
    {
        const MatterContributorEntry * result = nullptr;
        mMatterPool.ForEachActiveObject([&](const MatterContributorEntry * entry) {
            if (entry->mNodeId == nodeId && entry->mEndpointId == endpointId)
            {
                result = entry;
                return Loop::Break;
            }
            return Loop::Continue;
        });
        return result;
    }

    // Non-Matter contributor operations
    CHIP_ERROR AddNonMatterContributor(const CharSpan & name,
                                       AmbientSensingUnion::UnionContributorHealthEum health) override
    {
        if (name.empty())
        {
            // ContributorName is mandatory when NodeID is NULL
            return CHIP_ERROR_INVALID_ARGUMENT;
        }

        if (FindNonMatterContributor(name) != nullptr)
        {
            return CHIP_ERROR_DUPLICATE_KEY_ID;
        }

        NonMatterContributorEntry * entry = mNonMatterPool.CreateObject();
        if (entry == nullptr)
        {
            return CHIP_ERROR_NO_MEMORY;
        }

        entry->SetName(name);
        entry->mHealth = health;
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR RemoveNonMatterContributor(const CharSpan & name) override
    {
        NonMatterContributorEntry * entry = FindNonMatterContributorMutable(name);
        if (entry == nullptr)
        {
            return CHIP_ERROR_NOT_FOUND;
        }
        mNonMatterPool.ReleaseObject(entry);
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR UpdateNonMatterContributorHealth(const CharSpan & name,
                                                AmbientSensingUnion::UnionContributorHealthEum health) override
    {
        NonMatterContributorEntry * entry = FindNonMatterContributorMutable(name);
        if (entry == nullptr)
        {
            return CHIP_ERROR_NOT_FOUND;
        }
        entry->mHealth = health;
        return CHIP_NO_ERROR;
    }

    const NonMatterContributorEntry * FindNonMatterContributor(const CharSpan & name) const override
    {
        const NonMatterContributorEntry * result = nullptr;
        mNonMatterPool.ForEachActiveObject([&](const NonMatterContributorEntry * entry) {
            if (entry->GetName().data_equal(name))
            {
                result = entry;
                return Loop::Break;
            }
            return Loop::Continue;
        });
        return result;
    }

    // Common operations
    void ClearAllContributors() override
    {
        mMatterPool.ReleaseAll();
        mNonMatterPool.ReleaseAll();
    }

    size_t GetMatterContributorCount() const override { return mMatterPool.Allocated(); }
    size_t GetNonMatterContributorCount() const override { return mNonMatterPool.Allocated(); }

    // Iteration support
    CHIP_ERROR ForEachMatterContributor(
        std::function<Loop(const MatterContributorEntry &)> callback) const override
    {
        mMatterPool.ForEachActiveObject([&](const MatterContributorEntry * entry) {
            return callback(*entry);
        });
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR ForEachNonMatterContributor(
        std::function<Loop(const NonMatterContributorEntry &)> callback) const override
    {
        mNonMatterPool.ForEachActiveObject([&](const NonMatterContributorEntry * entry) {
            return callback(*entry);
        });
        return CHIP_NO_ERROR;
    }

private:
    MatterContributorEntry * FindMatterContributorMutable(NodeId nodeId, EndpointId endpointId)
    {
        MatterContributorEntry * result = nullptr;
        mMatterPool.ForEachActiveObject([&](MatterContributorEntry * entry) {
            if (entry->mNodeId == nodeId && entry->mEndpointId == endpointId)
            {
                result = entry;
                return Loop::Break;
            }
            return Loop::Continue;
        });
        return result;
    }

    NonMatterContributorEntry * FindNonMatterContributorMutable(const CharSpan & name)
    {
        NonMatterContributorEntry * result = nullptr;
        mNonMatterPool.ForEachActiveObject([&](NonMatterContributorEntry * entry) {
            if (entry->GetName().data_equal(name))
            {
                result = entry;
                return Loop::Break;
            }
            return Loop::Continue;
        });
        return result;
    }

    ObjectPool<MatterContributorEntry, kMaxMatterContributors> mMatterPool;
    ObjectPool<NonMatterContributorEntry, kMaxNonMatterContributors> mNonMatterPool;
};

/**
 * @brief Server-side implementation of the Ambient Sensing Union cluster.
 *
 */
class AmbientSensingUnionCluster : public DefaultServerCluster
{
public:
    static constexpr size_t kMaxUnionNameLength = 128;

    /**
     * @brief Configuration structure for the Ambient Sensing Union cluster.
     */
    struct Config
    {
        Config(EndpointId endpointId) : mEndpointId(endpointId) {}

        Config & WithUnionName(const CharSpan & unionName)
        {
            mUnionName = unionName;
            return *this;
        }

        Config & WithUnionHealth(AmbientSensingUnion::UnionHealthEnum unionHealth)
        {
            mUnionHealth = unionHealth;
            return *this;
        }

        Config & WithDelegate(AmbientSensingUnionDelegate * delegate)
        {
            mDelegate = delegate;
            return *this;
        }

        Config & WithContributorStorage(ContributorStorageDelegate * storage)
        {
            mContributorStorage = storage;
            return *this;
        }

        Config & WithPersistence(AmbientSensingUnionPersistenceDelegate * persistence)
        {
            mPersistence = persistence;
            return *this;
        }

        EndpointId mEndpointId;
        CharSpan mUnionName                                       = CharSpan();
        AmbientSensingUnion::UnionHealthEnum mUnionHealth         = AmbientSensingUnion::UnionHealthEnum::kFullyFunctional;
        AmbientSensingUnionDelegate * mDelegate                   = nullptr;
        ContributorStorageDelegate * mContributorStorage          = nullptr;
        AmbientSensingUnionPersistenceDelegate * mPersistence     = nullptr;
    };

    /**
     * @brief Constructs an AmbientSensingUnionCluster with the given configuration.
     * @param config The configuration for the cluster.
     */
    explicit AmbientSensingUnionCluster(const Config & config);
    ~AmbientSensingUnionCluster() override = default;

    // Disallow copy and move
    AmbientSensingUnionCluster(const AmbientSensingUnionCluster &)             = delete;
    AmbientSensingUnionCluster & operator=(const AmbientSensingUnionCluster &) = delete;
    AmbientSensingUnionCluster(AmbientSensingUnionCluster &&)                  = delete;
    AmbientSensingUnionCluster & operator=(AmbientSensingUnionCluster &&)      = delete;

    // DefaultServerCluster overrides
    CHIP_ERROR Startup(ServerClusterContext & context) override;
    void Shutdown(ClusterShutdownType shutdownType) override;
    DataModel::ActionReturnStatus ReadAttribute(const DataModel::ReadAttributeRequest & request,
                                                AttributeValueEncoder & encoder) override;
    DataModel::ActionReturnStatus WriteAttribute(const DataModel::WriteAttributeRequest & request,
                                                 AttributeValueDecoder & decoder) override;
    CHIP_ERROR Attributes(const ConcreteClusterPath & path, ReadOnlyBufferBuilder<DataModel::AttributeEntry> & builder) override;

    /**
     * @brief Sets the delegate for application callbacks.
     * @param delegate The delegate to use, or nullptr to clear.
     */
    void SetDelegate(AmbientSensingUnionDelegate * delegate) { mDelegate = delegate; }

    // Attribute setters
    /**
     * @brief Sets the union name (will be persisted if persistence delegate is provided).
     * @param unionName The new union name (max 128 characters).
     * @return CHIP_ERROR_INVALID_ARGUMENT if name exceeds max length, CHIP_NO_ERROR otherwise.
     */
    CHIP_ERROR SetUnionName(const CharSpan & unionName);

    /**
     * @brief Sets the union health state.
     * @param unionHealth The new union health state.
     */
    void SetUnionHealth(AmbientSensingUnion::UnionHealthEnum unionHealth);

    // Attribute getters
    CharSpan GetUnionName() const;
    AmbientSensingUnion::UnionHealthEnum GetUnionHealth() const;
    size_t GetContributorCount() const;

    // Matter contributor management (NodeID is not null)
    /**
     * @brief Adds a Matter contributor to the union.
     * @param nodeId The node ID of the contributor.
     * @param endpointId The endpoint ID of the contributor.
     * @param health The initial health state.
     * @return CHIP_ERROR_NO_MEMORY if full, CHIP_ERROR_DUPLICATE_KEY_ID if exists.
     */
    CHIP_ERROR AddMatterContributor(NodeId nodeId, EndpointId endpointId,
                                    AmbientSensingUnion::UnionContributorHealthEum health =
                                        AmbientSensingUnion::UnionContributorHealthEum::kUnionContributorOnline);

    /**
     * @brief Removes a Matter contributor from the union.
     * @param nodeId The node ID of the contributor.
     * @param endpointId The endpoint ID of the contributor.
     * @return CHIP_ERROR_NOT_FOUND if not found.
     */
    CHIP_ERROR RemoveMatterContributor(NodeId nodeId, EndpointId endpointId);

    /**
     * @brief Updates a Matter contributor's health state.
     * @param nodeId The node ID of the contributor.
     * @param endpointId The endpoint ID of the contributor.
     * @param health The new health state.
     * @return CHIP_ERROR_NOT_FOUND if not found.
     */
    CHIP_ERROR UpdateMatterContributorHealth(NodeId nodeId, EndpointId endpointId,
                                             AmbientSensingUnion::UnionContributorHealthEum health);

    // Non-Matter contributor management (NodeID is null, name is mandatory)
    /**
     * @brief Adds a non-Matter contributor to the union.
     * @param name The unique name of the contributor (mandatory per spec).
     * @param health The initial health state.
     * @return CHIP_ERROR_INVALID_ARGUMENT if name is empty, CHIP_ERROR_NO_MEMORY if full.
     */
    CHIP_ERROR AddNonMatterContributor(const CharSpan & name,
                                       AmbientSensingUnion::UnionContributorHealthEum health =
                                           AmbientSensingUnion::UnionContributorHealthEum::kUnionContributorOnline);

    /**
     * @brief Removes a non-Matter contributor from the union.
     * @param name The name of the contributor.
     * @return CHIP_ERROR_NOT_FOUND if not found.
     */
    CHIP_ERROR RemoveNonMatterContributor(const CharSpan & name);

    /**
     * @brief Updates a non-Matter contributor's health state.
     * @param name The name of the contributor.
     * @param health The new health state.
     * @return CHIP_ERROR_NOT_FOUND if not found.
     */
    CHIP_ERROR UpdateNonMatterContributorHealth(const CharSpan & name,
                                                AmbientSensingUnion::UnionContributorHealthEum health);

    /**
     * @brief Clears all contributors from the union.
     */
    void ClearAllContributors();

private:
    // Event emission helpers
    void EmitUnionContributorAddedEvent(const AmbientSensingUnion::Structs::UnionContributorStruct::Type & contributor);
    void EmitUnionContributorRemovedEvent(const AmbientSensingUnion::Structs::UnionContributorStruct::Type & contributor);
    void EmitUnionContributorHealthChangedEvent(const AmbientSensingUnion::Structs::UnionContributorStruct::Type & contributor);

    // Attribute encoding helpers
    CHIP_ERROR EncodeContributorList(AttributeValueEncoder & encoder);

    // Helper to recalculate union health based on contributor health states
    void RecalculateUnionHealth();

    // Persistence helper
    CHIP_ERROR LoadPersistedAttributes();
    CHIP_ERROR PersistUnionName();

    // Delegates
    AmbientSensingUnionDelegate * mDelegate;
    ContributorStorageDelegate * mContributorStorage;
    AmbientSensingUnionPersistenceDelegate * mPersistence;

    // Attribute storage
    char mUnionNameBuffer[kMaxUnionNameLength + 1];
    size_t mUnionNameLength;
    AmbientSensingUnion::UnionHealthEnum mUnionHealth;
};

} // namespace Clusters
} // namespace app
} // namespace chip
