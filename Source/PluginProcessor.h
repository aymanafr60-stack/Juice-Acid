#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "AcidVoice.h"
#include "Drive.h"

class SquelchProcessor : public juce::AudioProcessor
{
public:
    SquelchProcessor();
    ~SquelchProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    using AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.1; }

    // Factory presets are also exposed to the host's own preset menu.
    int getNumPrograms() override;
    int getCurrentProgram() override { return getPresetIndex(); }
    void setCurrentProgram (int index) override { loadPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- used by the editor
    void loadPreset (int index);
    int  getPresetIndex() const;
    int  getTheme() const;          // 0 = red, 1 = green
    void setTheme (int theme);

    std::atomic<float> displayCutoffHz { 1000.0f }; // live filter cutoff for the display
    std::atomic<float> outputPeak { 0.0f };         // max |sample| since the editor last read it

    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    void updateParams();

    squelch::AcidVoice voice;
    squelch::DriveStage drive;
    juce::dsp::Oversampling<float> oversampling { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    juce::AudioBuffer<float> monoBuffer;

    std::atomic<float>* pWave = nullptr;   std::atomic<float>* pPw = nullptr;
    std::atomic<float>* pTune = nullptr;   std::atomic<float>* pCutoff = nullptr;
    std::atomic<float>* pRes = nullptr;    std::atomic<float>* pEnvMod = nullptr;
    std::atomic<float>* pDecay = nullptr;  std::atomic<float>* pAccent = nullptr;
    std::atomic<float>* pSlide = nullptr;  std::atomic<float>* pSub = nullptr;
    std::atomic<float>* pDriveOn = nullptr; std::atomic<float>* pDriveMode = nullptr;
    std::atomic<float>* pDrive = nullptr;  std::atomic<float>* pTone = nullptr;
    std::atomic<float>* pLevel = nullptr;  std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pVolume = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SquelchProcessor)
};
