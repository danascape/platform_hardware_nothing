/*
 * SPDX-FileCopyrightText: Paranoid Android
 * SPDX-License-Identifier: Apache-2.0
 */

//! Drawing the strips: one animation at a time, and the levels between them.

mod animation;
mod curve;

pub use animation::{Animation, Track};
pub use curve::Curve;

use std::io::{self, Result};
use std::sync::mpsc::{self, Receiver, RecvTimeoutError, Sender};
use std::sync::{Mutex, PoisonError};
use std::thread;
use std::time::Instant;

use log::error;

use crate::device::{Device, Segments};

/// Plays one animation at a time, dropping whatever was playing before.
pub struct Player {
    commands: Mutex<Sender<Command>>,
}

enum Command {
    SetSegments { segments: Segments, level: u32 },
    SetAll { level: u32, reply: Sender<Result<()>> },
    Stop,
    Play(Animation),
}

impl Player {
    pub fn new(device: &'static Device) -> Self {
        let (commands, requests) = mpsc::channel();
        let worker = Worker {
            device,
            requests,
            resting: vec![0; device.segments],
            playback: None,
            redraw: None,
        };
        thread::spawn(move || worker.run());

        Player { commands: Mutex::new(commands) }
    }

    /// Sets some segments. The draw waits briefly for the rest of the frame.
    pub fn set_segments(&self, segments: Segments, level: u32) {
        self.send(Command::SetSegments { segments, level });
    }

    /// Sets every segment at once.
    pub fn set_all(&self, level: u32) -> Result<()> {
        let (reply, result) = mpsc::channel();
        self.send(Command::SetAll { level, reply });
        result.recv().unwrap_or_else(|_| Err(io::ErrorKind::BrokenPipe.into()))
    }

    /// Abandons whatever is playing, and draws the resting levels.
    pub fn stop(&self) {
        self.send(Command::Stop);
    }

    /// Plays an animation, replacing any that is already running.
    pub fn play(&self, animation: Animation) {
        if animation.frames() > 0 {
            self.send(Command::Play(animation));
        }
    }

    fn send(&self, command: Command) {
        // The worker runs until the player is dropped, so a send only fails after that.
        let _ = self.commands.lock().unwrap_or_else(PoisonError::into_inner).send(command);
    }
}

/// The thread that draws. Nothing else touches the strips.
struct Worker {
    device: &'static Device,
    requests: Receiver<Command>,

    /// Levels held between animations, so a plain setLightState still shows.
    resting: Vec<u32>,

    playback: Option<Playback>,

    /// When the resting levels are next drawn, if they have yet to reach the strips.
    redraw: Option<Instant>,
}

struct Playback {
    animation: Animation,

    /// The length the animation began with.
    length: u64,

    frame: u64,
    iteration: i32,

    /// When the next frame is drawn.
    due: Instant,
}

impl Playback {
    fn new(animation: Animation, due: Instant) -> Playback {
        Playback { length: animation.frames(), animation, frame: 0, iteration: 0, due }
    }
}

impl Worker {
    fn run(mut self) {
        loop {
            // Commands go first, so a late frame cannot hold one off.
            let received = match self.next_wake() {
                Some(wake) => {
                    self.requests.recv_timeout(wake.saturating_duration_since(Instant::now()))
                }
                None => self.requests.recv().map_err(|_| RecvTimeoutError::Disconnected),
            };

            match received {
                Ok(command) => self.handle(command),
                Err(RecvTimeoutError::Timeout) => self.tick(Instant::now()),
                Err(RecvTimeoutError::Disconnected) => return,
            }
        }
    }

    fn next_wake(&self) -> Option<Instant> {
        self.playback.as_ref().map(|playback| playback.due).or(self.redraw)
    }

    fn handle(&mut self, command: Command) {
        let now = Instant::now();
        match command {
            Command::SetSegments { segments, level } => {
                self.release(segments);
                for &segment in segments {
                    if let Some(resting) = self.resting.get_mut(segment) {
                        *resting = level;
                    }
                }

                // A frame arrives as one call per light, so wait for the rest of it.
                let window = self.device.controller.coalesce_window;
                self.redraw.get_or_insert(now + window);
            }
            Command::SetAll { level, reply } => {
                self.playback = None;
                self.redraw = None;
                self.resting.fill(level);
                let _ = reply.send(self.device.write_all(level));
            }
            Command::Stop => {
                self.playback = None;
                self.redraw = Some(now);
            }
            Command::Play(animation) => {
                self.redraw = None;
                self.playback = Some(Playback::new(animation, now));
            }
        }
    }

    /// Drops the tracks on these segments, and ends the animation if none is left.
    fn release(&mut self, segments: Segments) {
        let Some(playback) = self.playback.as_mut() else { return };

        playback.animation.tracks.retain(|track| !segments.contains(&track.segment));
        if playback.animation.tracks.is_empty() {
            self.playback = None;
        }
    }

    fn tick(&mut self, now: Instant) {
        if self.playback.is_some() {
            self.draw_frame(now);
        } else {
            self.draw_resting();
        }
    }

    fn draw_frame(&mut self, now: Instant) {
        let Some(playback) = self.playback.as_mut() else { return };
        let animation = &playback.animation;

        let levels = animation.frame(playback.frame, &self.resting);
        if let Err(e) = self.device.write_frame(&levels) {
            error!("Failed to draw the animation: {e}");
            return self.finish();
        }

        playback.frame += 1;
        if playback.frame == playback.length {
            playback.frame = 0;
            playback.iteration += 1;
            if animation.iterations != 0 && playback.iteration >= animation.iterations {
                return self.finish();
            }
        }

        // Counted from the last deadline, so frames do not drift.
        playback.due = (playback.due + animation.frame_period).max(now);
    }

    /// Ends the animation, and darkens what it drew. The rest keeps its levels.
    fn finish(&mut self) {
        if let Some(playback) = self.playback.take() {
            for track in &playback.animation.tracks {
                if let Some(resting) = self.resting.get_mut(track.segment) {
                    *resting = 0;
                }
            }
        }
        self.draw_resting();
    }

    fn draw_resting(&mut self) {
        self.redraw = None;
        if let Err(e) = self.device.write_frame(&self.resting) {
            error!("Failed to draw the strips: {e}");
        }
    }
}
