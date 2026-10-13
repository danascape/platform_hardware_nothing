/*
 * SPDX-FileCopyrightText: Paranoid Android
 * SPDX-License-Identifier: Apache-2.0
 */

package com.nothing.onlineconfig;

import android.content.ContentResolver;
import android.content.Context;
import android.database.ContentObserver;
import android.os.Handler;

import org.json.JSONArray;

public class ConfigObserver extends ContentObserver {

    public interface ConfigUpdater {
        void updateConfig(JSONArray config);
    }

    private final ConfigGrabber mConfigGrabber;
    private final ConfigUpdater mConfigUpdater;
    private final ContentResolver mResolver;

    public ConfigObserver(Context context, Handler handler, ConfigUpdater configUpdater,
            String projectName) {
        super(handler);
        mConfigGrabber = new ConfigGrabber(context, projectName);
        mConfigUpdater = configUpdater;
        mResolver = context.getContentResolver();
    }

    @Override
    public void onChange(boolean selfChange) {
        mConfigUpdater.updateConfig(mConfigGrabber.grabConfig());
    }

    public void register() {
        // There is no online config provider to observe
    }

    public void unregister() {
        mResolver.unregisterContentObserver(this);
    }
}
