// Offline test: renders an acid riff through AcidVoice to a WAV file and
// reports peak level / NaNs. Build: g++ -O2 -std=c++17 -I../Source render_test.cpp
#include "AcidVoice.h"
#include "Drive.h"
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <cmath>

struct Step { int note; bool accent; bool slide; bool rest; };

static void writeWav (const char* path, const std::vector<float>& data, int sr)
{
    FILE* f = std::fopen (path, "wb");
    const uint32_t n = (uint32_t) data.size(), bytes = n * 2;
    auto w32 = [&](uint32_t v){ std::fwrite (&v, 4, 1, f); };
    auto w16 = [&](uint16_t v){ std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + bytes); std::fwrite ("WAVEfmt ", 1, 8, f);
    w32 (16); w16 (1); w16 (1); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 2); w16 (2); w16 (16);
    std::fwrite ("data", 1, 4, f); w32 (bytes);
    for (float s : data) { int16_t v = (int16_t) std::lround (std::clamp (s, -1.0f, 1.0f) * 32767.0f); std::fwrite (&v, 2, 1, f); }
    std::fclose (f);
}

int main (int argc, char** argv)
{
    const int sr = 44100;
    const double bpm = 132.0;
    const int stepLen = (int) (sr * 60.0 / bpm / 4.0); // 16th notes

    // A classic-style 16-step line in A (A1 = 33)
    const Step pat[16] = {
        {33,true ,false,false},{33,false,false,false},{45,false,true ,false},{43,false,false,false},
        {33,false,false,false},{0 ,false,false,true },{36,true ,true ,false},{38,false,false,false},
        {33,false,false,false},{45,true ,false,false},{33,false,true ,false},{40,false,false,false},
        {33,true ,false,false},{43,false,true ,false},{45,true ,false,false},{33,false,false,false},
    };

    squelch::AcidVoice v;
    v.prepare (sr);
    squelch::DriveStage drive;
    drive.prepare (sr); // no oversampling in this quick test
    drive.setParams (argc > 3 ? (float) std::atof (argv[3]) : 0.0f,
                     argc > 4 ? (float) std::atof (argv[4]) : 0.0f, 0.7f);
    squelch::VoiceParams p;
    p.waveform = argc > 1 ? std::atoi (argv[1]) : 0;
    p.resonance = 0.8f; p.envMod = 0.55f; p.decay = 0.35f; p.accent = 0.75f; p.subLevel = 0.0f;

    const int bars = 8;
    std::vector<float> out;
    out.reserve ((size_t) stepLen * 16 * bars);
    int prevNote = -1;
    float peak = 0; int bad = 0;

    for (int s = 0; s < 16 * bars; ++s)
    {
        const Step& st = pat[s % 16];
        // Cutoff sweeps up then down over the render, like a hand on the knob
        const float t = (float) s / (16 * bars);
        p.cutoff = 0.15f + 0.45f * std::sin (3.14159f * t);
        v.setParams (p);

        if (! st.rest)
        {
            v.noteOn (st.note, st.accent ? 120 : 80);
            if (prevNote >= 0 && prevNote != st.note) v.noteOff (prevNote);
        }
        else if (prevNote >= 0) { v.noteOff (prevNote); }

        const bool slideIntoNext = st.slide && ! st.rest;
        const int gateLen = slideIntoNext ? stepLen : stepLen / 2;

        for (int i = 0; i < stepLen; ++i)
        {
            if (i == gateLen && ! st.rest) { v.noteOff (st.note); prevNote = -1; }
            float y = drive.process (v.processSample());
            if (! std::isfinite (y)) { ++bad; y = 0; }
            peak = std::max (peak, std::abs (y));
            out.push_back (y);
        }
        prevNote = (slideIntoNext && ! st.rest) ? st.note : -1;
    }

    writeWav (argc > 2 ? argv[2] : "acid_test.wav", out, sr);
    double sq = 0; for (float x : out) sq += (double) x * x;
    std::printf ("peak=%.3f rms=%.3f nonfinite=%d\n", peak, std::sqrt (sq / out.size()), bad);
    return bad ? 1 : 0;
}
