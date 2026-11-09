// Squelch — SEQ page components: step grid and the drag-MIDI button (flat style, v0.7).
#pragma once

#include "Ui.h"
#include "Sequencer.h"

namespace squelch::ui
{

// Drag this onto FL's piano roll / channel rack / playlist to drop the pattern in as MIDI.
class DragMidiButton : public Btn
{
public:
    explicit DragMidiButton (const Skin& s) : Btn (s, "DRAG MIDI", Style::outline, Icon::download)
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
        setTooltip ("NOTE: drag up/down or scroll. OCT: click. ACCENT / SLIDE / GATE: click, or click and drag across steps");
    }

    std::function<int()> getLength;

    enum Row { rowNum, rowNote, rowOct, rowAcc, rowSlide, rowGate, numRows };

    void paint (juce::Graphics& g) override
    {
        const auto& t = skin.theme;
        const int len = getLength ? getLength() : 16;
        const int playing = seq.currentStep.load();

        static const char* rowNames[] = { "", "NOTE", "OCT", "ACCENT", "SLIDE", "GATE" };
        g.setFont (skin.caption());
        g.setColour (t.label);
        for (int r = rowNote; r < numRows; ++r)
            g.drawText (rowNames[r], juce::Rectangle<float> (0.0f, rowTop[r], labelW, rowH[r]), juce::Justification::centredLeft, false);

        for (int grp = 0; grp < 4; ++grp)
            fillBox (g, groupBounds (grp), 12.0f, t.group);

        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const juce::String minus = juce::String::fromUTF8 ("\xe2\x88\x92");

        for (int i = 0; i < squelch::maxSteps; ++i)
        {
            const auto st = seq.getStep (i);
            const bool inLen = i < len;
            const bool now = i == playing && inLen;
            const float a = inLen ? 1.0f : 0.3f;

            // step number
            {
                auto c = cell (rowNum, i);
                if (now) fillBox (g, c.withSizeKeepingCentre (c.getWidth(), 17.0f), 6.0f, t.active);
                g.setColour (now ? t.onActive : t.label.withMultipliedAlpha (a));
                g.setFont (skin.font (12.0f, Weight::extraBold));
                g.drawText (juce::String (i + 1), c, juce::Justification::centred, false);
            }
            // note
            {
                auto c = cell (rowNote, i);
                fillBox (g, c, 9.0f, t.well.withMultipliedAlpha (a), (now ? t.active : t.wellBorder).withMultipliedAlpha (a), now ? 1.5f : 1.0f);
                g.setColour ((st.on ? t.text : t.label.withAlpha (0.5f)).withMultipliedAlpha (a));
                g.setFont (skin.font (st.on ? 17.0f : 15.0f, Weight::extraBold));
                g.drawText (st.on ? juce::String (names[st.note]) : juce::String::fromUTF8 ("\xe2\x80\x93"), c,
                            juce::Justification::centred, false);
            }
            // octave
            {
                auto c = cell (rowOct, i);
                if (st.oct != 0) fillBox (g, c, 6.0f, t.octOn.withMultipliedAlpha (a), t.octOnBorder.withMultipliedAlpha (a), 1.0f);
                else             fillBox (g, c, 6.0f, juce::Colours::transparentBlack, t.wellBorder.withMultipliedAlpha (a * 0.8f), 1.0f);
                if (st.oct != 0)
                {
                    g.setColour (t.text.withMultipliedAlpha (a));
                    g.setFont (skin.font (12.5f, Weight::extraBold));
                    g.drawText (st.oct < 0 ? minus + "1" : juce::String ("+1"), c, juce::Justification::centred, false);
                }
            }
            // accent
            {
                auto c = cell (rowAcc, i);
                if (st.accent)
                {
                    fillBox (g, c, 6.0f, t.selected.withMultipliedAlpha (a));
                    g.setColour (t.onSelected.withMultipliedAlpha (a));
                    g.setFont (skin.font (13.0f, Weight::extraBold));
                    g.drawText ("A", c, juce::Justification::centred, false);
                }
                else fillBox (g, c, 6.0f, juce::Colours::transparentBlack, t.wellBorder.withMultipliedAlpha (a * 0.8f), 1.0f);
            }
            // slide
            {
                auto c = cell (rowSlide, i);
                if (st.slide)
                {
                    fillBox (g, c, 6.0f, t.selected.withMultipliedAlpha (a));
                    juce::Path p;
                    const auto m = c.getCentre();
                    p.startNewSubPath (m.x - 10.0f, m.y + 4.0f);
                    p.cubicTo (m.x - 3.0f, m.y + 4.0f, m.x + 2.0f, m.y - 4.0f, m.x + 10.0f, m.y - 4.0f);
                    g.setColour (t.onSelected.withMultipliedAlpha (a));
                    g.strokePath (p, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                }
                else fillBox (g, c, 6.0f, juce::Colours::transparentBlack, t.wellBorder.withMultipliedAlpha (a * 0.8f), 1.0f);
            }
            // gate (plays / rest)
            {
                auto c = cell (rowGate, i);
                if (now && st.on) fillBox (g, c, 3.0f, t.active);
                else if (st.on)   fillBox (g, c, 3.0f, t.gateOn.withMultipliedAlpha (a));
                else              fillBox (g, c, 3.0f, juce::Colours::transparentBlack, t.wellBorder.withMultipliedAlpha (a), 1.0f);
            }
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

    // geometry (component is 898 x 190)
    static constexpr float labelW = 67.0f, groupW = 201.0f, groupGap = 9.0f, cellW = 41.0f, cellPitch = 48.0f, cellPad = 8.0f;
    static constexpr float rowTop[numRows] = { 8.0f, 32.0f, 79.0f, 107.0f, 139.0f, 172.0f };
    static constexpr float rowH[numRows]   = { 18.0f, 39.0f, 21.0f, 25.0f, 25.0f, 9.0f };

    juce::Rectangle<float> groupBounds (int grp) const
    {
        return { labelW + (float) grp * (groupW + groupGap), 0.0f, groupW, 189.0f };
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
            if (p.x >= c.getX() - 3.5f && p.x < c.getRight() + 3.5f) h.step = i;
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
