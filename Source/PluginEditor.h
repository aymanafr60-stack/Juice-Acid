#pragma once

#include "PluginProcessor.h"
#include "Ui.h"
#include "SeqUi.h"

// Everything is laid out on a fixed 1000 x 535 canvas, then scaled to the window size.
class SquelchView : public juce::Component
{
public:
    SquelchView (SquelchProcessor&, squelch::ui::Skin&);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;

    void tick(); // called ~30x per second by the editor
    std::function<void()> onThemeToggle;

    static constexpr int canvasW = 1000, canvasH = 535;

private:
    void updateShapeKnob (int wave);
    void setPage (int page);
    void clearPattern();
    void generateLine (bool small);
    juce::RangedAudioParameter& param (const char* id);

    SquelchProcessor& proc;
    squelch::ui::Skin& skin;

    // oscillator
    squelch::ui::PillButton saw, square, pulse, super;
    squelch::ui::Knob tune, pw, sub, slide;

    // filter + envelope
    squelch::ui::FilterDisplay display;
    squelch::ui::Knob envMod, cutoff, res, decay, accent;

    // drive fx
    squelch::ui::PillButton od, dist, fuzz;
    squelch::ui::LedToggle driveLed;
    squelch::ui::Knob drive, tone, level, mix;

    // header / footer
    squelch::ui::PresetBar presetBar;
    squelch::ui::OutMeter meter;
    squelch::ui::PillButton synthTab, seqTab;

    // seq page
    squelch::ui::PillButton seqOff, seqPattern, seqArp;
    squelch::ui::ParamPill play;
    squelch::ui::PillButton clear, generate, mutate;
    squelch::ui::DragMidiButton dragMidi;
    squelch::ui::StepGrid grid;
    squelch::ui::Knob length, rate, swing, gate, bpm, key, scale;
    squelch::ui::ParamPill latch, chord;
    squelch::ui::Knob arpMode, arpOct, arpAcc, arpSlide;
    squelch::ui::PillButton print;

    // these bind buttons to parameters, so they come after the buttons
    squelch::ui::ChoiceGroup waveGroup, modeGroup, seqModeGroup;

    std::vector<juce::Component*> synthPage, seqPage;
    int page = 0;
    int seqModeShown = -1;
    int lastPrintOk = 0, lastPrintFail = 0;
    juce::String status;
    juce::uint32 statusUntil = 0;

    int currentWave = 0;
    int lastPresetShown = -1;
    float meterLevel = 0.0f;
};

class SquelchEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SquelchEditor (SquelchProcessor&);
    ~SquelchEditor() override;

    void resized() override;

private:
    void timerCallback() override { view.tick(); }
    void applyTheme();

    SquelchProcessor& proc;
    squelch::ui::Skin skin;
    SquelchView view;
    juce::TooltipWindow tooltips { this, 600 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SquelchEditor)
};
