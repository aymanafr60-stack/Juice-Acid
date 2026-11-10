#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "AcidVoice.h"
#include "Drive.h"
#include "Sequencer.h"
#include "Delay.h"
#include "ReverbFx.h"
#include "Presets.h"

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
    int getCurrentProgram() override { const int i = getPresetIndex(); return i < squelch::numFactoryPresets ? i : 0; }
    void setCurrentProgram (int index) override { loadPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- used by the editor
    void loadPreset (int index);       // factory presets first, then user presets
    int  getPresetIndex() const;
    int  getNumPresetsTotal() const;
    juce::String getPresetDisplayName (int index) const;
    bool isUserPreset (int index) const { return index >= squelch::numFactoryPresets; }
    bool saveUserPreset (const juce::String& name);  // saves the current sound, selects it
    juce::File getUserPresetFolder() const;
    void refreshUserPresets();
    int  getTheme() const;          // 0 = phosphor lab, 1 = matrix, 2 = kawaii
    void setTheme (int theme);
    int  getPage() const;           // 0 = synth, 1 = seq, 2 = fx
    void setPage (int page);

    squelch::Sequencer seq;
    juce::File writePatternMidi() const; // for dragging the pattern into the DAW

    std::atomic<float> displayCutoffHz { 1000.0f }; // live filter cutoff for the display
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<float> hostTempo { 0.0f };

    // last output samples for the on-screen scope (written by the audio thread, read by the editor)
    static constexpr int scopeSize = 4096;
    float scopeBuffer[scopeSize] = {};
    std::atomic<int> scopeWrite { 0 };
    // copies the most recent `n` samples (oldest first) into dest
    void readScope (float* dest, int n) const
    {
        const int w = scopeWrite.load (std::memory_order_acquire);
        for (int i = 0; i < n; ++i) dest[i] = scopeBuffer[(w - n + i + scopeSize * 2) % scopeSize];
    }

    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    void updateParams();
    void resetSoundToDefaults();
    bool isSoundParam (const juce::String& id) const;
    juce::Array<juce::File> userPresetFiles;
    squelch::SeqSettings readSeqSettings() const;

    struct VoiceSink : squelch::NoteSink
    {
        squelch::AcidVoice* v = nullptr;
        void seqNoteOn (int n, int vel) override { v->noteOn (n, vel); }
        void seqNoteOff (int n) override         { v->noteOff (n); }
    } sink;

    squelch::AcidVoice voice;
    squelch::DriveStage drive;
    juce::dsp::Oversampling<float> oversampling { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    juce::AudioBuffer<float> monoBuffer;
    squelch::PingPongDelay delay;
    squelch::ReverbFx reverb;

    std::atomic<float>* pWave = nullptr;   std::atomic<float>* pPw = nullptr;
    std::atomic<float>* pTune = nullptr;   std::atomic<float>* pCutoff = nullptr;
    std::atomic<float>* pRes = nullptr;    std::atomic<float>* pEnvMod = nullptr;
    std::atomic<float>* pDecay = nullptr;  std::atomic<float>* pAccent = nullptr;
    std::atomic<float>* pSlide = nullptr;  std::atomic<float>* pSub = nullptr;
    std::atomic<float>* pDriveOn = nullptr; std::atomic<float>* pDriveMode = nullptr;
    std::atomic<float>* pDrive = nullptr;  std::atomic<float>* pTone = nullptr;
    std::atomic<float>* pLevel = nullptr;  std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pVolume = nullptr;

    std::atomic<float>* sMode = nullptr;  std::atomic<float>* sPlay = nullptr;
    std::atomic<float>* sLen = nullptr;   std::atomic<float>* sRate = nullptr;
    std::atomic<float>* sSwing = nullptr; std::atomic<float>* sGate = nullptr;
    std::atomic<float>* sBpm = nullptr;   std::atomic<float>* sKey = nullptr;
    std::atomic<float>* sScale = nullptr; std::atomic<float>* aMode = nullptr;
    std::atomic<float>* aOct = nullptr;   std::atomic<float>* aAcc = nullptr;
    std::atomic<float>* aSlide = nullptr; std::atomic<float>* aLatch = nullptr;
    std::atomic<float>* aChord = nullptr;

    std::atomic<float>* mSoftAtk = nullptr; std::atomic<float>* mAccDecay = nullptr;
    std::atomic<float>* mTrack = nullptr;   std::atomic<float>* mFm = nullptr;
    std::atomic<float>* mSweep = nullptr;
    std::atomic<float>* dOn = nullptr;   std::atomic<float>* dTime = nullptr;
    std::atomic<float>* dFb = nullptr;   std::atomic<float>* dTone = nullptr;
    std::atomic<float>* dMix = nullptr;
    std::atomic<float>* rOn = nullptr;   std::atomic<float>* rSize = nullptr;
    std::atomic<float>* rDamp = nullptr; std::atomic<float>* rMix = nullptr;
    std::atomic<float>* rPre = nullptr;  std::atomic<float>* rLowCut = nullptr;
    std::atomic<float>* rDuck = nullptr;
    std::atomic<float>* dPing = nullptr; std::atomic<float>* dSync = nullptr;
    std::atomic<float>* dMs = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SquelchProcessor)
};
