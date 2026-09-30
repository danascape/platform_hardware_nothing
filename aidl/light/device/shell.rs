/*
 * SPDX-FileCopyrightText: Paranoid Android
 * SPDX-License-Identifier: Apache-2.0
 */

//! The color of a phone's shell, which sets how bright its strips may be.

use std::fs;

use log::warn;
use rustutils::android::system_properties;

/// How bright the strips may be.
#[derive(Clone, Copy)]
pub struct Ceiling {
    /// Highest level the nodes accept.
    pub max_brightness: u32,

    /// The level the recording indicator lights at.
    pub indicator_level: u32,
}

/// The color of a phone's shell. A white one diffuses more light, so its strips take less.
#[derive(Clone, Copy, PartialEq)]
pub enum Shell {
    Black,
    White,
}

impl Shell {
    /// The factory's record of the color.
    const RECORD: &'static str = "/mnt/vendor/persist/color";

    /// Where the color is published, for whatever reads it.
    const PROPERTY: &'static str = "persist.vendor.glyph.color";

    /// Reads the color, which is black where the record cannot be read.
    pub fn read() -> Shell {
        match fs::read_to_string(Self::RECORD) {
            Ok(color) => Self::parse(&color),
            Err(e) => {
                warn!("Failed to read the shell color: {e}");
                Shell::Black
            }
        }
    }

    /// Publishes the color, unless the property already says it.
    pub fn publish(self) {
        let value = self.name();
        let published = system_properties::read(Self::PROPERTY).unwrap_or_default();
        if published.as_deref() == Some(value) {
            return;
        }
        if let Err(e) = system_properties::write(Self::PROPERTY, value) {
            warn!("Failed to publish the shell color: {e}");
        }
    }

    fn name(self) -> &'static str {
        match self {
            Shell::Black => "black",
            Shell::White => "white",
        }
    }

    /// Only white is told apart; black and gray take the same levels.
    fn parse(color: &str) -> Shell {
        if color.to_ascii_lowercase().contains("white") {
            Shell::White
        } else {
            Shell::Black
        }
    }
}
