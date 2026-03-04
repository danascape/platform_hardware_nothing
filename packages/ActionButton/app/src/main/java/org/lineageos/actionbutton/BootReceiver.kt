package org.lineageos.actionbutton

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.util.Log

class BootReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        if (intent.action != Intent.ACTION_LOCKED_BOOT_COMPLETED) return
        Log.d(TAG, "Boot completed, starting ActionButtonService")
        context.startService(Intent(context, ActionButtonService::class.java))
    }

    companion object {
        private const val TAG = "ActionButton"
    }
}
