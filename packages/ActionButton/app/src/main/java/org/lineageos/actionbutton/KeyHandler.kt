package org.lineageos.actionbutton

import android.content.Context
import android.content.Intent
import android.util.Log
import android.view.KeyEvent
import com.android.internal.os.DeviceKeyHandler

class KeyHandler(private val context: Context) : DeviceKeyHandler {

    override fun handleKeyEvent(event: KeyEvent): KeyEvent? {
        if (event.keyCode != KeyEvent.KEYCODE_ASSIST) return event
        if (event.action != KeyEvent.ACTION_DOWN) return event
        if (event.repeatCount != 0) return event

        Log.d(TAG, "Essential Space key intercepted")
        context.sendBroadcast(Intent(ACTION_KEY_PRESS).apply { setPackage(PACKAGE) })
        return null
    }

    companion object {
        private const val TAG = "ActionButton"
        const val PACKAGE = "org.lineageos.actionbutton"
        const val ACTION_KEY_PRESS = "org.lineageos.actionbutton.ACTION_KEY_PRESS"
    }
}
