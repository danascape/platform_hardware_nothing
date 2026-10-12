/*
 * SPDX-FileCopyrightText: Paranoid Android
 * SPDX-License-Identifier: Apache-2.0
 */

//! The lights interface over the glyph strips.

mod catalog;
mod color;
mod effect;

use std::sync::Arc;
use std::time::Duration;

use android_hardware_light::aidl::android::hardware::light::{
    HwLight::HwLight, HwLightEffect::HwLightEffect, HwLightState::HwLightState, ILights::ILights,
};
use binder::{ExceptionCode, Interface, Status};
use log::{error, warn};

use crate::device::Device;
use crate::indicator::Indicator;
use crate::player::{Animation, Player, Track};
use catalog::{Kind, Target};

pub struct GlyphLights {
    device: &'static Device,
    player: Player,
    indicator: Indicator,
}

impl Interface for GlyphLights {}

impl GlyphLights {
    pub fn new(device: &'static Device) -> Self {
        GlyphLights { device, player: Player::new(device), indicator: Indicator::new(device) }
    }
}

impl ILights for GlyphLights {
    fn setLightState(&self, id: i32, state: &HwLightState) -> binder::Result<()> {
        let Some(target) = catalog::resolve(self.device, id) else {
            return Err(Status::new_exception(ExceptionCode::UNSUPPORTED_OPERATION, None));
        };
        let max_brightness = self.device.max_brightness;
        let level = color::level(state.color, max_brightness);

        match target {
            Target::Single(Kind::All) => self.player.set_all(level).map_err(|e| {
                error!("Failed to set light {id}: {e}");
                Status::new_exception(ExceptionCode::ILLEGAL_STATE, None)
            })?,
            Target::Segments(segments) => self.player.set_segments(segments, level),
            Target::Single(Kind::Video) => {
                let Some(lit) = self.device.camera.video(state.color) else {
                    warn!("Ignoring video command {}", state.color);
                    return Ok(());
                };

                self.indicator.set(lit);
            }
            Target::Single(Kind::Camera) => {
                let Some(fill) = self.device.camera.fill(state.color) else {
                    warn!("Ignoring camera command {}", state.color);
                    return Ok(());
                };

                let level = if fill.lit { max_brightness } else { 0 };
                if fill.camera_only {
                    self.player.set_segments(self.device.layout.camera, level);
                } else {
                    self.player.set_all(level).map_err(|e| {
                        error!("Failed to set the camera fill: {e}");
                        Status::new_exception(ExceptionCode::ILLEGAL_STATE, None)
                    })?;
                }
            }
            Target::Single(_) => {
                return Err(Status::new_exception(ExceptionCode::UNSUPPORTED_OPERATION, None))
            }
        }
        Ok(())
    }

    fn setLightEffects(&self, effects: &[HwLightEffect]) -> binder::Result<()> {
        if effects.is_empty() {
            self.player.stop();
            return Ok(());
        }

        let mut tracks = Vec::new();
        let mut frame_period = self.device.controller.min_update_period_millis;
        let mut iterations = 1;

        for effect in effects {
            // The all-segments node takes a level, not a series.
            let Some(Target::Segments(segments)) = catalog::resolve(self.device, effect.lightId)
            else {
                warn!("Ignoring effect for light {}", effect.lightId);
                continue;
            };
            if effect.colors.len() != effect.frames.len() {
                return Err(Status::new_exception(ExceptionCode::ILLEGAL_ARGUMENT, None));
            }
            let Some(curve) = effect::expand(effect, self.device.max_brightness) else {
                return Err(Status::new_exception(ExceptionCode::ILLEGAL_ARGUMENT, None));
            };

            // One clock for every track, so the slowest sets the pace.
            frame_period = frame_period.max(effect.framePeriodMillis);
            iterations = effect.iterations;

            let curve = Arc::new(curve);
            tracks.extend(
                segments.iter().map(|&segment| Track { segment, curve: Arc::clone(&curve) }),
            );
        }

        if tracks.is_empty() {
            return Err(Status::new_exception(ExceptionCode::UNSUPPORTED_OPERATION, None));
        }

        self.player.play(Animation {
            tracks,
            frame_period: Duration::from_millis(frame_period.max(1) as u64),
            iterations,
        });
        Ok(())
    }

    fn getLights(&self) -> binder::Result<Vec<HwLight>> {
        let mut lights = Vec::new();

        // LightsManager only sees the non-system types.
        for kind in catalog::KINDS {
            for index in 0..kind.count(self.device) {
                lights.push(HwLight {
                    id: kind.first_id() + index as i32,
                    ordinal: lights.len() as i32,
                    r#type: kind.light_type(),
                    minUpdatePeriodMillis: kind.min_update_period_ms(self.device.controller),
                });
            }
        }

        Ok(lights)
    }
}
