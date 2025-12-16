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
#include <clusters/AmbientSensingUnion/Structs.h>
#include <lib/core/DataModelTypes.h>
#include <lib/support/Span.h>
#include <vector>

namespace chip {
namespace app {
namespace Clusters {

class AmbientSensingUnionDelegate
{
public:
    virtual ~AmbientSensingUnionDelegate() = default;

    virtual void OnUnionNameChanged(chip::EndpointId endpointId, const CharSpan & unionName) {}
    virtual void OnUnionHealthChanged(chip::EndpointId endpointId, AmbientSensingUnion::UnionHealthEnum unionHealth) {}
    virtual void OnUnionContributorListChanged(chip::EndpointId endpointId, const Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type> & contributorList) {}
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

        Config & WithUnionHealth(AmbientSensingUnion::UnionHealthEnum unionHealth)
        {
            mUnionHealth = unionHealth;
            return *this;
        }

        Config & WithUnionContributorList(const Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type> & contributorList)
        {
            mUnionContributorList = contributorList;
            return *this;
        }

        EndpointId mEndpointId;
        BitMask<AmbientSensingUnion::Feature> mFeatureMap = 0;
        bool mHasUnionName = false;
        CharSpan mUnionName;
        AmbientSensingUnion::UnionHealthEnum mUnionHealth = AmbientSensingUnion::UnionHealthEnum::kFullyFunctional;
        Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type> mUnionContributorList;
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
    bool HasUnionName() const;
    CharSpan GetUnionName() const;
    bool HasLeaderFeature() const;
    AmbientSensingUnion::UnionHealthEnum GetUnionHealth() const;
    Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type> GetUnionContributorList() const;

    // Setter methods
    DataModel::ActionReturnStatus SetUnionName(const CharSpan & unionName);
    DataModel::ActionReturnStatus SetUnionHealth(AmbientSensingUnion::UnionHealthEnum unionHealth);
    DataModel::ActionReturnStatus SetUnionContributorList(const Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type> & contributorList);

    // Event generation
    CHIP_ERROR GenerateUnionContributorListChangeEvent();

    // Delegate methods
    AmbientSensingUnionDelegate * GetDelegate() const { return mDelegate; }
    void SetDelegate(AmbientSensingUnionDelegate * delegate) { mDelegate = delegate; }

private:
    static constexpr size_t kMaxUnionContributorListSize = 128;
    static constexpr size_t kMaxUnionNameSize = 128;

    bool IsValidContributorListSize(size_t size) const { return size <= kMaxUnionContributorListSize; }
    bool IsValidUnionHealth(AmbientSensingUnion::UnionHealthEnum health) const;

    AmbientSensingUnionDelegate * mDelegate;
    BitMask<AmbientSensingUnion::Feature> mFeatureMap;
    bool mHasUnionName;

    Storage::String<kMaxUnionNameSize> mUnionName;
    AmbientSensingUnion::UnionHealthEnum mUnionHealth;

    // Dynamic array for union contributors
    std::vector<AmbientSensingUnion::Structs::UnionMemberStruct::Type> mUnionContributorList;
};

} // namespace Clusters
} // namespace app
} // namespace chip
