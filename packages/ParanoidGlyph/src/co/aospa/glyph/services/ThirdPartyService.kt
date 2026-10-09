/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-FileCopyrightText: Paranoid Android
 * SPDX-License-Identifier: Apache-2.0
 */

package co.aospa.glyph.services

import android.app.Service
import android.content.Intent
import android.os.IBinder
import co.aospa.glyph.system.GlyphStream
import co.aospa.glyph.util.GlyphLog
import co.aospa.glyph.util.glyph
import com.nothing.thirdparty.IGlyphService

/**
 * Lets apps drive the strips through the glyph sdk, for as long as one is bound through the adapter
 * and the glyphs are enabled.
 */
class ThirdPartyService : Service() {
    companion object {
        private const val TAG = "GlyphThirdPartyService"
    }

    private val service =
        object : IGlyphService.Stub() {
            override fun setFrameColors(levels: IntArray?) {
                if (!glyph.preferences.isEnabled) return
                GlyphLog.d(TAG) { "Frame of ${levels.contentToString()}" }
                levels?.let { frame ->
                    glyph.animator.setFrame(
                        GlyphStream.CLIENT,
                        frame.map(Int::toFloat).toFloatArray(),
                    )
                }
            }

            override fun openSession() {
                GlyphLog.d(TAG) { "Session opened" }
            }

            override fun closeSession() {
                GlyphLog.d(TAG) { "Session closed" }
                glyph.animator.endFrames(GlyphStream.CLIENT)
            }

            override fun register(name: String) = true

            override fun registerSDK(name: String, key: String) = true

            // The matrix is the dot display of a later phone, which this is not.
            override fun registerMatrixSDK(name: String) = false

            override fun setMatrixColors(levels: IntArray?) {}

            override fun setGlyphMatrixTimeout(active: Boolean) {}

            override fun setAppMatrixColors(levels: IntArray?) {}

            override fun closeAppMatrix() {}
        }

    override fun onBind(intent: Intent?): IBinder = service

    // The adapter, and every client behind it, is gone.
    override fun onUnbind(intent: Intent?): Boolean {
        glyph.animator.endFrames(GlyphStream.CLIENT)
        return super.onUnbind(intent)
    }
}
