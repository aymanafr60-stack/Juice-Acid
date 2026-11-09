// Level-matches the factory presets. Renders an acid riff through each preset (voice, drive,
// delay; reverb left out), then searches the VOLUME that hits a common loudness.
// Prints a JSON map {name: volume} for tools/make_presets.py.
// Build: g++ -O2 -std=c++17 -I../Source preset_level_test.cpp
#include "Presets.h"
#include "AcidVoice.h"
#include "Drive.h"
#include "Delay.h"
#include "Sequencer.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>
using namespace squelch;

static float get (const Preset& p, const char* id)
{
    for (int i = 0; i < numPresetParams; ++i) if (std::strcmp (presetParamIds[i], id) == 0) return p.values[i];
    std::fprintf (stderr, "missing %s\n", id); std::exit (1);
}

struct Sink : NoteSink { AcidVoice* v; void seqNoteOn (int n, int vel) override { v->noteOn (n, vel); } void seqNoteOff (int n) override { v->noteOff (n); } };

// RMS (dB) of 4 bars of the starter pattern at 130 bpm, with the given volume
static double loudness (const Preset& p, float volume, std::vector<float>* capture = nullptr)
{
    const double sr = 44100.0;
    AcidVoice v; v.prepare (sr);
    VoiceParams vp;
    vp.waveform = (int) get (p, "wave"); vp.shape = get (p, "pw"); vp.tuneSemis = get (p, "tune");
    vp.cutoff = get (p, "cutoff"); vp.resonance = get (p, "res"); vp.envMod = get (p, "envmod");
    vp.decay = get (p, "decay"); vp.accent = get (p, "accent"); vp.slideTimeMs = get (p, "slide");
    vp.subLevel = get (p, "sub"); vp.softAttack = get (p, "soft_atk"); vp.accentDecay = get (p, "acc_decay");
    vp.tracking = get (p, "tracking"); vp.filterFm = get (p, "filter_fm"); vp.sweepSpeed = (int) get (p, "sweep");
    v.setParams (vp);

    DriveStage d; d.prepare (sr);   // (the plugin oversamples; close enough for loudness)
    DriveParams dp; dp.on = get (p, "drive_on") > 0.5f; dp.mode = (int) get (p, "drive_mode"); dp.drive = get (p, "drive");
    dp.tone = get (p, "tone"); dp.level = get (p, "level"); dp.mix = get (p, "mix"); dp.volume = volume;
    d.setParams (dp);

    PingPongDelay dl; dl.prepare (sr);
    DelayParams dlp; dlp.on = get (p, "dly_on") > 0.5f; dlp.time = (int) get (p, "dly_time"); dlp.feedback = get (p, "dly_fb");
    dlp.tone = get (p, "dly_tone"); dlp.mix = get (p, "dly_mix");
    dl.setParams (dlp, 130.0);

    Sequencer s; s.prepare (sr); Sink sink; sink.v = &v;
    SeqSettings st; st.mode = seqPattern; st.play = true; st.bpm = 130;
    const int total = (int) (sr * 60.0 / 130.0 * 16.0);
    double sq = 0;
    for (int done = 0; done < total; done += 512)
    {
        s.beginBlock (st, false, 0, 0, sink);
        for (int i = 0; i < 512; ++i)
        {
            s.tick (sink);
            const float y = d.process (v.processSample());
            float l = y, r = y; dl.process (y, l, r);
            if (done > total / 4) sq += 0.5 * ((double) l * l + (double) r * r); // skip the first bar (delay fill-up)
            if (capture && done < total / 2) { capture->push_back (l); capture->push_back (r); }
        }
    }
    return 10.0 * std::log10 (sq / (total * 0.75) + 1e-12);
}

static void writeWav (const char* path, const std::vector<float>& d, int sr)
{
    FILE* f = std::fopen (path, "wb");
    const uint32_t bytes = (uint32_t) d.size() * 2;
    auto w32 = [&](uint32_t v){ std::fwrite (&v, 4, 1, f); }; auto w16 = [&](uint16_t v){ std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF",1,4,f); w32 (36 + bytes); std::fwrite ("WAVEfmt ",1,8,f); w32 (16); w16 (1); w16 (2);
    w32 ((uint32_t) sr); w32 ((uint32_t) sr * 4); w16 (4); w16 (16); std::fwrite ("data",1,4,f); w32 (bytes);
    for (float x : d) { int16_t v = (int16_t) std::lround (std::clamp (x, -1.0f, 1.0f) * 32767.0f); std::fwrite (&v,2,1,f); }
    std::fclose (f);
}

int main (int argc, char** argv)
{
    if (argc > 2) // demo: render presets from index argv[2] onwards, 2 bars each
    {
        std::vector<float> audio;
        for (int i = std::atoi (argv[2]); i < numFactoryPresets; ++i)
        {
            loudness (factoryPresets[i], factoryPresets[i].values[16], &audio);
            for (int k = 0; k < 44100 / 2; ++k) { audio.push_back (0); audio.push_back (0); }
            std::fprintf (stderr, "%d %s\n", i + 1, factoryPresets[i].name);
        }
        writeWav (argv[1], audio, 44100);
        return 0;
    }

    std::vector<double> base;
    for (const auto& p : factoryPresets) base.push_back (loudness (p, 0.7f));
    const double target = loudness (factoryPresets[0], 0.7f);  // INIT at its default volume

    std::fprintf (stderr, "%-18s %8s %8s %8s\n", "preset", "before", "volume", "after");
    std::printf ("{\n");
    for (int i = 0; i < numFactoryPresets; ++i)
    {
        const auto& p = factoryPresets[i];
        float lo = 0.2f, hi = 1.0f;
        for (int it = 0; it < 22; ++it) { const float mid = 0.5f * (lo + hi); (loudness (p, mid) < target ? lo : hi) = mid; }
        const float vol = std::round (0.5f * (lo + hi) * 100.0f) / 100.0f;
        std::fprintf (stderr, "%-18s %7.1fdB %8.2f %7.1fdB\n", p.name, base[(size_t) i], vol, loudness (p, vol));
        std::printf ("  \"%s\": %.2f%s\n", p.name, vol, i + 1 < numFactoryPresets ? "," : "");
    }
    std::printf ("}\n");
}
