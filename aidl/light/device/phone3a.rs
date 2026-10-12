/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

//! Phone (3a) and Phone (3a) Pro.

use crate::controller::AW20036;

use super::Group::Each;
use super::{CameraCodes, Device, Layout, ShellLevel};

pub static DEVICE: Device<ShellLevel> = Device {
    controller: &AW20036,
    dir: "/sys/class/leds/aw20036_led",
    segments: 36,
    max_brightness: ShellLevel { black: 255, white: 255 },
    indicator: None,
    layout: Layout {
        camera: &[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19],
        glyphs: Each(&[
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
            24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
        ]),
        volume: Each(&[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]),
        battery: Each(&[30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20]),
    },
    camera: CameraCodes::DEFAULT,
};
