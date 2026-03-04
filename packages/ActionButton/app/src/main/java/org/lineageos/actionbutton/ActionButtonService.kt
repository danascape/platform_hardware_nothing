package org.lineageos.actionbutton

import android.app.Service
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.IBinder
import android.provider.Settings
import android.util.Log

class ActionButtonService : Service() {
    private val torchControllerDelegate = lazy { TorchController(this) }
    private val torchController by torchControllerDelegate

    private val glyphTorchControllerDelegate = lazy { GlyphTorchController(this) }
    private val glyphTorchController by glyphTorchControllerDelegate

    private val receiver =
        object : BroadcastReceiver() {
            override fun onReceive(context: Context, intent: Intent) {
                if (intent.action != KeyHandler.ACTION_KEY_PRESS) return
                val action =
                    Settings.System.getString(contentResolver, Actions.KEY) ?: Actions.DEFAULT
                Log.d(TAG, "Key press received, action=$action")
                when (action) {
                    Actions.GLYPH_TORCH -> glyphTorchController.toggle()
                    else -> torchController.toggle()
                }
            }
        }

    override fun onCreate() {
        super.onCreate()
        registerReceiver(receiver, IntentFilter(KeyHandler.ACTION_KEY_PRESS), RECEIVER_NOT_EXPORTED)
        Log.d(TAG, "ActionButtonService started")
    }

    override fun onDestroy() {
        unregisterReceiver(receiver)
        if (torchControllerDelegate.isInitialized()) torchController.close()
        if (glyphTorchControllerDelegate.isInitialized()) glyphTorchController.close()
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    companion object {
        private const val TAG = "ActionButton"
    }
}
