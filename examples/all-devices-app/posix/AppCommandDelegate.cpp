/*
 *
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

#include "include/AppCommandDelegate.h"

#include <app-common/zap-generated/cluster-objects.h>
#include <app/clusters/ambient-sensing-union-server/AmbientSensingUnionCluster.h>
#include <app/clusters/basic-information/BasicInformationCluster.h>
#include <app/clusters/boolean-state-server/BooleanStateCluster.h>
#include <app/clusters/occupancy-sensor-server/OccupancySensingCluster.h>
#include <app/clusters/on-off-server/OnOffCluster.h>
#include <platform/PlatformManager.h>

using namespace chip;
using namespace chip::app;

namespace {

struct CommandContext
{
    Json::Value value;
    EndpointId endpointId;
    AllDevicesAppCommandDelegate * delegate;
    AllDevicesAppNamedPipeCommandHandler * handler;
};

class IncreaseConfigurationVersionCommandHandler : public AllDevicesAppNamedPipeCommandHandler
{
public:
    const char * GetName() const override { return "IncreaseConfigurationVersion"; }
    void Handle(const Json::Value & json, AllDevicesAppCommandDelegate * delegate, EndpointId endpointId) override
    {
        auto * cluster =
            delegate->GetClusterImplementationRegistry().GetClusterByEndpoint<chip::app::Clusters::BasicInformationCluster>(
                endpointId);
        if (!cluster)
        {
            ChipLogError(AppServer, "BasicInformationCluster not found on endpoint %d", endpointId);
            return;
        }

        CHIP_ERROR err = cluster->IncreaseConfigurationVersion();
        ChipLogProgress(AppServer, "IncreaseConfigurationVersion on endpoint %d: %" CHIP_ERROR_FORMAT, endpointId, err.Format());
    }
};

class SetOccupancyCommandHandler : public AllDevicesAppNamedPipeCommandHandler
{
public:
    const char * GetName() const override { return "SetOccupancy"; }
    void Handle(const Json::Value & json, AllDevicesAppCommandDelegate * delegate, EndpointId endpointId) override
    {
        auto * cluster =
            delegate->GetClusterImplementationRegistry().GetClusterByEndpoint<chip::app::Clusters::OccupancySensingCluster>(
                endpointId);
        if (!cluster)
        {
            ChipLogError(AppServer, "OccupancySensingCluster not found on endpoint %d", endpointId);
            return;
        }

        if (!json.isMember("Occupancy") || !json["Occupancy"].isUInt())
        {
            ChipLogError(AppServer, "Invalid SetOccupancy command: missing 'Occupancy' field");
            return;
        }

        unsigned int occupancyVal = json["Occupancy"].asUInt();
        if (occupancyVal != 0 && occupancyVal != 1)
        {
            ChipLogError(AppServer, "Invalid occupancy value: %u", occupancyVal);
            return;
        }
        uint8_t occupancy = static_cast<uint8_t>(occupancyVal);

        cluster->SetOccupancy(occupancy != 0);
        ChipLogProgress(AppServer, "SetOccupancy to %d on endpoint %d", occupancy, endpointId);
    }
};

class SetHoldTimeCommandHandler : public AllDevicesAppNamedPipeCommandHandler
{
public:
    const char * GetName() const override { return "SetHoldTime"; }
    void Handle(const Json::Value & json, AllDevicesAppCommandDelegate * delegate, EndpointId endpointId) override
    {
        auto * cluster =
            delegate->GetClusterImplementationRegistry().GetClusterByEndpoint<chip::app::Clusters::OccupancySensingCluster>(
                endpointId);
        if (!cluster)
        {
            ChipLogError(AppServer, "OccupancySensingCluster not found on endpoint %d", endpointId);
            return;
        }

        if (!json.isMember("HoldTime") || !json["HoldTime"].isUInt())
        {
            ChipLogError(AppServer, "Invalid SetHoldTime command: missing 'HoldTime' field");
            return;
        }

        unsigned int holdTimeVal = json["HoldTime"].asUInt();
        if (holdTimeVal > 0xFFFF)
        {
            ChipLogError(AppServer, "Invalid HoldTime value (out of range): %u", holdTimeVal);
            return;
        }
        uint16_t holdTime = static_cast<uint16_t>(holdTimeVal);
        cluster->SetHoldTime(holdTime);
        ChipLogProgress(AppServer, "SetHoldTime to %d on endpoint %d", holdTime, endpointId);
    }
};

class SetBooleanStateCommandHandler : public AllDevicesAppNamedPipeCommandHandler
{
public:
    const char * GetName() const override { return "SetBooleanState"; }
    void Handle(const Json::Value & json, AllDevicesAppCommandDelegate * delegate, EndpointId endpointId) override
    {
        auto * cluster =
            delegate->GetClusterImplementationRegistry().GetClusterByEndpoint<chip::app::Clusters::BooleanStateCluster>(endpointId);
        if (!cluster)
        {
            ChipLogError(AppServer, "BooleanStateCluster not found on endpoint %d", endpointId);
            return;
        }

        if (!json.isMember("NewState") || !json["NewState"].isBool())
        {
            ChipLogError(AppServer, "Invalid SetBooleanState command: missing 'NewState' field");
            return;
        }

        bool newState = json["NewState"].asBool();
        cluster->SetStateValue(newState);
        ChipLogProgress(AppServer, "SetBooleanState to %d on endpoint %d", newState, endpointId);
    }
};

class SetOnOffCommandHandler : public AllDevicesAppNamedPipeCommandHandler
{
public:
    const char * GetName() const override { return "SetOnOff"; }
    void Handle(const Json::Value & json, AllDevicesAppCommandDelegate * delegate, EndpointId endpointId) override
    {
        auto * cluster =
            delegate->GetClusterImplementationRegistry().GetClusterByEndpoint<chip::app::Clusters::OnOffCluster>(endpointId);
        if (!cluster)
        {
            ChipLogError(AppServer, "OnOffCluster not found on endpoint %d", endpointId);
            return;
        }

        if (!json.isMember("OnOff") || !json["OnOff"].isBool())
        {
            ChipLogError(AppServer, "Invalid SetOnOff command: missing 'OnOff' field");
            return;
        }

        bool onOff     = json["OnOff"].asBool();
        CHIP_ERROR err = cluster->SetOnOff(onOff);
        ChipLogProgress(AppServer, "SetOnOff to %d on endpoint %d: %" CHIP_ERROR_FORMAT, onOff, endpointId, err.Format());
    }
};

// ---------------------------------------------------------------------------
// AmbientSensingUnion command handlers
// ---------------------------------------------------------------------------

/**
 * Parses a NodeId from a JSON value that may be a hex string (e.g. "0x1234...") or a number.
 * Returns kUndefinedNodeId on failure.
 */
static NodeId ParseNodeId(const Json::Value & json)
{
    if (!json.isMember("NodeId"))
    {
        return kUndefinedNodeId;
    }
    if (json["NodeId"].isString())
    {
        std::string s = json["NodeId"].asString();
        return static_cast<NodeId>(strtoull(s.c_str(), nullptr, 0));
    }
    if (json["NodeId"].isNumeric())
    {
        return static_cast<NodeId>(json["NodeId"].asUInt64());
    }
    return kUndefinedNodeId;
}

/**
 * Named pipe handler for adding a Matter contributor to AmbientSensingUnion.
 *
 * Usage example:
 *   echo '{"Name": "AddAmbientSensingContributor", "EndpointId": 1,
 *          "NodeId": "0x1234567890ABCDEF", "ContributorEndpointId": 2, "Status": 0}'
 *        > /tmp/chip_all_devices_fifo_<pid>
 *
 * JSON Arguments:
 *   - "Name": "AddAmbientSensingContributor"
 *   - "EndpointId": endpoint hosting the AmbientSensingUnion cluster
 *   - "NodeId": Node ID of the Matter contributor (hex string or number)
 *   - "ContributorEndpointId": endpoint of the contributor
 *   - "Status": 0 = Online, 1 = Offline
 */
class AddAmbientSensingContributorCommandHandler : public AllDevicesAppNamedPipeCommandHandler
{
public:
    const char * GetName() const override { return "AddAmbientSensingContributor"; }
    void Handle(const Json::Value & json, AllDevicesAppCommandDelegate * delegate, EndpointId endpointId) override
    {
        auto * cluster =
            delegate->GetClusterImplementationRegistry()
                .GetClusterByEndpoint<chip::app::Clusters::AmbientSensingUnionCluster>(endpointId);
        if (!cluster)
        {
            ChipLogError(AppServer, "AmbientSensingUnionCluster not found on endpoint %d", endpointId);
            return;
        }

        NodeId nodeId = ParseNodeId(json);
        if (nodeId == kUndefinedNodeId)
        {
            ChipLogError(AppServer, "Invalid or missing NodeId for AddAmbientSensingContributor");
            return;
        }

        if (!json.isMember("ContributorEndpointId") || !json["ContributorEndpointId"].isUInt())
        {
            ChipLogError(AppServer, "Missing 'ContributorEndpointId' for AddAmbientSensingContributor");
            return;
        }

        if (!json.isMember("Status") || !json["Status"].isUInt())
        {
            ChipLogError(AppServer, "Missing 'Status' for AddAmbientSensingContributor");
            return;
        }

        EndpointId contributorEndpointId = static_cast<EndpointId>(json["ContributorEndpointId"].asUInt());
        auto status = static_cast<chip::app::Clusters::AmbientSensingUnion::UnionContributorStatusEnum>(
            json["Status"].asUInt());

        CHIP_ERROR err = cluster->AddMatterContributor(nodeId, contributorEndpointId, status);
        if (err != CHIP_NO_ERROR)
        {
            ChipLogError(AppServer, "AddMatterContributor failed on endpoint %d: %" CHIP_ERROR_FORMAT, endpointId, err.Format());
        }
        else
        {
            ChipLogProgress(AppServer, "Added Matter contributor NodeId: 0x" ChipLogFormatX64 ", ContributorEndpoint: %d on endpoint %d",
                            ChipLogValueX64(nodeId), contributorEndpointId, endpointId);
        }
    }
};

/**
 * Named pipe handler for removing a Matter contributor from AmbientSensingUnion.
 *
 * Usage example:
 *   echo '{"Name": "RemoveAmbientSensingContributor", "EndpointId": 1,
 *          "NodeId": "0x1234567890ABCDEF", "ContributorEndpointId": 2}'
 *        > /tmp/chip_all_devices_fifo_<pid>
 */
class RemoveAmbientSensingContributorCommandHandler : public AllDevicesAppNamedPipeCommandHandler
{
public:
    const char * GetName() const override { return "RemoveAmbientSensingContributor"; }
    void Handle(const Json::Value & json, AllDevicesAppCommandDelegate * delegate, EndpointId endpointId) override
    {
        auto * cluster =
            delegate->GetClusterImplementationRegistry()
                .GetClusterByEndpoint<chip::app::Clusters::AmbientSensingUnionCluster>(endpointId);
        if (!cluster)
        {
            ChipLogError(AppServer, "AmbientSensingUnionCluster not found on endpoint %d", endpointId);
            return;
        }

        NodeId nodeId = ParseNodeId(json);
        if (nodeId == kUndefinedNodeId)
        {
            ChipLogError(AppServer, "Invalid or missing NodeId for RemoveAmbientSensingContributor");
            return;
        }

        if (!json.isMember("ContributorEndpointId") || !json["ContributorEndpointId"].isUInt())
        {
            ChipLogError(AppServer, "Missing 'ContributorEndpointId' for RemoveAmbientSensingContributor");
            return;
        }

        EndpointId contributorEndpointId = static_cast<EndpointId>(json["ContributorEndpointId"].asUInt());

        CHIP_ERROR err = cluster->RemoveMatterContributor(nodeId, contributorEndpointId);
        if (err != CHIP_NO_ERROR)
        {
            ChipLogError(AppServer, "RemoveMatterContributor failed on endpoint %d: %" CHIP_ERROR_FORMAT, endpointId, err.Format());
        }
        else
        {
            ChipLogProgress(AppServer, "Removed Matter contributor NodeId: 0x" ChipLogFormatX64 " on endpoint %d",
                            ChipLogValueX64(nodeId), endpointId);
        }
    }
};

/**
 * Named pipe handler for adding a non-Matter contributor to AmbientSensingUnion.
 *
 * Usage example:
 *   echo '{"Name": "AddAmbientSensingNonMatterContributor", "EndpointId": 1,
 *          "ContributorName": "ZigbeeSensor1", "Status": 0}'
 *        > /tmp/chip_all_devices_fifo_<pid>
 */
class AddAmbientSensingNonMatterContributorCommandHandler : public AllDevicesAppNamedPipeCommandHandler
{
public:
    const char * GetName() const override { return "AddAmbientSensingNonMatterContributor"; }
    void Handle(const Json::Value & json, AllDevicesAppCommandDelegate * delegate, EndpointId endpointId) override
    {
        auto * cluster =
            delegate->GetClusterImplementationRegistry()
                .GetClusterByEndpoint<chip::app::Clusters::AmbientSensingUnionCluster>(endpointId);
        if (!cluster)
        {
            ChipLogError(AppServer, "AmbientSensingUnionCluster not found on endpoint %d", endpointId);
            return;
        }

        if (!json.isMember("ContributorName") || !json["ContributorName"].isString())
        {
            ChipLogError(AppServer, "Missing 'ContributorName' for AddAmbientSensingNonMatterContributor");
            return;
        }

        if (!json.isMember("Status") || !json["Status"].isUInt())
        {
            ChipLogError(AppServer, "Missing 'Status' for AddAmbientSensingNonMatterContributor");
            return;
        }

        std::string contributorName = json["ContributorName"].asString();
        if (contributorName.empty())
        {
            ChipLogError(AppServer, "ContributorName cannot be empty");
            return;
        }

        auto status = static_cast<chip::app::Clusters::AmbientSensingUnion::UnionContributorStatusEnum>(
            json["Status"].asUInt());

        CHIP_ERROR err = cluster->AddNonMatterContributor(chip::CharSpan::fromCharString(contributorName.c_str()), status);
        if (err != CHIP_NO_ERROR)
        {
            ChipLogError(AppServer, "AddNonMatterContributor failed on endpoint %d: %" CHIP_ERROR_FORMAT, endpointId, err.Format());
        }
        else
        {
            ChipLogProgress(AppServer, "Added non-Matter contributor '%s' on endpoint %d", contributorName.c_str(), endpointId);
        }
    }
};

/**
 * Named pipe handler for removing a non-Matter contributor from AmbientSensingUnion.
 *
 * Usage example:
 *   echo '{"Name": "RemoveAmbientSensingNonMatterContributor", "EndpointId": 1,
 *          "ContributorName": "ZigbeeSensor1"}'
 *        > /tmp/chip_all_devices_fifo_<pid>
 */
class RemoveAmbientSensingNonMatterContributorCommandHandler : public AllDevicesAppNamedPipeCommandHandler
{
public:
    const char * GetName() const override { return "RemoveAmbientSensingNonMatterContributor"; }
    void Handle(const Json::Value & json, AllDevicesAppCommandDelegate * delegate, EndpointId endpointId) override
    {
        auto * cluster =
            delegate->GetClusterImplementationRegistry()
                .GetClusterByEndpoint<chip::app::Clusters::AmbientSensingUnionCluster>(endpointId);
        if (!cluster)
        {
            ChipLogError(AppServer, "AmbientSensingUnionCluster not found on endpoint %d", endpointId);
            return;
        }

        if (!json.isMember("ContributorName") || !json["ContributorName"].isString())
        {
            ChipLogError(AppServer, "Missing 'ContributorName' for RemoveAmbientSensingNonMatterContributor");
            return;
        }

        std::string contributorName = json["ContributorName"].asString();
        if (contributorName.empty())
        {
            ChipLogError(AppServer, "ContributorName cannot be empty");
            return;
        }

        CHIP_ERROR err = cluster->RemoveNonMatterContributor(chip::CharSpan::fromCharString(contributorName.c_str()));
        if (err != CHIP_NO_ERROR)
        {
            ChipLogError(AppServer, "RemoveNonMatterContributor failed on endpoint %d: %" CHIP_ERROR_FORMAT, endpointId,
                         err.Format());
        }
        else
        {
            ChipLogProgress(AppServer, "Removed non-Matter contributor '%s' on endpoint %d", contributorName.c_str(), endpointId);
        }
    }
};

/**
 * Named pipe handler for updating contributor status in AmbientSensingUnion.
 *
 * Usage example (Matter contributor):
 *   echo '{"Name": "UpdateAmbientSensingContributorStatus", "EndpointId": 1,
 *          "NodeId": "0x1234567890ABCDEF", "ContributorEndpointId": 2, "Status": 1}'
 *        > /tmp/chip_all_devices_fifo_<pid>
 *
 * Usage example (non-Matter contributor):
 *   echo '{"Name": "UpdateAmbientSensingContributorStatus", "EndpointId": 1,
 *          "ContributorName": "ZigbeeSensor1", "Status": 1}'
 *        > /tmp/chip_all_devices_fifo_<pid>
 */
class UpdateAmbientSensingContributorStatusCommandHandler : public AllDevicesAppNamedPipeCommandHandler
{
public:
    const char * GetName() const override { return "UpdateAmbientSensingContributorStatus"; }
    void Handle(const Json::Value & json, AllDevicesAppCommandDelegate * delegate, EndpointId endpointId) override
    {
        auto * cluster =
            delegate->GetClusterImplementationRegistry()
                .GetClusterByEndpoint<chip::app::Clusters::AmbientSensingUnionCluster>(endpointId);
        if (!cluster)
        {
            ChipLogError(AppServer, "AmbientSensingUnionCluster not found on endpoint %d", endpointId);
            return;
        }

        if (!json.isMember("Status") || !json["Status"].isUInt())
        {
            ChipLogError(AppServer, "Missing 'Status' for UpdateAmbientSensingContributorStatus");
            return;
        }

        auto status = static_cast<chip::app::Clusters::AmbientSensingUnion::UnionContributorStatusEnum>(
            json["Status"].asUInt());

        CHIP_ERROR err = CHIP_NO_ERROR;

        if (json.isMember("NodeId"))
        {
            // Matter contributor update path
            NodeId nodeId = ParseNodeId(json);
            if (nodeId == kUndefinedNodeId)
            {
                ChipLogError(AppServer, "Invalid NodeId for UpdateAmbientSensingContributorStatus");
                return;
            }

            if (!json.isMember("ContributorEndpointId") || !json["ContributorEndpointId"].isUInt())
            {
                ChipLogError(AppServer, "Missing 'ContributorEndpointId' for Matter contributor status update");
                return;
            }

            EndpointId contributorEndpointId = static_cast<EndpointId>(json["ContributorEndpointId"].asUInt());
            err                              = cluster->UpdateMatterContributorStatus(nodeId, contributorEndpointId, status);
            if (err == CHIP_NO_ERROR)
            {
                ChipLogProgress(AppServer,
                                "Updated Matter contributor (NodeId: 0x" ChipLogFormatX64 ", ContributorEndpoint: %d) status to %u on endpoint %d",
                                ChipLogValueX64(nodeId), contributorEndpointId, static_cast<unsigned>(status), endpointId);
            }
        }
        else if (json.isMember("ContributorName") && json["ContributorName"].isString())
        {
            // Non-Matter contributor update path
            std::string contributorName = json["ContributorName"].asString();
            if (contributorName.empty())
            {
                ChipLogError(AppServer, "ContributorName cannot be empty");
                return;
            }

            err = cluster->UpdateNonMatterContributorStatus(chip::CharSpan::fromCharString(contributorName.c_str()), status);
            if (err == CHIP_NO_ERROR)
            {
                ChipLogProgress(AppServer, "Updated non-Matter contributor '%s' status to %u on endpoint %d",
                                contributorName.c_str(), static_cast<unsigned>(status), endpointId);
            }
        }
        else
        {
            ChipLogError(AppServer, "Must specify either 'NodeId' or 'ContributorName' for UpdateAmbientSensingContributorStatus");
            return;
        }

        if (err != CHIP_NO_ERROR)
        {
            ChipLogError(AppServer, "UpdateContributorStatus failed on endpoint %d: %" CHIP_ERROR_FORMAT, endpointId, err.Format());
        }
    }
};

/**
 * Named pipe handler for setting the union name on AmbientSensingUnion.
 *
 * Usage example:
 *   echo '{"Name": "SetAmbientSensingUnionName", "EndpointId": 1, "UnionName": "LivingRoomUnion"}'
 *        > /tmp/chip_all_devices_fifo_<pid>
 */
class SetAmbientSensingUnionNameCommandHandler : public AllDevicesAppNamedPipeCommandHandler
{
public:
    const char * GetName() const override { return "SetAmbientSensingUnionName"; }
    void Handle(const Json::Value & json, AllDevicesAppCommandDelegate * delegate, EndpointId endpointId) override
    {
        auto * cluster =
            delegate->GetClusterImplementationRegistry()
                .GetClusterByEndpoint<chip::app::Clusters::AmbientSensingUnionCluster>(endpointId);
        if (!cluster)
        {
            ChipLogError(AppServer, "AmbientSensingUnionCluster not found on endpoint %d", endpointId);
            return;
        }

        if (!json.isMember("UnionName") || !json["UnionName"].isString())
        {
            ChipLogError(AppServer, "Missing 'UnionName' for SetAmbientSensingUnionName");
            return;
        }

        std::string unionName = json["UnionName"].asString();
        if (unionName.empty())
        {
            ChipLogError(AppServer, "UnionName cannot be empty");
            return;
        }

        CHIP_ERROR err = cluster->SetUnionName(chip::CharSpan::fromCharString(unionName.c_str()));
        if (err != CHIP_NO_ERROR)
        {
            ChipLogError(AppServer, "SetUnionName failed on endpoint %d: %" CHIP_ERROR_FORMAT, endpointId, err.Format());
        }
        else
        {
            ChipLogProgress(AppServer, "Set union name to '%s' on endpoint %d", unionName.c_str(), endpointId);
        }
    }
};

} // namespace

void AllDevicesAppCommandDelegate::OnEventCommandReceived(const char * json)
{
    Json::Reader reader;
    Json::Value value;
    if (!reader.parse(json, value))
    {
        ChipLogError(AppServer, "Failed to parse JSON command: %s", reader.getFormattedErrorMessages().c_str());
        return;
    }

    if (!value.isMember("Name") || !value["Name"].isString() || !value.isMember("EndpointId") || !value["EndpointId"].isUInt())
    {
        ChipLogError(AppServer, "Invalid command format: %s", json);
        return;
    }

    std::string commandName = value["Name"].asString();

    unsigned int endpointIdVal = value["EndpointId"].asUInt();
    if (endpointIdVal > 0xFFFF)
    {
        ChipLogError(AppServer, "Invalid EndpointId (out of range): %u", endpointIdVal);
        return;
    }

    EndpointId endpointId = static_cast<EndpointId>(endpointIdVal);
    auto handlerIt        = mCommandHandlers.find(commandName);

    if (handlerIt == mCommandHandlers.end())
    {
        ChipLogError(AppServer, "Unknown command: %s", commandName.c_str());
        return;
    }

    auto * context = Platform::New<CommandContext>();
    if (context == nullptr)
    {
        ChipLogError(AppServer, "Failure to allocate command context! Ignoring command.");
        return;
    }
    context->value      = value;
    context->endpointId = endpointId;
    context->delegate   = this;
    context->handler    = handlerIt->second.get();

    CHIP_ERROR err = DeviceLayer::PlatformMgr().ScheduleWork(DispatchCommand, reinterpret_cast<intptr_t>(context));
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(AppServer, "Failed to schedule work: %" CHIP_ERROR_FORMAT, err.Format());
        Platform::Delete(context);
    }
}

void AllDevicesAppCommandDelegate::DispatchCommand(intptr_t context)
{
    auto * cmdContext = reinterpret_cast<CommandContext *>(context);
    cmdContext->handler->Handle(cmdContext->value, cmdContext->delegate, cmdContext->endpointId);
    Platform::Delete(cmdContext);
}

void AllDevicesAppCommandDelegate::RegisterCommandHandler(std::unique_ptr<AllDevicesAppNamedPipeCommandHandler> handler)
{
    mCommandHandlers[handler->GetName()] = std::move(handler);
}

void AllDevicesAppCommandDelegate::RegisterCommandHandlers()
{
    RegisterCommandHandler(std::make_unique<IncreaseConfigurationVersionCommandHandler>());
    RegisterCommandHandler(std::make_unique<SetOccupancyCommandHandler>());
    RegisterCommandHandler(std::make_unique<SetHoldTimeCommandHandler>());
    RegisterCommandHandler(std::make_unique<SetBooleanStateCommandHandler>());
    RegisterCommandHandler(std::make_unique<SetOnOffCommandHandler>());
    // AmbientSensingUnion command handlers
    RegisterCommandHandler(std::make_unique<AddAmbientSensingContributorCommandHandler>());
    RegisterCommandHandler(std::make_unique<RemoveAmbientSensingContributorCommandHandler>());
    RegisterCommandHandler(std::make_unique<AddAmbientSensingNonMatterContributorCommandHandler>());
    RegisterCommandHandler(std::make_unique<RemoveAmbientSensingNonMatterContributorCommandHandler>());
    RegisterCommandHandler(std::make_unique<UpdateAmbientSensingContributorStatusCommandHandler>());
    RegisterCommandHandler(std::make_unique<SetAmbientSensingUnionNameCommandHandler>());
}
