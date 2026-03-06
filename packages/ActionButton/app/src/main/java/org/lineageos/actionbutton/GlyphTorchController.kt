package org.lineageos.actionbutton

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.IBinder
import android.service.quicksettings.TileService
import android.util.Log
import com.nothing.thirdparty.IGlyphService

class GlyphTorchController(private val context: Context) : AutoCloseable {
    private var glyphService: IGlyphService? = null
    private var isGlyphOn = false
    private var pendingToggle = false

    private val connection =
        object : ServiceConnection {
            override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
                glyphService = IGlyphService.Stub.asInterface(service)
                Log.d(TAG, "Glyph service connected")
                if (pendingToggle) {
                    pendingToggle = false
                    toggle()
                }
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
        if (glyphService == null) {
            pendingToggle = !pendingToggle
            Log.w(TAG, "Glyph service not connected yet, pendingToggle=$pendingToggle")
            return
        }
        isGlyphOn = !isGlyphOn
        try {
            glyphService?.setGlyphTorch(isGlyphOn)
            Log.d(TAG, "Glyph torch toggled: $isGlyphOn")
            TileService.requestListeningState(
                context,
                ComponentName(PARANOID_GLYPH_PACKAGE, TORCH_TILE_CLASS),
            )
        } catch (e: Exception) {
            Log.e(TAG, "Failed to toggle glyph torch", e)
            isGlyphOn = !isGlyphOn
        }
    }

    override fun close() {
        if (isGlyphOn) {
            // ensure LEDs are off before unbinding
            try {
                glyphService?.setGlyphTorch(false)
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
        private const val PARANOID_GLYPH_PACKAGE = "co.aospa.glyph"
        private const val TORCH_TILE_CLASS = "co.aospa.glyph.tiles.TorchTileService"
    }
}
