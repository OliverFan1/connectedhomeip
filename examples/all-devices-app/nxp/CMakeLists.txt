#
# Copyright (c) 2026 Project CHIP Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# ****************************************************************
# Matter All-Devices application for NXP FreeRTOS platforms
# ****************************************************************

cmake_minimum_required(VERSION 3.30)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Retrieve CHIP_ROOT if provided
if (DEFINED CHIP_ROOT)
    get_filename_component(CHIP_ROOT ${CHIP_ROOT} ABSOLUTE)
else()
    get_filename_component(CHIP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../ REALPATH)
endif()

# Export CHIP_ROOT as an environment variable to be recognized by Kconfig
set(COMMON_KCONFIG_ENV_SETTINGS CHIP_ROOT=${CHIP_ROOT})

get_filename_component(GEN_DIR                         ${CHIP_ROOT}/zzz_generated/ REALPATH)
get_filename_component(NXP_EXAMPLE_DIR                 ${CMAKE_CURRENT_SOURCE_DIR}/common REALPATH)
get_filename_component(ALL_DEVICES_COMMON_DIR          ${CHIP_ROOT}/examples/all-devices-app/all-devices-common REALPATH)
get_filename_component(EXAMPLE_PLATFORM_NXP_COMMON_DIR ${CHIP_ROOT}/examples/platform/nxp/common REALPATH)
get_filename_component(NXP_MATTER_SUPPORT_DIR          ${CHIP_ROOT}/third_party/nxp/nxp_matter_support REALPATH)

# Add the CHIP stack & NXP port libs as an external module
list(APPEND EXTRA_MCUX_MODULES ${CHIP_ROOT}/config/nxp/chip-cmake-freertos)

# If CONF_FILE_NAME is provided, append its full path to CONF_FILE
if (DEFINED CONF_FILE_NAME)
    set(CONF_FILE ${CHIP_ROOT}/examples/platform/nxp/config/${CONF_FILE_NAME} ${CONF_FILE})
endif()

# Add application prj.conf in front of CONF_FILE
set(CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/prj.conf ${CONF_FILE})

set(KCONFIG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig)

# Load the MCUX SDK package
find_package(McuxSDK 3.0.0)

# Get compiler flags and includes and pass them to CHIP target
list(APPEND CONFIG_CHIP_EXTERNAL_TARGETS McuxSDK)

if (NOT DEFINED NXP_SDK_RECONFIG_CMAKE_DIR)
    get_filename_component(NXP_SDK_RECONFIG_CMAKE_DIR
        ${NXP_MATTER_SUPPORT_DIR}/examples/platform/${CONFIG_CHIP_NXP_PLATFORM_FAMILY} REALPATH)
endif()

# Include the nxp_sdk_reconfig.cmake to customize the SDK for Matter application
include(${NXP_SDK_RECONFIG_CMAKE_DIR}/nxp_sdk_reconfig.cmake)

# Pull in enabled_devices (device type sources + generates app_config/enabled_devices.h)
include(${ALL_DEVICES_COMMON_DIR}/device-factory/enabled_devices.cmake)

# Define application project name
project(chip-nxp-all-devices-app-example)

# This app uses a fully code-driven data model (CodeDrivenDataModelProvider)
# with no ZAP file. Do NOT call chip_configure_data_model() -- that function
# unconditionally invokes chip_zapgen() even when no .zap file is provided.
# chip_data_model.cmake is already included by the CHIP module in
# config/nxp/chip-cmake-freertos/CMakeLists.txt; we just set CHIP_APP_BASE_DIR
# explicitly here for clarity.
set(CHIP_APP_BASE_DIR ${CHIP_ROOT}/src/app)

# Add NXP common application files
include(${EXAMPLE_PLATFORM_NXP_COMMON_DIR}/app_common.cmake)

# cmd_server.cpp is added by app_common.cmake for CLI builds. It registers a
# "server" shell command group (start/stop/port/sessions/clusters/endpoints).
# The "clusters" and "endpoints" sub-commands call Ember APIs such as
# emberAfEndpointCount() and emberAfGetNthCluster() which require a
# ZAP-generated gen_config.h. This app is code-driven and has no ZAP file,
# so those sub-commands cannot be used.
#
# In addition, the "server start/stop" sub-commands are redundant here: the
# all-devices app uses its own "devtype set <type>" CLI command to initialize
# the code-driven data model and start the server. Removing cmd_server.cpp has
# no functional impact on this application.
get_target_property(_app_sources app SOURCES)
list(REMOVE_ITEM _app_sources "${CHIP_ROOT}/examples/shell/shell_common/cmd_server.cpp")
set_target_properties(app PROPERTIES SOURCES "${_app_sources}")
target_compile_definitions(app PRIVATE CHIP_SHELL_ENABLE_CMD_SERVER=0)

target_include_directories(app
    PRIVATE
    ${NXP_EXAMPLE_DIR}/main/include
    ${NXP_EXAMPLE_DIR}/main
    ${GEN_DIR}/app-common
    ${ALL_DEVICES_COMMON_DIR}
    ${CHIP_ROOT}/src/data-model-providers/codedriven
    ${CHIP_ROOT}/src/data-model-providers
    ${CMAKE_CURRENT_BINARY_DIR}   # for generated app_config/enabled_devices.h
)

target_sources(app
    PRIVATE
    # Application entry points
    ${NXP_EXAMPLE_DIR}/main/main.cpp
    ${NXP_EXAMPLE_DIR}/main/AppTask.cpp
    ${NXP_EXAMPLE_DIR}/main/AllDevicesServer.cpp
    ${NXP_EXAMPLE_DIR}/main/DeviceTypeShell.cpp
    ${NXP_EXAMPLE_DIR}/main/DeviceAppShell.cpp

    # Matter server-side infrastructure. These sources are added here directly
    # rather than through chip_configure_data_model() to avoid the ZAP code
    # generation that chip_configure_data_model() triggers unconditionally.
    ${CHIP_APP_BASE_DIR}/SafeAttributePersistenceProvider.cpp
    ${CHIP_APP_BASE_DIR}/server/AclStorage.cpp
    ${CHIP_APP_BASE_DIR}/server/CommissioningWindowManager.cpp
    ${CHIP_APP_BASE_DIR}/server/DefaultAclStorage.cpp
    ${CHIP_APP_BASE_DIR}/server/DefaultTermsAndConditionsProvider.cpp
    ${CHIP_APP_BASE_DIR}/server/Dnssd.cpp
    ${CHIP_APP_BASE_DIR}/server/EchoHandler.cpp
    ${CHIP_APP_BASE_DIR}/server/Server.cpp

    # Code-driven data model infrastructure
    ${CHIP_ROOT}/src/data-model-providers/codedriven/CodeDrivenDataModelProvider.cpp
    ${CHIP_ROOT}/src/data-model-providers/codedriven/endpoint/EndpointInterfaceRegistry.cpp
    ${CHIP_ROOT}/src/app/server-cluster/ServerClusterInterfaceRegistry.cpp
    ${CHIP_ROOT}/src/app/StorageDelegateWrapper.cpp
    ${CHIP_ROOT}/src/app/persistence/DefaultAttributePersistenceProvider.cpp
    ${CHIP_ROOT}/src/app/persistence/AttributePersistenceMigration.cpp
    ${CHIP_ROOT}/src/app/persistence/AttributePersistenceProviderInstance.cpp

    # All-devices device type sources and required cluster sources
    ${ALL_DEVICES_DEVICE_SOURCES}
    ${ALL_DEVICES_CLUSTER_SOURCES}
)

# Root-node specialization: compile the correct connectivity variant.
# WifiRootNode and ThreadRootNode each provide the vtable implementation for
# the root-node device type used by AllDevicesServer.cpp.
if (CONFIG_CHIP_WIFI AND CONFIG_NET_L2_OPENTHREAD)
    message(FATAL_ERROR "WiFi + Thread (Border Router) configuration is not supported in this application.")
elseif (CONFIG_CHIP_WIFI)
    target_sources(app PRIVATE ${ALL_DEVICES_COMMON_DIR}/device/types/root-node/WifiRootNode.cpp)
elseif (CONFIG_NET_L2_OPENTHREAD)
    target_sources(app PRIVATE ${ALL_DEVICES_COMMON_DIR}/device/types/root-node/ThreadRootNode.cpp)
else()
    message(FATAL_ERROR "No supported network layer found. Set CONFIG_CHIP_WIFI=y or CONFIG_NET_L2_OPENTHREAD=y.")
endif()

# Required by the address-resolve layer used in Server.cpp.
target_compile_options(app PRIVATE
    "-DCHIP_ADDRESS_RESOLVE_IMPL_INCLUDE_HEADER=<lib/address_resolve/AddressResolve_DefaultImpl.h>"
)

target_compile_definitions(app PRIVATE
    CONFIG_NXP_ALL_DEVICES_APP=1
)