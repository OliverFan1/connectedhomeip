#include <app/clusters/ambient-sensing-union-server/CodegenIntegration.h>

using namespace chip;
using namespace chip::app::Clusters;

void AddContributorToUnion(EndpointId endpointId, NodeId nodeId, EndpointId contributorEndpointId)
{
    AmbientSensingUnion::AmbientSensingUnionCluster * cluster = 
        AmbientSensingUnion::FindClusterOnEndpoint(endpointId);

    if (cluster != nullptr)
    {
        cluster->AddMatterContributor(nodeId, contributorEndpointId,
            AmbientSensingUnion::UnionContributorHealthEum::kUnionContributorOnline);
    }
}

void UpdateContributorHealth(EndpointId endpointId, NodeId nodeId, EndpointId contributorEndpointId, bool isOnline)
{
    AmbientSensingUnion::AmbientSensingUnionCluster * cluster = 
        AmbientSensingUnion::FindClusterOnEndpoint(endpointId);

    if (cluster != nullptr)
    {
        auto health = isOnline 
            ? AmbientSensingUnion::UnionContributorHealthEum::kUnionContributorOnline
            : AmbientSensingUnion::UnionContributorHealthEum::kUnionContributorOffline;

        cluster->UpdateMatterContributorHealth(nodeId, contributorEndpointId, health);
    }
}
