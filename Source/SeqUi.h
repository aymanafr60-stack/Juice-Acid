// Squelch — SEQ page components: step grid and the drag-MIDI button (hardware style, v0.8).
#pragma once

#include "Ui.h"
#include "Sequencer.h"

namespace squelch::ui
{

// Drag this onto FL's piano roll / channel rack / playlist to drop the pattern in as MIDI.
class DragMidiButton : public Btn
{
public:
    explicit DragMidiButton (const Skin& s) : Btn (s, "DRAG MIDI", false, Icon::download)
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        setTooltip ("Drag me into FL (piano roll, channel or playlist) to drop the pattern in as MIDI");
    }

    std::function<juce::File()> makeFile;

    void mouseDown (const juce::MouseEvent& e) override { dragging = false; Btn::mouseDown (e); }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 6 || ! makeFile) return;
        dragging = true;
        const auto file = makeFile();
        if (file.existsAsFile())
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this);
    }

private:
    bool dragging = false;
};

// The 16-step grid: number / NOTE / OCT / ACCENT / SLIDE / GATE, in four groups of four.
class StepGrid : public juce::Component, public juce::SettableTooltipClient
{
public:
    StepGrid (const Skin& s, squelch::Sequencer& sq) : skin (s), seq (sq)
    {
        setTooltip ("NOTE: drag up/down or scroll (double-click = C). OCT: click. ACCENT / SLIDE / GATE: click, or click and drag across steps");
    }

    std::function<int()> getLength;

    enum Row { rowNum, rowNote, rowOct, rowAcc, rowSlide, rowGate, numRows };

    void paint (juce::Graphics& g) override
    {
        const auto& t = skin.theme;
        const int len = getLength ? getLength() : 16;
        const int playing = seq.currentStep.load();

        static const char* rowNames[] = { "", "NOTE", "OCT", "ACCENT", "SLIDE", "GATE" };
        g.setFont (skin.label (9.5f, 0.14f));
        g.setColour (t.ink);
        for (int r = rowNote; r < numRows; ++r)
            g.drawText (rowNames[r], juce::Rectangle<float> (0.0f, rowTop[r] - 3.0f, labelW, rowH[r] + 6.0f), juce::Justification::centredLeft, false);

        // the four groups: recessed plates
        for (int grp = 0; grp < 4; ++grp)
        {
            const auto gb = groupBounds (grp);
            g.setColour (t.kind == Theme::Kind::kawaii ? Colour (0x80ffe3ef) : (t.kind == Theme::Kind::matrix ? Colour (0x5903140a) : Colour (0x263a1236)));
            g.fillRoundedRectangle (gb, 8.0f);
            g.setColour (t.sectionBorder.withMultipliedAlpha (0.6f));
            g.drawRoundedRectangle (gb, 8.0f, 1.0f);
        }

        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const juce::String minus = juce::String::fromUTF8 ("\xe2\x88\x92");

        for (int i = 0; i < squelch::maxSteps; ++i)
        {
            const auto st = seq.getStep (i);
            const bool inLen = i < len;
            const bool now = i == playing && inLen;
            const float a = inLen ? 1.0f : 0.35f;
            juce::Graphics::ScopedSaveState save (g);
            if (! inLen) g.beginTransparencyLayer (a);

            // step number with the "now playing" light
            {
                auto c = cell (rowNum, i);
                drawLed (g, juce::Rectangle<float> (c.getX() + 2.0f, c.getCentreY() - 3.0f, 6.0f, 6.0f), now, t);
                g.setColour (now ? (t.kind == Theme::Kind::kawaii ? t.title : t.ink) : t.ink2);
                g.setFont (skin.mono (11.0f, true, 0.05f));
                if (t.roundedFont) g.setFont (skin.label (11.0f, 0.05f));
                g.drawText (juce::String (i + 1).paddedLeft ('0', 2), c.withTrimmedLeft (10.0f), juce::Justification::centred, false);
            }
            // note: a little display window
            {
                auto c = cell (rowNote, i);
                g.setColour (now ? t.phos.withAlpha (0.8f) : t.screenRing.withMultipliedAlpha (0.8f));
                g.drawRoundedRectangle (c.expanded (1.5f), 6.0f, now ? 1.6f : 1.0f);
                g.setColour (t.readoutBg);
                g.fillRoundedRectangle (c, 5.0f);
                if (now) { g.setColour (t.phos.withAlpha (0.12f)); g.fillRoundedRectangle (c, 5.0f); }
                const auto txt = st.on ? juce::String (names[st.note]) : juce::String::fromUTF8 ("\xe2\x80\x93");
                g.setFont (skin.readout (st.on ? 19.0f : 16.0f));
                g.setColour ((st.on ? t.phos : t.phos.withAlpha (0.35f)).withMultipliedAlpha (0.3f));
                g.drawText (txt, c.translated (0.0f, 0.6f).expanded (1.0f, 0.0f), juce::Justification::centred, false);
                g.setColour (st.on ? t.phos : t.phos.withAlpha (0.35f));
                g.drawText (txt, c, juce::Justification::centred, false);
                g.setColour (juce::Colours::black.withAlpha (0.25f));
                for (float y = c.getY() + 2.0f; y < c.getBottom() - 1.0f; y += 3.0f) g.fillRect (c.getX() + 2.0f, y, c.getWidth() - 4.0f, 1.0f);
            }
            // octave / accent / slide: little keys that light up
            auto smallKey = [&] (int row, bool on, const juce::String& text, bool slideIcon)
            {
                auto c = cell (row, i).withTrimmedBottom (3.0f);
                auto tt = t; tt.keyRadius = std::min (t.keyRadius, 7.0f);
                drawKey (g, c, on, false, tt);
                const auto ink = on ? t.keyOnText : t.keyText.withMultipliedAlpha (0.55f);
                if (slideIcon)
                {
                    if (! on) return;
                    juce::Path p; const auto m = c.getCentre();
                    p.startNewSubPath (m.x - 10.0f, m.y + 4.0f);
                    p.cubicTo (m.x - 3.0f, m.y + 4.0f, m.x + 2.0f, m.y - 4.0f, m.x + 10.0f, m.y - 4.0f);
                    g.setColour (ink);
                    g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                    return;
                }
                if (text.isEmpty()) return;
                g.setColour (ink);
                g.setFont (skin.label (10.5f, 0.05f));
                g.drawText (text, c, juce::Justification::centred, false);
            };
            smallKey (rowOct, st.oct != 0, st.oct < 0 ? minus + "1" : (st.oct > 0 ? juce::String ("+1") : juce::String()), false);
            smallKey (rowAcc, st.accent, st.accent ? "A" : "", false);
            smallKey (rowSlide, st.slide, "", true);

            // gate: an LED bar (plays / rest)
            {
                auto c = cell (rowGate, i);
                if (st.on)
                {
                    const auto col = t.kind == Theme::Kind::kawaii ? (now ? t.title : t.keyOnMid) : (now ? t.phos : t.meterOn);
                    g.setColour (col.withAlpha (now ? 0.45f : 0.25f)); g.fillRoundedRectangle (c.expanded (2.0f), 4.0f);
                    g.setColour (col); g.fillRoundedRectangle (c, 2.5f);
                }
                else
                {
                    g.setColour (t.kind == Theme::Kind::kawaii ? Colour (0xffffd0e4) : t.meterOff); g.fillRoundedRectangle (c, 2.5f);
                }
            }
            if (! inLen) g.endTransparencyLayer();
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
            case rowAcc: case rowSlide: case rowGate:
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
        else if (hit.row == rowAcc || hit.row == rowSlide || hit.row == rowGate)
        {
            const auto h = hitTestCell ({ e.position.x, rowTop[hit.row] + rowH[hit.row] * 0.5f });
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
        if (h.row == rowNote) setNoteValue (h.step, 0);
    }

private:
    struct Hit { int row = -1, step = -1; };

    // geometry (component is 926 x 172)
    static constexpr float labelW = 60.0f, groupW = 208.0f, groupGap = 8.0f, cellW = 42.0f, cellPitch = 50.0f, cellPad = 9.0f;
    static constexpr float rowTop[numRows] = { 4.0f, 28.0f, 72.0f, 98.0f, 128.0f, 160.0f };
    static constexpr float rowH[numRows]   = { 18.0f, 38.0f, 23.0f, 27.0f, 27.0f, 7.0f };

    juce::Rectangle<float> groupBounds (int grp) const
    {
        return { labelW + (float) grp * (groupW + groupGap), 0.0f, groupW, 172.0f };
    }

    juce::Rectangle<float> cell (int r, int i) const
    {
        const auto gb = groupBounds (i / 4);
        return { gb.getX() + cellPad + (float) (i % 4) * cellPitch, rowTop[r], cellW, rowH[r] };
    }

    Hit hitTestCell (juce::Point<float> p) const
    {
        Hit h;
        for (int r = rowNote; r < numRows; ++r)
            if (p.y >= rowTop[r] - 3.0f && p.y < rowTop[r] + rowH[r] + 3.0f) h.row = r;
        if (h.row < 0) return {};
        for (int i = 0; i < squelch::maxSteps; ++i)
        {
            const auto c = cell (h.row, i);
            if (p.x >= c.getX() - 4.0f && p.x < c.getRight() + 4.0f) h.step = i;
        }
        if (h.step < 0) return {};
        return h;
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
