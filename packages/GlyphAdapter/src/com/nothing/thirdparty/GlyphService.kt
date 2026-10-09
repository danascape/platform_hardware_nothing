/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-FileCopyrightText: Paranoid Android
 * SPDX-License-Identifier: Apache-2.0
 */

package com.nothing.thirdparty

import android.app.Service
import android.content.ComponentName
import android.content.Intent
import android.content.ServiceConnection
import android.os.IBinder
import android.os.RemoteException
import android.util.Log

/**
 * Serves the strips under the package name and permission the glyph sdk looks for, and hands
 * everything on to the glyph app, which owns them.
 */
class GlyphService : Service() {
    companion object {
        private const val TAG = "GlyphAdapterService"
        private const val DEBUG = true

        private val GLYPH_SERVICE =
            ComponentName("co.aospa.glyph", "co.aospa.glyph.services.ThirdPartyService")
    }

    /** The glyph app's service, or null while it is not bound. */
    private var glyph: IGlyphService? = null

    private val connection =
        object : ServiceConnection {
            override fun onServiceConnected(name: ComponentName, binder: IBinder) {
                if (DEBUG) Log.d(TAG, "Bound to $name")
                glyph = IGlyphService.Stub.asInterface(binder)
            }

            override fun onServiceDisconnected(name: ComponentName) {
                if (DEBUG) Log.d(TAG, "Lost $name")
                glyph = null
            }
        }

    private val service =
        object : IGlyphService.Stub() {
            override fun setFrameColors(levels: IntArray?) {
                glyph?.setFrameColors(levels)
            }

            override fun openSession() {
                glyph?.openSession()
            }

            override fun closeSession() {
                glyph?.closeSession()
            }

            override fun register(name: String) = true

            override fun registerSDK(name: String, key: String) = true

            override fun registerMatrixSDK(name: String) = glyph?.registerMatrixSDK(name) ?: false

            override fun setMatrixColors(levels: IntArray?) {
                glyph?.setMatrixColors(levels)
            }

            override fun setGlyphMatrixTimeout(active: Boolean) {
                glyph?.setGlyphMatrixTimeout(active)
            }

            override fun setAppMatrixColors(levels: IntArray?) {
                glyph?.setAppMatrixColors(levels)
            }

            override fun closeAppMatrix() {
                glyph?.closeAppMatrix()
            }
        }

    override fun onCreate() {
        super.onCreate()
        bindService(Intent().setComponent(GLYPH_SERVICE), connection, BIND_AUTO_CREATE)
    }

    override fun onDestroy() {
        unbindService(connection)
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder = service

    // The last client is gone, and one that dies never closes its session.
    override fun onUnbind(intent: Intent?): Boolean {
        try {
            glyph?.closeSession()
        } catch (e: RemoteException) {
            Log.w(TAG, "Could not close the client session", e)
        }
        return super.onUnbind(intent)
    }
}
