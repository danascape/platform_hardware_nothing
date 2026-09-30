/*
 * SPDX-FileCopyrightText: Paranoid Android
 * SPDX-License-Identifier: Apache-2.0
 */

//! The recording indicator, on an LED of its own.

use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::Arc;
use std::thread;
use std::time::Duration;

use log::error;

use crate::device::Device;

/// Half a second lit, half a second dark.
const PHASE: Duration = Duration::from_millis(500);

/// How often a phase checks whether it has been called off.
const TICK: Duration = Duration::from_millis(20);

/// Blinks the indicator, or holds it dark.
pub struct Indicator {
    device: &'static Device,

    /// Bumped on every call; a running blink stops when it changes.
    generation: Arc<AtomicU64>,
}

impl Indicator {
    pub fn new(device: &'static Device) -> Self {
        Indicator { device, generation: Arc::new(AtomicU64::new(0)) }
    }

    /// Starts the blink, or stops it and clears the LED.
    pub fn set(&self, on: bool) {
        let mine = self.generation.fetch_add(1, Ordering::SeqCst) + 1;

        if !on {
            if let Err(e) = self.device.write_indicator(0) {
                error!("Failed to clear the indicator: {e}");
            }
            return;
        }

        let generation = Arc::clone(&self.generation);
        let device = self.device;

        thread::spawn(move || {
            while generation.load(Ordering::SeqCst) == mine {
                for level in [device.indicator_level, 0] {
                    if let Err(e) = device.write_indicator(level) {
                        error!("Failed to blink the indicator: {e}");
                        return;
                    }

                    let mut left = PHASE;
                    while !left.is_zero() {
                        if generation.load(Ordering::SeqCst) != mine {
                            return;
                        }

                        let step = TICK.min(left);
                        thread::sleep(step);
                        left -= step;
                    }
                }
            }
        });
    }
}
