#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout SquelchProcessor::createLayout()
{
    using namespace juce;
    APVTS::ParameterLayout layout;

    auto pct = [] (float v, int) { return String (juce::roundToInt (v * 100.0f)) + " %"; };

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "wave", 1 }, "Waveform",
                                                        StringArray { "Saw", "Square" }, 0));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "tune", 1 }, "Tune",
                                                       NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f,
                                                       AudioParameterFloatAttributes().withLabel ("st")));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "cutoff", 1 }, "Cutoff",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.35f,
                                                       AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "res", 1 }, "Resonance",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.6f,
                                                       AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "envmod", 1 }, "Env Mod",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.5f,
                                                       AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "decay", 1 }, "Decay",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.4f,
                                                       AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "accent", 1 }, "Accent",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.6f,
                                                       AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "slide", 1 }, "Slide Time",
                                                       NormalisableRange<float> (10.0f, 500.0f, 0.1f, 0.4f), 60.0f,
                                                       AudioParameterFloatAttributes().withLabel ("ms")));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "sub", 1 }, "Sub Osc",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.0f,
                                                       AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "volume", 1 }, "Volume",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.7f,
                                                       AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    return layout;
}

SquelchProcessor::SquelchProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pWave   = apvts.getRawParameterValue ("wave");
    pTune   = apvts.getRawParameterValue ("tune");
    pCutoff = apvts.getRawParameterValue ("cutoff");
    pRes    = apvts.getRawParameterValue ("res");
    pEnvMod = apvts.getRawParameterValue ("envmod");
    pDecay  = apvts.getRawParameterValue ("decay");
    pAccent = apvts.getRawParameterValue ("accent");
    pSlide  = apvts.getRawParameterValue ("slide");
    pSub    = apvts.getRawParameterValue ("sub");
    pVolume = apvts.getRawParameterValue ("volume");
}

bool SquelchProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void SquelchProcessor::prepareToPlay (double sampleRate, int)
{
    voice.prepare (sampleRate);
    keyboardState.reset();
}

void SquelchProcessor::updateVoiceParams()
{
    squelch::VoiceParams p;
    p.waveform    = (int) pWave->load();
    p.tuneSemis   = pTune->load();
    p.cutoff      = pCutoff->load();
    p.resonance   = pRes->load();
    p.envMod      = pEnvMod->load();
    p.decay       = pDecay->load();
    p.accent      = pAccent->load();
    p.slideTimeMs = pSlide->load();
    p.subLevel    = pSub->load();
    p.volume      = pVolume->load();
    voice.setParams (p);
}

void SquelchProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    // merge notes played on the on-screen keyboard
    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    updateVoiceParams();

    auto* left = buffer.getWritePointer (0);
    int pos = 0;

    // sample-accurate MIDI: render up to each event, then apply it
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const int evPos = juce::jlimit (0, numSamples, meta.samplePosition);

        for (; pos < evPos; ++pos)
            left[pos] = voice.processSample();

        if (msg.isNoteOn())
            voice.noteOn (msg.getNoteNumber(), msg.getVelocity());
        else if (msg.isNoteOff())
            voice.noteOff (msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            voice.allNotesOff();
    }

    for (; pos < numSamples; ++pos)
        left[pos] = voice.processSample();

    // mono synth -> copy to the other channel(s)
    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
}

void SquelchProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SquelchProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* SquelchProcessor::createEditor()
{
    return new SquelchEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SquelchProcessor();
}
