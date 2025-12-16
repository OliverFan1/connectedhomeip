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
#include <app/persistence/AttributePersistence.h>
#include <pw_unit_test/framework.h>

#include <app/server-cluster/testing/AttributeTesting.h>
#include <app/server-cluster/testing/ClusterTester.h>
#include <app/server-cluster/AttributeListBuilder.h>
#include <app/server-cluster/testing/TestEventGenerator.h>
#include <app/server-cluster/testing/TestServerClusterContext.h>
#include <clusters/AmbientSensingUnion/Attributes.h>
#include <clusters/AmbientSensingUnion/Events.h>
#include <clusters/AmbientSensingUnion/Metadata.h>
#include <clusters/AmbientSensingUnion/Structs.h>
#include <lib/support/Span.h>
#include <numeric>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::AmbientSensingUnion;
using namespace chip::Testing;

namespace {

constexpr EndpointId kTestEndpointId = 1;
constexpr char kTestUnionName[] = "Test Union";

bool onUnionNameChangedCalled = false;
bool onUnionHealthChangedCalled = false;
bool onUnionContributorListChangedCalled = false;

class TestAmbientSensingUnionDelegate : public AmbientSensingUnionDelegate
{
public:
    void OnUnionNameChanged(EndpointId endpoint, const CharSpan & unionName) override { onUnionNameChangedCalled = true; }
    void OnUnionHealthChanged(EndpointId endpoint, AmbientSensingUnion::UnionHealthEnum unionHealth) override { onUnionHealthChangedCalled = true; }
    void OnUnionContributorListChanged(EndpointId endpoint, const Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type> & contributorList) override { onUnionContributorListChangedCalled = true; }
};

TestAmbientSensingUnionDelegate gTestAmbientSensingUnionDelegate;

struct TestAmbientSensingUnionCluster : public ::testing::Test
{
    static void SetUpTestSuite() { ASSERT_EQ(Platform::MemoryInit(), CHIP_NO_ERROR); }
    static void TearDownTestSuite() { Platform::MemoryShutdown(); }

    void SetUp() override
    {
        onUnionNameChangedCalled = false;
        onUnionHealthChangedCalled = false;
        onUnionContributorListChangedCalled = false;
    }
};

TEST_F(TestAmbientSensingUnionCluster, TestReadClusterRevision)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId } };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
    chip::Testing::ClusterTester tester(cluster);

    uint16_t clusterRevision;
    EXPECT_EQ(tester.ReadAttribute(Attributes::ClusterRevision::Id, clusterRevision), CHIP_NO_ERROR);
    EXPECT_EQ(clusterRevision, AmbientSensingUnion::kRevision);
}

TEST_F(TestAmbientSensingUnionCluster, TestReadFeatureMap)
{
    chip::Testing::TestServerClusterContext context;
    uint32_t featureMap = 0;

    // Test case: No features
    {
        AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId } };
        EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
        chip::Testing::ClusterTester tester(cluster);
        EXPECT_EQ(tester.ReadAttribute(Attributes::FeatureMap::Id, featureMap), CHIP_NO_ERROR);
        EXPECT_EQ(featureMap, 0U);
    }

    // Test case: Leader feature
    {
        constexpr uint32_t featureMapLeader = 0x1U;
        AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                                .WithFeatures(AmbientSensingUnion::Feature(featureMapLeader)) };
        EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
        chip::Testing::ClusterTester tester(cluster);
        EXPECT_EQ(tester.ReadAttribute(Attributes::FeatureMap::Id, featureMap), CHIP_NO_ERROR);
        EXPECT_EQ(featureMap, featureMapLeader);
    }
}

TEST_F(TestAmbientSensingUnionCluster, TestUnionNameAttribute)
{
    chip::Testing::TestServerClusterContext context;

    // Test case: UnionName not configured (should return UnsupportedAttribute)
    {
        AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId } };
        EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
        chip::Testing::ClusterTester tester(cluster);

        CharSpan unionName;
        EXPECT_EQ(tester.ReadAttribute(Attributes::UnionName::Id, unionName), CHIP_IM_GLOBAL_STATUS(UnsupportedAttribute));
        EXPECT_FALSE(cluster.HasUnionName());
    }

    // Test case: UnionName configured
    {
        AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                                .WithUnionName(CharSpan::fromCharString(kTestUnionName)) };
        EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
        chip::Testing::ClusterTester tester(cluster);

        CharSpan unionName;
        EXPECT_EQ(tester.ReadAttribute(Attributes::UnionName::Id, unionName), CHIP_NO_ERROR);
        EXPECT_TRUE(unionName.data_equal(CharSpan::fromCharString(kTestUnionName)));
        EXPECT_TRUE(cluster.HasUnionName());
        EXPECT_TRUE(cluster.GetUnionName().data_equal(CharSpan::fromCharString(kTestUnionName)));

        // Test writing to UnionName
        constexpr char kNewUnionName[] = "New Union Name";
        EXPECT_EQ(tester.WriteAttribute(Attributes::UnionName::Id, CharSpan::fromCharString(kNewUnionName)), CHIP_NO_ERROR);
        EXPECT_EQ(tester.ReadAttribute(Attributes::UnionName::Id, unionName), CHIP_NO_ERROR);
        EXPECT_TRUE(unionName.data_equal(CharSpan::fromCharString(kNewUnionName)));

        // Test writing the same value (should return NoOp)
        EXPECT_EQ(cluster.SetUnionName(CharSpan::fromCharString(kNewUnionName)), 
                  DataModel::ActionReturnStatus::FixedStatus::kWriteSuccessNoOp);

        // Test writing a name that's too long (should return ConstraintError)
        std::string longName(129, 'A'); // 129 characters, exceeds limit
        EXPECT_EQ(tester.WriteAttribute(Attributes::UnionName::Id, CharSpan(longName.data(), longName.size())), 
                  CHIP_IM_GLOBAL_STATUS(ConstraintError));
    }
}

TEST_F(TestAmbientSensingUnionCluster, TestUnionNamePersistence)
{
    chip::Testing::TestServerClusterContext context;
    constexpr char kInitialUnionName[] = "Initial Union";
    constexpr char kNewUnionName[] = "Modified Union";

    // 1. Create a cluster with union name. On startup, it should store the initial name.
    {
        AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                                .WithUnionName(CharSpan::fromCharString(kInitialUnionName)) };
        EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
        chip::Testing::ClusterTester tester(cluster);

        CharSpan unionName;
        EXPECT_EQ(tester.ReadAttribute(Attributes::UnionName::Id, unionName), CHIP_NO_ERROR);
        EXPECT_TRUE(unionName.data_equal(CharSpan::fromCharString(kInitialUnionName)));
    }

    // 2. Write a new value to the attribute. This should update the value in persistence.
    {
        AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                                .WithUnionName(CharSpan::fromCharString(kInitialUnionName)) };
        EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
        chip::Testing::ClusterTester tester(cluster);
        EXPECT_EQ(tester.WriteAttribute(Attributes::UnionName::Id, CharSpan::fromCharString(kNewUnionName)), CHIP_NO_ERROR);
        EXPECT_TRUE(cluster.GetUnionName().data_equal(CharSpan::fromCharString(kNewUnionName)));
    }

    // 3. Create a new cluster instance. It should load the new value from persistence on startup.
    {
        AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                                .WithUnionName(CharSpan::fromCharString(kInitialUnionName)) };
        EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
        chip::Testing::ClusterTester tester(cluster);

        CharSpan unionName;
        EXPECT_EQ(tester.ReadAttribute(Attributes::UnionName::Id, unionName), CHIP_NO_ERROR);
        EXPECT_TRUE(unionName.data_equal(CharSpan::fromCharString(kNewUnionName)));
    }
}

TEST_F(TestAmbientSensingUnionCluster, TestLeaderFeatureAttributes)
{
    chip::Testing::TestServerClusterContext context;

    // Test case: Leader feature not enabled (should return UnsupportedAttribute)
    {
        AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId } };
        EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
        chip::Testing::ClusterTester tester(cluster);

        AmbientSensingUnion::UnionHealthEnum unionHealth;
        EXPECT_EQ(tester.ReadAttribute(Attributes::UnionHealth::Id, unionHealth), CHIP_IM_GLOBAL_STATUS(UnsupportedAttribute));

        DataModel::DecodableList<AmbientSensingUnion::Structs::UnionMemberStruct::DecodableType> unionContributorList;
        EXPECT_EQ(tester.ReadAttribute(Attributes::UnionContributorList::Id, unionContributorList), CHIP_IM_GLOBAL_STATUS(UnsupportedAttribute));

        EXPECT_FALSE(cluster.HasLeaderFeature());
    }

    // Test case: Leader feature enabled
    {
        const std::vector<AmbientSensingUnion::Structs::UnionMemberStruct::Type> kTestUnionContributorList = {
            { .contributorNodeID = 0x1234567890ABCDEFULL, .contributorEndpointID = 1, .contributorHealth = 0x01 },
            { .contributorNodeID = 0xFEDCBA0987654321ULL, .contributorEndpointID = 2, .contributorHealth = 0x04 },
            { .contributorNodeID = 0x1111111111111111ULL, .contributorEndpointID = 3, .contributorHealth = 0x01 }
        };
        
        AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                                .WithFeatures(Feature::kLeader)
                                                .WithUnionHealth(AmbientSensingUnion::UnionHealthEnum::kFullyFunctional)
                                                .WithUnionContributorList(Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type>(kTestUnionContributorList.data(), kTestUnionContributorList.size())) };
        EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
        chip::Testing::ClusterTester tester(cluster);

        // Test UnionHealth
        AmbientSensingUnion::UnionHealthEnum unionHealth;
        EXPECT_EQ(tester.ReadAttribute(Attributes::UnionHealth::Id, unionHealth), CHIP_NO_ERROR);
        EXPECT_EQ(unionHealth, AmbientSensingUnion::UnionHealthEnum::kFullyFunctional);
        EXPECT_EQ(cluster.GetUnionHealth(), AmbientSensingUnion::UnionHealthEnum::kFullyFunctional);

        // Test UnionContributorList
        DataModel::DecodableList<AmbientSensingUnion::Structs::UnionMemberStruct::DecodableType> unionContributorList;
        EXPECT_EQ(tester.ReadAttribute(Attributes::UnionContributorList::Id, unionContributorList), CHIP_NO_ERROR);
        
        std::vector<AmbientSensingUnion::Structs::UnionMemberStruct::Type> readContributorList;
        auto iter = unionContributorList.begin();
        while (iter.Next())
        {
            const auto & value = iter.GetValue();
            readContributorList.push_back({ 
                .contributorNodeID = value.contributorNodeID, 
                .contributorEndpointID = value.contributorEndpointID, 
                .contributorHealth = value.contributorHealth 
            });
        }
        EXPECT_EQ(iter.GetStatus(), CHIP_NO_ERROR);
        EXPECT_EQ(readContributorList.size(), kTestUnionContributorList.size());
        
        for (size_t i = 0; i < readContributorList.size(); ++i)
        {
            EXPECT_EQ(readContributorList[i].contributorNodeID, kTestUnionContributorList[i].contributorNodeID);
            EXPECT_EQ(readContributorList[i].contributorEndpointID, kTestUnionContributorList[i].contributorEndpointID);
            EXPECT_EQ(readContributorList[i].contributorHealth, kTestUnionContributorList[i].contributorHealth);
        }

        EXPECT_TRUE(cluster.HasLeaderFeature());
    }
}

TEST_F(TestAmbientSensingUnionCluster, TestSetUnionHealth)
{
    chip::Testing::TestServerClusterContext context;
    
    // Test case: Leader feature not enabled (should return UnsupportedAttribute)
    {
        AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId } };
        EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
        EXPECT_EQ(cluster.SetUnionHealth(AmbientSensingUnion::UnionHealthEnum::kFullyFunctional), 
                  Protocols::InteractionModel::Status::UnsupportedAttribute);
    }

    // Test case: Leader feature enabled
    {
        AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                                .WithFeatures(Feature::kLeader) };
        EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

        // Test setting union health
        EXPECT_EQ(cluster.SetUnionHealth(AmbientSensingUnion::UnionHealthEnum::kLimitedDegraded), 
                  Protocols::InteractionModel::Status::Success);
        EXPECT_EQ(cluster.GetUnionHealth(), AmbientSensingUnion::UnionHealthEnum::kLimitedDegraded);

        // Test setting the same value (should return NoOp)
        EXPECT_EQ(cluster.SetUnionHealth(AmbientSensingUnion::UnionHealthEnum::kLimitedDegraded), 
                  DataModel::ActionReturnStatus::FixedStatus::kWriteSuccessNoOp);

        // Test setting NonFunctional
        EXPECT_EQ(cluster.SetUnionHealth(AmbientSensingUnion::UnionHealthEnum::kNonFunctional), 
                  Protocols::InteractionModel::Status::Success);
        EXPECT_EQ(cluster.GetUnionHealth(), AmbientSensingUnion::UnionHealthEnum::kNonFunctional);
    }
}

TEST_F(TestAmbientSensingUnionCluster, TestSetUnionContributorList)
{
    chip::Testing::TestServerClusterContext context;

    // Test case: Leader feature not enabled (should return UnsupportedAttribute)
    {
        AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId } };
        EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
        
        const std::vector<AmbientSensingUnion::Structs::UnionMemberStruct::Type> testList = {
            { .contributorNodeID = 0x1111111111111111ULL, .contributorEndpointID = 1, .contributorHealth = 0x01 }
        };
        EXPECT_EQ(cluster.SetUnionContributorList(Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type>(testList.data(), testList.size())), 
                  Protocols::InteractionModel::Status::UnsupportedAttribute);
    }

    // Test case: Leader feature enabled
    {
        AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                                .WithFeatures(Feature::kLeader) };
        EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

        const std::vector<AmbientSensingUnion::Structs::UnionMemberStruct::Type> kNewContributorList = {
            { .contributorNodeID = 0xAAAAAAAAAAAAAAAAULL, .contributorEndpointID = 10, .contributorHealth = 0x01 },
            { .contributorNodeID = 0xBBBBBBBBBBBBBBBBULL, .contributorEndpointID = 20, .contributorHealth = 0x04 },
            { .contributorNodeID = 0xCCCCCCCCCCCCCCCCULL, .contributorEndpointID = 30, .contributorHealth = 0x02 }
        };
        
        // Test setting union contributor list
        EXPECT_EQ(cluster.SetUnionContributorList(Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type>(kNewContributorList.data(), kNewContributorList.size())), 
                  Protocols::InteractionModel::Status::Success);
        
        Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type> retrievedList = cluster.GetUnionContributorList();
        EXPECT_EQ(retrievedList.size(), kNewContributorList.size());
        
        for (size_t i = 0; i < retrievedList.size(); ++i)
        {
            EXPECT_EQ(retrievedList[i].contributorNodeID, kNewContributorList[i].contributorNodeID);
            EXPECT_EQ(retrievedList[i].contributorEndpointID, kNewContributorList[i].contributorEndpointID);
            EXPECT_EQ(retrievedList[i].contributorHealth, kNewContributorList[i].contributorHealth);
        }

        // Test setting the same value (should return NoOp)
        EXPECT_EQ(cluster.SetUnionContributorList(Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type>(kNewContributorList.data(), kNewContributorList.size())), 
                  DataModel::ActionReturnStatus::FixedStatus::kWriteSuccessNoOp);

        // Test setting a list that's too large
        std::vector<AmbientSensingUnion::Structs::UnionMemberStruct::Type> largeList(129); // Exceeds kMaxUnionContributorListSize (128)
        for (size_t i = 0; i < largeList.size(); ++i)
        {
            largeList[i] = { .contributorNodeID = i, .contributorEndpointID = static_cast<uint16_t>(i), .contributorHealth = 0x01 };
        }
        EXPECT_EQ(cluster.SetUnionContributorList(Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type>(largeList.data(), largeList.size())), 
                  Protocols::InteractionModel::Status::ConstraintError);
    }
}

TEST_F(TestAmbientSensingUnionCluster, TestUnionContributorListChangeEvent)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithFeatures(Feature::kLeader) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    const std::vector<AmbientSensingUnion::Structs::UnionMemberStruct::Type> kNewContributorList = {
        { .contributorNodeID = 0x1111111111111111ULL, .contributorEndpointID = 5, .contributorHealth = 0x01 },
        { .contributorNodeID = 0x2222222222222222ULL, .contributorEndpointID = 6, .contributorHealth = 0x04 },
        { .contributorNodeID = 0x3333333333333333ULL, .contributorEndpointID = 7, .contributorHealth = 0x01 }
    };
    
    // Set union contributor list and verify event is generated
    cluster.SetUnionContributorList(Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type>(kNewContributorList.data(), kNewContributorList.size()));
    
    auto eventInfo = context.EventsGenerator().GetNextEvent();
    ASSERT_NE(eventInfo, std::nullopt);
    
    if (eventInfo.has_value())
    {
        AmbientSensingUnion::Events::UnionContributorListChange::DecodableType decodedEvent;
        EXPECT_EQ(eventInfo->GetEventData(decodedEvent), CHIP_NO_ERROR);
        
        std::vector<AmbientSensingUnion::Structs::UnionMemberStruct::Type> eventContributorList;
        auto iter = decodedEvent.unionContributorList.begin();
        while (iter.Next())
        {
            const auto & value = iter.GetValue();
            eventContributorList.push_back({ 
                .contributorNodeID = value.contributorNodeID, 
                .contributorEndpointID = value.contributorEndpointID, 
                .contributorHealth = value.contributorHealth 
            });
        }
        EXPECT_EQ(iter.GetStatus(), CHIP_NO_ERROR);
        EXPECT_EQ(eventContributorList.size(), kNewContributorList.size());
        
        for (size_t i = 0; i < eventContributorList.size(); ++i)
        {
            EXPECT_EQ(eventContributorList[i].contributorNodeID, kNewContributorList[i].contributorNodeID);
            EXPECT_EQ(eventContributorList[i].contributorEndpointID, kNewContributorList[i].contributorEndpointID);
            EXPECT_EQ(eventContributorList[i].contributorHealth, kNewContributorList[i].contributorHealth);
        }
    }
}

TEST_F(TestAmbientSensingUnionCluster, TestAttributeListMandatoryOnly)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId } };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    ReadOnlyBufferBuilder<DataModel::AttributeEntry> attributes;
    EXPECT_EQ(cluster.Attributes(ConcreteClusterPath(kTestEndpointId, AmbientSensingUnion::Id), attributes), CHIP_NO_ERROR);

    // With the updated XML, there are no mandatory attributes beyond global ones
    ReadOnlyBufferBuilder<DataModel::AttributeEntry> expected;
    AttributeListBuilder listBuilder(expected);
    EXPECT_EQ(listBuilder.Append(Span(AmbientSensingUnion::Attributes::kMandatoryMetadata), {}), CHIP_NO_ERROR);
    EXPECT_TRUE(EqualAttributeSets(attributes.TakeBuffer(), expected.TakeBuffer()));
}

TEST_F(TestAmbientSensingUnionCluster, TestAttributeListWithOptionalAttributes)
{
    chip::Testing::TestServerClusterContext context;
    const std::vector<AmbientSensingUnion::Structs::UnionMemberStruct::Type> kTestUnionContributorList = {
        { .contributorNodeID = 0x1234567890ABCDEFULL, .contributorEndpointID = 1, .contributorHealth = 0x01 },
        { .contributorNodeID = 0xFEDCBA0987654321ULL, .contributorEndpointID = 2, .contributorHealth = 0x04 }
    };
    
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithUnionName(CharSpan::fromCharString(kTestUnionName))
                                            .WithFeatures(Feature::kLeader)
                                            .WithUnionContributorList(Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type>(kTestUnionContributorList.data(), kTestUnionContributorList.size())) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    ReadOnlyBufferBuilder<DataModel::AttributeEntry> attributes;
    EXPECT_EQ(cluster.Attributes(ConcreteClusterPath(kTestEndpointId, AmbientSensingUnion::Id), attributes), CHIP_NO_ERROR);

    const AttributeListBuilder::OptionalAttributeEntry expectedOptionalAttributes[] = {
        { true, Attributes::UnionName::kMetadataEntry },
        { true, Attributes::UnionHealth::kMetadataEntry },
        { true, Attributes::UnionContributorList::kMetadataEntry },
    };

    ReadOnlyBufferBuilder<DataModel::AttributeEntry> expected;
    AttributeListBuilder listBuilder(expected);
    EXPECT_EQ(listBuilder.Append(Span(AmbientSensingUnion::Attributes::kMandatoryMetadata), Span(expectedOptionalAttributes)), CHIP_NO_ERROR);
    EXPECT_TRUE(EqualAttributeSets(attributes.TakeBuffer(), expected.TakeBuffer()));
}

TEST_F(TestAmbientSensingUnionCluster, TestClusterDelegate)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithUnionName(CharSpan::fromCharString(kTestUnionName))
                                            .WithFeatures(Feature::kLeader)
                                            .WithDelegate(&gTestAmbientSensingUnionDelegate) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Test OnUnionNameChanged callback
    onUnionNameChangedCalled = false;
    cluster.SetUnionName(CharSpan::fromCharString("New Name"));
    EXPECT_TRUE(onUnionNameChangedCalled);

    // Test OnUnionHealthChanged callback
    onUnionHealthChangedCalled = false;
    cluster.SetUnionHealth(AmbientSensingUnion::UnionHealthEnum::kLimitedDegraded);
    EXPECT_TRUE(onUnionHealthChangedCalled);

    // Test OnUnionContributorListChanged callback
    onUnionContributorListChangedCalled = false;
    const std::vector<AmbientSensingUnion::Structs::UnionMemberStruct::Type> kNewContributorList = {
        { .contributorNodeID = 0xAAAAAAAAAAAAAAAAULL, .contributorEndpointID = 10, .contributorHealth = 0x01 },
        { .contributorNodeID = 0xBBBBBBBBBBBBBBBBULL, .contributorEndpointID = 20, .contributorHealth = 0x04 }
    };
    cluster.SetUnionContributorList(Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type>(kNewContributorList.data(), kNewContributorList.size()));
    EXPECT_TRUE(onUnionContributorListChangedCalled);
}

TEST_F(TestAmbientSensingUnionCluster, TestConstraintValidation)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithUnionName(CharSpan::fromCharString(kTestUnionName))
                                            .WithFeatures(Feature::kLeader) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
    chip::Testing::ClusterTester tester(cluster);

    // Test UnionName constraint (max 128 chars)
    {
        std::string longName(129, 'A'); // 129 characters, exceeds limit
        EXPECT_EQ(tester.WriteAttribute(Attributes::UnionName::Id, CharSpan(longName.data(), longName.size())), 
                  CHIP_IM_GLOBAL_STATUS(ConstraintError));
    }

    // Test empty UnionName (should be allowed)
    {
        CharSpan emptySpan; // Default constructor creates empty span
        EXPECT_EQ(tester.WriteAttribute(Attributes::UnionName::Id, emptySpan), CHIP_NO_ERROR);
        CharSpan unionName;
        EXPECT_EQ(tester.ReadAttribute(Attributes::UnionName::Id, unionName), CHIP_NO_ERROR);
        EXPECT_EQ(unionName.size(), 0U); // Use 0U for unsigned comparison
    }

    // Test maximum allowed UnionName length (128 chars)
    {
        std::string maxName(128, 'B'); // Exactly 128 characters
        EXPECT_EQ(tester.WriteAttribute(Attributes::UnionName::Id, CharSpan(maxName.data(), maxName.size())), CHIP_NO_ERROR);
        CharSpan unionName;
        EXPECT_EQ(tester.ReadAttribute(Attributes::UnionName::Id, unionName), CHIP_NO_ERROR);
        EXPECT_EQ(unionName.size(), 128U); // Use 128U for unsigned comparison
        EXPECT_TRUE(unionName.data_equal(CharSpan(maxName.data(), maxName.size())));
    }
}

TEST_F(TestAmbientSensingUnionCluster, TestGetterMethods)
{
    chip::Testing::TestServerClusterContext context;
    const std::vector<AmbientSensingUnion::Structs::UnionMemberStruct::Type> kTestUnionContributorList = {
        { .contributorNodeID = 0x1234567890ABCDEFULL, .contributorEndpointID = 1, .contributorHealth = 0x01 },
        { .contributorNodeID = 0xFEDCBA0987654321ULL, .contributorEndpointID = 2, .contributorHealth = 0x04 },
        { .contributorNodeID = 0x1111111111111111ULL, .contributorEndpointID = 3, .contributorHealth = 0x01 }
    };
    
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithUnionName(CharSpan::fromCharString(kTestUnionName))
                                            .WithFeatures(Feature::kLeader)
                                            .WithUnionHealth(AmbientSensingUnion::UnionHealthEnum::kFullyFunctional)
                                            .WithUnionContributorList(Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type>(kTestUnionContributorList.data(), kTestUnionContributorList.size())) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Test all getter methods
    EXPECT_TRUE(cluster.HasUnionName());
    EXPECT_TRUE(cluster.GetUnionName().data_equal(CharSpan::fromCharString(kTestUnionName)));
    EXPECT_TRUE(cluster.HasLeaderFeature());
    EXPECT_EQ(cluster.GetUnionHealth(), AmbientSensingUnion::UnionHealthEnum::kFullyFunctional);

    Span<const AmbientSensingUnion::Structs::UnionMemberStruct::Type> retrievedList = cluster.GetUnionContributorList();
    EXPECT_EQ(retrievedList.size(), kTestUnionContributorList.size());
}

} // namespace