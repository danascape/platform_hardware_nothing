package org.lineageos.actionbutton

object Actions {
    const val KEY = "action_button_action"

    const val TORCH = "torch"
    const val GLYPH_TORCH = "glyph_torch"
    const val RINGER = "ringer"

    const val DEFAULT = TORCH

    // To add a new action: add a constant above, add an entry here,
    // and add a matching <SelectorWithWidgetPreference> in action_button_settings.xml
    val PREF_MAP =
        linkedMapOf(
            "action_torch" to TORCH,
            "action_glyph_torch" to GLYPH_TORCH,
            "action_ringer" to RINGER,
        )
}
