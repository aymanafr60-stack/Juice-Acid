// Squelch — interface components (hardware style, v0.8).
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Theme.h"
#include <complex>

namespace squelch::ui
{

constexpr float startAngle = juce::MathConstants<float>::pi * -0.75f;
constexpr float endAngle   = juce::MathConstants<float>::pi *  0.75f;

// ============================================================ knob
// The knob body (ticks, ridged skirt, cap, light) is a pre-rendered image per theme and size;
// only the pointer is drawn live.
class KnobSlider : public juce::Slider
{
public:
    KnobSlider (const Skin& s) : skin (s)
    {
        setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setRotaryParameters (startAngle + juce::MathConstants<float>::twoPi, endAngle + juce::MathConstants<float>::twoPi, true);
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    }

    int bodySize = 46;  // skirt diameter

    void paint (juce::Graphics& g) override
    {
        const auto& t = skin.theme;
        const juce::String name = "knob_" + t.knobPrefix + "_" + juce::String (bodySize) + "_png";
        int dataSize = 0;
        if (auto* data = BinaryData::getNamedResource (name.toRawUTF8(), dataSize))
        {
            auto img = juce::ImageCache::getFromMemory (data, dataSize);
            g.drawImage (img, getLocalBounds().toFloat(), juce::RectanglePlacement::centred);
        }
        const auto c = getLocalBounds().toFloat().getCentre();
        const float R = (float) bodySize * 0.5f;
        const float pos = (float) valueToProportionOfLength (getValue());
        const float angle = startAngle + pos * (endAngle - startAngle);
        const auto p1 = c.getPointOnCircumference (R * 0.2f, angle), p2 = c.getPointOnCircumference (R * 0.95f, angle);
        g.setColour (t.pointer);
        juce::Path line; line.startNewSubPath (p1); line.lineTo (p2);
        g.strokePath (line, juce::PathStrokeType (bodySize > 60 ? 3.2f : 2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

private:
    const Skin& skin;
};

// Knob + caption + readout window, bound to one parameter.
class Knob : public juce::Component
{
public:
    Knob (const Skin& s, juce::RangedAudioParameter& p, const juce::String& labelText)
        : skin (s), param (p), label (labelText), slider (s), attachment (p, slider)
    {
        addAndMakeVisible (slider);
        slider.setDoubleClickReturnValue (true, p.convertFrom0to1 (p.getDefaultValue()));
        slider.onValueChange = [this] { repaint(); };
        slider.setTooltip (p.getName (64) + " (double-click to reset)");
    }

    std::function<juce::String()> valueText;
    void setLabel (const juce::String& l) { if (l != label) { label = l; repaint(); } }
    void setTooltip (const juce::String& tip) { slider.setTooltip (tip); }
    void setBodySize (int d) { slider.bodySize = d; resized(); }
    int  frame() const { return slider.bodySize + 46; }

    void resized() override
    {
        const int w = frame();
        slider.setBounds ((getWidth() - w) / 2, 0, w, w);
    }

    void paint (juce::Graphics& g) override
    {
        const auto& t = skin.theme;
        const float w = (float) frame();
        g.setColour (t.ink);
        g.setFont (skin.label (10.0f));
        g.drawText (label, juce::Rectangle<float> (0.0f, w - 6.0f, (float) getWidth(), 14.0f), juce::Justification::centred, false);
        const auto text = valueText ? valueText() : param.getCurrentValueAsText();
        const float rw = (float) text.length() * 7.4f + 14.0f;
        drawReadout (g, juce::Rectangle<float> (((float) getWidth() - rw) * 0.5f, w + 10.0f, rw, 17.0f), text, skin);
    }

private:
    const Skin& skin;
    juce::RangedAudioParameter& param;
    juce::String label;
    KnobSlider slider;
    juce::SliderParameterAttachment attachment;
};

// ============================================================ push buttons
enum class Icon { none, sparkle, download, play, stop, left, right };

class Btn : public juce::Button
{
public:
    Btn (const Skin& s, const juce::String& text, bool showLed = true, Icon ic = Icon::none, bool alwaysLit = false)
        : juce::Button (text), skin (s), led (showLed), icon (ic), lit (alwaysLit)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void setIcon (Icon ic) { icon = ic; repaint(); }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        const auto& t = skin.theme;
        const bool on = getToggleState() || lit;
        auto r = getLocalBounds().toFloat().reduced (0.5f, 0.0f).withTrimmedBottom (3.5f);
        drawKey (g, r, on, down, t);
        if (down) r = r.translated (0.0f, 1.0f);
        if (over && ! on) { g.setColour (juce::Colours::white.withAlpha (0.08f)); g.fillRoundedRectangle (r, t.keyRadius); }

        const auto ink = on ? t.keyOnText : t.keyText;
        const auto txt = getButtonText();
        g.setFont (skin.label (11.0f, 0.13f));
        if (txt.contains ("\n"))
        {
            g.setColour (ink);
            g.setFont (skin.label (9.5f, 0.1f));
            g.drawFittedText (txt, r.toNearestInt(), juce::Justification::centred, 2, 1.0f);
            return;
        }
        float textW = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), txt);
        const float lead = icon != Icon::none ? 16.0f : (led ? 15.0f : 0.0f);
        for (float h = 10.0f; lead + textW > r.getWidth() - 10.0f && h >= 8.0f; h -= 0.5f) // too long for this key: shrink the lettering
        {
            g.setFont (skin.label (h, 0.06f));
            textW = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), txt);
        }
        auto content = r.withSizeKeepingCentre (std::min (r.getWidth() - 6.0f, lead + textW), r.getHeight());
        if (icon != Icon::none)
            drawIcon (g, content.removeFromLeft (txt.isEmpty() ? content.getWidth() : 12.0f).withSizeKeepingCentre (12.0f, 12.0f), ink);
        else if (led)
            drawLed (g, content.removeFromLeft (8.0f).withSizeKeepingCentre (8.0f, 8.0f), on, t);
        if (txt.isNotEmpty())
        {
            content.removeFromLeft (lead - (icon != Icon::none ? 12.0f : 8.0f));
            g.setColour (ink);
            g.drawText (txt, content, juce::Justification::centredLeft, false);
        }
    }

private:
    void drawIcon (juce::Graphics& g, juce::Rectangle<float> b, juce::Colour ink)
    {
        g.setColour (ink);
        const auto c = b.getCentre();
        juce::Path p;
        switch (icon)
        {
            case Icon::sparkle:
            {
                const float R = 6.5f, k = 1.4f;
                p.startNewSubPath (c.x, c.y - R);
                p.quadraticTo (c.x + k, c.y - k, c.x + R, c.y); p.quadraticTo (c.x + k, c.y + k, c.x, c.y + R);
                p.quadraticTo (c.x - k, c.y + k, c.x - R, c.y); p.quadraticTo (c.x - k, c.y - k, c.x, c.y - R);
                p.closeSubPath(); g.fillPath (p); break;
            }
            case Icon::download:
                p.startNewSubPath (c.x, c.y - 6); p.lineTo (c.x, c.y + 2);
                p.startNewSubPath (c.x - 4, c.y - 2); p.lineTo (c.x, c.y + 2); p.lineTo (c.x + 4, c.y - 2);
                p.startNewSubPath (c.x - 6, c.y + 6); p.lineTo (c.x + 6, c.y + 6);
                g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); break;
            case Icon::play:  p.addTriangle (c.x - 4, c.y - 5.5f, c.x - 4, c.y + 5.5f, c.x + 5.5f, c.y); g.fillPath (p); break;
            case Icon::stop:  g.fillRoundedRectangle (b.withSizeKeepingCentre (9.0f, 9.0f), 1.5f); break;
            case Icon::left:  p.addTriangle (c.x + 4, c.y - 5, c.x + 4, c.y + 5, c.x - 4.5f, c.y); g.fillPath (p); break;
            case Icon::right: p.addTriangle (c.x - 4, c.y - 5, c.x - 4, c.y + 5, c.x + 4.5f, c.y); g.fillPath (p); break;
            case Icon::none: break;
        }
    }

    const Skin& skin;
    bool led;
    Icon icon;
    bool lit;
};

// Lays out a group of buttons in a grid with gaps (no background: they're separate keys).
class SegmentBox : public juce::Component
{
public:
    SegmentBox (const Skin&, std::vector<Btn*> b, int columns, int gapX = 8, int gapY = 10)
        : buttons (std::move (b)), cols (columns), gx (gapX), gy (gapY)
    {
        for (auto* btn : buttons) addAndMakeVisible (btn);
    }

    void resized() override
    {
        const int rows = ((int) buttons.size() + cols - 1) / cols;
        const float cw = ((float) getWidth() - (float) gx * (float) (cols - 1)) / (float) cols;
        const float rh = ((float) getHeight() - (float) gy * (float) (rows - 1)) / (float) rows;
        for (int i = 0; i < (int) buttons.size(); ++i)
        {
            const int col = i % cols, row = i / cols;
            buttons[(size_t) i]->setBounds (juce::Rectangle<float> ((cw + (float) gx) * (float) col, (rh + (float) gy) * (float) row, cw, rh).toNearestInt());
        }
    }

private:
    std::vector<Btn*> buttons;
    int cols, gx, gy;
};

// Binds a row of buttons to a choice parameter (one selected at a time).
class ChoiceGroup
{
public:
    ChoiceGroup (juce::RangedAudioParameter& p, std::vector<Btn*> b)
        : buttons (std::move (b)), attachment (p, [this] (float v) { show ((int) std::round (v)); })
    {
        for (int i = 0; i < (int) buttons.size(); ++i)
            buttons[(size_t) i]->onClick = [this, i] { attachment.setValueAsCompleteGesture ((float) i); };
        attachment.sendInitialUpdate();
    }

    std::function<void (int)> onChange;

private:
    void show (int value)
    {
        for (int i = 0; i < (int) buttons.size(); ++i)
            buttons[(size_t) i]->setToggleState (i == value, juce::dontSendNotification);
        if (onChange) onChange (value);
    }

    std::vector<Btn*> buttons;
    juce::ParameterAttachment attachment;
};

// A key bound to an on/off parameter (PLAY, LATCH, CHORD, PING-PONG, SYNC).
class ParamToggleButton : public Btn
{
public:
    ParamToggleButton (const Skin& s, const juce::String& text, juce::RangedAudioParameter& p, bool showLed = true)
        : Btn (s, text, showLed),
          attachment (p, [this] (float v) { setToggleState (v > 0.5f, juce::dontSendNotification); if (onToggled) onToggled(); })
    {
        onClick = [this] { attachment.setValueAsCompleteGesture (getToggleState() ? 0.0f : 1.0f); };
    }
    void init() { attachment.sendInitialUpdate(); }
    std::function<void()> onToggled;

private:
    juce::ParameterAttachment attachment;
};

// ============================================================ on/off: bat toggle + light
class ToggleSwitch : public juce::Component, public juce::SettableTooltipClient
{
public:
    ToggleSwitch (const Skin& s, juce::RangedAudioParameter& p)
        : skin (s), attachment (p, [this] (float v) { on = v > 0.5f; repaint(); })
    {
        attachment.sendInitialUpdate();
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void paint (juce::Graphics& g) override
    {
        const auto& t = skin.theme;
        drawToggleSwitch (g, { 17.0f, 17.0f }, on, t);
        g.setColour (t.ink);
        g.setFont (skin.label (10.0f));
        g.drawText (on ? "ON" : "OFF", juce::Rectangle<float> (40.0f, 10.0f, 30.0f, 14.0f), juce::Justification::centredLeft, false);
        drawLed (g, { 72.0f, 13.0f, 8.0f, 8.0f }, on, t, t.kind == Theme::Kind::kawaii ? juce::Colour (0xffff4f9a) : juce::Colour());
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

// ============================================================ value box (readout with up/down)
class Stepper : public juce::Component, public juce::SettableTooltipClient
{
public:
    Stepper (const Skin& s, juce::RangedAudioParameter& p)
        : skin (s), param (p), attachment (p, [this] (float) { repaint(); })
    {
        attachment.sendInitialUpdate();
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    }

    std::function<juce::String()> valueText;

    void paint (juce::Graphics& g) override
    {
        const auto& t = skin.theme;
        auto r = getLocalBounds().toFloat().reduced (4.5f);
        g.setColour (t.screenRing); g.drawRoundedRectangle (r.expanded (4.0f), 9.0f, 1.0f);
        g.setColour (t.bezelMid);   g.fillRoundedRectangle (r.expanded (3.0f), 8.5f);
        g.setColour (t.readoutBg);  g.fillRoundedRectangle (r, 6.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
        const auto text = (valueText ? valueText() : param.getCurrentValueAsText()).toUpperCase();
        g.setFont (skin.readout (15.0f));
        g.setColour (t.phos.withAlpha (0.25f));
        g.drawText (text, r.withTrimmedLeft (11.0f).withTrimmedRight (22.0f).translated (0, 0.6f), juce::Justification::centredLeft, true);
        g.setColour (t.phos);
        g.drawText (text, r.withTrimmedLeft (11.0f).withTrimmedRight (22.0f), juce::Justification::centredLeft, true);
        const float cx = r.getRight() - 12.0f, cy = r.getCentreY();
        juce::Path up, dn;
        up.addTriangle (cx - 4, cy - 2, cx + 4, cy - 2, cx, cy - 7);
        dn.addTriangle (cx - 4, cy + 2, cx + 4, cy + 2, cx, cy + 7);
        g.setColour (t.phos.withAlpha (0.7f));
        g.fillPath (up); g.fillPath (dn);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        dragStartY = e.position.y; startValue = param.getValue(); dragged = false; attachment.beginGesture();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        const float dy = dragStartY - e.position.y;
        if (std::abs (dy) < 3.0f && ! dragged) return;
        dragged = true;
        setNormalised (startValue + dy * (isDiscrete() ? stepSize() / 8.0f : 1.0f / 200.0f));
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        attachment.endGesture();
        if (dragged) return;
        if (e.position.x > (float) getWidth() - 30.0f) step (e.position.y < (float) getHeight() * 0.5f ? 1 : -1);
        else if (isDiscrete() && numValues() <= 40)    showMenu();
    }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        if (std::abs (w.deltaY) > 1.0e-4f) step (w.deltaY > 0 ? 1 : -1);
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (e.position.x <= (float) getWidth() - 30.0f && ! isDiscrete())
            attachment.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue()));
    }

private:
    bool isDiscrete() const { return param.getNumSteps() > 1 && param.getNumSteps() < 1000; }
    int numValues() const   { return param.getNumSteps(); }
    float stepSize() const  { return isDiscrete() ? 1.0f / (float) (numValues() - 1) : 0.01f; }

    void setNormalised (float v)
    {
        v = juce::jlimit (0.0f, 1.0f, v);
        if (isDiscrete()) v = std::round (v / stepSize()) * stepSize();
        attachment.setValueAsPartOfGesture (param.convertFrom0to1 (v));
    }
    void step (int dir)
    {
        attachment.setValueAsCompleteGesture (param.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, param.getValue() + (float) dir * stepSize())));
    }
    void showMenu()
    {
        juce::PopupMenu menu;
        const int n = numValues();
        const float cur = param.getValue();
        for (int i = 0; i < n; ++i)
        {
            const float v = (float) i / (float) (n - 1);
            menu.addItem (i + 1, param.getText (v, 64), true, std::abs (v - cur) < 1.0e-4f);
        }
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                            [safe = juce::Component::SafePointer<Stepper> (this), n] (int result)
                            {
                                if (safe == nullptr || result <= 0) return;
                                safe->attachment.setValueAsCompleteGesture (safe->param.convertFrom0to1 ((float) (result - 1) / (float) (n - 1)));
                            });
    }

    const Skin& skin;
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
    float dragStartY = 0.0f, startValue = 0.0f;
    bool dragged = false;
};

// ============================================================ CRT filter screen (live)
class FilterDisplay : public juce::Component
{
public:
    explicit FilterDisplay (const Skin& s) : skin (s) {}

    std::function<float()> getCutoffHz, getResonance;
    std::function<void (float*, int)> readScope;

    void paint (juce::Graphics& g) override
    {
        const auto& t = skin.theme;
        auto outer = getLocalBounds().toFloat();

        // bezel
        {
            juce::ColourGradient bez (t.bezelTop, 0, outer.getY(), t.bezelBottom, 0, outer.getBottom(), false);
            bez.addColour (0.35, t.bezelMid);
            g.setColour (juce::Colours::black.withAlpha (t.kind == Theme::Kind::kawaii ? 0.0f : 0.35f));
            g.fillRoundedRectangle (outer.translated (0, 4).expanded (1), t.bezelRadius);
            if (t.kind == Theme::Kind::kawaii) { g.setColour (juce::Colour (0xfff2a6c8)); g.fillRoundedRectangle (outer.translated (0, 3), t.bezelRadius); }
            g.setGradientFill (bez);
            g.fillRoundedRectangle (outer, t.bezelRadius);
            g.setColour (t.bezelHighlight);
            g.drawLine (outer.getX() + t.bezelRadius, outer.getY() + 1, outer.getRight() - t.bezelRadius, outer.getY() + 1, 1.0f);
            for (auto [x, y, top] : { std::tuple { 7.0f, 7.0f, true }, { outer.getRight() - 7.0f, 7.0f, true },
                                      { 7.0f, outer.getBottom() - 7.0f, false }, { outer.getRight() - 7.0f, outer.getBottom() - 7.0f, false } })
            {
                g.setColour (top ? t.screwTop : t.screwBottom);
                g.fillEllipse (x - 3, y - 3, 6, 6);
            }
        }

        auto box = outer.reduced (12.0f);
        const float W = box.getWidth(), H = box.getHeight();
        juce::Graphics::ScopedSaveState save (g);
        juce::Path clip; clip.addRoundedRectangle (box, 8.0f);
        g.reduceClipRegion (clip);
        g.setOrigin (box.getPosition().toInt());
        g.setColour (t.screen);
        g.fillRect (0.0f, 0.0f, W, H);

        // graticule
        g.setColour (t.grid);
        for (int i = 1; i < 10; ++i) g.fillRect ((float) i * W / 10.0f, 0.0f, 1.0f, H);
        for (int j = 1; j < 6; ++j)  g.fillRect (0.0f, (float) j * H / 6.0f, W, 1.0f);
        g.setColour (t.gridTick);
        for (int i = 0; i < 50; ++i) g.fillRect ((float) i * W / 50.0f, H * 0.5f - 3.0f, 1.0f, 6.0f);
        for (int j = 0; j < 30; ++j) g.fillRect (W * 0.5f - 3.0f, (float) j * H / 30.0f, 6.0f, 1.0f);

        // the mascot drawn by the beam
        drawGlitchSmiley (g, { W * 0.5f, H * 0.5f + 8.0f }, 40.0f, t, 1.8f, 0.42f);

        const float fc = getCutoffHz ? getCutoffHz() : 1000.0f;
        const float res = getResonance ? getResonance() : 0.5f;
        auto curve = [&] (float f0)
        {
            const float k = 3.95f * juce::jlimit (0.0f, 0.92f, res);
            juce::Path p;
            for (int i = 0; i <= 200; ++i)
            {
                const float f = 20.0f * std::pow (1000.0f, (float) i / 200.0f);
                const std::complex<double> sj (0.0, (double) (f / f0));
                const auto den = std::pow (1.0 + sj, 4) + (double) k;
                const float db = 20.0f * std::log10 ((1.0f + 0.6f * k) / (float) std::abs (den) + 1.0e-9f);
                const float x = W * (float) i / 200.0f, y = juce::jlimit (4.0f, H + 5.0f, H * (0.1f + (22.0f - db) / 78.0f));
                if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
            }
            return p;
        };

        // phosphor persistence: recent positions fade out behind the beam
        history[(size_t) (histPos++ % history.size())] = fc;
        for (size_t k = 1; k < history.size(); ++k)
        {
            const float old = history[(histPos - 1 - k + history.size() * 8) % history.size()];
            if (old <= 0.0f || std::abs (old - fc) < 1.0f) continue;
            g.setColour (t.phos.withAlpha (0.16f * (float) (history.size() - k) / (float) history.size()));
            g.strokePath (curve (old), juce::PathStrokeType (2.0f));
        }

        // CH2: the actual output waveform, triggered on a rising zero crossing
        if (readScope)
        {
            constexpr int n = 1400, show = 700;
            float buf[n];
            readScope (buf, n);
            int start = 0;
            for (int i = 1; i < n - show; ++i) if (buf[i - 1] < 0.0f && buf[i] >= 0.0f) { start = i; break; }
            juce::Path wave;
            for (int i = 0; i < show; ++i)
            {
                const float x = W * (float) i / (float) (show - 1);
                const float y = H * 0.8f - juce::jlimit (-1.0f, 1.0f, buf[start + i]) * H * 0.2f;
                if (i == 0) wave.startNewSubPath (x, y); else wave.lineTo (x, y);
            }
            glowStroke (g, wave, t.phos2.withAlpha (0.5f), 1.2f, 0.6f);
        }

        glowStroke (g, curve (fc), t.phos, 2.2f);

        // labels
        g.setFont (skin.readout (11.0f));
        auto lab = [&] (const juce::String& s, float x, float y, juce::Justification j, juce::Colour c)
        {
            g.setColour (c.withAlpha (0.85f));
            g.drawText (s, juce::Rectangle<float> (j == juce::Justification::centredRight ? x - 140.0f : x, y - 10.0f, 140.0f, 14.0f), j, false);
        };
        lab ("CH1 LPF 24", 8.0f, 14.0f, juce::Justification::centredLeft, t.phos);
        lab ("CH2 OUT", 8.0f, 28.0f, juce::Justification::centredLeft, t.phos2);
        lab ("RES " + juce::String (juce::roundToInt (res * 100.0f)) + "%", W - 8.0f, 14.0f, juce::Justification::centredRight, t.phos);
        lab ("20Hz", 8.0f, H - 8.0f, juce::Justification::centredLeft, t.phos);
        lab ("20kHz", W - 8.0f, H - 8.0f, juce::Justification::centredRight, t.phos);
        const float xc = W * std::log (fc / 20.0f) / std::log (1000.0f);
        const juce::String hz = fc < 1000.0f ? juce::String (juce::roundToInt (fc)) + " Hz" : juce::String (fc / 1000.0f, 1) + " kHz";
        const float k = 3.95f * juce::jlimit (0.0f, 0.92f, res);
        const float peakDb = 20.0f * std::log10 ((1.0f + 0.6f * k) / std::abs (k - 4.0f) + 1.0e-9f);
        const float peakY = H * (0.1f + (22.0f - peakDb) / 78.0f);
        lab (hz, juce::jlimit (8.0f, W - 70.0f, xc + 8.0f), juce::jlimit (26.0f, H - 24.0f, peakY + 4.0f), juce::Justification::centredLeft, t.phos);

        // the room's light on the glass, then scanlines
        g.setColour (t.glare);
        g.fillEllipse (W * 0.22f - W * 0.45f, -10.0f - 46.0f, W * 0.9f, 92.0f);
        {
            juce::Path streak; streak.startNewSubPath (W * 0.04f, 10.0f); streak.quadraticTo (W * 0.5f, 2.0f, W * 0.96f, 12.0f);
            g.setColour (t.glareLine.withMultipliedAlpha (0.12f)); g.strokePath (streak, juce::PathStrokeType (7.0f));
            g.setColour (t.glareLine.withMultipliedAlpha (0.25f)); g.strokePath (streak, juce::PathStrokeType (3.0f));
            juce::Path band; band.startNewSubPath (W * 0.62f, H); band.lineTo (W * 0.78f, 0.0f);
            g.setColour (t.glare.withMultipliedAlpha (0.5f)); g.strokePath (band, juce::PathStrokeType (26.0f));
        }
        g.setColour (juce::Colours::black.withAlpha (t.kind == Theme::Kind::kawaii ? 0.25f : 0.35f));
        for (float y = 0.0f; y < H; y += 3.0f) g.fillRect (0.0f, y, W, 1.0f);
    }

private:
    const Skin& skin;
    std::array<float, 7> history {};
    size_t histPos = 0;
};

// ============================================================ preset bar: arrow keys + display
class PresetBar : public juce::Component
{
public:
    PresetBar (const Skin& s) : skin (s), prev (s, "", false, Icon::left), next (s, "", false, Icon::right)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        addAndMakeVisible (prev);
        addAndMakeVisible (next);
        prev.onClick = [this] { if (getCount && load && getIndex) { const int n = getCount(); load ((getIndex() + n - 1) % n); repaint(); } };
        next.onClick = [this] { if (getCount && load && getIndex) { const int n = getCount(); load ((getIndex() + 1) % n); repaint(); } };
        nameEditor.setJustification (juce::Justification::centredLeft);
        nameEditor.setInputRestrictions (32);
        nameEditor.setSelectAllWhenFocused (true);
        nameEditor.onReturnKey = [this] { finishSave (true); };
        nameEditor.onEscapeKey = [this] { finishSave (false); };
        nameEditor.onFocusLost = [this] { finishSave (false); };
        addChildComponent (nameEditor);
    }

    std::function<int()> getIndex, getCount, getFactoryCount;
    std::function<juce::String (int)> getName;
    std::function<void (int)> load;
    std::function<bool (const juce::String&)> save;
    std::function<void()> openFolder;

    void startSave()
    {
        const auto& t = skin.theme;
        nameEditor.setFont (skin.readout (16.0f));
        nameEditor.setColour (juce::TextEditor::backgroundColourId, t.readoutBg);
        nameEditor.setColour (juce::TextEditor::textColourId, t.phos);
        nameEditor.setColour (juce::TextEditor::highlightColourId, t.phos.withAlpha (0.3f));
        nameEditor.setColour (juce::TextEditor::outlineColourId, t.phos);
        nameEditor.setColour (juce::TextEditor::focusedOutlineColourId, t.phos);
        nameEditor.setColour (juce::CaretComponent::caretColourId, t.phos);
        nameEditor.setText ("MY SOUND", false);
        nameEditor.setVisible (true);
        nameEditor.grabKeyboardFocus();
        nameEditor.selectAll();
        repaint();
    }

    void resized() override
    {
        prev.setBounds (0, 0, 36, getHeight());
        next.setBounds (getWidth() - 36, 0, 36, getHeight());
        nameEditor.setBounds (display().reduced (40, 5).toNearestInt());
    }

    void paint (juce::Graphics& g) override
    {
        const auto& t = skin.theme;
        auto d = display();
        g.setColour (t.kind == Theme::Kind::kawaii ? juce::Colours::white : t.screenRing);
        g.drawRoundedRectangle (d.expanded (t.kind == Theme::Kind::kawaii ? 3.5f : 3.5f), 9.0f, t.kind == Theme::Kind::kawaii ? 4.0f : 1.0f);
        g.setColour (t.kind == Theme::Kind::kawaii ? juce::Colour (0xffffc3dc) : t.bezelMid);
        g.drawRoundedRectangle (d.expanded (1.5f), 7.5f, 3.0f);
        g.setColour (t.readoutBg);
        g.fillRoundedRectangle (d, 6.0f);
        {
            juce::ColourGradient sheen (t.glareLine.withMultipliedAlpha (0.3f), d.getX(), d.getY(), juce::Colours::transparentBlack, d.getX() + 40, d.getBottom(), false);
            g.setGradientFill (sheen); g.fillRoundedRectangle (d, 6.0f);
        }
        const int idx = getIndex ? getIndex() : 0;
        const int factory = getFactoryCount ? getFactoryCount() : 1000;
        auto text = d.reduced (12.0f, 0.0f);
        g.setFont (skin.readout (16.0f));
        g.setColour (t.phos.withAlpha (0.6f));
        const auto num = nameEditor.isVisible() ? juce::String ("SAVE") : idx < factory ? juce::String (idx + 1).paddedLeft ('0', 2)
                                                                                         : "U" + juce::String (idx - factory + 1);
        g.drawText (num, text.removeFromLeft (34.0f), juce::Justification::centredLeft, false);
        juce::Path dd;
        const float cx = d.getRight() - 16.0f, cy = d.getCentreY();
        dd.addTriangle (cx - 5, cy - 3, cx + 5, cy - 3, cx, cy + 3);
        g.setColour (t.phos.withAlpha (0.7f)); g.fillPath (dd);
        if (! nameEditor.isVisible())
        {
            g.setColour (t.phos);
            g.drawText (getName ? getName (idx).toUpperCase() : juce::String(), text.withTrimmedRight (24.0f), juce::Justification::centredLeft, true);
        }
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (display().contains (e.position) && ! nameEditor.isVisible() && getCount) showMenu();
    }

private:
    enum { saveId = 100000, folderId = 100001 };

    juce::Rectangle<float> display() const { return getLocalBounds().toFloat().reduced (48.0f, 0.0f).withTrimmedBottom (3.0f); }

    void showMenu()
    {
        const int n = getCount(), idx = getIndex();
        const int factory = getFactoryCount ? getFactoryCount() : n;
        juce::PopupMenu menu;
        menu.addItem (saveId, "Save current sound...");
        menu.addSeparator();
        menu.addSectionHeader ("FACTORY");
        for (int i = 0; i < factory; ++i) menu.addItem (i + 1, juce::String (i + 1).paddedLeft ('0', 2) + "   " + getName (i), true, i == idx);
        menu.addSectionHeader ("YOUR PRESETS");
        if (n == factory) menu.addItem (-1, "(none yet: use Save current sound)", false);
        for (int i = factory; i < n; ++i) menu.addItem (i + 1, "U" + juce::String (i - factory + 1) + "   " + getName (i), true, i == idx);
        menu.addSeparator();
        menu.addItem (folderId, "Open presets folder");
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                            [safe = juce::Component::SafePointer<PresetBar> (this)] (int result)
                            {
                                if (safe == nullptr || result <= 0) return;
                                if (result == saveId)        safe->startSave();
                                else if (result == folderId) { if (safe->openFolder) safe->openFolder(); }
                                else if (safe->load)         safe->load (result - 1);
                                safe->repaint();
                            });
    }

    void finishSave (bool doSave)
    {
        if (! nameEditor.isVisible()) return;
        const auto name = nameEditor.getText().trim();
        nameEditor.setVisible (false);
        if (doSave && name.isNotEmpty() && save) save (name);
        repaint();
    }

    const Skin& skin;
    Btn prev, next;
    juce::TextEditor nameEditor;
};

// ============================================================ output meter (footer) + volume
class OutMeter : public juce::Component, public juce::SettableTooltipClient
{
public:
    OutMeter (const Skin& s, juce::RangedAudioParameter& volumeParam)
        : skin (s), param (volumeParam), attachment (volumeParam, [this] (float v) { volume = v; repaint(); })
    {
        attachment.sendInitialUpdate();
        setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
        setTooltip ("Output level. Drag left/right to set the volume (double-click resets)");
    }

    void setLevel (float peakLinear) { level = peakLinear; repaint(); }
    static constexpr int cells = 22;

    void paint (juce::Graphics& g) override
    {
        const auto& t = skin.theme;
        g.setColour (t.footText);
        g.setFont (skin.mono (10.5f, true, 0.12f));
        g.drawText ("OUT", juce::Rectangle<float> (0.0f, 0.0f, 34.0f, (float) getHeight()), juce::Justification::centredLeft, false);
        meter = juce::Rectangle<float> (36.0f, (float) getHeight() * 0.5f - 8.5f, 150.0f, 17.0f);
        const float db = juce::Decibels::gainToDecibels (level, -60.0f);
        const float lit = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f) * (float) cells;
        for (int row = 0; row < 2; ++row)
            for (int i = 0; i < cells; ++i)
            {
                const juce::Rectangle<float> c (meter.getX() + 6.8f * (float) i, meter.getY() + 10.0f * (float) row, 5.0f, 7.0f);
                const bool on = (float) i < lit, hot = i >= 19;
                const auto col = on ? (hot ? t.meterHot : t.meterOn) : t.meterOff;
                if (on) { g.setColour (col.withAlpha (0.3f)); g.fillRoundedRectangle (c.expanded (1.5f), 2.0f); }
                g.setColour (col); g.fillRoundedRectangle (c, 1.0f);
            }
        const float x = meter.getX() + meter.getWidth() * volume;
        g.setColour (t.footText);
        g.fillRoundedRectangle (x - 1.0f, meter.getY() - 3.0f, 2.0f, meter.getHeight() + 6.0f, 1.0f);
        g.setFont (skin.readout (13.0f));
        g.setColour (t.kind == Theme::Kind::kawaii ? juce::Colours::white : t.phos);
        g.drawText (db <= -59.9f ? juce::String ("-INF") : juce::String (db, 1), juce::Rectangle<float> (meter.getRight() + 4.0f, 0.0f, 42.0f, (float) getHeight()),
                    juce::Justification::centredRight, false);
    }

    void mouseDown (const juce::MouseEvent& e) override { attachment.beginGesture(); setFromMouse (e); }
    void mouseDrag (const juce::MouseEvent& e) override { setFromMouse (e); }
    void mouseUp (const juce::MouseEvent&) override     { attachment.endGesture(); }
    void mouseDoubleClick (const juce::MouseEvent&) override { attachment.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue())); }

private:
    void setFromMouse (const juce::MouseEvent& e)
    {
        if (meter.getWidth() <= 0.0f) return;
        attachment.setValueAsPartOfGesture (juce::jlimit (0.0f, 1.0f, (e.position.x - meter.getX()) / meter.getWidth()));
    }

    const Skin& skin;
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
    float volume = 0.7f, level = 0.0f;
    juce::Rectangle<float> meter;
};

} // namespace squelch::ui
