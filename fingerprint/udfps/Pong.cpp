/*
 * Copyright (C) 2022-2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "UdfpsHandler.nothing_Pong"

#include "UdfpsHandler.h"

class PongUdfpsHandler : public UdfpsHandler {
  public:
    void init(fingerprint_device_t* device) override { mDevice = device; }

    void onFingerDown(uint32_t /*x*/, uint32_t /*y*/, float /*minor*/, float /*major*/) override {
        if (mDevice) {
            mDevice->goodixExtCmd(mDevice, 1, 0);
        }
    }

    void onFingerUp() override {
        if (mDevice) {
            mDevice->goodixExtCmd(mDevice, 0, 0);
        }
    }

  private:
    fingerprint_device_t* mDevice = nullptr;
};

static UdfpsHandler* create() {
    return new PongUdfpsHandler();
}

static void destroy(UdfpsHandler* handler) {
    delete handler;
}

extern "C" UdfpsHandlerFactory UDFPS_HANDLER_FACTORY = {
        .create = create,
        .destroy = destroy,
};
