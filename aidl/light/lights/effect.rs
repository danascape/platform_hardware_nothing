/*
 * SPDX-FileCopyrightText: Paranoid Android
 * SPDX-License-Identifier: Apache-2.0
 */

//! Turning an effect's control points into a curve.

use android_hardware_light::aidl::android::hardware::light::{
    HwLightEffect::HwLightEffect, InterpolationType::InterpolationType,
};

use super::color;
use crate::player::Curve;

/// Joins an effect's control points into a curve, or `None` if they are not valid.
///
/// `frames[i]` is how many frames to spend reaching `colors[i]`. LINEAR fills
/// the gaps between them.
pub fn expand(effect: &HwLightEffect, max_brightness: u32) -> Option<Curve> {
    let points = effect
        .frames
        .iter()
        .zip(&effect.colors)
        .map(|(&frames, &color)| (frames, color::level(color, max_brightness)));

    Curve::new(points, matches!(effect.interpolationType, InterpolationType::LINEAR))
}
