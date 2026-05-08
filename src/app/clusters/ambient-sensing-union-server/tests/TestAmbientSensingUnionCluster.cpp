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
#include <pw_unit_test/framework.h>

#include <app/server-cluster/testing/AttributeTesting.h>
#include <app/server-cluster/testing/ClusterTester.h>
#include <app/server-cluster/testing/TestEventGenerator.h>
#include <app/server-cluster/testing/TestServerClusterContext.h>
#include <app/server-cluster/testing/ValidateGlobalAttributes.h>
#include <clusters/AmbientSensingUnion/Metadata.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::AmbientSensingUnion;
using namespace chip::Testing;

namespace {

constexpr EndpointId kTestEndpointId = 1;
constexpr NodeId kTestNodeId1        = 0x1234567890ABCDEF;
constexpr NodeId kTestNodeId2        = 0xFEDCBA0987654321;
constexpr EndpointId kContributorEp1 = 1;
constexpr EndpointId kContributorEp2 = 2;

// Test delegate to track callbacks
class TestAmbientSensingUnionDelegate : public AmbientSensingUnionDelegate
{
public:
    void Reset()
    {
        mUnionNameChangedCalled       = false;
        mUnionHealthChangedCalled     = false;
        mContributorAddedCalled       = false;
        mContributorRemovedCalled     = false;
        mContributorHealthChangedCalled = false;
        mLastUnionName.clear();
        mLastUnionHealth = UnionHealthEnum::kFullyFunctional;
    }

    void OnUnionNameChanged(const CharSpan & unionName) override
    {
        mUnionNameChangedCalled = true;
        mLastUnionName          = std::string(unionName.data(), unionName.size());
    }

    void OnUnionHealthChanged(UnionHealthEnum unionHealth) override
    {
        mUnionHealthChangedCalled = true;
        mLastUnionHealth          = unionHealth;
    }

    void OnContributorAdded(const Structs::UnionContributorStruct::Type & contributor) override
    {
        mContributorAddedCalled = true;
        mLastContributor        = contributor;
    }

    void OnContributorRemoved(const Structs::UnionContributorStruct::Type & contributor) override
    {
        mContributorRemovedCalled = true;
        mLastContributor          = contributor;
    }

    void OnContributorHealthChanged(const Structs::UnionContributorStruct::Type & contributor) override
    {
        mContributorHealthChangedCalled = true;
        mLastContributor                = contributor;
    }

    bool mUnionNameChangedCalled           = false;
    bool mUnionHealthChangedCalled         = false;
    bool mContributorAddedCalled           = false;
    bool mContributorRemovedCalled         = false;
    bool mContributorHealthChangedCalled   = false;
    std::string mLastUnionName;
    UnionHealthEnum mLastUnionHealth = UnionHealthEnum::kFullyFunctional;
    Structs::UnionContributorStruct::Type mLastContributor;
};

// Test persistence delegate
class TestPersistenceDelegate : public AmbientSensingUnionPersistenceDelegate
{
public:
    CHIP_ERROR LoadUnionName(char * buffer, size_t bufferSize, size_t & outLength) override
    {
        if (!mHasStoredName)
        {
            return CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;
        }

        outLength = std::min(mStoredName.size(), bufferSize);
        memcpy(buffer, mStoredName.data(), outLength);
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR SaveUnionName(const CharSpan & unionName) override
    {
        mStoredName   = std::string(unionName.data(), unionName.size());
        mHasStoredName = true;
        mSaveCount++;
        return CHIP_NO_ERROR;
    }

    void Reset()
    {
        mStoredName.clear();
        mHasStoredName = false;
        mSaveCount     = 0;
    }

    std::string mStoredName;
    bool mHasStoredName = false;
    int mSaveCount      = 0;
};

struct TestAmbientSensingUnionCluster : public ::testing::Test
{
    static void SetUpTestSuite() { ASSERT_EQ(Platform::MemoryInit(), CHIP_NO_ERROR); }
    static void TearDownTestSuite() { Platform::MemoryShutdown(); }

    void SetUp() override
    {
        mDelegate.Reset();
        mPersistence.Reset();
        mContributorStorage.ClearAllContributors();
    }

    void TearDown() override
    {
        mContributorStorage.ClearAllContributors();
    }

    TestAmbientSensingUnionDelegate mDelegate;
    TestPersistenceDelegate mPersistence;
    DefaultContributorStorage<32, 8> mContributorStorage;
};

// =============================================================================
// Basic Attribute Tests
// =============================================================================

TEST_F(TestAmbientSensingUnionCluster, TestReadClusterRevision)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
    chip::Testing::ClusterTester tester(cluster);

    uint16_t clusterRevision;
    EXPECT_EQ(tester.ReadAttribute(Globals::Attributes::ClusterRevision::Id, clusterRevision), CHIP_NO_ERROR);
    EXPECT_EQ(clusterRevision, AmbientSensingUnion::kRevision);
}

TEST_F(TestAmbientSensingUnionCluster, TestReadFeatureMap)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
    chip::Testing::ClusterTester tester(cluster);

    uint32_t featureMap;
    EXPECT_EQ(tester.ReadAttribute(Globals::Attributes::FeatureMap::Id, featureMap), CHIP_NO_ERROR);
    // No features defined for this cluster
    EXPECT_EQ(featureMap, 0u);
}

TEST_F(TestAmbientSensingUnionCluster, TestReadUnionName)
{
    chip::Testing::TestServerClusterContext context;
    constexpr char kTestName[] = "LivingRoomUnion";

    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithUnionName(CharSpan::fromCharString(kTestName))
                                            .WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
    chip::Testing::ClusterTester tester(cluster);

    CharSpan unionName;
    EXPECT_EQ(tester.ReadAttribute(Attributes::UnionName::Id, unionName), CHIP_NO_ERROR);
    EXPECT_TRUE(unionName.data_equal(CharSpan::fromCharString(kTestName)));

    // Also verify via getter
    EXPECT_TRUE(cluster.GetUnionName().data_equal(CharSpan::fromCharString(kTestName)));
}

TEST_F(TestAmbientSensingUnionCluster, TestReadUnionHealth)
{
    chip::Testing::TestServerClusterContext context;

    // Test health with no contributors (NonFunctional)
    {
        AmbientSensingUnionCluster cluster{
            AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
        EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
        chip::Testing::ClusterTester tester(cluster);

        UnionHealthEnum health;
        EXPECT_EQ(tester.ReadAttribute(Attributes::UnionHealth::Id, health), CHIP_NO_ERROR);
        // With 0 contributors, health is NonFunctional
        EXPECT_EQ(health, UnionHealthEnum::kNonFunctional);
    }

    // Clear storage before next test case
    mContributorStorage.ClearAllContributors();

    // Test health with all online contributors (FullyFunctional)
    {
        AmbientSensingUnionCluster cluster{
            AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
        EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
        
        // Add online contributor to get FullyFunctional health
        EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1, 
                  UnionContributorHealthEum::kUnionContributorOnline), CHIP_NO_ERROR);
        
        chip::Testing::ClusterTester tester(cluster);

        UnionHealthEnum health;
        EXPECT_EQ(tester.ReadAttribute(Attributes::UnionHealth::Id, health), CHIP_NO_ERROR);
        EXPECT_EQ(health, UnionHealthEnum::kFullyFunctional);
    }

    // Clear storage before next test case
    mContributorStorage.ClearAllContributors();

    // Test health with mixed online/offline contributors (LimitedDegraded)
    {
        AmbientSensingUnionCluster cluster{
            AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
        EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
        
        // Add one online and one offline contributor to get LimitedDegraded health
        EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1, 
                  UnionContributorHealthEum::kUnionContributorOnline), CHIP_NO_ERROR);
        EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId2, kContributorEp1, 
                  UnionContributorHealthEum::kUnionContributorOffline), CHIP_NO_ERROR);
        
        chip::Testing::ClusterTester tester(cluster);

        UnionHealthEnum health;
        EXPECT_EQ(tester.ReadAttribute(Attributes::UnionHealth::Id, health), CHIP_NO_ERROR);
        EXPECT_EQ(health, UnionHealthEnum::kLimitedDegraded);
    }
}

TEST_F(TestAmbientSensingUnionCluster, TestReadEmptyContributorList)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    EXPECT_EQ(cluster.GetContributorCount(), 0u);
}

// =============================================================================
// UnionName Write Tests
// =============================================================================

TEST_F(TestAmbientSensingUnionCluster, TestWriteUnionName)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithContributorStorage(&mContributorStorage)
                                            .WithDelegate(&mDelegate) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
    chip::Testing::ClusterTester tester(cluster);

    constexpr char kNewName[] = "BedroomUnion";

    // Write via WriteAttribute
    EXPECT_EQ(tester.WriteAttribute(Attributes::UnionName::Id, CharSpan::fromCharString(kNewName)), CHIP_NO_ERROR);

    // Verify it was written
    CharSpan unionName;
    EXPECT_EQ(tester.ReadAttribute(Attributes::UnionName::Id, unionName), CHIP_NO_ERROR);
    EXPECT_TRUE(unionName.data_equal(CharSpan::fromCharString(kNewName)));

    // Verify delegate was called
    EXPECT_TRUE(mDelegate.mUnionNameChangedCalled);
    EXPECT_EQ(mDelegate.mLastUnionName, kNewName);
}

TEST_F(TestAmbientSensingUnionCluster, TestWriteUnionNameViaSetMethod)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithContributorStorage(&mContributorStorage)
                                            .WithDelegate(&mDelegate) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    constexpr char kNewName[] = "KitchenUnion";

    EXPECT_EQ(cluster.SetUnionName(CharSpan::fromCharString(kNewName)), CHIP_NO_ERROR);
    EXPECT_TRUE(cluster.GetUnionName().data_equal(CharSpan::fromCharString(kNewName)));
    EXPECT_TRUE(mDelegate.mUnionNameChangedCalled);
}

TEST_F(TestAmbientSensingUnionCluster, TestWriteUnionNameSameValueNoOp)
{
    chip::Testing::TestServerClusterContext context;
    constexpr char kTestName[] = "TestUnion";

    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithUnionName(CharSpan::fromCharString(kTestName))
                                            .WithContributorStorage(&mContributorStorage)
                                            .WithDelegate(&mDelegate) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Writing the same value should not trigger delegate
    EXPECT_EQ(cluster.SetUnionName(CharSpan::fromCharString(kTestName)), CHIP_NO_ERROR);
    EXPECT_FALSE(mDelegate.mUnionNameChangedCalled);
}

TEST_F(TestAmbientSensingUnionCluster, TestWriteUnionNameMaxLength)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
    chip::Testing::ClusterTester tester(cluster);

    // Create a 128-character string (max allowed)
    char maxLengthName[129];
    memset(maxLengthName, 'A', 128);
    maxLengthName[128] = '\0';

    EXPECT_EQ(tester.WriteAttribute(Attributes::UnionName::Id, CharSpan(maxLengthName, 128)), CHIP_NO_ERROR);

    CharSpan unionName;
    EXPECT_EQ(tester.ReadAttribute(Attributes::UnionName::Id, unionName), CHIP_NO_ERROR);
    EXPECT_EQ(unionName.size(), 128u);
}

TEST_F(TestAmbientSensingUnionCluster, TestWriteUnionNameTooLong)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
    chip::Testing::ClusterTester tester(cluster);

    // Create a 129-character string (exceeds max)
    char tooLongName[130];
    memset(tooLongName, 'A', 129);
    tooLongName[129] = '\0';

    EXPECT_EQ(tester.WriteAttribute(Attributes::UnionName::Id, CharSpan(tooLongName, 129)),
              CHIP_IM_GLOBAL_STATUS(ConstraintError));
}

TEST_F(TestAmbientSensingUnionCluster, TestWriteUnionNameEmptyString)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithUnionName(CharSpan::fromCharString("InitialName"))
                                            .WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
    chip::Testing::ClusterTester tester(cluster);

    // Empty string should be valid
    EXPECT_EQ(tester.WriteAttribute(Attributes::UnionName::Id, CharSpan()), CHIP_NO_ERROR);

    CharSpan unionName;
    EXPECT_EQ(tester.ReadAttribute(Attributes::UnionName::Id, unionName), CHIP_NO_ERROR);
    EXPECT_EQ(unionName.size(), 0u);
}

// =============================================================================
// Matter Contributor Tests
// =============================================================================

TEST_F(TestAmbientSensingUnionCluster, TestAddMatterContributor)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithContributorStorage(&mContributorStorage)
                                            .WithDelegate(&mDelegate) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Add a Matter contributor
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1, UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_NO_ERROR);

    // Verify count
    EXPECT_EQ(cluster.GetContributorCount(), 1u);

    // Verify delegate was called
    EXPECT_TRUE(mDelegate.mContributorAddedCalled);
    EXPECT_FALSE(mDelegate.mLastContributor.contributorNodeID.IsNull());
    EXPECT_EQ(mDelegate.mLastContributor.contributorNodeID.Value(), kTestNodeId1);
    EXPECT_EQ(mDelegate.mLastContributor.contributorEndpointID.Value(), kContributorEp1);
    EXPECT_EQ(mDelegate.mLastContributor.contributorHealth, UnionContributorHealthEum::kUnionContributorOnline);
}

TEST_F(TestAmbientSensingUnionCluster, TestAddMatterContributorDuplicate)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Add first contributor
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1), CHIP_NO_ERROR);

    // Try to add duplicate
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1), CHIP_ERROR_DUPLICATE_KEY_ID);

    // Count should still be 1
    EXPECT_EQ(cluster.GetContributorCount(), 1u);
}

TEST_F(TestAmbientSensingUnionCluster, TestAddMultipleMatterContributors)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp2), CHIP_NO_ERROR); // Same node, different endpoint
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId2, kContributorEp1), CHIP_NO_ERROR); // Different node

    EXPECT_EQ(cluster.GetContributorCount(), 3u);
}

TEST_F(TestAmbientSensingUnionCluster, TestRemoveMatterContributor)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithContributorStorage(&mContributorStorage)
                                            .WithDelegate(&mDelegate) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Add and then remove
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.GetContributorCount(), 1u);

    mDelegate.Reset();
    EXPECT_EQ(cluster.RemoveMatterContributor(kTestNodeId1, kContributorEp1), CHIP_NO_ERROR);

    // Verify count
    EXPECT_EQ(cluster.GetContributorCount(), 0u);

    // Verify delegate was called
    EXPECT_TRUE(mDelegate.mContributorRemovedCalled);
    EXPECT_EQ(mDelegate.mLastContributor.contributorNodeID.Value(), kTestNodeId1);
}

TEST_F(TestAmbientSensingUnionCluster, TestRemoveMatterContributorNotFound)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    EXPECT_EQ(cluster.RemoveMatterContributor(kTestNodeId1, kContributorEp1), CHIP_ERROR_NOT_FOUND);
}

TEST_F(TestAmbientSensingUnionCluster, TestUpdateMatterContributorHealth)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithContributorStorage(&mContributorStorage)
                                            .WithDelegate(&mDelegate) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Add contributor as online
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1, UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_NO_ERROR);

    mDelegate.Reset();

    // Update to offline
    EXPECT_EQ(cluster.UpdateMatterContributorHealth(kTestNodeId1, kContributorEp1,
                                                     UnionContributorHealthEum::kUnionContributorOffline),
              CHIP_NO_ERROR);

    // Verify delegate was called
    EXPECT_TRUE(mDelegate.mContributorHealthChangedCalled);
    EXPECT_EQ(mDelegate.mLastContributor.contributorHealth, UnionContributorHealthEum::kUnionContributorOffline);
}

TEST_F(TestAmbientSensingUnionCluster, TestUpdateMatterContributorHealthSameValue)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithContributorStorage(&mContributorStorage)
                                            .WithDelegate(&mDelegate) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1, UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_NO_ERROR);

    mDelegate.Reset();

    // Update to same value - should not trigger callback
    EXPECT_EQ(cluster.UpdateMatterContributorHealth(kTestNodeId1, kContributorEp1,
                                                     UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_NO_ERROR);

    EXPECT_FALSE(mDelegate.mContributorHealthChangedCalled);
}

TEST_F(TestAmbientSensingUnionCluster, TestUpdateMatterContributorHealthNotFound)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    EXPECT_EQ(cluster.UpdateMatterContributorHealth(kTestNodeId1, kContributorEp1,
                                                     UnionContributorHealthEum::kUnionContributorOffline),
              CHIP_ERROR_NOT_FOUND);
}

// =============================================================================
// Non-Matter Contributor Tests
// =============================================================================

TEST_F(TestAmbientSensingUnionCluster, TestAddNonMatterContributor)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithContributorStorage(&mContributorStorage)
                                            .WithDelegate(&mDelegate) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    constexpr char kContributorName[] = "ZigbeeSensor1";

    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan::fromCharString(kContributorName),
                                               UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_NO_ERROR);

    EXPECT_EQ(cluster.GetContributorCount(), 1u);

    // Verify delegate
    EXPECT_TRUE(mDelegate.mContributorAddedCalled);
    EXPECT_TRUE(mDelegate.mLastContributor.contributorNodeID.IsNull());
    EXPECT_TRUE(mDelegate.mLastContributor.contributorEndpointID.IsNull());
    EXPECT_TRUE(mDelegate.mLastContributor.contributorName.HasValue());
}

TEST_F(TestAmbientSensingUnionCluster, TestAddNonMatterContributorEmptyName)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Per spec: ContributorName is mandatory when NodeID is NULL
    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan(), UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_ERROR_INVALID_ARGUMENT);

    EXPECT_EQ(cluster.GetContributorCount(), 0u);
}

TEST_F(TestAmbientSensingUnionCluster, TestAddNonMatterContributorDuplicate)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    constexpr char kContributorName[] = "LegacySensor";

    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan::fromCharString(kContributorName)), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan::fromCharString(kContributorName)), CHIP_ERROR_DUPLICATE_KEY_ID);

    EXPECT_EQ(cluster.GetContributorCount(), 1u);
}

TEST_F(TestAmbientSensingUnionCluster, TestRemoveNonMatterContributor)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithContributorStorage(&mContributorStorage)
                                            .WithDelegate(&mDelegate) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    constexpr char kContributorName[] = "BluetoothSensor";

    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan::fromCharString(kContributorName)), CHIP_NO_ERROR);
    
    // Consume the add event to free buffer space
    (void) context.EventsGenerator().GetNextEvent();
    
    mDelegate.Reset();

    EXPECT_EQ(cluster.RemoveNonMatterContributor(CharSpan::fromCharString(kContributorName)), CHIP_NO_ERROR);

    EXPECT_EQ(cluster.GetContributorCount(), 0u);
    EXPECT_TRUE(mDelegate.mContributorRemovedCalled);
}

TEST_F(TestAmbientSensingUnionCluster, TestUpdateNonMatterContributorHealth)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithContributorStorage(&mContributorStorage)
                                            .WithDelegate(&mDelegate) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    constexpr char kContributorName[] = "WifiSensor";

    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan::fromCharString(kContributorName),
                                               UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_NO_ERROR);
    mDelegate.Reset();

    EXPECT_EQ(cluster.UpdateNonMatterContributorHealth(CharSpan::fromCharString(kContributorName),
                                                        UnionContributorHealthEum::kUnionContributorOffline),
              CHIP_NO_ERROR);

    EXPECT_TRUE(mDelegate.mContributorHealthChangedCalled);
    EXPECT_EQ(mDelegate.mLastContributor.contributorHealth, UnionContributorHealthEum::kUnionContributorOffline);
}

// =============================================================================
// Mixed Contributor Tests
// =============================================================================

TEST_F(TestAmbientSensingUnionCluster, TestMixedContributors)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Add Matter contributors
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId2, kContributorEp1), CHIP_NO_ERROR);

    // Add non-Matter contributors
    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan::fromCharString("Zigbee1")), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan::fromCharString("Zigbee2")), CHIP_NO_ERROR);

    EXPECT_EQ(cluster.GetContributorCount(), 4u);

    // Clear all
    cluster.ClearAllContributors();
    EXPECT_EQ(cluster.GetContributorCount(), 0u);
}

// =============================================================================
// Maximum Contributor Count Boundary Test (128 per spec)
// =============================================================================

TEST_F(TestAmbientSensingUnionCluster, TestMaxTotalContributorsBoundary)
{
    chip::Testing::TestServerClusterContext context;

    // Create storage with exactly 128 total capacity (spec maximum)
    // Using 120 Matter + 8 non-Matter to verify both pool types reach capacity
    DefaultContributorStorage<120, 8> maxCapacityStorage;

    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&maxCapacityStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Fill Matter contributor pool (120 contributors)
    for (uint16_t i = 0; i < 120; i++)
    {
        // Use unique NodeId for each contributor
        NodeId nodeId = static_cast<NodeId>(0x1000000000000000ULL + i);
        ASSERT_EQ(cluster.AddMatterContributor(nodeId, kContributorEp1,
                                               UnionContributorHealthEum::kUnionContributorOnline),
                  CHIP_NO_ERROR)
            << "Failed to add Matter contributor " << i;
    }

    EXPECT_EQ(cluster.GetContributorCount(), 120u);

    // Fill non-Matter contributor pool (8 contributors)
    for (uint8_t i = 0; i < 8; i++)
    {
        char name[32];
        snprintf(name, sizeof(name), "Sensor%u", i);
        ASSERT_EQ(cluster.AddNonMatterContributor(CharSpan::fromCharString(name),
                                                   UnionContributorHealthEum::kUnionContributorOnline),
                  CHIP_NO_ERROR)
            << "Failed to add non-Matter contributor " << static_cast<int>(i);
    }

    // Verify exactly 128 contributors (spec maximum)
    EXPECT_EQ(cluster.GetContributorCount(), 128u);

    // Verify health is FullyFunctional with all online contributors
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kFullyFunctional);

#if CHIP_SYSTEM_CONFIG_POOL_USE_HEAP
    // Heap pools can grow beyond template size - this is platform-dependent behavior
    // Just verify the operations succeed beyond the static limit
    NodeId extraNodeId = static_cast<NodeId>(0x2000000000000000ULL);
    EXPECT_EQ(cluster.AddMatterContributor(extraNodeId, kContributorEp1), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.GetContributorCount(), 129u);

    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan::fromCharString("ExtraSensor")), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.GetContributorCount(), 130u);
#else
    // Static pools enforce capacity limits - 129th contributor should fail with NO_MEMORY

    // Attempt to add 121st Matter contributor (exceeds Matter pool capacity)
    NodeId extraNodeId = static_cast<NodeId>(0x2000000000000000ULL);
    EXPECT_EQ(cluster.AddMatterContributor(extraNodeId, kContributorEp1), CHIP_ERROR_NO_MEMORY);

    // Attempt to add 9th non-Matter contributor (exceeds non-Matter pool capacity)
    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan::fromCharString("ExtraSensor")), CHIP_ERROR_NO_MEMORY);

    // Verify count remains at spec maximum of 128
    EXPECT_EQ(cluster.GetContributorCount(), 128u);

    // Verify health unchanged after failed additions
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kFullyFunctional);
#endif

    // Cleanup
    maxCapacityStorage.ClearAllContributors();
}

TEST_F(TestAmbientSensingUnionCluster, TestContributorPoolsIndependent)
{
    chip::Testing::TestServerClusterContext context;

    // Small storage to verify pools are independent
    DefaultContributorStorage<2, 2> smallStorage;

    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&smallStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Fill Matter pool
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId2, kContributorEp1), CHIP_NO_ERROR);

#if !CHIP_SYSTEM_CONFIG_POOL_USE_HEAP
    // Matter pool full - should fail
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp2), CHIP_ERROR_NO_MEMORY);
#endif

    // Non-Matter pool should still have capacity (independent pool)
    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan::fromCharString("Sensor1")), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan::fromCharString("Sensor2")), CHIP_NO_ERROR);

#if !CHIP_SYSTEM_CONFIG_POOL_USE_HEAP
    // Non-Matter pool now full - should fail
    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan::fromCharString("Sensor3")), CHIP_ERROR_NO_MEMORY);

    // Verify total count
    EXPECT_EQ(cluster.GetContributorCount(), 4u);
#endif

    // Cleanup
    smallStorage.ClearAllContributors();
}

// =============================================================================
// Union Health Recalculation Tests
// =============================================================================

TEST_F(TestAmbientSensingUnionCluster, TestUnionHealthAllOnline)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithContributorStorage(&mContributorStorage)
                                            .WithDelegate(&mDelegate) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Add online contributors
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1, UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_NO_ERROR);
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId2, kContributorEp1, UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_NO_ERROR);

    // Health should be FullyFunctional
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kFullyFunctional);
}

TEST_F(TestAmbientSensingUnionCluster, TestUnionHealthAllOffline)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithContributorStorage(&mContributorStorage)
                                            .WithDelegate(&mDelegate) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Add offline contributors
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1, UnionContributorHealthEum::kUnionContributorOffline),
              CHIP_NO_ERROR);
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId2, kContributorEp1, UnionContributorHealthEum::kUnionContributorOffline),
              CHIP_NO_ERROR);

    // Health should be NonFunctional
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kNonFunctional);
}

TEST_F(TestAmbientSensingUnionCluster, TestUnionHealthPartialOffline)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithContributorStorage(&mContributorStorage)
                                            .WithDelegate(&mDelegate) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Add mixed contributors
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1, UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_NO_ERROR);
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId2, kContributorEp1, UnionContributorHealthEum::kUnionContributorOffline),
              CHIP_NO_ERROR);

    // Health should be LimitedDegraded
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kLimitedDegraded);
}

TEST_F(TestAmbientSensingUnionCluster, TestUnionHealthNoContributors)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithContributorStorage(&mContributorStorage)
                                            .WithDelegate(&mDelegate) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Add and then remove contributor
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.RemoveMatterContributor(kTestNodeId1, kContributorEp1), CHIP_NO_ERROR);

    // Health should be NonFunctional with no contributors
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kNonFunctional);
}

TEST_F(TestAmbientSensingUnionCluster, TestUnionHealthTransitions)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithContributorStorage(&mContributorStorage)
                                            .WithDelegate(&mDelegate) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Start with one online contributor
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1, UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_NO_ERROR);
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kFullyFunctional);

    // Add offline contributor - becomes degraded
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId2, kContributorEp1, UnionContributorHealthEum::kUnionContributorOffline),
              CHIP_NO_ERROR);
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kLimitedDegraded);

    // Make offline contributor online - becomes fully functional
    EXPECT_EQ(cluster.UpdateMatterContributorHealth(kTestNodeId2, kContributorEp1,
                                                     UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_NO_ERROR);
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kFullyFunctional);

    // Make both offline - becomes non-functional
    EXPECT_EQ(cluster.UpdateMatterContributorHealth(kTestNodeId1, kContributorEp1,
                                                     UnionContributorHealthEum::kUnionContributorOffline),
              CHIP_NO_ERROR);
    EXPECT_EQ(cluster.UpdateMatterContributorHealth(kTestNodeId2, kContributorEp1,
                                                     UnionContributorHealthEum::kUnionContributorOffline),
              CHIP_NO_ERROR);
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kNonFunctional);
}

// =============================================================================
// Event Tests
// =============================================================================

TEST_F(TestAmbientSensingUnionCluster, TestUnionContributorAddedEvent)
{
    chip::Testing::TestServerClusterContext context;

    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }
            .WithContributorStorage(&mContributorStorage)
            .WithDelegate(&mDelegate) };

    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Add a Matter contributor - should generate event
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1, UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_NO_ERROR);

    // Verify event was generated
    AmbientSensingUnion::Events::UnionContributorAdded::DecodableType decodedEvent;
    auto eventInfo = context.EventsGenerator().GetNextEvent();
    ASSERT_NE(eventInfo, std::nullopt);
    if (eventInfo.has_value())
    {
        EXPECT_EQ(eventInfo->GetEventData(decodedEvent), CHIP_NO_ERROR);
        EXPECT_FALSE(decodedEvent.addedContributor.contributorNodeID.IsNull());
        EXPECT_EQ(decodedEvent.addedContributor.contributorNodeID.Value(), kTestNodeId1);
    }
}

TEST_F(TestAmbientSensingUnionCluster, TestUnionContributorRemovedEvent)
{
    chip::Testing::TestServerClusterContext context;

    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }
            .WithContributorStorage(&mContributorStorage)
            .WithDelegate(&mDelegate) };

    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Add contributor
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1), CHIP_NO_ERROR);
    
    // Consume the add event
    (void) context.EventsGenerator().GetNextEvent();

    // Remove contributor - should generate event
    EXPECT_EQ(cluster.RemoveMatterContributor(kTestNodeId1, kContributorEp1), CHIP_NO_ERROR);

    // Verify remove event was generated
    AmbientSensingUnion::Events::UnionContributorRemoved::DecodableType decodedEvent;
    auto eventInfo = context.EventsGenerator().GetNextEvent();
    ASSERT_NE(eventInfo, std::nullopt);
    if (eventInfo.has_value())
    {
        EXPECT_EQ(eventInfo->GetEventData(decodedEvent), CHIP_NO_ERROR);
        EXPECT_FALSE(decodedEvent.removedContributor.contributorNodeID.IsNull());
        EXPECT_EQ(decodedEvent.removedContributor.contributorNodeID.Value(), kTestNodeId1);
    }
}

TEST_F(TestAmbientSensingUnionCluster, TestUnionContributorHealthChangedEvent)
{
    chip::Testing::TestServerClusterContext context;

    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }
            .WithContributorStorage(&mContributorStorage)
            .WithDelegate(&mDelegate) };

    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Add contributor
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1, UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_NO_ERROR);
    
    // Consume the add event
    (void) context.EventsGenerator().GetNextEvent();

    // Update health - should generate event
    EXPECT_EQ(cluster.UpdateMatterContributorHealth(kTestNodeId1, kContributorEp1,
                                                     UnionContributorHealthEum::kUnionContributorOffline),
              CHIP_NO_ERROR);

    // Verify health changed event was generated
    AmbientSensingUnion::Events::UnionContributorHealthChanged::DecodableType decodedEvent;
    auto eventInfo = context.EventsGenerator().GetNextEvent();
    ASSERT_NE(eventInfo, std::nullopt);
    if (eventInfo.has_value())
    {
        EXPECT_EQ(eventInfo->GetEventData(decodedEvent), CHIP_NO_ERROR);
        EXPECT_EQ(decodedEvent.contributorHealth.contributorHealth, UnionContributorHealthEum::kUnionContributorOffline);
    }
}


// =============================================================================
// Persistence Tests
// =============================================================================

TEST_F(TestAmbientSensingUnionCluster, TestUnionNamePersistence)
{
    chip::Testing::TestServerClusterContext context;

    constexpr char kInitialName[] = "InitialUnion";
    constexpr char kNewName[]     = "UpdatedUnion";

    // First cluster instance - set initial name
    {
        AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                                .WithUnionName(CharSpan::fromCharString(kInitialName))
                                                .WithContributorStorage(&mContributorStorage)
                                                .WithPersistence(&mPersistence) };
        EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

        // Update name - should persist
        EXPECT_EQ(cluster.SetUnionName(CharSpan::fromCharString(kNewName)), CHIP_NO_ERROR);
        EXPECT_EQ(mPersistence.mSaveCount, 1);
        EXPECT_EQ(mPersistence.mStoredName, kNewName);
    }

    // Second cluster instance - should load persisted name
    {
        AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                                .WithUnionName(CharSpan::fromCharString(kInitialName))
                                                .WithContributorStorage(&mContributorStorage)
                                                .WithPersistence(&mPersistence) };
        EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

        // Should have loaded the persisted name, not the config name
        EXPECT_TRUE(cluster.GetUnionName().data_equal(CharSpan::fromCharString(kNewName)));
    }
}

TEST_F(TestAmbientSensingUnionCluster, TestUnionNameNoPersistence)
{
    chip::Testing::TestServerClusterContext context;

    constexpr char kTestName[] = "TestUnion";

    // Cluster without persistence delegate
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithUnionName(CharSpan::fromCharString(kTestName))
                                            .WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Should still work, just won't persist
    EXPECT_EQ(cluster.SetUnionName(CharSpan::fromCharString("NewName")), CHIP_NO_ERROR);
    EXPECT_TRUE(cluster.GetUnionName().data_equal(CharSpan::fromCharString("NewName")));
}

// =============================================================================
// Error Handling Tests
// =============================================================================

TEST_F(TestAmbientSensingUnionCluster, TestStartupWithoutContributorStorage)
{
    chip::Testing::TestServerClusterContext context;

    // Cluster without contributor storage should fail startup
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId } };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_ERROR_INVALID_ARGUMENT);
}

TEST_F(TestAmbientSensingUnionCluster, TestSetUnionNameTooLong)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Create a 129-character string
    char tooLongName[130];
    memset(tooLongName, 'A', 129);
    tooLongName[129] = '\0';

    EXPECT_EQ(cluster.SetUnionName(CharSpan(tooLongName, 129)), CHIP_ERROR_INVALID_ARGUMENT);
}

// =============================================================================
// Attribute Notification Tests
// =============================================================================

TEST_F(TestAmbientSensingUnionCluster, TestUnionNameChangeNotification)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Verify attribute is not dirty initially
    EXPECT_FALSE(context.ChangeListener().IsDirty({ kTestEndpointId, AmbientSensingUnion::Id, Attributes::UnionName::Id }));

    // Change name
    EXPECT_EQ(cluster.SetUnionName(CharSpan::fromCharString("NewName")), CHIP_NO_ERROR);

    // Verify attribute is now dirty
    EXPECT_TRUE(context.ChangeListener().IsDirty({ kTestEndpointId, AmbientSensingUnion::Id, Attributes::UnionName::Id }));
}

TEST_F(TestAmbientSensingUnionCluster, TestUnionHealthChangeNotification_OnContributorAdd)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Add online contributor - will trigger health recalculation
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1, UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_NO_ERROR);

    // Verify health attribute notification (health changed due to contributor addition)
    EXPECT_TRUE(context.ChangeListener().IsDirty({ kTestEndpointId, AmbientSensingUnion::Id, Attributes::UnionHealth::Id }));
}

TEST_F(TestAmbientSensingUnionCluster, TestUnionHealthChangeNotification_OnHealthUpdate)
{
    chip::Testing::TestServerClusterContext context;
    
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }
            .WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Verify initial state - health should be NonFunctional with 0 contributors
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kNonFunctional);

    // Add an OFFLINE contributor - health stays NonFunctional (0/1 online)
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1, 
              UnionContributorHealthEum::kUnionContributorOffline), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kNonFunctional);

    // Now update to ONLINE - health should change to FullyFunctional (1/1 online)
    EXPECT_EQ(cluster.UpdateMatterContributorHealth(kTestNodeId1, kContributorEp1,
              UnionContributorHealthEum::kUnionContributorOnline), CHIP_NO_ERROR);
    
    // Verify health changed
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kFullyFunctional);
    
    // Verify health attribute notification was sent
    EXPECT_TRUE(context.ChangeListener().IsDirty(
        { kTestEndpointId, AmbientSensingUnion::Id, Attributes::UnionHealth::Id }));
}


TEST_F(TestAmbientSensingUnionCluster, TestContributorListChangeNotification_OnAdd)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Verify list is not dirty initially
    EXPECT_FALSE(
        context.ChangeListener().IsDirty({ kTestEndpointId, AmbientSensingUnion::Id, Attributes::UnionContributorList::Id }));

    // Add contributor
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1, UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_NO_ERROR);

    // Verify list is now dirty
    EXPECT_TRUE(
        context.ChangeListener().IsDirty({ kTestEndpointId, AmbientSensingUnion::Id, Attributes::UnionContributorList::Id }));
}

TEST_F(TestAmbientSensingUnionCluster, TestContributorListChangeNotification_OnRemove)
{
    chip::Testing::TestServerClusterContext context;
    
    // Use local storage for this test
    DefaultContributorStorage<32, 8> localStorage;
    
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&localStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Add then remove contributor
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.RemoveMatterContributor(kTestNodeId1, kContributorEp1), CHIP_NO_ERROR);

    // Verify list is dirty after remove
    EXPECT_TRUE(
        context.ChangeListener().IsDirty({ kTestEndpointId, AmbientSensingUnion::Id, Attributes::UnionContributorList::Id }));

    localStorage.ClearAllContributors();
}

TEST_F(TestAmbientSensingUnionCluster, TestContributorListChangeNotification_OnHealthUpdate)
{
    chip::Testing::TestServerClusterContext context;
    
    // Use local storage for this test
    DefaultContributorStorage<32, 8> localStorage;
    
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&localStorage) };
		
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Add contributor
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1, UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_NO_ERROR);

    // Update health
    EXPECT_EQ(cluster.UpdateMatterContributorHealth(kTestNodeId1, kContributorEp1,
                                                     UnionContributorHealthEum::kUnionContributorOffline),
              CHIP_NO_ERROR);

    // Verify list is dirty after health update (list contains health info)
    EXPECT_TRUE(
        context.ChangeListener().IsDirty({ kTestEndpointId, AmbientSensingUnion::Id, Attributes::UnionContributorList::Id }));

    localStorage.ClearAllContributors();
}

// =============================================================================
// SetUnionHealth Direct Call Tests
// =============================================================================

TEST_F(TestAmbientSensingUnionCluster, TestSetUnionHealthDirect)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithContributorStorage(&mContributorStorage)
                                            .WithDelegate(&mDelegate) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Direct call to SetUnionHealth
    cluster.SetUnionHealth(UnionHealthEnum::kLimitedDegraded);

    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kLimitedDegraded);
    EXPECT_TRUE(mDelegate.mUnionHealthChangedCalled);
    EXPECT_EQ(mDelegate.mLastUnionHealth, UnionHealthEnum::kLimitedDegraded);
}

TEST_F(TestAmbientSensingUnionCluster, TestSetUnionHealthSameValueNoOp)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithContributorStorage(&mContributorStorage)
                                            .WithDelegate(&mDelegate) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // After Startup(), health is kNonFunctional (0 contributors)
    // Reset delegate to clear any callbacks from Startup
    mDelegate.Reset();

    // Get current health state
    UnionHealthEnum currentHealth = cluster.GetUnionHealth();
    EXPECT_EQ(currentHealth, UnionHealthEnum::kNonFunctional);

    // Set to SAME value - should NOT trigger delegate
    cluster.SetUnionHealth(currentHealth);

    // Verify delegate was NOT called
    EXPECT_FALSE(mDelegate.mUnionHealthChangedCalled);
}


// =============================================================================
// ClearAllContributors Tests
// =============================================================================

TEST_F(TestAmbientSensingUnionCluster, TestClearAllContributors)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Add multiple contributors
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId2, kContributorEp1), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan::fromCharString("Sensor1")), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.GetContributorCount(), 3u);

    // Clear all
    cluster.ClearAllContributors();

    EXPECT_EQ(cluster.GetContributorCount(), 0u);

    // Health should be NonFunctional
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kNonFunctional);
}

TEST_F(TestAmbientSensingUnionCluster, TestClearAllContributorsNotification)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1), CHIP_NO_ERROR);

    cluster.ClearAllContributors();

    // List should be dirty
    EXPECT_TRUE(
        context.ChangeListener().IsDirty({ kTestEndpointId, AmbientSensingUnion::Id, Attributes::UnionContributorList::Id }));
}

// =============================================================================
// Contributor Storage Capacity Tests
// =============================================================================

TEST_F(TestAmbientSensingUnionCluster, TestContributorStorageCapacity)
{
    chip::Testing::TestServerClusterContext context;

    // Use a small storage for this test
    DefaultContributorStorage<2, 1> smallStorage;

    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&smallStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Fill Matter contributor capacity
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId2, kContributorEp1), CHIP_NO_ERROR);

#if CHIP_SYSTEM_CONFIG_POOL_USE_HEAP
    // On heap pools, we can still add more - just verify it works
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp2), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.GetContributorCount(), 3u);
    
    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan::fromCharString("Sensor1")), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan::fromCharString("Sensor2")), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.GetContributorCount(), 5u);
#else
    // On static pools, capacity is limited
    // Third Matter contributor should fail
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp2), CHIP_ERROR_NO_MEMORY);

    // Non-Matter should still work (separate pool)
    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan::fromCharString("Sensor1")), CHIP_NO_ERROR);

    // Second non-Matter should fail
    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan::fromCharString("Sensor2")), CHIP_ERROR_NO_MEMORY);
#endif
    
    // Clean up
    smallStorage.ClearAllContributors();
}

// =============================================================================
// Non-Matter Contributor Name Length Tests
// =============================================================================

TEST_F(TestAmbientSensingUnionCluster, TestNonMatterContributorMaxNameLength)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Create max length name (128 chars per spec)
    char maxLengthName[129];
    memset(maxLengthName, 'X', 128);
    maxLengthName[128] = '\0';

    // Note: Event generation may fail with large names due to test buffer limits
    // This is expected and doesn't affect functionality
    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan(maxLengthName, 128)), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.GetContributorCount(), 1u);

    // Verify the name was actually stored correctly
    const auto* entry = mContributorStorage.FindNonMatterContributor(CharSpan(maxLengthName, 128));
    EXPECT_NE(entry, nullptr);
    EXPECT_EQ(entry->GetName().size(), 128u);
}

// =============================================================================
// Delegate Not Set Tests
// =============================================================================

TEST_F(TestAmbientSensingUnionCluster, TestOperationsWithoutDelegate)
{
    chip::Testing::TestServerClusterContext context;

    // Cluster without delegate - operations should still work
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // All operations should succeed without delegate
    EXPECT_EQ(cluster.SetUnionName(CharSpan::fromCharString("TestUnion")), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.UpdateMatterContributorHealth(kTestNodeId1, kContributorEp1,
                                                     UnionContributorHealthEum::kUnionContributorOffline),
              CHIP_NO_ERROR);
    EXPECT_EQ(cluster.RemoveMatterContributor(kTestNodeId1, kContributorEp1), CHIP_NO_ERROR);
}

// =============================================================================
// Read-Only Attribute Write Tests
// =============================================================================

TEST_F(TestAmbientSensingUnionCluster, TestWriteReadOnlyUnionHealth)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
    chip::Testing::ClusterTester tester(cluster);

    // Attempt to write UnionHealth - should fail
    EXPECT_EQ(tester.WriteAttribute(Attributes::UnionHealth::Id, UnionHealthEnum::kNonFunctional),
              CHIP_IM_GLOBAL_STATUS(UnsupportedWrite));
}

TEST_F(TestAmbientSensingUnionCluster, TestWriteReadOnlyContributorList)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);
    chip::Testing::ClusterTester tester(cluster);

    // Create a test contributor struct to attempt writing
    Structs::UnionContributorStruct::Type contributor;
    contributor.contributorNodeID.SetNonNull(kTestNodeId1);
    contributor.contributorEndpointID.SetNonNull(kContributorEp1);
    contributor.contributorHealth = UnionContributorHealthEum::kUnionContributorOnline;

    app::DataModel::List<Structs::UnionContributorStruct::Type> contributorList(&contributor, 1);

    // Attempt to write to ContributorList - should fail with UnsupportedWrite
    EXPECT_EQ(tester.WriteAttribute(Attributes::UnionContributorList::Id, contributorList,
              chip::Testing::ListWritingPattern::ReplaceAll),
              CHIP_IM_GLOBAL_STATUS(UnsupportedWrite));
}


// =============================================================================
// Shutdown Tests
// =============================================================================

TEST_F(TestAmbientSensingUnionCluster, TestShutdown)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{
        AmbientSensingUnionCluster::Config{ kTestEndpointId }.WithContributorStorage(&mContributorStorage) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // Add some state
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.SetUnionName(CharSpan::fromCharString("TestUnion")), CHIP_NO_ERROR);

    // Shutdown should not crash
    cluster.Shutdown(ClusterShutdownType::kClusterShutdown);

    // State should still be accessible (shutdown doesn't clear state)
    EXPECT_EQ(cluster.GetContributorCount(), 1u);
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST_F(TestAmbientSensingUnionCluster, TestFullWorkflow)
{
    chip::Testing::TestServerClusterContext context;
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithUnionName(CharSpan::fromCharString("SmartHomeUnion"))
                                            .WithContributorStorage(&mContributorStorage)
                                            .WithDelegate(&mDelegate)
                                            .WithPersistence(&mPersistence) };
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // 1. Initial state - health is NonFunctional with 0 contributors
    EXPECT_EQ(cluster.GetContributorCount(), 0u);
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kNonFunctional);  // Fixed: 0 contributors = NonFunctional

    // 2. Add first Matter contributor (online)
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1, UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_NO_ERROR);
    EXPECT_EQ(cluster.GetContributorCount(), 1u);
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kFullyFunctional);  // 1 online = FullyFunctional

    // 3. Add second Matter contributor (offline)
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId2, kContributorEp1, UnionContributorHealthEum::kUnionContributorOffline),
              CHIP_NO_ERROR);
    EXPECT_EQ(cluster.GetContributorCount(), 2u);
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kLimitedDegraded);  // 1 online, 1 offline = Degraded

    // 4. Add non-Matter contributor (online)
    EXPECT_EQ(cluster.AddNonMatterContributor(CharSpan::fromCharString("ZigbeeSensor"),
                                               UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_NO_ERROR);
    EXPECT_EQ(cluster.GetContributorCount(), 3u);
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kLimitedDegraded);  // Still degraded (1 offline)

    // 5. Bring offline contributor back online
    EXPECT_EQ(cluster.UpdateMatterContributorHealth(kTestNodeId2, kContributorEp1,
                                                     UnionContributorHealthEum::kUnionContributorOnline),
              CHIP_NO_ERROR);
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kFullyFunctional);  // All online now

    // 6. Update union name
    EXPECT_EQ(cluster.SetUnionName(CharSpan::fromCharString("UpdatedUnion")), CHIP_NO_ERROR);
    EXPECT_TRUE(cluster.GetUnionName().data_equal(CharSpan::fromCharString("UpdatedUnion")));
    EXPECT_EQ(mPersistence.mSaveCount, 1);

    // 7. Remove a contributor
    EXPECT_EQ(cluster.RemoveMatterContributor(kTestNodeId1, kContributorEp1), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.GetContributorCount(), 2u);

    // 8. Verify events were generated
    // We should have: 3 adds, 1 health change, 1 remove = at least 5 events
    int eventCount = 0;
    while (context.EventsGenerator().GetNextEvent().has_value())
    {
        eventCount++;
    }
    EXPECT_GE(eventCount, 5);
}

TEST_F(TestAmbientSensingUnionCluster, TestConfigurationChaining)
{
    chip::Testing::TestServerClusterContext context;

    // Test that configuration chaining works correctly
    AmbientSensingUnionCluster cluster{ AmbientSensingUnionCluster::Config{ kTestEndpointId }
                                            .WithUnionName(CharSpan::fromCharString("ChainedConfig"))
                                            .WithUnionHealth(UnionHealthEnum::kLimitedDegraded)
                                            .WithContributorStorage(&mContributorStorage)
                                            .WithDelegate(&mDelegate)
                                            .WithPersistence(&mPersistence) };
    
    // Before Startup(), config values should be set
    EXPECT_TRUE(cluster.GetUnionName().data_equal(CharSpan::fromCharString("ChainedConfig")));
    // Note: GetUnionHealth() before Startup() would return the configured value,
    // but this is not a valid state for the cluster
    
    EXPECT_EQ(cluster.Startup(context.Get()), CHIP_NO_ERROR);

    // After Startup(), name persists but health is recalculated
    EXPECT_TRUE(cluster.GetUnionName().data_equal(CharSpan::fromCharString("ChainedConfig")));
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kNonFunctional);  // 0 contributors
    
    // Add a contributor to show health calculation works
    EXPECT_EQ(cluster.AddMatterContributor(kTestNodeId1, kContributorEp1, 
              UnionContributorHealthEum::kUnionContributorOnline), CHIP_NO_ERROR);
    EXPECT_EQ(cluster.GetUnionHealth(), UnionHealthEnum::kFullyFunctional);  // 1 online
}


} // namespace
