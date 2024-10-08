/**
 *
 *    Copyright (c) 2021 Project CHIP Authors
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

#import "MTRDeviceController.h"

#import <Foundation/Foundation.h>

#include <app/OperationalSessionSetup.h>
#include <controller/CHIPDeviceController.h>
#include <lib/core/ReferenceCounted.h>
#include <messaging/ExchangeMgr.h>
#include <transport/Session.h>

NS_ASSUME_NONNULL_BEGIN

// Either exchangeManager will be non-nil and session will have a value, or
// error will be non-nil.  If error is non-nil, retryDelay might be non-nil.  In
// that case it will contain an NSTimeInterval indicating how long consumers
// should delay before trying again (e.g. based on the information communicated
// in a BUSY response).
typedef void (^MTRInternalDeviceConnectionCallback)(chip::Messaging::ExchangeManager * _Nullable exchangeManager,
    const chip::Optional<chip::SessionHandle> & session, NSError * _Nullable error, NSNumber * _Nullable retryDelay);

/**
 * Helper to establish or look up a CASE session and hand its session
 * information back to the consumer.
 */
class MTRDeviceConnectionBridge : public chip::ReferenceCounted<MTRDeviceConnectionBridge> {
public:
    MTRDeviceConnectionBridge(MTRInternalDeviceConnectionCallback completionHandler)
        : mCompletionHandler(completionHandler)
        , mOnConnected(OnConnected, this)
        , mOnConnectFailed(OnConnectionFailure, this)
    {
    }

    ~MTRDeviceConnectionBridge()
    {
        mOnConnected.Cancel();
        mOnConnectFailed.Cancel();
    }

    /**
     * connect must be called on the Matter queue, and will invoke the
     * completionHandler on the Matter queue as well.
     */
    CHIP_ERROR connect(chip::Controller::DeviceController * controller, chip::NodeId deviceID)
    {
        return controller->GetConnectedDevice(deviceID, &mOnConnected, &mOnConnectFailed);
    }

private:
    MTRInternalDeviceConnectionCallback mCompletionHandler;
    chip::Callback::Callback<chip::OnDeviceConnected> mOnConnected;
    chip::Callback::Callback<chip::OperationalSessionSetup::OnSetupFailure> mOnConnectFailed;

    static void OnConnected(
        void * context, chip::Messaging::ExchangeManager & exchangeMgr, const chip::SessionHandle & sessionHandle);
    static void OnConnectionFailure(void * context, const chip::OperationalSessionSetup::ConnectionFailureInfo & failureInfo);
};

NS_ASSUME_NONNULL_END
