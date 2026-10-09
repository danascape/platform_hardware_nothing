/*
 * SPDX-FileCopyrightText: Paranoid Android
 * SPDX-License-Identifier: Apache-2.0
 */

package co.aospa.glyph.ui.preference

import android.content.Context
import android.graphics.drawable.Drawable
import android.graphics.drawable.GradientDrawable
import android.graphics.drawable.LayerDrawable
import android.util.AttributeSet
import androidx.core.graphics.ColorUtils
import androidx.preference.PreferenceViewHolder
import co.aospa.glyph.R
import com.android.settingslib.widget.SliderPreference
import com.google.android.material.slider.Slider
import kotlin.math.roundToInt

/** A slider whose thumb is an icon that shows how bright the step it is on is. */
class BrightnessSliderPreference(context: Context, attrs: AttributeSet?) :
    SliderPreference(context, attrs) {
    private companion object {
        /** The icons, dimmest first, which any number of steps is spread over. */
        private val ICONS =
            intArrayOf(
                R.drawable.ic_brightness_low_24dp,
                R.drawable.ic_brightness_medium_24dp,
                R.drawable.ic_brightness_full_24dp,
            )
    }

    override fun onBindViewHolder(holder: PreferenceViewHolder) {
        super.onBindViewHolder(holder)

        val slider = slider ?: return
        showStep(slider)
        slider.addOnChangeListener { _, _, _ -> showStep(slider) }
    }

    /** Puts the icon nearest to where the thumb is along the slider on the thumb. */
    private fun showStep(slider: Slider) {
        val range = slider.valueTo - slider.valueFrom
        val position = if (range > 0f) (slider.value - slider.valueFrom) / range else 1f
        slider.setCustomThumbDrawable(thumb(ICONS[(position * ICONS.lastIndex).roundToInt()]))
    }

    /** A round thumb with the icon on it. The slider does not hand it its state, so it is set. */
    private fun thumb(icon: Int): Drawable {
        val state = if (isEnabled) intArrayOf(android.R.attr.state_enabled) else intArrayOf()
        val size = context.resources.getDimensionPixelSize(R.dimen.brightness_thumb_size)
        val inset =
            (size - context.resources.getDimensionPixelSize(R.dimen.brightness_icon_size)) / 2

        // Over the card, so that the track does not show through a faded thumb.
        val container =
            ColorUtils.compositeColors(
                context.getColorStateList(R.color.brightness_thumb).getColorForState(state, 0),
                context.getColor(R.color.brightness_thumb_ground),
            )
        val circle =
            GradientDrawable().apply {
                shape = GradientDrawable.OVAL
                setColor(container)
                setSize(size, size)
            }
        val glyph =
            checkNotNull(context.getDrawable(icon)).mutate().apply {
                setTint(
                    context
                        .getColorStateList(R.color.brightness_thumb_icon)
                        .getColorForState(state, 0)
                )
            }

        return LayerDrawable(arrayOf(circle, glyph)).apply {
            setLayerInset(1, inset, inset, inset, inset)
        }
    }
}
