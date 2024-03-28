/*
 * Copyright (C) 2022-2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "UdfpsHandler.nothing_Pong"

#include "UdfpsHandler.h"

#include <android-base/file.h>

#define FOD_HBM_PATH "/sys/devices/platform/soc/soc:qcom,dsi-display-primary/force_fod_ui"

static void setFodHbm(bool status) {
    android::base::WriteStringToFile(status ? "1" : "0", FOD_HBM_PATH);
}

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

    void onEnroll() override { setFodHbm(true); }

    void onAuthenticate() override { setFodHbm(true); }

    void onChallengeRevoked() override {
        setFodHbm(false);
        onFingerUp();
    }

    void cancel() override {
        setFodHbm(false);
        onFingerUp();
    }

    void onError(int32_t /*error*/, int32_t /*vendorCode*/) override {
        setFodHbm(false);
        onFingerUp();
    }

    void onAuthenticationSucceeded() override {
        setFodHbm(false);
        onFingerUp();
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
