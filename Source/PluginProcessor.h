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

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState; // on-screen keyboard

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    void updateVoiceParams();

    squelch::AcidVoice voice;
    squelch::DriveStage drive;
    juce::dsp::Oversampling<float> oversampling { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    juce::AudioBuffer<float> monoBuffer;

    std::atomic<float>* pWave   = nullptr;
    std::atomic<float>* pTune   = nullptr;
    std::atomic<float>* pCutoff = nullptr;
    std::atomic<float>* pRes    = nullptr;
    std::atomic<float>* pEnvMod = nullptr;
    std::atomic<float>* pDecay  = nullptr;
    std::atomic<float>* pAccent = nullptr;
    std::atomic<float>* pSlide  = nullptr;
    std::atomic<float>* pSub    = nullptr;
    std::atomic<float>* pVolume = nullptr;
    std::atomic<float>* pDist   = nullptr;
    std::atomic<float>* pFuzz   = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SquelchProcessor)
};
