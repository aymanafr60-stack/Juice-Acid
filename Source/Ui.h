// Squelch — interface components (flat style, v0.7).
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Theme.h"
#include <complex>

namespace squelch::ui
{

constexpr float startAngle = juce::MathConstants<float>::pi * -0.75f;
constexpr float endAngle   = juce::MathConstants<float>::pi *  0.75f;

// "ACID SCREAM" -> "Acid Scream", "GROWL FM" -> "Growl FM"
inline juce::String titleCase (const juce::String& s)
{
    juce::StringArray words;
    words.addTokens (s, " ", "");
    for (auto& w : words)
        if (w.length() > 2 && w.containsAnyOf ("ABCDEFGHIJKLMNOPQRSTUVWXYZ"))
            w = w.substring (0, 1).toUpperCase() + w.substring (1).toLowerCase();
    return words.joinIntoString (" ");
}

// ============================================================ arc knob
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
    }

    void paint (juce::Graphics& g) override
    {
        const auto& t = skin.theme;
        auto area = getLocalBounds().toFloat();
        const float arcW = big ? 6.5f : 5.0f;
        const float r = std::min (area.getWidth(), area.getHeight()) * 0.5f - arcW * 0.5f - 0.5f;
        const auto c = area.getCentre();
        const float pos = (float) valueToProportionOfLength (getValue());
        const float angle = startAngle + pos * (endAngle - startAngle);
        const juce::PathStrokeType stroke (arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

        juce::Path track;
        track.addCentredArc (c.x, c.y, r, r, 0.0f, startAngle, endAngle, true);
        g.setColour (t.track);
        g.strokePath (track, stroke);

        if (pos > 0.001f)
        {
            juce::Path value;
            value.addCentredArc (c.x, c.y, r, r, 0.0f, startAngle, angle, true);
            g.setColour (t.active);
            g.strokePath (value, stroke);
        }

        const float bodyR = r - arcW * 0.5f - (big ? 5.0f : 3.5f);
        g.setColour (t.knobBody);
        g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2, bodyR * 2);
        g.setColour (t.knobBorder);
        g.drawEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2, bodyR * 2, 1.5f);

        const auto p1 = c.getPointOnCircumference (bodyR * 0.3f, angle);
        const auto p2 = c.getPointOnCircumference (bodyR * 0.72f, angle);
        g.setColour (t.pointer);
        g.drawLine ({ p1, p2 }, big ? 3.0f : 2.5f);
    }

private:
    const Skin& skin;
    bool big;
};

// Knob + caption + value, bound to one parameter.
class Knob : public juce::Component
{
public:
    Knob (const Skin& s, juce::RangedAudioParameter& p, const juce::String& labelText, bool isBig = false)
        : skin (s), param (p), label (labelText), slider (s, isBig), attachment (p, slider)
    {
        addAndMakeVisible (slider);
        slider.setDoubleClickReturnValue (true, p.convertFrom0to1 (p.getDefaultValue()));
        slider.onValueChange = [this] { repaint(); };
        slider.setTooltip (p.getName (64) + " (double-click to reset)");
    }

    std::function<juce::String()> valueText;
    void setLabel (const juce::String& l) { if (l != label) { label = l; repaint(); } }
    void setTooltip (const juce::String& tip) { slider.setTooltip (tip); }

    int knobSize = 50;

    void resized() override
    {
        slider.setBounds (getLocalBounds().removeFromTop (knobSize).withSizeKeepingCentre (knobSize, knobSize));
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        area.removeFromTop ((float) knobSize + 7.0f);
        g.setColour (skin.theme.label);
        g.setFont (skin.caption());
        g.drawText (label, area.removeFromTop (16.0f), juce::Justification::centred, false);
        g.setColour (skin.theme.text);
        g.setFont (skin.font (14.0f, Weight::extraBold));
        g.drawText (valueText ? valueText() : param.getCurrentValueAsText(), area.removeFromTop (19.0f),
                    juce::Justification::centred, false);
    }

private:
    const Skin& skin;
    juce::RangedAudioParameter& param;
    juce::String label;
    KnobSlider slider;
    juce::SliderParameterAttachment attachment;
};

// ============================================================ buttons
enum class Icon { none, sparkle, download, play, stop };

class Btn : public juce::Button
{
public:
    enum class Style { segment, outline, primary, pill, play };

    Btn (const Skin& s, const juce::String& label, Style st = Style::outline, Icon ic = Icon::none)
        : juce::Button (label), skin (s), style (st), icon (ic)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void setIcon (Icon ic) { icon = ic; repaint(); }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        const auto& t = skin.theme;
        const bool on = getToggleState();
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        if (down) r = r.reduced (1.0f);

        juce::Colour fill = juce::Colours::transparentBlack, border = juce::Colours::transparentBlack, ink = t.text;
        float radius = 10.0f;
        const auto idleBorder = t.wellBorder.interpolatedWith (t.label, 0.25f);
        switch (style)
        {
            case Style::segment:
                radius = 8.0f;
                if (on) { fill = t.selected; ink = t.onSelected; }
                else if (over) fill = t.text.withAlpha (0.06f);
                break;
            case Style::outline:
                border = over ? t.label : idleBorder;
                break;
            case Style::primary:
                fill = over ? t.active.brighter (0.15f) : t.active; ink = t.onActive;
                break;
            case Style::pill:
                if (on) { fill = t.pillOnFill; border = t.selected; ink = t.selected; }
                else    { border = over ? t.label : idleBorder; }
                break;
            case Style::play:
                if (on) { fill = t.playFill; border = t.active; ink = t.active; }
                else    { border = over ? t.label : idleBorder; }
                break;
        }
        fillBox (g, r, radius, fill, border, 1.5f);

        g.setFont (skin.font (13.5f, Weight::extraBold, 0.1f));
        const auto label = getButtonText();
        g.setColour (ink);
        if (label.contains ("\n"))
        {
            g.setFont (skin.font (12.5f, Weight::extraBold, 0.1f));
            g.drawFittedText (label, r.toNearestInt(), juce::Justification::centred, 2, 1.0f);
            return;
        }

        const float iconW = icon == Icon::none ? 0.0f : 26.0f;
        const float textW = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), label);
        auto content = r.withSizeKeepingCentre (std::min (iconW + textW, r.getWidth() - 8.0f), r.getHeight());
        if (icon != Icon::none)
        {
            drawIcon (g, content.removeFromLeft (18.0f).withSizeKeepingCentre (16.0f, 16.0f), ink);
            content.removeFromLeft (8.0f);
        }
        g.setColour (ink);
        g.drawText (label, content, juce::Justification::centredLeft, false);
    }

private:
    void drawIcon (juce::Graphics& g, juce::Rectangle<float> b, juce::Colour ink)
    {
        g.setColour (ink);
        const auto c = b.getCentre();
        switch (icon)
        {
            case Icon::sparkle:
            {
                juce::Path p;
                const float R = b.getWidth() * 0.5f, k = R * 0.2f;
                p.startNewSubPath (c.x, c.y - R);
                p.quadraticTo (c.x + k, c.y - k, c.x + R, c.y);
                p.quadraticTo (c.x + k, c.y + k, c.x, c.y + R);
                p.quadraticTo (c.x - k, c.y + k, c.x - R, c.y);
                p.quadraticTo (c.x - k, c.y - k, c.x, c.y - R);
                p.closeSubPath();
                g.fillPath (p);
                break;
            }
            case Icon::download:
            {
                const juce::PathStrokeType st (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
                juce::Path p;
                p.startNewSubPath (c.x, b.getY() + 1.0f); p.lineTo (c.x, c.y + 3.0f);
                p.startNewSubPath (c.x - 4.5f, c.y - 1.5f); p.lineTo (c.x, c.y + 3.0f); p.lineTo (c.x + 4.5f, c.y - 1.5f);
                p.startNewSubPath (b.getX() + 1.0f, b.getBottom() - 1.5f); p.lineTo (b.getRight() - 1.0f, b.getBottom() - 1.5f);
                g.strokePath (p, st);
                break;
            }
            case Icon::play:
            {
                juce::Path p;
                p.addTriangle (c.x - 4.0f, c.y - 6.0f, c.x - 4.0f, c.y + 6.0f, c.x + 6.0f, c.y);
                g.fillPath (p);
                break;
            }
            case Icon::stop:
                g.fillRoundedRectangle (b.withSizeKeepingCentre (11.0f, 11.0f), 2.0f);
                break;
            case Icon::none: break;
        }
    }

    const Skin& skin;
    Style style;
    Icon icon;
};

// A dark well holding segment buttons laid out in a grid.
class SegmentBox : public juce::Component
{
public:
    SegmentBox (const Skin& s, std::vector<Btn*> b, int columns) : skin (s), buttons (std::move (b)), cols (columns)
    {
        for (auto* btn : buttons) addAndMakeVisible (btn);
    }

    void paint (juce::Graphics& g) override
    {
        fillBox (g, getLocalBounds().toFloat(), 11.0f, skin.theme.well, skin.theme.wellBorder.withAlpha (0.6f), 1.0f);
    }

    void resized() override
    {
        const int rows = ((int) buttons.size() + cols - 1) / cols;
        auto area = getLocalBounds().reduced (4);
        const float cw = (float) area.getWidth() / (float) cols, rh = (float) area.getHeight() / (float) rows;
        for (int i = 0; i < (int) buttons.size(); ++i)
        {
            const int col = i % cols, row = i / cols;
            buttons[(size_t) i]->setBounds (juce::Rectangle<float> ((float) area.getX() + cw * (float) col,
                                                                    (float) area.getY() + rh * (float) row, cw, rh).toNearestInt());
        }
    }

private:
    const Skin& skin;
    std::vector<Btn*> buttons;
    int cols;
};

// Binds a row of buttons to a choice parameter (one selected at a time).
class ChoiceGroup
{
public:
    ChoiceGroup (juce::RangedAudioParameter& p, std::vector<Btn*> b)
        : buttons (std::move (b)),
          attachment (p, [this] (float v) { show ((int) std::round (v)); })
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

// A button bound to an on/off parameter (pill toggles, PLAY).
class ParamToggleButton : public Btn
{
public:
    ParamToggleButton (const Skin& s, const juce::String& label, juce::RangedAudioParameter& p, Style st = Style::pill)
        : Btn (s, label, st),
          attachment (p, [this] (float v) { setToggleState (v > 0.5f, juce::dontSendNotification); if (onToggled) onToggled(); })
    {
        onClick = [this] { attachment.setValueAsCompleteGesture (getToggleState() ? 0.0f : 1.0f); };
    }
    void init() { attachment.sendInitialUpdate(); }
    std::function<void()> onToggled;

private:
    juce::ParameterAttachment attachment;
};

// ============================================================ on/off switch
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
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (on ? t.toggleOnTrack : t.toggleOffTrack);
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        const float d = r.getHeight() - 6.0f;
        const float x = on ? r.getRight() - d - 3.0f : r.getX() + 3.0f;
        g.setColour (on ? t.active : t.toggleOffKnob);
        g.fillEllipse (x, r.getCentreY() - d * 0.5f, d, d);
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

// ============================================================ stepper box  (value  ⌃⌄)
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
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        fillBox (g, r, 10.0f, t.well, t.wellBorder, 1.0f);
        g.setColour (t.text);
        g.setFont (skin.font (15.5f, Weight::extraBold));
        g.drawText (valueText ? valueText() : param.getCurrentValueAsText(), r.withTrimmedLeft (14.0f).withTrimmedRight (28.0f),
                    juce::Justification::centredLeft, true);

        const float cx = r.getRight() - 17.0f, cy = r.getCentreY();
        juce::Path p;
        p.startNewSubPath (cx - 4.0f, cy - 3.0f); p.lineTo (cx, cy - 7.0f); p.lineTo (cx + 4.0f, cy - 3.0f);
        p.startNewSubPath (cx - 4.0f, cy + 3.0f); p.lineTo (cx, cy + 7.0f); p.lineTo (cx + 4.0f, cy + 3.0f);
        g.setColour (t.label);
        g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        dragStartY = e.position.y;
        startValue = param.getValue();
        dragged = false;
        attachment.beginGesture();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        const float dy = dragStartY - e.position.y;
        if (std::abs (dy) < 3.0f && ! dragged) return;
        dragged = true;
        const float perPixel = isDiscrete() ? stepSize() / 8.0f : 1.0f / 200.0f;
        setNormalised (startValue + dy * perPixel);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        attachment.endGesture();
        if (dragged) return;

        if (e.position.x > (float) getWidth() - 30.0f)
            step (e.position.y < (float) getHeight() * 0.5f ? 1 : -1);
        else if (isDiscrete() && numValues() <= 40)
            showMenu();
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
                                safe->attachment.setValueAsCompleteGesture (
                                    safe->param.convertFrom0to1 ((float) (result - 1) / (float) (n - 1)));
                            });
    }

    const Skin& skin;
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attachment;
    float dragStartY = 0.0f, startValue = 0.0f;
    bool dragged = false;
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
        auto box = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (displayBackground (t));
        g.fillRoundedRectangle (box, 12.0f);

        g.saveState();
        juce::Path clip; clip.addRoundedRectangle (box, 12.0f);
        g.reduceClipRegion (clip);

        const float fc = getCutoffHz ? getCutoffHz() : 1000.0f;
        const float k = 3.95f * juce::jlimit (0.0f, 0.92f, getResonance ? getResonance() : 0.5f);

        auto magDb = [&] (float f)
        {
            const std::complex<double> s (0.0, (double) (f / fc));
            const auto den = std::pow (1.0 + s, 4) + (double) k;
            return 20.0f * std::log10 ((1.0f + 0.6f * k) / (float) std::abs (den) + 1.0e-9f);
        };
        auto xForF  = [&] (float f)  { return box.getX() + box.getWidth() * std::log (f / 20.0f) / std::log (1000.0f); };
        auto yForDb = [&] (float db) { return box.getY() + box.getHeight() * (0.09f + (22.0f - db) / 78.0f); };

        g.setColour (t.grid);
        for (int i = 1; i < 6; ++i)
            g.fillRect (box.getX() + box.getWidth() * (float) i / 6.0f, box.getY(), 1.0f, box.getHeight());
        g.fillRect (box.getX(), yForDb (0.0f), box.getWidth(), 1.0f);

        juce::Path curve;
        const int n = 240;
        for (int i = 0; i <= n; ++i)
        {
            const float f = 20.0f * std::pow (1000.0f, (float) i / (float) n);
            const float x = xForF (f), y = juce::jlimit (box.getY() + 4.0f, box.getBottom() + 6.0f, yForDb (magDb (f)));
            if (i == 0) curve.startNewSubPath (x, y); else curve.lineTo (x, y);
        }
        juce::Path area (curve);
        area.lineTo (box.getRight(), box.getBottom() + 8.0f);
        area.lineTo (box.getX(), box.getBottom() + 8.0f);
        area.closeSubPath();
        g.setColour (t.displayFill.withAlpha (0.85f));
        g.fillPath (area);

        const float xc = xForF (fc);
        juce::Path dash; dash.startNewSubPath (xc, box.getY()); dash.lineTo (xc, box.getBottom());
        juce::Path dashed;
        const float pattern[] = { 3.0f, 4.0f };
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, dash, pattern, 2);
        g.setColour (t.label.withAlpha (0.55f));
        g.fillPath (dashed);

        g.setColour (t.active);
        g.strokePath (curve, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const juce::String hz = fc < 1000.0f ? juce::String (juce::roundToInt (fc)) + " Hz" : juce::String (fc / 1000.0f, 1) + " kHz";
        g.setFont (skin.font (12.0f, Weight::extraBold));
        const float peakY = juce::jlimit (box.getY() + 6.0f, box.getBottom() - 20.0f, yForDb (magDb (fc)) - 2.0f);
        const bool flip = xc > box.getRight() - 70.0f;
        g.drawText (hz, juce::Rectangle<float> (flip ? xc - 66.0f : xc + 6.0f, peakY, 60.0f, 16.0f),
                    flip ? juce::Justification::centredRight : juce::Justification::centredLeft, false);
        g.setColour (t.label.withAlpha (0.8f));
        g.setFont (skin.font (10.5f, Weight::bold));
        g.drawText ("20 Hz", box.reduced (10.0f, 6.0f), juce::Justification::bottomLeft, false);
        g.drawText ("20 kHz", box.reduced (10.0f, 6.0f), juce::Justification::bottomRight, false);
        g.restoreState();

        g.setColour (t.wellBorder);
        g.drawRoundedRectangle (box, 12.0f, 1.0f);
    }

private:
    const Skin& skin;
};

// ============================================================ preset bar
class PresetBar : public juce::Component
{
public:
    PresetBar (const Skin& s) : skin (s)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
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

    static constexpr int arrowW = 40, gap = 6;

    void startSave()
    {
        const auto& t = skin.theme;
        nameEditor.setFont (skin.font (17.0f, Weight::extraBold));
        nameEditor.setColour (juce::TextEditor::backgroundColourId, t.well);
        nameEditor.setColour (juce::TextEditor::textColourId, t.active);
        nameEditor.setColour (juce::TextEditor::highlightColourId, t.active.withAlpha (0.3f));
        nameEditor.setColour (juce::TextEditor::outlineColourId, t.active);
        nameEditor.setColour (juce::TextEditor::focusedOutlineColourId, t.active);
        nameEditor.setColour (juce::CaretComponent::caretColourId, t.active);
        nameEditor.setText ("My Sound", false);
        nameEditor.setVisible (true);
        nameEditor.grabKeyboardFocus();
        nameEditor.selectAll();
        repaint();
    }

    void resized() override
    {
        nameEditor.setBounds (displayBounds().reduced (54, 6));
    }

    void paint (juce::Graphics& g) override
    {
        const auto& t = skin.theme;
        auto b = getLocalBounds().toFloat();
        const auto prev = b.removeFromLeft ((float) arrowW).reduced (0.5f);
        const auto next = b.removeFromRight ((float) arrowW).reduced (0.5f);

        auto arrow = [&] (juce::Rectangle<float> box, bool left)
        {
            fillBox (g, box, 10.0f, t.panel, t.wellBorder, 1.0f);
            juce::Path chevron;
            const auto c = box.getCentre();
            const float s = 5.5f;
            if (left) { chevron.startNewSubPath (c.x + s * 0.5f, c.y - s); chevron.lineTo (c.x - s * 0.5f, c.y); chevron.lineTo (c.x + s * 0.5f, c.y + s); }
            else      { chevron.startNewSubPath (c.x - s * 0.5f, c.y - s); chevron.lineTo (c.x + s * 0.5f, c.y); chevron.lineTo (c.x - s * 0.5f, c.y + s); }
            g.setColour (t.text);
            g.strokePath (chevron, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        };
        arrow (prev, true);
        arrow (next, false);

        auto disp = displayBounds().toFloat().reduced (0.5f);
        fillBox (g, disp, 10.0f, t.well, t.wellBorder, 1.0f);

        const int idx = getIndex ? getIndex() : 0;
        const int factory = getFactoryCount ? getFactoryCount() : 1000;
        auto text = disp.reduced (14.0f, 0.0f);
        g.setColour (t.label);
        g.setFont (skin.font (13.5f, Weight::bold));
        const auto number = nameEditor.isVisible() ? juce::String ("SAVE")
                          : idx < factory ? juce::String (idx + 1).paddedLeft ('0', 2)
                                          : "U" + juce::String (idx - factory + 1);
        g.drawText (number, text.removeFromLeft (30.0f), juce::Justification::centredLeft, false);

        const float cx = disp.getRight() - 20.0f, cy = disp.getCentreY();
        juce::Path dd; dd.startNewSubPath (cx - 4.5f, cy - 2.0f); dd.lineTo (cx, cy + 2.5f); dd.lineTo (cx + 4.5f, cy - 2.0f);
        g.setColour (t.text);
        g.strokePath (dd, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (! nameEditor.isVisible())
        {
            g.setColour (t.active);
            g.setFont (skin.font (17.5f, Weight::extraBold));
            g.drawText (titleCase (getName ? getName (idx) : juce::String()), text.withTrimmedRight (30.0f),
                        juce::Justification::centredLeft, true);
        }
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! getCount || ! load || ! getIndex || nameEditor.isVisible()) return;
        const int n = getCount(), idx = getIndex();
        if (e.position.x < (float) arrowW)                     load ((idx + n - 1) % n);
        else if (e.position.x > (float) (getWidth() - arrowW)) load ((idx + 1) % n);
        else showMenu();
        repaint();
    }

private:
    enum { saveId = 100000, folderId = 100001 };

    juce::Rectangle<int> displayBounds() const { return getLocalBounds().reduced (arrowW + gap, 0); }

    void showMenu()
    {
        const int n = getCount(), idx = getIndex();
        const int factory = getFactoryCount ? getFactoryCount() : n;

        juce::PopupMenu menu;
        menu.addItem (saveId, "Save current sound...");
        menu.addSeparator();
        menu.addSectionHeader ("FACTORY");
        for (int i = 0; i < factory; ++i)
            menu.addItem (i + 1, juce::String (i + 1).paddedLeft ('0', 2) + "   " + titleCase (getName (i)), true, i == idx);
        menu.addSectionHeader ("YOUR PRESETS");
        if (n == factory) menu.addItem (-1, "(none yet: use Save current sound)", false);
        for (int i = factory; i < n; ++i)
            menu.addItem (i + 1, "U" + juce::String (i - factory + 1) + "   " + titleCase (getName (i)), true, i == idx);
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
    juce::TextEditor nameEditor;
};

// ============================================================ output meter (LED rows) + volume
class OutMeter : public juce::Component, public juce::SettableTooltipClient
{
public:
    OutMeter (const Skin& s, juce::RangedAudioParameter& volumeParam)
        : skin (s), param (volumeParam),
          attachment (volumeParam, [this] (float v) { volume = v; repaint(); })
    {
        attachment.sendInitialUpdate();
        setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
        setTooltip ("Output level. Drag left/right to set the volume (double-click resets)");
    }

    void setLevel (float peakLinear) { level = peakLinear; repaint(); }

    static constexpr int cells = 24;

    void paint (juce::Graphics& g) override
    {
        const auto& t = skin.theme;
        auto r = getLocalBounds().toFloat();
        auto caption = r.removeFromLeft (40.0f);
        auto readout = r.removeFromRight (64.0f);
        meter = r.withSizeKeepingCentre (r.getWidth() - 6.0f, 16.0f);

        g.setColour (t.label);
        g.setFont (skin.caption());
        g.drawText ("OUT", caption, juce::Justification::centredLeft, false);

        const float db = juce::Decibels::gainToDecibels (level, -60.0f);
        const float lit = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f) * (float) cells;
        const float cw = meter.getWidth() / (float) cells;
        for (int row = 0; row < 2; ++row)
            for (int i = 0; i < cells; ++i)
            {
                const juce::Rectangle<float> cell (meter.getX() + cw * (float) i, meter.getY() + 9.0f * (float) row, cw - 1.6f, 7.0f);
                g.setColour ((float) i < lit ? t.active : t.meterOff);
                g.fillRoundedRectangle (cell, 1.2f);
            }

        const float x = meter.getX() + meter.getWidth() * volume;
        g.setColour (t.text);
        g.fillRoundedRectangle (x - 1.25f, meter.getY() - 4.0f, 2.5f, meter.getHeight() + 8.0f, 1.2f);

        const juce::String minus = juce::String::fromUTF8 ("\xe2\x88\x92");
        g.setFont (skin.font (14.0f, Weight::extraBold));
        g.drawText (db <= -59.9f ? minus + "inf dB" : juce::String (db, 1).replace ("-", minus) + " dB",
                    readout, juce::Justification::centredRight, false);
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
