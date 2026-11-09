// Squelch — reverb with pre-delay, low cut and ducking (uses juce::Reverb for the tail).
//
//   PRE-DELAY  gap before the reverb starts (0..200 ms): keeps the attack of each note clear
//   LOW CUT    removes lows from what goes INTO the reverb (20 Hz..1 kHz): no muddy bass wash
//   DUCKING    turns the reverb down while the synth is playing and lets it bloom in the gaps
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <cmath>
#include <algorithm>

namespace squelch
{

struct ReverbParams
{
    bool  on = false;
    float size = 0.5f, damp = 0.5f, mix = 0.25f;
    float preDelayMs = 10.0f;   // 0..200
    float lowCutHz = 150.0f;    // 20..1000
    float ducking = 0.0f;       // 0..1
};

class ReverbFx
{
public:
    void prepare (double sampleRate, int maxBlock)
    {
        fs = (float) sampleRate;
        reverb.setSampleRate (sampleRate);
        preSize = (int) (sampleRate * 0.21) + 4;
        preL.assign ((size_t) preSize, 0.0f);
        preR.assign ((size_t) preSize, 0.0f);
        wetL.assign ((size_t) std::max (1, maxBlock), 0.0f);
        wetR.assign ((size_t) std::max (1, maxBlock), 0.0f);
        reset();
    }

    void reset()
    {
        reverb.reset();
        std::fill (preL.begin(), preL.end(), 0.0f);
        std::fill (preR.begin(), preR.end(), 0.0f);
        preWrite = 0;
        for (auto& v : hp) v = 0.0f;
        env = 0.0f;
        tail = 0;
    }

    // left/right hold the dry signal; the reverb is added on top.
    void process (const ReverbParams& p, float* left, float* right, int n)
    {
        if (p.on) tail = (int) (fs * 5.0f);
        if (! p.on && tail <= 0) return;
        if ((int) wetL.size() < n) return; // host broke its block-size promise: skip rather than allocate

        juce::Reverb::Parameters rp;
        rp.roomSize = 0.3f + 0.65f * p.size;
        rp.damping  = p.damp;
        rp.wetLevel = p.on ? p.mix * 0.15f : 0.0f;   // juce::Reverb scales wet by 3
        rp.dryLevel = 0.0f;                           // dry is added back separately
        rp.width    = 1.0f;
        reverb.setParameters (rp);

        const int delay = std::clamp ((int) (p.preDelayMs * 0.001f * fs), 0, preSize - 2);
        const float hpA = std::exp (-2.0f * 3.14159265f * std::clamp (p.lowCutHz, 20.0f, 1000.0f) / fs);

        // 1. pre-delay + low cut on what feeds the reverb
        for (int i = 0; i < n; ++i)
        {
            const float l = left[i], r = right != nullptr ? right[i] : left[i];
            preL[(size_t) preWrite] = l;
            preR[(size_t) preWrite] = r;
            int readPos = preWrite - delay;
            if (readPos < 0) readPos += preSize;
            if (++preWrite >= preSize) preWrite = 0;
            wetL[(size_t) i] = highPass (highPass (preL[(size_t) readPos], 0, hpA), 1, hpA);
            wetR[(size_t) i] = highPass (highPass (preR[(size_t) readPos], 2, hpA), 3, hpA);
        }

        // 2. the tail
        reverb.processStereo (wetL.data(), wetR.data(), n);

        // 3. ducking against the dry signal, then mix in
        const float att = 1.0f - std::exp (-1.0f / (0.005f * fs));
        const float rel = 1.0f - std::exp (-1.0f / (0.25f * fs));
        for (int i = 0; i < n; ++i)
        {
            const float dry = 0.5f * (std::abs (left[i]) + (right != nullptr ? std::abs (right[i]) : std::abs (left[i])));
            env += (dry - env) * (dry > env ? att : rel);
            const float g = 1.0f - std::clamp (p.ducking, 0.0f, 1.0f) * std::min (1.0f, env * 4.0f);
            left[i] += wetL[(size_t) i] * g;
            if (right != nullptr) right[i] += wetR[(size_t) i] * g;
            else left[i] += wetR[(size_t) i] * g;
        }

        if (! p.on) tail -= n;
    }

private:
    // one-pole high-pass; state slots: [in, out] x 4 filters
    float highPass (float x, int k, float a)
    {
        const float y = a * (hp[k * 2 + 1] + x - hp[k * 2]);
        hp[k * 2] = x;
        hp[k * 2 + 1] = y;
        return y;
    }

    juce::Reverb reverb;
    float fs = 44100.0f;
    std::vector<float> preL, preR, wetL, wetR;
    int preSize = 0, preWrite = 0, tail = 0;
    float hp[8] = {};
    float env = 0.0f;
};

} // namespace squelch
