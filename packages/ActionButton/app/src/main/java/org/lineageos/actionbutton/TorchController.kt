package org.lineageos.actionbutton

import android.content.Context
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import android.os.Handler
import android.os.Looper
import android.util.Log

class TorchController(context: Context) : AutoCloseable {
    private val cameraManager = context.getSystemService(CameraManager::class.java)
    private val cameraId: String? =
        cameraManager.cameraIdList.firstOrNull { id ->
            cameraManager
                .getCameraCharacteristics(id)
                .get(CameraCharacteristics.FLASH_INFO_AVAILABLE) == true
        }
    private var isTorchOn = false

    private val torchCallback =
        object : CameraManager.TorchCallback() {
            override fun onTorchModeChanged(id: String, enabled: Boolean) {
                if (id == cameraId) isTorchOn = enabled
            }
        }

    init {
        cameraManager.registerTorchCallback(torchCallback, Handler(Looper.getMainLooper()))
        Log.d(TAG, "TorchController initialized, cameraId=$cameraId")
    }

    fun toggle() {
        val id =
            cameraId
                ?: run {
                    Log.w(TAG, "No flash-capable camera found")
                    return
                }
        val next = !isTorchOn
        try {
            cameraManager.setTorchMode(id, next)
            Log.d(TAG, "Torch toggled: $next")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to set torch mode", e)
        }
    }

    override fun close() {
        cameraManager.unregisterTorchCallback(torchCallback)
    }

    companion object {
        private const val TAG = "ActionButton"
    }
}
