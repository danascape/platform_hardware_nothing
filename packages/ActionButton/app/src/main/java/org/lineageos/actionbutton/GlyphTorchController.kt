package org.lineageos.actionbutton

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.IBinder
import android.util.Log
import com.nothing.thirdparty.IGlyphService

class GlyphTorchController(private val context: Context) : AutoCloseable {
    private var glyphService: IGlyphService? = null
    private var isGlyphOn = false

    private val connection =
        object : ServiceConnection {
            override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
                glyphService = IGlyphService.Stub.asInterface(service)
                Log.d(TAG, "Glyph service connected")
            }

            override fun onServiceDisconnected(name: ComponentName?) {
                glyphService = null
                Log.w(TAG, "Glyph service disconnected")
            }
        }

    init {
        val intent =
            Intent(GLYPH_SERVICE_ACTION).apply {
                component = ComponentName(GLYPH_PACKAGE, GLYPH_SERVICE_CLASS)
            }
        context.bindService(intent, connection, Context.BIND_AUTO_CREATE)
    }

    fun toggle() {
        isGlyphOn = !isGlyphOn
        val frame = IntArray(FRAME_LENGTH) { if (isGlyphOn) MAX_BRIGHTNESS else 0 }
        try {
            glyphService?.setFrameColors(frame) ?: Log.w(TAG, "Glyph service not connected yet")
            Log.d(TAG, "Glyph torch toggled: $isGlyphOn")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to set glyph frame colors", e)
            isGlyphOn = !isGlyphOn
        }
    }

    override fun close() {
        if (isGlyphOn) {
            // ensure LEDs are off before unbinding
            try {
                glyphService?.setFrameColors(IntArray(FRAME_LENGTH))
            } catch (_: Exception) {}
        }
        context.unbindService(connection)
        glyphService = null
    }

    companion object {
        private const val TAG = "ActionButton"
        private const val GLYPH_PACKAGE = "com.nothing.thirdparty"
        private const val GLYPH_SERVICE_ACTION = "com.nothing.thirdparty.IGlyphService"
        private const val GLYPH_SERVICE_CLASS = "com.nothing.thirdparty.GlyphService"
        private const val FRAME_LENGTH = 36 // phone3a: 36 LED channels
        private const val MAX_BRIGHTNESS = 255
    }
}
