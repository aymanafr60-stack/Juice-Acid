// Checks the acid line generator and renders a few generated lines to a WAV.
// Build: g++ -O2 -std=c++17 -I../Source gen_test.cpp
#include "Generator.h"
#include "AcidVoice.h"
#include "Drive.h"
#include <cstdio>
#include <cstdint>
#include <string>
using namespace squelch;

static const char* names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };

static std::string show (Sequencer& s)
{
    std::string out;
    for (int i = 0; i < maxSteps; ++i)
    {
        auto st = s.getStep (i);
        if (! st.on) { out += "  -   "; continue; }
        std::string n = names[st.note];
        n += st.oct > 0 ? "'" : (st.oct < 0 ? "," : "");
        n += st.accent ? "!" : "";
        n += st.slide ? "~" : "";
        while (n.size() < 6) n += ' ';
        out += n;
    }
    return out;
}

static void writeWav (const char* path, const std::vector<float>& d, int sr)
{
    FILE* f = std::fopen (path, "wb");
    const uint32_t n = (uint32_t) d.size(), bytes = n * 2;
    auto w32 = [&](uint32_t v){ std::fwrite (&v, 4, 1, f); }; auto w16 = [&](uint16_t v){ std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF",1,4,f); w32 (36 + bytes); std::fwrite ("WAVEfmt ",1,8,f); w32 (16); w16 (1); w16 (1);
    w32 ((uint32_t) sr); w32 ((uint32_t) sr * 2); w16 (2); w16 (16); std::fwrite ("data",1,4,f); w32 (bytes);
    for (float x : d) { int16_t v = (int16_t) std::lround (std::clamp (x, -1.0f, 1.0f) * 32767.0f); std::fwrite (&v,2,1,f); }
    std::fclose (f);
}

struct VoiceSink : NoteSink
{
    AcidVoice* v;
    void seqNoteOn (int n, int vel) override { v->noteOn (n, vel); }
    void seqNoteOff (int n) override { v->noteOff (n); }
};

int main (int argc, char** argv)
{
    int fails = 0;
    // 1. rules hold across many seeds, keys and scales
    int bad = 0, total = 0, quality = 0;
    for (int scale = 0; scale < numScales; ++scale)
        for (int key = 0; key < 12; ++key)
            for (uint32_t seed = 1; seed <= 150; ++seed)
            {
                Sequencer s; AcidGenerator::generate (s, key, scale, seed * 7919u + (uint32_t) key);
                ++total;
                const auto& sc = scales[scale];
                bool ok = s.getStep (0).on && (s.getStep (0).note - key + 12) % 12 == 0;
                int ons = 0, acc = 0, sl = 0;
                for (int i = 0; i < maxSteps; ++i)
                {
                    auto st = s.getStep (i);
                    if (st.slide && ! s.getStep ((i + 1) % maxSteps).on) ok = false;
                    if (! st.on) continue;
                    ++ons; acc += st.accent; sl += st.slide;
                    const int rel = ((st.note - key) % 12 + 12) % 12;
                    bool inScale = false;
                    for (int d = 0; d < sc.size; ++d) inScale |= sc.steps[d] == rel;
                    ok &= inScale;
                }
                if (! ok) ++bad;
                if (ons >= 11 && acc >= 2 && acc <= 7 && sl >= 1 && sl <= 6) ++quality;
            }
    std::printf ("%s  every line: starts on the root, stays in the scale, no slides into rests (%d bad of %d)\n",
                 bad ? "FAIL" : "ok  ", bad, total);
    std::printf ("%s  %.1f%% of lines meet the density/accent/slide targets\n",
                 quality > total * 0.97 ? "ok  " : "FAIL", 100.0 * quality / total);
    fails += bad != 0; fails += quality <= total * 0.97;

    // 2. mutate changes a little, not everything, and keeps step 1
    {
        Sequencer s; AcidGenerator::generate (s, 9, 0, 42);
        int before[16]; for (int i = 0; i < 16; ++i) before[i] = s.pattern[(size_t) i].load();
        AcidGenerator::mutate (s, 9, 0, 16, 99);
        int changed = 0; for (int i = 0; i < 16; ++i) changed += before[i] != s.pattern[(size_t) i].load();
        const bool ok = changed >= 1 && changed <= 4 && before[0] == s.pattern[0].load();
        std::printf ("%s  mutate changed %d steps and kept step 1\n", ok ? "ok  " : "FAIL", changed);
        fails += ! ok;
    }

    // 3. examples + audio: 4 lines in A minor, 2 bars each, 132 bpm
    const int sr = 44100;
    AcidVoice voice; voice.prepare (sr);
    VoiceParams vp; vp.resonance = 0.85f; vp.envMod = 0.6f; vp.decay = 0.35f; vp.accent = 0.75f; vp.cutoff = 0.3f;
    voice.setParams (vp);
    DriveStage drive; drive.prepare (sr);
    DriveParams dp; dp.on = true; dp.mode = 1; dp.drive = 0.35f; dp.mix = 0.6f; drive.setParams (dp);
    VoiceSink sink; sink.v = &voice;

    std::vector<float> audio;
    std::printf ("\nexamples (A minor):\n");
    for (int line = 0; line < 4; ++line)
    {
        Sequencer s; s.prepare (sr);
        AcidGenerator::generate (s, 9, 0, 1000u + (uint32_t) line * 31u);
        std::printf ("  %s\n", show (s).c_str());
        SeqSettings st; st.mode = seqPattern; st.play = true; st.bpm = 132; st.gate = 0.5f;
        const int samples = (int) (sr * 60.0 / 132.0 * 8.0); // 2 bars
        for (int done = 0; done < samples; done += 512)
        {
            s.beginBlock (st, false, 0, 0, sink);
            for (int i = 0; i < 512; ++i) { s.tick (sink); audio.push_back (drive.process (voice.processSample())); }
        }
        st.play = false; s.beginBlock (st, false, 0, 0, sink);
        for (int i = 0; i < sr / 4; ++i) audio.push_back (drive.process (voice.processSample())); // short gap
    }
    if (argc > 1) writeWav (argv[1], audio, sr);

    std::printf (fails ? "\n%d FAILED\n" : "\nall passed\n", fails);
    return fails ? 1 : 0;
}
