// Squelch — SEQ page components: step grid, parameter toggle pills, drag-MIDI button.
#pragma once

#include "Ui.h"
#include "Sequencer.h"

namespace squelch::ui
{

// A pill button bound to an on/off parameter.
class ParamPill : public PillButton
{
public:
    ParamPill (const Skin& s, const juce::String& label, juce::RangedAudioParameter& p)
        : PillButton (s, label),
          attachment (p, [this] (float v) { setToggleState (v > 0.5f, juce::dontSendNotification); })
    {
        attachment.sendInitialUpdate();
        onClick = [this] { attachment.setValueAsCompleteGesture (getToggleState() ? 0.0f : 1.0f); };
    }

private:
    juce::ParameterAttachment attachment;
};

// Drag this onto an FL channel / playlist to drop the pattern in as a MIDI clip.
class DragMidiButton : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit DragMidiButton (const Skin& s) : skin (s)
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        setTooltip ("Drag me into FL (piano roll, channel or playlist) to drop the pattern in as MIDI");
    }

    std::function<juce::File()> makeFile;

    void paint (juce::Graphics& g) override
    {
        const auto& t = skin.theme;
        auto r = getLocalBounds().toFloat().withTrimmedRight (shadowOff).withTrimmedBottom (shadowOff);
        drawStickerBox (g, r, r.getHeight() * 0.5f, t.button, t.buttonOutline, 2.5f, t.shadow, shadowOff);

        // little "MIDI file" icon
        auto icon = r.removeFromLeft (r.getHeight()).reduced (11.0f, 9.0f).translated (8.0f, 0.0f);
        g.setColour (t.accent);
        g.drawRoundedRectangle (icon, 2.5f, 2.0f);
        for (int i = 0; i < 3; ++i)
            g.fillRect (icon.getX() + 3.0f, icon.getY() + 4.0f + 4.5f * (float) i, icon.getWidth() - 6.0f, 2.0f);

        g.setColour (t.buttonText);
        g.setFont (skin.font (15.0f));
        g.drawText ("DRAG MIDI", r.withTrimmedRight (10.0f), juce::Justification::centred, false);
    }

    void mouseDown (const juce::MouseEvent&) override { dragging = false; }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 6 || ! makeFile) return;
        dragging = true;
        const auto file = makeFile();
        if (file.existsAsFile())
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this);
    }

private:
    const Skin& skin;
    bool dragging = false;
};

// The 16-step grid: NOTE / OCT / ACC / SLIDE / ON rows.
class StepGrid : public juce::Component, public juce::SettableTooltipClient
{
public:
    StepGrid (const Skin& s, squelch::Sequencer& sq) : skin (s), seq (sq)
    {
        setTooltip ("NOTE: drag up/down or scroll. OCT: click to change. ACC / SLIDE / ON: click, or click and drag across steps");
    }

    std::function<int()> getLength;

    enum Row { rowLed, rowNote, rowOct, rowAcc, rowSlide, rowOn, numRows };

    void paint (juce::Graphics& g) override
    {
        const auto& t = skin.theme;
        const int len = getLength ? getLength() : 16;
        const int playing = seq.currentStep.load();

        static const char* rowNames[] = { "", "NOTE", "OCT", "ACC", "SLIDE", "ON" };
        g.setFont (skin.font (12.5f));
        for (int r = rowNote; r < numRows; ++r)
        {
            g.setColour (t.valueText);
            g.drawText (rowNames[r], rowBounds (r).withX (0.0f).withWidth (labelW - 6.0f),
                        juce::Justification::centredLeft, false);
        }

        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

        for (int i = 0; i < squelch::maxSteps; ++i)
        {
            const auto st = seq.getStep (i);
            const float alpha = i < len ? 1.0f : 0.28f;

            // step light + number
            {
                auto led = cell (rowLed, i);
                const bool lit = i == playing && i < len;
                const auto dot = led.withSizeKeepingCentre (10.0f, 10.0f).withX (led.getX() + 2.0f);
                g.setColour ((lit ? t.accent : t.button).withMultipliedAlpha (alpha));
                g.fillEllipse (dot);
                g.setColour (t.knobOutline.withMultipliedAlpha (alpha));
                g.drawEllipse (dot, 1.5f);
                g.setColour (t.valueText.withMultipliedAlpha (alpha));
                g.setFont (skin.font (11.5f, false));
                g.drawText (juce::String (i + 1), led.withTrimmedLeft (14.0f), juce::Justification::centredLeft, false);
            }

            // note
            {
                auto c = cell (rowNote, i);
                g.setColour (t.display.withMultipliedAlpha (alpha));
                g.fillRoundedRectangle (c, 7.0f);
                g.setColour ((i == playing && i < len ? t.accent : t.displayOutline).withMultipliedAlpha (alpha));
                g.drawRoundedRectangle (c.reduced (1.0f), 7.0f, i == playing && i < len ? 2.5f : 1.5f);
                g.setColour ((st.on ? t.accent : t.accent.withAlpha (0.4f)).withMultipliedAlpha (alpha));
                g.setFont (skin.font (st.on ? 17.0f : 15.0f));
                g.drawText (st.on ? juce::String (names[st.note]) : juce::String ("-"), c, juce::Justification::centred, false);
            }

            // octave
            {
                auto c = cell (rowOct, i);
                drawToggleCell (g, c, st.oct != 0, alpha, st.oct < 0 ? "DN" : (st.oct > 0 ? "UP" : "-"));
            }
            drawToggleCell (g, cell (rowAcc, i),   st.accent, alpha, st.accent ? "A" : "");
            drawToggleCell (g, cell (rowSlide, i), st.slide,  alpha, st.slide ? "S" : "");
            drawToggleCell (g, cell (rowOn, i),    st.on,     alpha, "");
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        hit = hitTestCell (e.position);
        if (hit.row < 0) return;

        auto st = seq.getStep (hit.step);
        switch (hit.row)
        {
            case rowNote:
                dragStartY = e.position.y;
                dragStartValue = st.oct * 12 + st.note;
                if (! st.on) { st.on = true; seq.setStep (hit.step, st); }
                break;
            case rowOct:
                st.oct = st.oct == 0 ? 1 : (st.oct == 1 ? -1 : 0);
                seq.setStep (hit.step, st);
                break;
            case rowAcc: case rowSlide: case rowOn:
                paintValue = ! getFlag (st, hit.row);
                setFlag (st, hit.row, paintValue);
                seq.setStep (hit.step, st);
                break;
            default: break;
        }
        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (hit.row == rowNote)
        {
            const int delta = (int) std::round ((dragStartY - e.position.y) / 7.0f);
            setNoteValue (hit.step, dragStartValue + delta);
        }
        else if (hit.row == rowAcc || hit.row == rowSlide || hit.row == rowOn)
        {
            const auto h = hitTestCell (e.position.withY (rowBounds (hit.row).getCentreY()));
            if (h.step >= 0)
            {
                auto st = seq.getStep (h.step);
                if (getFlag (st, hit.row) != paintValue) { setFlag (st, hit.row, paintValue); seq.setStep (h.step, st); repaint(); }
            }
        }
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        const auto h = hitTestCell (e.position);
        if (h.row != rowNote || std::abs (w.deltaY) < 1.0e-6f) return;
        const auto st = seq.getStep (h.step);
        setNoteValue (h.step, st.oct * 12 + st.note + (w.deltaY > 0 ? 1 : -1));
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        const auto h = hitTestCell (e.position);
        if (h.row == rowNote) setNoteValue (h.step, 0); // back to C
    }

private:
    struct Hit { int row = -1, step = -1; };

    static constexpr float labelW = 52.0f, gap = 6.0f;
    static constexpr float rowY[numRows + 1] = { 0.0f, 16.0f, 60.0f, 86.0f, 112.0f, 138.0f, 164.0f };

    float colW() const { return ((float) getWidth() - labelW - 3.0f * gap) / (float) squelch::maxSteps; }

    juce::Rectangle<float> rowBounds (int r) const
    {
        return { labelW, rowY[r], (float) getWidth() - labelW, rowY[r + 1] - rowY[r] - 4.0f };
    }

    juce::Rectangle<float> cell (int r, int i) const
    {
        const float x = labelW + (float) i * colW() + (float) (i / 4) * gap;
        return juce::Rectangle<float> (x, rowY[r], colW(), rowY[r + 1] - rowY[r] - 4.0f).reduced (2.0f, 0.0f);
    }

    Hit hitTestCell (juce::Point<float> p) const
    {
        Hit h;
        for (int r = rowNote; r < numRows; ++r)
            if (p.y >= rowY[r] && p.y < rowY[r + 1]) h.row = r;
        if (h.row < 0 || p.x < labelW) return {};
        for (int i = 0; i < squelch::maxSteps; ++i)
            if (cell (h.row, i).expanded (2.0f, 0.0f).contains (p.x, cell (h.row, i).getCentreY())) h.step = i;
        if (h.step < 0) return {};
        return h;
    }

    void drawToggleCell (juce::Graphics& g, juce::Rectangle<float> c, bool on, float alpha, const juce::String& text)
    {
        const auto& t = skin.theme;
        g.setColour ((on ? t.ring : t.button).withMultipliedAlpha (alpha));
        g.fillRoundedRectangle (c, 6.0f);
        g.setColour (t.knobOutline.withMultipliedAlpha (alpha));
        g.drawRoundedRectangle (c.reduced (0.75f), 6.0f, 1.5f);
        if (text.isNotEmpty())
        {
            g.setColour ((on ? t.onRing : t.buttonText).withMultipliedAlpha (alpha));
            g.setFont (skin.font (12.5f));
            g.drawText (text, c, juce::Justification::centred, false);
        }
    }

    static bool getFlag (const squelch::Step& s, int row)
    {
        return row == rowAcc ? s.accent : (row == rowSlide ? s.slide : s.on);
    }
    static void setFlag (squelch::Step& s, int row, bool v)
    {
        if (row == rowAcc) s.accent = v; else if (row == rowSlide) s.slide = v; else s.on = v;
    }

    void setNoteValue (int step, int value)
    {
        value = juce::jlimit (-12, 23, value);
        auto st = seq.getStep (step);
        st.oct = value < 0 ? -1 : (value >= 12 ? 1 : 0);
        st.note = value - 12 * st.oct;
        st.on = true;
        seq.setStep (step, st);
        repaint();
    }

    const Skin& skin;
    squelch::Sequencer& seq;
    Hit hit;
    float dragStartY = 0.0f;
    int dragStartValue = 0;
    bool paintValue = true;
};

} // namespace squelch::ui
