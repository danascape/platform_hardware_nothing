/*
 * Copyright (C) 2025-2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "UdfpsHandler.nothing_asteroids"

#include "UdfpsHandler.h"

#include <android-base/file.h>
#include <android-base/logging.h>

#define UI_STATUS_PATH "/sys/panel_feature/ui_status"

static void setUiStatus(bool status) {
    android::base::WriteStringToFile(status ? "1" : "0", UI_STATUS_PATH);
}

#ifdef TARGET_USES_LHBM
#define PANEL_LHBM_PATH "/proc/touchpanel/fod_mode"

enum lhbm_fod_modes {
    FOD_OFF,
    FOD_ENABLE,
    FOD_PRESSED,
    FOD_FINISHED,
};

static void setFOD(int val) {
    if (val > FOD_FINISHED || val < FOD_OFF) return;
    if (!android::base::WriteStringToFile(std::to_string(val), PANEL_LHBM_PATH)) {
        PLOG(WARNING) << "setFOD: failed to write " << val;
        return;
    }
    LOG(DEBUG) << "set fod_mode: " << val;
}
#endif

class AsteroidsUdfpsHandler : public UdfpsHandler {
  public:
    void onFingerDown(uint32_t /*x*/, uint32_t /*y*/, float /*minor*/, float /*major*/) override {
#ifdef TARGET_USES_LHBM
        setFOD(FOD_ENABLE);
#else
        setUiStatus(true);
#endif
    }

    void onFingerUp() override {
#ifndef TARGET_USES_LHBM
        setUiStatus(false);
#endif
    }

    void onUiReady() override {
#ifndef TARGET_USES_LHBM
        setUiStatus(true);
#endif
    }

    void cancel() override {
#ifdef TARGET_USES_LHBM
        setFOD(FOD_FINISHED);
#else
        setUiStatus(false);
#endif
    }

#ifdef TARGET_USES_LHBM
    void onEnumerate() override { setFOD(FOD_OFF); }

    void setIgnoreDisplayTouches(bool shouldIgnore) override {
        setFOD(shouldIgnore ? FOD_FINISHED : FOD_ENABLE);
    }

    void onAuthenticationSucceeded() override { setFOD(FOD_FINISHED); }

    void onAuthenticationFailed() override { setFOD(FOD_FINISHED); }

    void onSessionClosed() override { setFOD(FOD_FINISHED); }
#endif
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
