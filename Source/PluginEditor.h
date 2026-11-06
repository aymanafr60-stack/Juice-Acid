#pragma once

#include "PluginProcessor.h"

class SquelchLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SquelchLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
};

class SquelchEditor : public juce::AudioProcessorEditor
{
public:
    explicit SquelchEditor (SquelchProcessor&);
    ~SquelchEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void addKnob (Knob& k, const juce::String& paramId, const juce::String& name);

    SquelchProcessor& processor;
    SquelchLookAndFeel lnf;

    Knob cutoff, res, envMod, decay, accent, tune, slide, sub, volume;

    juce::ComboBox waveBox;
    juce::Label    waveLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveAttachment;

    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SquelchEditor)
};
