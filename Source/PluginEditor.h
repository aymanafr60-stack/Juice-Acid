#pragma once

#include "PluginProcessor.h"
#include "Ui.h"
#include "SeqUi.h"

// Everything is laid out on a fixed 1000 x 600 canvas, then scaled to the window size.
class SquelchView : public juce::Component
{
public:
    SquelchView (SquelchProcessor&, squelch::ui::Skin&);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;

    void tick();          // ~30x per second
    void themeChanged();  // rebuild the cached background
    std::function<void()> onThemeToggle;

    static constexpr int canvasW = 1000, canvasH = 600;

private:
    using Btn = squelch::ui::Btn;
    using Knob = squelch::ui::Knob;
    using Stepper = squelch::ui::Stepper;

    juce::RangedAudioParameter& param (const char* id);
    float value (const char* id) const;
    void setPage (int page);
    void refreshVisibility();
    void updateShapeKnob (int wave);
    void clearPattern();
    void generateLine (bool small);
    void paintStatic (juce::Graphics&);   // everything that only changes with theme / page / mode
    void paintStickers (juce::Graphics&);
    void paintStatus (juce::Graphics&);   // live part of the footer

    SquelchProcessor& proc;
    squelch::ui::Skin& skin;

    // header
    Btn tabSynth, tabSeq, tabFx;
    squelch::ui::PresetBar presetBar;
    squelch::ui::OutMeter meter;

    // synth page
    Btn saw, square, pulse, super;
    squelch::ui::SegmentBox waveBox;
    Knob tune, pw, sub, slide;
    squelch::ui::FilterDisplay display;
    Knob envMod, cutoff, res, decay, accent;
    squelch::ui::ToggleSwitch driveSwitch;
    Btn od, dist, fuzz;
    squelch::ui::SegmentBox driveBox;
    Knob drive, tone, level, mix;

    // seq page
    Btn modeOff, modePattern, modeArp;
    squelch::ui::SegmentBox seqModeBox;
    squelch::ui::ParamToggleButton play;
    Btn clear, mutate, generate;
    squelch::ui::DragMidiButton dragMidi;
    squelch::ui::StepGrid grid;
    Stepper lengthStep, rateStep, bpmStep, keyStep, scaleStep;
    Knob swing, gate;
    Btn arpUp, arpDown, arpUpDown, arpRandom, arpPlayed;
    squelch::ui::SegmentBox arpModeBox;
    squelch::ui::ParamToggleButton latch, chord;
    Btn print;
    Stepper arpOctStep, arpAccStep;
    Knob arpSlide;

    // fx page
    Knob softAtk, accDecay, tracking, filterFm;
    Btn sweepSlow, sweepNormal, sweepFast;
    squelch::ui::SegmentBox sweepBox;
    squelch::ui::ToggleSwitch delaySwitch, reverbSwitch;
    Stepper dlyTimeStep, dlyMsStep;
    squelch::ui::ParamToggleButton pingPong, sync;
    Knob dlyFb, dlyTone, dlyMix;
    Knob revSize, revDamp, revMix, revPre, revLowCut, revDuck;

    // parameter bindings for button groups (after the buttons)
    squelch::ui::ChoiceGroup waveGroup, driveModeGroup, seqModeGroup, arpModeGroup, sweepGroup;

    std::vector<juce::Component*> synthPage, seqPage, fxPage, arpOnly, patternOnly;
    int page = 0;
    int shownMode = -1, shownSync = -1;
    int currentWave = 0;
    int lastPresetShown = -1;
    float meterLevel = 0.0f;
    int lastPrintOk = 0, lastPrintFail = 0;
    juce::String toast, lastStatus;
    juce::uint32 toastUntil = 0;

    juce::Image background;          // cached static layer
    float backgroundScale = 0.0f;
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
