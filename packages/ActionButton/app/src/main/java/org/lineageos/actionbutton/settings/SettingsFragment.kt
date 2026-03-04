package org.lineageos.actionbutton.settings

import android.os.Bundle
import android.provider.Settings
import com.android.settingslib.widget.SelectorWithWidgetPreference
import com.android.settingslib.widget.SettingsBasePreferenceFragment
import org.lineageos.actionbutton.Actions
import org.lineageos.actionbutton.R

class SettingsFragment :
    SettingsBasePreferenceFragment(), SelectorWithWidgetPreference.OnClickListener {

    private val prefs = mutableMapOf<String, SelectorWithWidgetPreference>()

    override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        addPreferencesFromResource(R.xml.action_button_settings)
        Actions.PREF_MAP.keys.forEach { key ->
            prefs[key] =
                findPreference<SelectorWithWidgetPreference>(key)!!.also {
                    it.setOnClickListener(this)
                }
        }
        updateCheckedState()
    }

    override fun onRadioButtonClicked(emitter: SelectorWithWidgetPreference) {
        val action = Actions.PREF_MAP[emitter.key] ?: return
        Settings.System.putString(requireContext().contentResolver, Actions.KEY, action)
        updateCheckedState()
    }

    private fun updateCheckedState() {
        val current =
            Settings.System.getString(requireContext().contentResolver, Actions.KEY)
                ?: Actions.DEFAULT
        Actions.PREF_MAP.forEach { (key, action) -> prefs[key]?.isChecked = current == action }
    }
}
