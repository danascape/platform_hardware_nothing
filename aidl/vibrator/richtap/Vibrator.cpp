/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "vibrator.nothing-rt_ics"

#include "Vibrator.h"

#include <android-base/file.h>
#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <android-base/strings.h>
#include <json/reader.h>
#include <log/log.h>

#include <cstdlib>
#include <fstream>
#include <thread>

#include "aac_vibra_function.h"
#include "ics_haptic_function.h"

#define LK_F0_PATH "/sys/class/leds/vibrator_nt/lk_f0"
#define CALI_DATA_PATH "/mnt/vendor/persist/haptic/cali_data"
#define PROC_F0_PATH "/proc/haptic/f0"

#define LK_F0_TARGET 1700
#define LK_F0_TOLERANCE 150

#define EFFECTS_PATH "/vendor/etc/richtapresources/"

#define HE_VERSION 1
#define HE_EVENTS_MAX 16
#define HE_EVENT_INTS 17
#define HE_TYPE_CONTINUOUS 4096
#define HE_TYPE_TRANSIENT 4097
#define HE_EVENT_SPACING_MS 400

#define PERFORM_DURATION_MS 30
#define PERFORM_INTERVAL_MS 50
#define ENVELOPE_MIN_MS 55
#define ENVELOPE_RAMP_MS 5
#define ON_START_DELAY_MS 2

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

using ::android::base::GetProperty;
using ::android::base::ParseInt;
using ::android::base::ReadFileToString;
using ::android::base::Trim;
using ::android::base::WriteStringToFile;

static const std::map<Effect, const char*> kEffectFiles = {
        {Effect::CLICK, "click.he"},
        {Effect::DOUBLE_CLICK, "double_click.he"},
        {Effect::TICK, "tick.he"},
        {Effect::THUD, "thud.he"},
        {Effect::POP, "pop.he"},
        {Effect::HEAVY_CLICK, "heavy_click.he"},
        {Effect::TEXTURE_TICK, "texture_tick.he"},
};

static const std::map<EffectStrength, const char*> kStrengthDirs = {
        {EffectStrength::LIGHT, "weak/"},
        {EffectStrength::MEDIUM, "default/"},
        {EffectStrength::STRONG, "strong/"},
};

static bool inRange(int value, int min, int max) {
    return value >= min && value <= max;
}

static std::vector<int32_t> loadHe(const std::string& path) {
    std::ifstream file(path);
    Json::Value root;
    Json::CharReaderBuilder builder;
    if (!file || !Json::parseFromStream(builder, file, &root, nullptr)) {
        ALOGE("Failed to read %s", path.c_str());
        return {};
    }

    if (root["Metadata"]["Version"].asInt() != HE_VERSION) {
        return {};
    }

    const Json::Value& pattern = root["Pattern"];
    int count = std::min(static_cast<int>(pattern.size()), HE_EVENTS_MAX);
    std::vector<int32_t> he(count * HE_EVENT_INTS + 1);
    he[0] = HE_VERSION;

    for (int i = 0; i < count; i++) {
        const Json::Value& event = pattern[i]["Event"];
        std::string type = event["Type"].asString();
        int time = event.isMember("RelativeTime") ? event["RelativeTime"].asInt()
                                                  : i * HE_EVENT_SPACING_MS;
        int intensity = event["Parameters"]["Intensity"].asInt();
        int frequency = event["Parameters"]["Frequency"].asInt();

        if ((type != "continuous" && type != "transient") || !inRange(time, 0, 50000) ||
            !inRange(intensity, 0, 100) || !inRange(frequency, 0, 100)) {
            ALOGE("Invalid event %d in %s", i, path.c_str());
            return {};
        }

        int32_t* slot = &he[i * HE_EVENT_INTS + 1];
        slot[0] = type == "continuous" ? HE_TYPE_CONTINUOUS : HE_TYPE_TRANSIENT;
        slot[1] = time;
        slot[2] = intensity;
        slot[3] = frequency;

        // Only transient events are understood, so stop at the first continuous one.
        if (slot[0] == HE_TYPE_CONTINUOUS) {
            break;
        }
    }

    return he;
}

static std::vector<int32_t> transientFor(int32_t timeoutMs) {
    std::vector<int32_t> he(HE_EVENT_INTS + 1);
    int32_t intensity = 100;
    int32_t frequency;

    if (timeoutMs <= 15) {
        frequency = 95;
    } else if (timeoutMs <= 25) {
        frequency = 85;
    } else if (timeoutMs <= 35) {
        frequency = 75;
    } else if (timeoutMs <= 45) {
        intensity = 85;
        frequency = 65;
    } else {
        frequency = 55;
    }

    he[0] = HE_VERSION;
    he[1] = HE_TYPE_TRANSIENT;
    he[3] = intensity;
    he[4] = frequency;
    return he;
}

// The bootloader measures f0, keep it as the calibration while it is sane.
static void saveLkF0() {
    std::string value;
    if (!ReadFileToString(LK_F0_PATH, &value)) {
        ALOGE("Failed to read %s", LK_F0_PATH);
        return;
    }

    int f0 = atoi(value.c_str());
    if (std::abs(f0 - LK_F0_TARGET) > LK_F0_TOLERANCE) {
        ALOGE("lk f0 %d is out of range", f0);
        return;
    }

    std::string data = std::to_string(f0);
    data.resize(10, '\0');
    if (!WriteStringToFile(data, CALI_DATA_PATH)) {
        ALOGE("Failed to write %s", CALI_DATA_PATH);
    }
}

static void loadCaliF0() {
    std::string value;
    int f0 = 0;
    if (!ReadFileToString(CALI_DATA_PATH, &value)) {
        ALOGE("Failed to read %s", CALI_DATA_PATH);
        return;
    }

    if (ParseInt(Trim(value).c_str(), &f0) && f0 > 0 &&
        !WriteStringToFile(std::to_string(f0) + "\n", PROC_F0_PATH)) {
        ALOGE("Failed to write %s", PROC_F0_PATH);
    }
}

static void sfdcF0Callback(float f0) {
    ALOGD("sfdc f0 update: %.1f", f0);
}

Vibrator::Vibrator() {
    for (const auto& [strength, dir] : kStrengthDirs) {
        for (const auto& [effect, file] : kEffectFiles) {
            std::vector<int32_t> he = loadHe(std::string(EFFECTS_PATH) + dir + file);
            if (!he.empty()) {
                mEffects[{strength, effect}] = std::move(he);
            }
        }
    }
}

void Vibrator::init() {
    saveLkF0();
    loadCaliF0();

    uint32_t deviceType = 0;
    int32_t ret = aac_vibra_init(&deviceType);
    if (ret) {
        ALOGE("AAC init failed: %d", ret);
        return;
    }

    aac_vibra_looper_start();
    sfdc_initialize(sfdcF0Callback);
    mInited = true;

    ALOGI("AAC init success: %u", deviceType);
}

int32_t Vibrator::playHe(int32_t looper, int32_t interval, int32_t amplitude, int32_t freq,
                         const std::vector<int32_t>& he) {
    float f0;

    if (GetProperty("persist.vendor.sfdc", "") == "false") {
        f0 = sfdc_get_manufactory_f0();
    } else if (sfdc_calibrate(he.data(), he.size()) >= 0) {
        f0 = sfdc_get_transient_fc();
    } else {
        f0 = sfdc_get_continuous_f0();
    }

    aac_vibra_setting_f0(0, f0);
    return aac_vibra_looper_post(he.data(), he.size(), interval, looper, amplitude, freq);
}

void Vibrator::notifyAfter(int32_t delayMs, const std::shared_ptr<IVibratorCallback>& callback) {
    uint32_t generation = ++mGeneration;
    if (callback == nullptr) {
        return;
    }

    std::thread([this, delayMs, callback, generation] {
        usleep(delayMs * 1000);
        if (mGeneration == generation) {
            callback->onComplete();
        }
    }).detach();
}

ndk::ScopedAStatus Vibrator::getCapabilities(int32_t* _aidl_return) {
    *_aidl_return = IVibrator::CAP_ON_CALLBACK | IVibrator::CAP_PERFORM_CALLBACK |
                    IVibrator::CAP_AMPLITUDE_CONTROL;

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::off() {
    std::lock_guard lock(mLock);
    ++mGeneration;
    mPendingOnMs = 0;
    if (mInited) {
        aac_vibra_looper_stopPerformHe();
    }

    return ndk::ScopedAStatus::ok();
}

// The amplitude is baked into the stream when playback starts, so wait for the
// framework to set it, which it does right after turning the vibrator on.
void Vibrator::startOnLocked() {
    int32_t timeoutMs = mPendingOnMs;
    mPendingOnMs = 0;

    if (timeoutMs > ENVELOPE_MIN_MS) {
        // Relative time, scale and frequency of each point.
        const int32_t envelope[4][3] = {
                {0, 0, 0},
                {ENVELOPE_RAMP_MS, 100, 0},
                {timeoutMs - ENVELOPE_RAMP_MS, 100, 0},
                {timeoutMs, 0, 0},
        };
        aac_vibra_looper_envelope(&envelope[0][0], sizeof(envelope) / sizeof(int32_t), true);
    } else {
        playHe(1, 0, mAmplitude, 0, transientFor(timeoutMs));
    }
}

ndk::ScopedAStatus Vibrator::on(int32_t timeoutMs,
                                const std::shared_ptr<IVibratorCallback>& callback) {
    std::lock_guard lock(mLock);
    if (mInited) {
        aac_vibra_looper_stopPerformHe();
        mPendingOnMs = timeoutMs;

        uint32_t generation = ++mOnGeneration;
        std::thread([this, generation] {
            usleep(ON_START_DELAY_MS * 1000);
            std::lock_guard lock(mLock);
            if (mPendingOnMs > 0 && mOnGeneration == generation) {
                startOnLocked();
            }
        }).detach();
    }

    notifyAfter(timeoutMs, callback);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::perform(Effect effect, EffectStrength strength,
                                     const std::shared_ptr<IVibratorCallback>& callback,
                                     int32_t* _aidl_return) {
    auto he = mEffects.find({strength, effect});
    if (he == mEffects.end()) {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }

    std::lock_guard lock(mLock);
    mPendingOnMs = 0;

    // Drop effects that follow each other too closely, they would only blur together.
    auto now = std::chrono::steady_clock::now();
    if (now - mLastPerform >= std::chrono::milliseconds(PERFORM_INTERVAL_MS)) {
        mLastPerform = now;
        if (mInited) {
            aac_vibra_looper_stopPerformHe();
            playHe(1, 0, 0xff, 0, he->second);
        }
    }

    *_aidl_return = PERFORM_DURATION_MS;
    notifyAfter(PERFORM_DURATION_MS, callback);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getSupportedEffects(std::vector<Effect>* _aidl_return) {
    for (const auto& [effect, file] : kEffectFiles) {
        _aidl_return->push_back(effect);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setAmplitude(float amplitude) {
    std::lock_guard lock(mLock);
    mAmplitude = static_cast<uint8_t>(amplitude * 0xff);
    if (mInited) {
        aac_vibra_setAmplitude(mAmplitude);
        if (mPendingOnMs > 0) {
            startOnLocked();
        }
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setExternalControl(bool enabled __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getCompositionDelayMax(int32_t* maxDelayMs __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getCompositionSizeMax(int32_t* maxSize __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getSupportedPrimitives(
        std::vector<CompositePrimitive>* supported __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getPrimitiveDuration(CompositePrimitive primitive __unused,
                                                  int32_t* durationMs __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::compose(const std::vector<CompositeEffect>& composite __unused,
                                     const std::shared_ptr<IVibratorCallback>& callback __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getSupportedAlwaysOnEffects(
        std::vector<Effect>* _aidl_return __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::alwaysOnEnable(int32_t id __unused, Effect effect __unused,
                                            EffectStrength strength __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::alwaysOnDisable(int32_t id __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getResonantFrequency(float* resonantFreqHz __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getQFactor(float* qFactor __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getFrequencyResolution(float* freqResolutionHz __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getFrequencyMinimum(float* freqMinimumHz __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getBandwidthAmplitudeMap(std::vector<float>* _aidl_return __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getPwlePrimitiveDurationMax(int32_t* durationMs __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getPwleCompositionSizeMax(int32_t* maxSize __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getSupportedBraking(std::vector<Braking>* supported __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::composePwle(const std::vector<PrimitivePwle>& composite __unused,
                                         const std::shared_ptr<IVibratorCallback>& callback
                                                 __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
