package org.lineageos.actionbutton.settings

import android.os.Bundle
import com.android.settingslib.collapsingtoolbar.CollapsingToolbarBaseActivity
import com.android.settingslib.collapsingtoolbar.R as CollapsingR

class SettingsActivity : CollapsingToolbarBaseActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (supportFragmentManager.findFragmentById(CollapsingR.id.content_frame) == null) {
            supportFragmentManager
                .beginTransaction()
                .add(CollapsingR.id.content_frame, SettingsFragment())
                .commit()
        }
    }
}
