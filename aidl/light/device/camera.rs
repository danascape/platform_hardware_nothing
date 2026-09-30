/*
 * SPDX-FileCopyrightText: Paranoid Android
 * SPDX-License-Identifier: Apache-2.0
 */

//! What the camera app sends to its fill light, which a device may change.

/// What a fill command does to the strips.
#[derive(Clone, Copy)]
pub struct Fill {
    /// Whether the strips light or go dark.
    pub lit: bool,

    /// Whether only the camera glyph is drawn, rather than every segment.
    pub camera_only: bool,
}

/// The value the camera app sends for each command.
#[derive(Clone, Copy)]
pub struct CameraCodes {
    pub video_on: i32,
    pub video_off: i32,
    pub fill_on: i32,
    pub fill_off: i32,
    pub countdown_on: i32,
    pub countdown_off: i32,

    /// Sent when the camera session ends.
    pub session_cleared: i32,
}

impl CameraCodes {
    /// What the camera app sends unless a device says otherwise.
    pub const DEFAULT: Self = Self {
        video_on: 1,
        video_off: 0,
        fill_on: 11,
        fill_off: 10,
        countdown_on: 7,
        countdown_off: 6,
        session_cleared: 0,
    };

    /// Returns whether a video command lights the indicator, or `None` if it is not one.
    pub fn video(&self, command: i32) -> Option<bool> {
        [(self.video_on, true), (self.video_off, false)]
            .into_iter()
            .find_map(|(code, lit)| (code == command).then_some(lit))
    }

    /// Returns what a fill command does, or `None` if it is not one.
    pub fn fill(&self, command: i32) -> Option<Fill> {
        [
            (self.fill_on, Fill { lit: true, camera_only: false }),
            (self.fill_off, Fill { lit: false, camera_only: false }),
            (self.countdown_on, Fill { lit: true, camera_only: true }),
            (self.countdown_off, Fill { lit: false, camera_only: true }),
            (self.session_cleared, Fill { lit: false, camera_only: false }),
        ]
        .into_iter()
        .find_map(|(code, fill)| (code == command).then_some(fill))
    }
}
