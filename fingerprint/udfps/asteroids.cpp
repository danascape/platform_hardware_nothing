/*
 * Copyright (C) 2025-2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "UdfpsHandler.nothing_asteroids"

#include "UdfpsHandler.h"

#include <android-base/file.h>

#define UI_STATUS_PATH "/sys/panel_feature/ui_status"

static void setUiStatus(bool status) {
    android::base::WriteStringToFile(status ? "1" : "0", UI_STATUS_PATH);
}

class AsteroidsUdfpsHandler : public UdfpsHandler {
  public:
    void onFingerDown(uint32_t /*x*/, uint32_t /*y*/, float /*minor*/, float /*major*/) override {
        setUiStatus(true);
    }

    void onFingerUp() override { setUiStatus(false); }

    void onUiReady() override { setUiStatus(true); }

    void cancel() override { setUiStatus(false); }
};

static UdfpsHandler* create() {
    return new AsteroidsUdfpsHandler();
}

static void destroy(UdfpsHandler* handler) {
    delete handler;
}

extern "C" UdfpsHandlerFactory UDFPS_HANDLER_FACTORY = {
        .create = create,
        .destroy = destroy,
};
