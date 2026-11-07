// Squelch — interface components (knobs, buttons, filter display, preset bar, output meter).
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Theme.h"
#include <complex>

namespace squelch::ui
{

constexpr float startAngle = juce::MathConstants<float>::pi * -0.75f;
constexpr float endAngle   = juce::MathConstants<float>::pi *  0.75f;
constexpr float shadowOff  = 3.5f;

// ============================================================ knob
class KnobSlider : public juce::Slider
{
public:
    KnobSlider (const Skin& s, bool isBig) : skin (s), big (isBig)
    {
        setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setRotaryParameters (startAngle + juce::MathConstants<float>::twoPi,
                             endAngle + juce::MathConstants<float>::twoPi, true);
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        setVelocityBasedMode (false);
    }

    void paint (juce::Graphics& g) override
    {
        const auto& t = skin.theme;
        auto area = getLocalBounds().toFloat();
        const float r = std::min (area.getWidth(), area.getHeight()) * 0.5f - shadowOff - 1.0f;
        const auto c = area.getCentre().translated (-shadowOff * 0.5f, -shadowOff * 0.5f);
        const float pos = (float) valueToProportionOfLength (getValue());
        const float angle = startAngle + pos * (endAngle - startAngle);

        auto circle = [&] (float rad) { return juce::Rectangle<float> (c.x - rad, c.y - rad, rad * 2, rad * 2); };

        g.setColour (t.shadow);
        g.fillEllipse (circle (r).translated (shadowOff, shadowOff));

        float knobR = r, dotDist, dotR;
        juce::Colour dotColour = t.knobDot;

        if (big)
        {
            g.setColour (t.ring);
            g.fillEllipse (circle (r));
            g.setColour (t.knobOutline.withMultipliedAlpha (0.9f));
            g.drawEllipse (circle (r).reduced (1.25f), 2.5f);
            knobR = r * 0.52f;
            g.setColour (t.shadow.withAlpha (0.6f));
            g.fillEllipse (circle (knobR).translated (2.0f, 2.0f));
            dotDist = knobR * 0.6f;
            dotR = knobR * 0.24f;
            dotColour = t.bigKnobDot;
        }
        else
        {
            dotDist = r * 0.68f;
            dotR = r * 0.17f;
        }

        g.setColour (t.knob);
        g.fillEllipse (circle (knobR));
        g.setColour (t.knobOutline);
        g.drawEllipse (circle (knobR).reduced (1.25f), 2.5f);

        const auto dot = c.getPointOnCircumference (dotDist, angle);
        g.setColour (dotColour);
        g.fillEllipse (dot.x - dotR, dot.y - dotR, dotR * 2, dotR * 2);
    }

private:
    const Skin& skin;
    bool big;
};

// Knob + label + value underneath, bound to one parameter.
class Knob : public juce::Component
{
public:
    Knob (const Skin& s, juce::RangedAudioParameter& p, const juce::String& labelText, bool isBig = false)
        : skin (s), param (p), label (labelText), slider (s, isBig),
          attachment (p, slider)
    {
        addAndMakeVisible (slider);
        slider.setDoubleClickReturnValue (true, p.convertFrom0to1 (p.getDefaultValue()));
        slider.onValueChange = [this] { repaint(); };
        slider.setTooltip (p.getName (64) + " (double-click to reset)");
    }

    // Lets the editor show a different value text or label (e.g. PW vs DETUNE).
    std::function<juce::String()> valueText;
    void setLabel (const juce::String& l) { if (l != label) { label = l; repaint(); } }

    int knobSize = 54;

    void resized() override
    {
        slider.setBounds (getLocalBounds().removeFromTop (knobSize + (int) shadowOff + 2)
                              .withSizeKeepingCentre (knobSize + (int) shadowOff + 2, knobSize + (int) shadowOff + 2));
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        area.removeFromTop ((float) knobSize + shadowOff + 6.0f);
        g.setColour (skin.theme.text);
        g.setFont (skin.font (15.0f));
        g.drawText (label, area.removeFromTop (18.0f), juce::Justification::centred, false);
        g.setColour (skin.theme.valueText);
        g.setFont (skin.font (12.5f, false));
        g.drawText (valueText ? valueText() : param.getCurrentValueAsText(), area.removeFromTop (18.0f),
                    juce::Justification::centred, false);
    }

private:
    const Skin& skin;
    juce::RangedAudioParameter& param;
    juce::String label;
    KnobSlider slider;
    juce::SliderParameterAttachment attachment;
};

// ============================================================ pill button
class PillButton : public juce::Button
{
public:
    PillButton (const Skin& s, const juce::String& buttonText) : juce::Button (buttonText), skin (s)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        const auto& t = skin.theme;
        const bool on = getToggleState();
        auto r = getLocalBounds().toFloat().withTrimmedRight (shadowOff).withTrimmedBottom (shadowOff);
        if (down) r = r.translated (1.5f, 1.5f);

        auto fill = on ? t.ring : t.button;
        if (over && ! on) fill = fill.interpolatedWith (t.ring, 0.15f);
        drawStickerBox (g, r, r.getHeight() * 0.5f, fill, on ? t.buttonOutline : t.buttonOutline,
                        2.5f, t.shadow, down ? 1.5f : shadowOff);

        g.setColour (on ? t.onRing : t.buttonText);
        g.setFont (skin.font (15.5f));
        g.drawText (getButtonText(), r, juce::Justification::centred, false);
    }

private:
    const Skin& skin;
};

// A row of pill buttons that together choose one value of a choice parameter.
class ChoiceGroup
{
public:
    ChoiceGroup (juce::RangedAudioParameter& p, std::vector<PillButton*> b)
        : buttons (std::move (b)),
          attachment (p, [this] (float v) { show ((int) std::round (v)); })
    {
        for (int i = 0; i < (int) buttons.size(); ++i)
            buttons[(size_t) i]->onClick = [this, i] { attachment.setValueAsCompleteGesture ((float) i); };
        attachment.sendInitialUpdate();
    }

    std::function<void (int)> onChange;

private:
    void show (int index)
    {
        for (int i = 0; i < (int) buttons.size(); ++i)
            buttons[(size_t) i]->setToggleState (i == index, juce::dontSendNotification);
        if (onChange) onChange (index);
    }

    std::vector<PillButton*> buttons;
    juce::ParameterAttachment attachment;
};

// ============================================================ on/off LED
class LedToggle : public juce::Component
{
public:
    LedToggle (const Skin& s, juce::RangedAudioParameter& p)
        : skin (s), attachment (p, [this] (float v) { on = v > 0.5f; repaint(); })
    {
        attachment.sendInitialUpdate();
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setTitle ("Drive on/off");
    }

    void paint (juce::Graphics& g) override
    {
        const auto& t = skin.theme;
        auto r = getLocalBounds().toFloat().reduced (2.0f);
        if (on)
        {
            g.setColour (t.accent.withAlpha (0.35f));
            g.fillEllipse (r.expanded (2.0f));
            g.setColour (t.accent);
            g.fillEllipse (r);
        }
        else
        {
            g.setColour (t.button);
            g.fillEllipse (r);
        }
        g.setColour (t.knobOutline);
        g.drawEllipse (r, 2.0f);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (getLocalBounds().contains (e.getPosition()))
            attachment.setValueAsCompleteGesture (on ? 0.0f : 1.0f);
    }

private:
    const Skin& skin;
    bool on = false;
    juce::ParameterAttachment attachment;
};

// ============================================================ filter display
class FilterDisplay : public juce::Component
{
public:
    explicit FilterDisplay (const Skin& s) : skin (s) {}

    std::function<float()> getCutoffHz, getResonance;

    void paint (juce::Graphics& g) override
    {
        const auto& t = skin.theme;
        auto box = getLocalBounds().toFloat().reduced (1.5f);
        g.setColour (t.display);
        g.fillRoundedRectangle (box, 12.0f);

        const float fc = getCutoffHz ? getCutoffHz() : 1000.0f;
        const float k = 3.95f * juce::jlimit (0.0f, 0.92f, getResonance ? getResonance() : 0.5f);

        // magnitude of a 4-pole ladder low-pass, same bass compensation as the engine
        auto magDb = [&] (float f)
        {
            const std::complex<double> s (0.0, (double) (f / fc));
            const auto den = std::pow (1.0 + s, 4) + (double) k;
            return 20.0f * std::log10 ((1.0f + 0.6f * k) / (float) std::abs (den) + 1.0e-9f);
        };

        auto inner = box.reduced (4.0f, 10.0f);
        auto xForF = [&] (float f) { return inner.getX() + inner.getWidth() * std::log (f / 20.0f) / std::log (1000.0f); };
        auto yForDb = [&] (float db) { return inner.getY() + inner.getHeight() * (20.0f - db) / 62.0f; };

        juce::Path curve;
        const int n = 220;
        for (int i = 0; i <= n; ++i)
        {
            const float f = 20.0f * std::pow (1000.0f, (float) i / (float) n);
            const float y = juce::jlimit (inner.getY(), box.getBottom() + 4.0f, yForDb (magDb (f)));
            const float x = xForF (f);
            if (i == 0) curve.startNewSubPath (x, y); else curve.lineTo (x, y);
        }

        g.saveState();
        g.reduceClipRegion (box.toNearestInt());
        g.setColour (t.accent);
        g.strokePath (curve, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // the mascot rides the curve, just left of the cutoff
        const float sf = fc / 4.0f;
        if (sf > 25.0f && sf < 18000.0f)
        {
            const float r = box.getHeight() * 0.3f;
            const juce::Point<float> c (xForF (sf), yForDb (magDb (sf)));
            drawSmiley (g, c, r, t.display, t.accent, t.accent, 3.5f);
        }
        g.restoreState();

        g.setColour (t.displayOutline);
        g.drawRoundedRectangle (box, 12.0f, 3.0f);
    }

private:
    const Skin& skin;
};

// ============================================================ preset bar
class PresetBar : public juce::Component
{
public:
    PresetBar (const Skin& s) : skin (s) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

    std::function<int()> getIndex;
    std::function<int()> getCount;
    std::function<juce::String (int)> getName;
    std::function<void (int)> load;

    void paint (juce::Graphics& g) override
    {
        const auto& t = skin.theme;
        auto r = getLocalBounds().toFloat();
        const float bw = r.getHeight();
        auto prev = r.removeFromLeft (bw);
        auto next = r.removeFromRight (bw);
        r = r.reduced (10.0f, 0.0f);

        auto arrowBox = [&] (juce::Rectangle<float> b, bool left)
        {
            b = b.withTrimmedRight (shadowOff).withTrimmedBottom (shadowOff);
            drawStickerBox (g, b, 12.0f, t.presetButton, t.shadow, 2.5f, t.shadow, shadowOff);
            juce::Path chevron;
            const auto c = b.getCentre();
            const float s = b.getHeight() * 0.14f;
            if (left) { chevron.startNewSubPath (c.x + s * 0.6f, c.y - s * 1.3f); chevron.lineTo (c.x - s * 0.7f, c.y); chevron.lineTo (c.x + s * 0.6f, c.y + s * 1.3f); }
            else      { chevron.startNewSubPath (c.x - s * 0.6f, c.y - s * 1.3f); chevron.lineTo (c.x + s * 0.7f, c.y); chevron.lineTo (c.x - s * 0.6f, c.y + s * 1.3f); }
            g.setColour (t.presetIcon);
            g.strokePath (chevron, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        };
        arrowBox (prev, true);
        arrowBox (next, false);

        auto disp = r.withTrimmedRight (shadowOff).withTrimmedBottom (shadowOff);
        drawStickerBox (g, disp, 14.0f, t.presetDisplay, t.shadow, 0.0f, t.shadow, shadowOff);

        const int idx = getIndex ? getIndex() : 0;
        auto text = disp.reduced (16.0f, 0.0f);
        g.setColour (juce::Colours::white);
        g.setFont (skin.font (14.5f));
        g.drawText ("PRESET " + juce::String (idx + 1).paddedLeft ('0', 2), text, juce::Justification::centredLeft, false);
        g.setColour (t.accent);
        g.setFont (skin.font (19.0f));
        g.drawText (getName ? getName (idx) : juce::String(), text, juce::Justification::centredRight, true);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! getCount || ! load || ! getIndex) return;
        const int n = getCount(), idx = getIndex();
        const float bw = (float) getHeight();
        if (e.position.x < bw)                    load ((idx + n - 1) % n);
        else if (e.position.x > getWidth() - bw)  load ((idx + 1) % n);
        else
        {
            juce::PopupMenu menu;
            for (int i = 0; i < n; ++i)
                menu.addItem (i + 1, juce::String (i + 1).paddedLeft ('0', 2) + "   " + getName (i), true, i == idx);
            menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                                [safe = juce::Component::SafePointer<PresetBar> (this)] (int result)
                                {
                                    if (safe != nullptr && result > 0 && safe->load) safe->load (result - 1);
                                });
        }
        repaint();
    }

private:
    const Skin& skin;
};

// ============================================================ output meter + volume
class OutMeter : public juce::Component, public juce::SettableTooltipClient
{
public:
    OutMeter (const Skin& s, juce::RangedAudioParameter& volumeParam)
        : skin (s), param (volumeParam),
          attachment (volumeParam, [this] (float v) { volume = v; repaint(); })
    {
        attachment.sendInitialUpdate();
        setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
        setTooltip ("Output. Drag left/right to set the volume.");
    }

    void setLevel (float peakLinear)
    {
        level = peakLinear;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto& t = skin.theme;
        auto r = getLocalBounds().toFloat().withTrimmedRight (shadowOff).withTrimmedBottom (shadowOff);
        drawStickerBox (g, r, r.getHeight() * 0.5f, t.panel, t.panelOutline, 2.5f, t.shadow, shadowOff);

        auto inner = r.reduced (14.0f, 0.0f);
        g.setColour (t.text);
        g.setFont (skin.font (15.5f));
        g.drawText ("OUT", inner.removeFromLeft (40.0f), juce::Justification::centredLeft, false);

        const float db = juce::Decibels::gainToDecibels (level, -60.0f);
        g.setColour (t.text);
        g.setFont (skin.font (14.0f));
        g.drawText (db <= -59.9f ? "-inf dB" : juce::String (db, 1) + " dB", inner.removeFromRight (64.0f),
                    juce::Justification::centredRight, false);

        track = inner.reduced (6.0f, 0.0f).withSizeKeepingCentre (inner.getWidth() - 12.0f, 12.0f);
        g.setColour (juce::Colour (0xff111111));
        g.fillRoundedRectangle (track, 6.0f);
        const float fill = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
        if (fill > 0.0f)
        {
            g.setColour (t.accent);
            g.fillRoundedRectangle (track.withWidth (std::max (12.0f, track.getWidth() * fill)), 6.0f);
        }

        // volume handle
        const float x = track.getX() + track.getWidth() * volume;
        auto handle = juce::Rectangle<float> (x - 3.0f, track.getY() - 5.0f, 6.0f, track.getHeight() + 10.0f);
        g.setColour (juce::Colours::white);
        g.fillRoundedRectangle (handle, 3.0f);
        g.setColour (t.shadow);
        g.drawRoundedRectangle (handle, 3.0f, 1.5f);
    }

    void mouseDown (const juce::MouseEvent& e) override { attachment.beginGesture(); setFromMouse (e); }
    void mouseDrag (const juce::MouseEvent& e) override { setFromMouse (e); }
    void mouseUp (const juce::MouseEvent&) override     { attachment.endGesture(); }
    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        attachment.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue()));
    }

private:
    void setFromMouse (const juce::MouseEvent& e)
    {
        if (track.getWidth() <= 0.0f) return;
        const float v = juce::jlimit (0.0f, 1.0f, (e.position.x - track.getX()) / track.getWidth());
        attachment.setValueAsPartOfGesture (v);
    }

    const Skin& skin;
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
    float volume = 0.7f, level = 0.0f;
    juce::Rectangle<float> track;
};

} // namespace squelch::ui
