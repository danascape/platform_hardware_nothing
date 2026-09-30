/*
 * SPDX-FileCopyrightText: Paranoid Android
 * SPDX-License-Identifier: Apache-2.0
 */

//! The phone this is built for, and how it uses its glyph controller.

mod camera;
mod layout;
mod shell;

// Selected by the nothing_glyph device soong config.
#[cfg_attr(phone1, path = "device/phone1.rs")]
#[cfg_attr(phone2, path = "device/phone2.rs")]
mod phone;

#[cfg(not(any(phone1, phone2)))]
compile_error!("Set the nothing_glyph device soong config to a phone this service supports");

pub use camera::CameraCodes;
pub use layout::{Group, Layout, Segments};
pub use phone::DEVICE;
pub use shell::{Ceiling, Shell};

use std::io::Result;

use crate::controller::{Controller, Node};

/// A phone: the controller it carries, and how it wires and uses it.
#[derive(Clone)]
pub struct Device {
    pub controller: &'static Controller,

    /// The directory the driver registered its nodes in.
    dir: &'static str,

    /// Levels one frame carries.
    pub segments: usize,

    /// Highest level the nodes accept.
    pub max_brightness: u32,

    /// The LED the recording indicator sits on, which no frame reaches.
    indicator: usize,

    /// The level the indicator lights at.
    pub indicator_level: u32,

    /// What the two above are held to on a white shell.
    white: Ceiling,

    pub layout: Layout,

    /// What the camera app sends.
    pub camera: CameraCodes,
}

impl Device {
    /// This phone fitted for its shell.
    pub fn for_shell(&self, shell: Shell) -> Device {
        let mut device = self.clone();
        if shell == Shell::White {
            device.max_brightness = self.white.max_brightness;
            device.indicator_level = self.white.indicator_level;
        }
        device
    }

    /// Draws one frame: every segment's level, in order.
    pub fn write_frame(&self, levels: &[u32]) -> Result<()> {
        let levels = levels.iter().map(|&level| self.clamp(level)).collect::<Vec<_>>();
        let value = levels.iter().map(u32::to_string).collect::<Vec<_>>().join(" ");
        self.controller.write(self.dir, Node::Frame, &value, levels.iter().any(|&level| level > 0))
    }

    /// Draws every segment at the same level, in one write.
    pub fn write_all(&self, level: u32) -> Result<()> {
        let level = self.clamp(level);
        self.controller.write(self.dir, Node::All, &level.to_string(), level > 0)
    }

    /// Lights the recording indicator, which no frame can reach.
    pub fn write_indicator(&self, level: u32) -> Result<()> {
        let level = level.min(self.indicator_level);
        let value = format!("{} {level}", self.indicator);
        self.controller.write(self.dir, Node::Single, &value, level > 0)
    }

    fn clamp(&self, level: u32) -> u32 {
        level.min(self.max_brightness)
    }
}
