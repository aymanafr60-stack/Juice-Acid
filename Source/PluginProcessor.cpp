#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using APVTS = juce::AudioProcessorValueTreeState;

static const juce::Identifier presetProp { "preset" };
static const juce::Identifier themeProp  { "theme" };
static const juce::Identifier pageProp   { "page" };
static const juce::Identifier patternProp { "pattern" };
static const juce::Identifier presetNameProp { "presetName" };

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

    // --- TD-3-MO style extras
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "soft_atk", 1 }, "Soft Attack", unit(), 0.5f,
                    AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                    { const float ms = 0.3f * std::pow (100.0f, v); return ms < 10.0f ? String (ms, 1) + " ms" : String (roundToInt (ms)) + " ms"; })));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "acc_decay", 1 }, "Accent Decay", unit(), 0.4628f,
                    AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                    { const float ms = 50.0f * std::pow (20.0f, v); return ms < 1000.0f ? String (roundToInt (ms)) + " ms" : String (ms / 1000.0f, 2) + " s"; })));
    addPct ("tracking", "Filter Tracking", 0.0f);
    addPct ("filter_fm", "Filter FM", 0.0f);
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "sweep", 1 }, "Accent Sweep",
                                                        StringArray { "Slow", "Normal", "Fast" }, 1));

    // --- delay
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "dly_on", 1 }, "Delay On", false));
    {
        StringArray names;
        for (const auto& t : squelch::delayTimes) names.add (t.name);
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "dly_time", 1 }, "Delay Time", names, 3));
    }
    addPct ("dly_fb", "Delay Feedback", 0.45f);
    addPct ("dly_tone", "Delay Tone", 0.5f);
    addPct ("dly_mix", "Delay Mix", 0.3f);
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "dly_pingpong", 1 }, "Delay Ping-Pong", true));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "dly_sync", 1 }, "Delay Sync", true));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "dly_ms", 1 }, "Delay Time (free)",
                    NormalisableRange<float> (10.0f, 2000.0f, 1.0f, 0.4f), 375.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                    { return v < 1000.0f ? String (roundToInt (v)) + " ms" : String (v / 1000.0f, 2) + " s"; })));

    // --- reverb
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "rev_on", 1 }, "Reverb On", false));
    addPct ("rev_size", "Reverb Size", 0.5f);
    addPct ("rev_damp", "Reverb Damping", 0.5f);
    addPct ("rev_mix", "Reverb Mix", 0.25f);
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "rev_predelay", 1 }, "Reverb Pre-Delay",
                    NormalisableRange<float> (0.0f, 200.0f, 1.0f), 10.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " ms"; })));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "rev_lowcut", 1 }, "Reverb Low Cut",
                    NormalisableRange<float> (20.0f, 1000.0f, 1.0f, 0.4f), 150.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " Hz"; })));
    addPct ("rev_duck", "Reverb Ducking", 0.0f);

    // --- sequencer / arp (not part of presets)
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "seq_mode", 1 }, "Seq Mode",
                                                        StringArray { "Off", "Pattern", "Arp" }, 0));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "seq_play", 1 }, "Seq Play", false));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { "seq_len", 1 }, "Seq Length", 1, 16, 16,
                    AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return String (v) + (v == 1 ? " step" : " steps"); })));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "seq_rate", 1 }, "Seq Rate",
                                                        StringArray { "1/8", "1/16", "1/32", "1/8 T", "1/16 T" }, 1));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "seq_swing", 1 }, "Swing", unit(), 0.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (50.0f + v * 25.0f)) + "%"; })));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "seq_gate", 1 }, "Gate",
                    NormalisableRange<float> (0.05f, 1.0f), 0.5f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { "seq_bpm", 1 }, "Internal BPM", 60, 200, 130,
                    AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return String (v) + " bpm"; })));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "key", 1 }, "Key",
                    StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));
    {
        StringArray names;
        for (const auto& sc : squelch::scales) names.add (sc.name);
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "scale", 1 }, "Scale", names, 0));
    }
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "arp_mode", 1 }, "Arp Mode",
                    StringArray { "Up", "Down", "Up-Down", "Random", "As Played" }, 0));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { "arp_oct", 1 }, "Arp Octaves", 1, 4, 1,
                    AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return String (v) + " oct"; })));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "arp_acc", 1 }, "Arp Accent",
                    StringArray { "Off", "Every 2", "Every 3", "Every 4", "Every 5", "Every 7", "Auto" }, 3));
    addPct ("arp_slide", "Arp Slide", 0.15f);
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "arp_latch", 1 }, "Arp Latch", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "arp_chord", 1 }, "Arp Chord", false));
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

    sMode = get ("seq_mode"); sPlay = get ("seq_play"); sLen = get ("seq_len");   sRate = get ("seq_rate");
    sSwing = get ("seq_swing"); sGate = get ("seq_gate"); sBpm = get ("seq_bpm"); sKey = get ("key");
    sScale = get ("scale");  aMode = get ("arp_mode"); aOct = get ("arp_oct");    aAcc = get ("arp_acc");
    aSlide = get ("arp_slide"); aLatch = get ("arp_latch"); aChord = get ("arp_chord");

    mSoftAtk = get ("soft_atk"); mAccDecay = get ("acc_decay"); mTrack = get ("tracking");
    mFm = get ("filter_fm");     mSweep = get ("sweep");
    dOn = get ("dly_on"); dTime = get ("dly_time"); dFb = get ("dly_fb"); dTone = get ("dly_tone"); dMix = get ("dly_mix");
    rOn = get ("rev_on"); rSize = get ("rev_size"); rDamp = get ("rev_damp"); rMix = get ("rev_mix");
    rPre = get ("rev_predelay"); rLowCut = get ("rev_lowcut"); rDuck = get ("rev_duck");
    dPing = get ("dly_pingpong"); dSync = get ("dly_sync"); dMs = get ("dly_ms");

    sink.v = &voice;
    refreshUserPresets();

    apvts.state.setProperty (presetProp, 0, nullptr);
    apvts.state.setProperty (themeProp, 0, nullptr); // Phosphor Lab by default
    apvts.state.setProperty (pageProp, 0, nullptr);
}

bool SquelchProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void SquelchProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    voice.prepare (sampleRate);
    seq.prepare (sampleRate);
    delay.prepare (sampleRate);
    reverb.prepare (sampleRate, samplesPerBlock);
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
    p.softAttack  = mSoftAtk->load();
    p.accentDecay = mAccDecay->load();
    p.tracking    = mTrack->load();
    p.filterFm    = mFm->load();
    p.sweepSpeed  = (int) mSweep->load();
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

squelch::SeqSettings SquelchProcessor::readSeqSettings() const
{
    squelch::SeqSettings s;
    s.mode      = (int) sMode->load();
    s.play      = sPlay->load() > 0.5f;
    s.length    = (int) std::round (sLen->load());
    s.rate      = (int) sRate->load();
    s.swing     = sSwing->load();
    s.gate      = sGate->load();
    s.bpm       = sBpm->load();
    s.key       = (int) sKey->load();
    s.scale     = (int) sScale->load();
    s.arpMode   = (int) aMode->load();
    s.arpOct    = (int) std::round (aOct->load());
    s.arpAccent = (int) aAcc->load();
    s.arpSlide  = aSlide->load();
    s.latch     = aLatch->load() > 0.5f;
    s.chord     = aChord->load() > 0.5f;
    return s;
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

    // where is the host? (FL's transport, if it's playing)
    bool hostPlaying = false, hostBpmKnown = false;
    double hostPpq = 0.0, hostBpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) { hostBpm = *b; hostBpmKnown = true; }
            if (auto q = pos->getPpqPosition()) { hostPpq = *q; hostPlaying = pos->getIsPlaying(); }
        }

    seq.beginBlock (readSeqSettings(), hostPlaying, hostPpq, hostBpmKnown ? hostBpm : 0.0, sink);
    hostTempo.store (hostBpmKnown ? (float) hostBpm : 0.0f);

    int pos = 0;
    auto renderTo = [&] (int end)
    {
        for (; pos < end; ++pos)
        {
            seq.tick (sink);
            mono[pos] = voice.processSample();
        }
    };

    // sample-accurate MIDI: render up to each event, then apply it
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        renderTo (juce::jlimit (0, numSamples, meta.samplePosition));

        if (msg.isNoteOn())
            seq.midiNote (msg.getNoteNumber(), true, msg.getVelocity(), sink);
        else if (msg.isNoteOff())
            seq.midiNote (msg.getNoteNumber(), false, 0, sink);
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            seq.allNotesOff (sink);
            voice.allNotesOff();
        }
    }
    renderTo (numSamples);

    // drive fx + volume at 4x sample rate to keep heavy settings clean
    juce::dsp::AudioBlock<float> block (monoBuffer.getArrayOfWritePointers(), 1, (size_t) numSamples);
    auto up = oversampling.processSamplesUp (block);
    auto* u = up.getChannelPointer (0);
    for (size_t i = 0; i < up.getNumSamples(); ++i)
        u[i] = drive.process (u[i]);
    oversampling.processSamplesDown (block);

    // --- delay (mono in, stereo out)
    {
        squelch::DelayParams dp;
        dp.on = dOn->load() > 0.5f;
        dp.time = (int) dTime->load();
        dp.feedback = dFb->load();
        dp.tone = dTone->load();
        dp.mix = dMix->load();
        dp.pingPong = dPing->load() > 0.5f;
        dp.sync = dSync->load() > 0.5f;
        dp.timeMs = dMs->load();
        const double delayBpm = hostBpmKnown ? hostBpm : (double) sBpm->load();
        delay.setParams (dp, delayBpm);
    }

    const int numCh = buffer.getNumChannels();
    auto* outL = buffer.getWritePointer (0);
    auto* outR = numCh > 1 ? buffer.getWritePointer (1) : nullptr;
    for (int i = 0; i < numSamples; ++i)
    {
        float l = mono[i], r = mono[i];
        delay.process (mono[i], l, r);
        outL[i] = l;
        if (outR != nullptr) outR[i] = r; else outL[i] = 0.5f * (l + r);
    }

    // --- reverb (pre-delay, low cut, ducking)
    {
        squelch::ReverbParams rp;
        rp.on = rOn->load() > 0.5f;
        rp.size = rSize->load();
        rp.damp = rDamp->load();
        rp.mix = rMix->load();
        rp.preDelayMs = rPre->load();
        rp.lowCutHz = rLowCut->load();
        rp.ducking = rDuck->load();
        reverb.process (rp, outL, outR, numSamples);
    }

    // --- safety: soften anything that would clip
    for (int ch = 0; ch < std::min (numCh, 2); ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const float a = std::abs (d[i]);
            if (a > 0.95f) d[i] = std::copysign (0.95f + 0.05f * std::tanh ((a - 0.95f) / 0.05f), d[i]);
        }
    }
    for (int ch = 2; ch < numCh; ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);

    // for the editor: scope feed
    {
        int w = scopeWrite.load (std::memory_order_relaxed);
        const auto* src = buffer.getReadPointer (0);
        for (int i = 0; i < numSamples; ++i) { scopeBuffer[w] = src[i]; w = (w + 1) % scopeSize; }
        scopeWrite.store (w, std::memory_order_release);
    }

    // for the editor: meter + filter display
    const float peak = buffer.getMagnitude (0, numSamples);
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

bool SquelchProcessor::isSoundParam (const juce::String& id) const
{
    static const juce::StringArray notSound { "seq_mode", "seq_play", "seq_len", "seq_rate", "seq_swing", "seq_gate",
                                              "seq_bpm", "key", "scale", "arp_mode", "arp_oct", "arp_acc",
                                              "arp_slide", "arp_latch", "arp_chord" };
    return ! notSound.contains (id);
}

void SquelchProcessor::resetSoundToDefaults()
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (isSoundParam (rp->getParameterID()))
            {
                rp->beginChangeGesture();
                rp->setValueNotifyingHost (rp->getDefaultValue());
                rp->endChangeGesture();
            }
}

juce::File SquelchProcessor::getUserPresetFolder() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Squelch").getChildFile ("Presets");
}

void SquelchProcessor::refreshUserPresets()
{
    userPresetFiles = getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.sqpreset");
    userPresetFiles.sort();

    // keep pointing at the same preset if the list changed
    const auto name = apvts.state.getProperty (presetNameProp).toString();
    if (name.isNotEmpty())
        for (int i = 0; i < getNumPresetsTotal(); ++i)
            if (getPresetDisplayName (i) == name) { apvts.state.setProperty (presetProp, i, nullptr); break; }
}

int SquelchProcessor::getNumPresetsTotal() const { return squelch::numFactoryPresets + userPresetFiles.size(); }

juce::String SquelchProcessor::getPresetDisplayName (int index) const
{
    if (juce::isPositiveAndBelow (index, squelch::numFactoryPresets)) return squelch::factoryPresets[index].name;
    const int u = index - squelch::numFactoryPresets;
    if (juce::isPositiveAndBelow (u, userPresetFiles.size())) return userPresetFiles[u].getFileNameWithoutExtension().toUpperCase();
    return {};
}

bool SquelchProcessor::saveUserPreset (const juce::String& rawName)
{
    const auto name = juce::File::createLegalFileName (rawName.trim()).substring (0, 40);
    if (name.isEmpty()) return false;

    juce::XmlElement xml ("SquelchPreset");
    xml.setAttribute ("name", name);
    xml.setAttribute ("version", 1);
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (isSoundParam (rp->getParameterID()))
            {
                auto* e = xml.createNewChildElement ("P");
                e->setAttribute ("id", rp->getParameterID());
                e->setAttribute ("v", rp->convertFrom0to1 (rp->getValue()));
            }

    auto folder = getUserPresetFolder();
    if (! folder.createDirectory()) return false;
    const auto file = folder.getChildFile (name + ".sqpreset");
    if (! xml.writeTo (file)) return false;

    apvts.state.setProperty (presetNameProp, name.toUpperCase(), nullptr);
    refreshUserPresets();
    return true;
}

void SquelchProcessor::loadPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, getNumPresetsTotal()))
        return;

    resetSoundToDefaults();

    if (isUserPreset (index))
    {
        if (auto xml = juce::XmlDocument::parse (userPresetFiles[index - squelch::numFactoryPresets]))
            for (auto* e : xml->getChildWithTagNameIterator ("P"))
                if (auto* rp = apvts.getParameter (e->getStringAttribute ("id")))
                {
                    rp->beginChangeGesture();
                    rp->setValueNotifyingHost (rp->convertTo0to1 ((float) e->getDoubleAttribute ("v")));
                    rp->endChangeGesture();
                }
        apvts.state.setProperty (presetProp, index, nullptr);
        apvts.state.setProperty (presetNameProp, getPresetDisplayName (index), nullptr);
        return;
    }

    const auto& preset = squelch::factoryPresets[index];
    for (int i = 0; i < squelch::numPresetParams; ++i)
        if (auto* param = apvts.getParameter (squelch::presetParamIds[i]))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 (preset.values[i]));
            param->endChangeGesture();
        }

    apvts.state.setProperty (presetProp, index, nullptr);
    apvts.state.setProperty (presetNameProp, getPresetDisplayName (index), nullptr);
}

int SquelchProcessor::getPage() const        { return (int) apvts.state.getProperty (pageProp, 0); }
void SquelchProcessor::setPage (int page)     { apvts.state.setProperty (pageProp, page, nullptr); }

juce::File SquelchProcessor::writePatternMidi() const
{
    const auto st = readSeqSettings();
    const int tpq = 960;
    const double stepTicks = squelch::Sequencer::rateToPpq (st.rate) * tpq;
    const int len = juce::jlimit (1, squelch::maxSteps, st.length);

    juce::MidiMessageSequence track;
    track.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / std::max (1.0f, st.bpm))), 0.0);

    for (int i = 0; i < len; ++i)
    {
        const auto step = seq.getStep (i);
        if (! step.on) continue;

        // a slide into the same pitch is a tie: skip steps that continue an earlier note...
        if (i > 0 && seq.getStep (i - 1).on && seq.getStep (i - 1).slide
            && seq.getStep (i - 1).midiNote() == step.midiNote())
            continue;

        // ...and extend this note over all the steps it's tied into
        int end = i;
        while (seq.getStep (end).slide && end + 1 < len && seq.getStep (end + 1).on
               && seq.getStep (end + 1).midiNote() == step.midiNote())
            ++end;

        const double swingOf = st.swing * 0.5 * stepTicks;
        auto startOf = [&] (int k) { return k * stepTicks + ((k & 1) ? swingOf : 0.0); };
        const double on = startOf (i);
        const bool slidesOn = seq.getStep (end).slide && end + 1 < len && seq.getStep (end + 1).on;
        const double off = slidesOn ? startOf (end + 1) + stepTicks * 0.1   // overlap -> Squelch slides
                                    : startOf (end) + juce::jlimit (0.05, 0.95, (double) st.gate) * stepTicks;
        const int vel = step.accent ? 120 : 80;
        track.addEvent (juce::MidiMessage::noteOn (1, step.midiNote(), (juce::uint8) vel), on);
        track.addEvent (juce::MidiMessage::noteOff (1, step.midiNote()), off);
    }
    track.updateMatchedPairs();

    juce::MidiFile file;
    file.setTicksPerQuarterNote (tpq);
    file.addTrack (track);

    auto out = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Squelch Pattern.mid");
    out.deleteFile();
    juce::FileOutputStream stream (out);
    if (stream.openedOk()) file.writeTo (stream);
    return out;
}

int SquelchProcessor::getPresetIndex() const { return (int) apvts.state.getProperty (presetProp, 0); }
int SquelchProcessor::getTheme() const       { return (int) apvts.state.getProperty (themeProp, 0); }
void SquelchProcessor::setTheme (int theme)  { apvts.state.setProperty (themeProp, theme, nullptr); }

// ------------------------------------------------------------------ state
void SquelchProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::StringArray steps;
    for (int i = 0; i < squelch::maxSteps; ++i) steps.add (juce::String (seq.pattern[(size_t) i].load()));
    apvts.state.setProperty (patternProp, steps.joinIntoString (","), nullptr);

    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SquelchProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));

            const auto steps = juce::StringArray::fromTokens (apvts.state.getProperty (patternProp).toString(), ",", "");
            if (steps.size() == squelch::maxSteps)
            {
                for (int i = 0; i < squelch::maxSteps; ++i) seq.pattern[(size_t) i].store (steps[i].getIntValue());
                seq.patternVersion.fetch_add (1);
            }
        }
}

juce::AudioProcessorEditor* SquelchProcessor::createEditor()
{
    return new SquelchEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SquelchProcessor();
}
