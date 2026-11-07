#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using APVTS = juce::AudioProcessorValueTreeState;

static const juce::Identifier presetProp { "preset" };
static const juce::Identifier themeProp  { "theme" };

APVTS::ParameterLayout SquelchProcessor::createLayout()
{
    using namespace juce;
    APVTS::ParameterLayout layout;

    auto pct  = [] (float v, int) { return String (roundToInt (v * 100.0f)) + "%"; };
    auto unit = [] { return NormalisableRange<float> (0.0f, 1.0f); };
    auto addPct = [&] (const char* id, const char* name, float def)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, unit(), def,
                                                           AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    };

    // --- oscillator
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "wave", 1 }, "Waveform",
                                                        StringArray { "Saw", "Square", "Pulse", "Super" }, 0));
    addPct ("pw", "Pulse Width / Detune", 0.5f);
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "tune", 1 }, "Tune",
                    NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                    {
                        const auto r = std::round (v * 10.0f) / 10.0f;
                        return (r >= 0 ? "+" : "") + String (r, std::abs (r - std::round (r)) < 0.05f ? 0 : 1) + " st";
                    })));
    addPct ("sub", "Sub Osc", 0.0f);
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "slide", 1 }, "Slide Time",
                    NormalisableRange<float> (10.0f, 500.0f, 0.1f, 0.4f), 60.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                    { return String (roundToInt (v)) + " ms"; })));

    // --- filter + envelope
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "cutoff", 1 }, "Cutoff", unit(), 0.35f,
                    AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                    {
                        const float hz = 50.0f * std::pow (100.0f, v); // same mapping as the engine
                        return hz < 1000.0f ? String (roundToInt (hz)) + " Hz" : String (hz / 1000.0f, 2) + " kHz";
                    })));
    addPct ("res", "Resonance", 0.6f);
    addPct ("envmod", "Env Mod", 0.5f);
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "decay", 1 }, "Decay", unit(), 0.4f,
                    AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                    {
                        const float ms = 200.0f * std::pow (12.5f, v); // same mapping as the engine
                        return ms < 1000.0f ? String (roundToInt (ms)) + " ms" : String (ms / 1000.0f, 2) + " s";
                    })));
    addPct ("accent", "Accent", 0.6f);

    // --- drive fx
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "drive_on", 1 }, "Drive On", false));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "drive_mode", 1 }, "Drive Mode",
                                                        StringArray { "OD", "DIST", "FUZZ" }, 1));
    addPct ("drive", "Drive", 0.4f);
    addPct ("tone", "Tone", 0.7f);
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "level", 1 }, "Drive Level", unit(), 0.6667f,
                    AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                    {
                        const float db = -12.0f + 18.0f * v;
                        return (db >= 0.05f ? "+" : "") + String (db, 1) + " dB";
                    })));
    addPct ("mix", "Drive Mix", 1.0f);

    // --- output
    addPct ("volume", "Volume", 0.7f);
    return layout;
}

SquelchProcessor::SquelchProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    pWave = get ("wave");   pPw = get ("pw");          pTune = get ("tune");     pCutoff = get ("cutoff");
    pRes = get ("res");     pEnvMod = get ("envmod");  pDecay = get ("decay");   pAccent = get ("accent");
    pSlide = get ("slide"); pSub = get ("sub");
    pDriveOn = get ("drive_on"); pDriveMode = get ("drive_mode"); pDrive = get ("drive");
    pTone = get ("tone");   pLevel = get ("level");    pMix = get ("mix");       pVolume = get ("volume");

    apvts.state.setProperty (presetProp, 0, nullptr);
    apvts.state.setProperty (themeProp, 0, nullptr);
}

bool SquelchProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void SquelchProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    voice.prepare (sampleRate);
    oversampling.initProcessing ((size_t) samplesPerBlock);
    oversampling.reset();
    drive.prepare (sampleRate * (double) oversampling.getOversamplingFactor());
    monoBuffer.setSize (1, samplesPerBlock);
    setLatencySamples ((int) std::round (oversampling.getLatencyInSamples()));
}

void SquelchProcessor::updateParams()
{
    squelch::VoiceParams p;
    p.waveform    = (int) pWave->load();
    p.shape       = pPw->load();
    p.tuneSemis   = pTune->load();
    p.cutoff      = pCutoff->load();
    p.resonance   = pRes->load();
    p.envMod      = pEnvMod->load();
    p.decay       = pDecay->load();
    p.accent      = pAccent->load();
    p.slideTimeMs = pSlide->load();
    p.subLevel    = pSub->load();
    voice.setParams (p);

    squelch::DriveParams d;
    d.on     = pDriveOn->load() > 0.5f;
    d.mode   = (int) pDriveMode->load();
    d.drive  = pDrive->load();
    d.tone   = pTone->load();
    d.level  = pLevel->load();
    d.mix    = pMix->load();
    d.volume = pVolume->load();
    drive.setParams (d);
}

void SquelchProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    updateParams();

    if (monoBuffer.getNumSamples() < numSamples)
        monoBuffer.setSize (1, numSamples, false, false, true); // only if the host breaks its promise

    auto* mono = monoBuffer.getWritePointer (0);
    int pos = 0;

    // sample-accurate MIDI: render up to each event, then apply it
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const int evPos = juce::jlimit (0, numSamples, meta.samplePosition);

        for (; pos < evPos; ++pos)
            mono[pos] = voice.processSample();

        if (msg.isNoteOn())
            voice.noteOn (msg.getNoteNumber(), msg.getVelocity());
        else if (msg.isNoteOff())
            voice.noteOff (msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            voice.allNotesOff();
    }

    for (; pos < numSamples; ++pos)
        mono[pos] = voice.processSample();

    // drive fx + volume at 4x sample rate to keep heavy settings clean
    juce::dsp::AudioBlock<float> block (monoBuffer.getArrayOfWritePointers(), 1, (size_t) numSamples);
    auto up = oversampling.processSamplesUp (block);
    auto* u = up.getChannelPointer (0);
    for (size_t i = 0; i < up.getNumSamples(); ++i)
        u[i] = drive.process (u[i]);
    oversampling.processSamplesDown (block);

    // mono synth -> every output channel
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, monoBuffer, 0, 0, numSamples);

    // for the editor: meter + filter display
    const float peak = monoBuffer.getMagnitude (0, 0, numSamples);
    if (peak > outputPeak.load()) outputPeak.store (peak);
    displayCutoffHz.store (voice.isActive() ? voice.getLastCutoffHz()
                                            : 50.0f * std::pow (100.0f, pCutoff->load()));
}

// ------------------------------------------------------------------ presets / theme
int SquelchProcessor::getNumPrograms() { return squelch::numFactoryPresets; }

const juce::String SquelchProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, squelch::numFactoryPresets)
         ? juce::String (squelch::factoryPresets[index].name) : juce::String();
}

void SquelchProcessor::loadPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, squelch::numFactoryPresets))
        return;

    const auto& preset = squelch::factoryPresets[index];
    for (int i = 0; i < squelch::numPresetParams; ++i)
        if (auto* param = apvts.getParameter (squelch::presetParamIds[i]))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 (preset.values[i]));
            param->endChangeGesture();
        }

    apvts.state.setProperty (presetProp, index, nullptr);
}

int SquelchProcessor::getPresetIndex() const { return (int) apvts.state.getProperty (presetProp, 0); }
int SquelchProcessor::getTheme() const       { return (int) apvts.state.getProperty (themeProp, 0); }
void SquelchProcessor::setTheme (int theme)  { apvts.state.setProperty (themeProp, theme, nullptr); }

// ------------------------------------------------------------------ state
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
