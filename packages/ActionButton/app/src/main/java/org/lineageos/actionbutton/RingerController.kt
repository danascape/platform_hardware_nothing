package org.lineageos.actionbutton

import android.content.Context
import android.media.AudioManager
import android.util.Log

class RingerController(context: Context) {
    private val audioManager = context.getSystemService(AudioManager::class.java)

    fun toggle() {
        val next =
            when (audioManager.ringerMode) {
                AudioManager.RINGER_MODE_NORMAL -> AudioManager.RINGER_MODE_VIBRATE
                AudioManager.RINGER_MODE_VIBRATE -> AudioManager.RINGER_MODE_SILENT
                else -> AudioManager.RINGER_MODE_NORMAL
            }
        try {
            audioManager.ringerMode = next
            Log.d(TAG, "Ringer mode set to: $next")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to set ringer mode", e)
        }
    }

    companion object {
        private const val TAG = "ActionButton"
    }
}
