// Squelch — tempo-synced ping-pong delay. Pure C++ (no JUCE).
//
// Mono in, stereo out. The echo bounces left -> right -> left. The feedback path is
// band-limited (low cut ~150 Hz, adjustable high cut) and gently saturated, so long
// feedback stays musical instead of building into mud or harsh fizz.
// MIX is a send level: the dry signal always stays at full volume.

#pragma once

#include <cmath>
#include <vector>
#include <algorithm>

namespace squelch
{

struct DelayParams
{
    bool  on = false;
    int   time = 3;          // index into delayTimes
    float feedback = 0.45f;  // 0..1  (mapped to 0..0.92)
    float tone = 0.5f;       // 0..1  high cut 1.2 kHz .. 14 kHz
    float mix = 0.3f;        // 0..1
    bool  pingPong = true;   // echoes bounce left/right; off = centred echoes
    bool  sync = true;       // time follows the tempo; off = free time in ms
    float timeMs = 375.0f;   // used when sync is off
};

struct DelayTime { const char* name; double beats; };
inline const DelayTime delayTimes[] =
{
    { "1/16", 0.25 }, { "1/8 T", 1.0 / 3.0 }, { "1/8", 0.5 }, { "3/16", 0.75 },
    { "1/4 T", 2.0 / 3.0 }, { "1/4", 1.0 }, { "3/8", 1.5 }, { "1/2", 2.0 },
};
constexpr int numDelayTimes = (int) (sizeof (delayTimes) / sizeof (delayTimes[0]));

class PingPongDelay
{
public:
    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        size = (int) (sampleRate * 4.0) + 8;   // up to 4 s (a half note at 30 bpm)
        bufL.assign ((size_t) size, 0.0f);
        bufR.assign ((size_t) size, 0.0f);
        write = 0;
        hpA = std::exp (-2.0f * 3.14159265f * 150.0f / fs);
        reset();
    }

    void reset()
    {
        std::fill (bufL.begin(), bufL.end(), 0.0f);
        std::fill (bufR.begin(), bufR.end(), 0.0f);
        lpL = lpR = hpL = hpR = hpInL = hpInR = 0.0f;
        delaySmooth = -1.0f;
        wetGain = 0.0f;
    }

    void setParams (const DelayParams& p, double bpm)
    {
        params = p;
        const double beats = delayTimes[std::clamp (p.time, 0, numDelayTimes - 1)].beats;
        const double secs = p.sync ? beats * 60.0 / std::clamp (bpm, 30.0, 300.0)
                                   : std::clamp ((double) p.timeMs, 10.0, 2000.0) / 1000.0;
        targetDelay = (float) std::clamp (secs * fs, 16.0, (double) size - 8.0);
    }

    // in: mono sample. Adds the echoes to outL / outR (which should already hold the dry signal).
    void process (float in, float& outL, float& outR)
    {
        if (delaySmooth < 0.0f) delaySmooth = targetDelay;
        delaySmooth += (targetDelay - delaySmooth) * (1.0f - std::exp (-1.0f / (0.05f * fs))); // glide over ~50 ms

        const float wetTarget = params.on ? params.mix : 0.0f;
        wetGain += (wetTarget - wetGain) * (1.0f - std::exp (-1.0f / (0.02f * fs)));

        // nothing to do once the delay is off and the echoes have been faded out
        if (! params.on && wetGain < 1.0e-4f)
        {
            if (! cleared) { std::fill (bufL.begin(), bufL.end(), 0.0f); std::fill (bufR.begin(), bufR.end(), 0.0f); cleared = true; }
            return;
        }
        cleared = false;

        const float dl = read (bufL, delaySmooth);
        const float dr = read (bufR, delaySmooth);

        const float fb = 0.92f * std::clamp (params.feedback, 0.0f, 1.0f);
        const float lpHz = 1200.0f * std::pow (14000.0f / 1200.0f, std::clamp (params.tone, 0.0f, 1.0f));
        const float lpA = 1.0f - std::exp (-2.0f * 3.14159265f * std::min (lpHz, 0.45f * fs) / fs);

        const float input = in * (params.on ? 1.0f : 0.0f);
        if (params.pingPong)
        {
            // feedback paths cross over: left echo feeds the right line, right feeds left
            const float toR = filter (dl, lpL, hpL, hpInL, lpA) * fb;
            const float toL = filter (dr, lpR, hpR, hpInR, lpA) * fb;
            bufL[(size_t) write] = std::tanh (input + toL);
            bufR[(size_t) write] = std::tanh (toR);
            outL += dl * wetGain;
            outR += dr * wetGain;
        }
        else
        {
            // centred echoes: one line feeding itself, heard in both speakers (plus any ping-pong tail fading out)
            const float back = filter (dl, lpL, hpL, hpInL, lpA) * fb;
            bufL[(size_t) write] = std::tanh (input + back);
            bufR[(size_t) write] = 0.0f;
            outL += (dl + dr) * wetGain;
            outR += (dl + dr) * wetGain;
        }
        if (++write >= size) write = 0;
    }

private:
    float read (const std::vector<float>& buf, float delay) const
    {
        float pos = (float) write - delay;
        while (pos < 0.0f) pos += (float) size;
        const int i0 = (int) pos;
        const int i1 = (i0 + 1) % size;
        const float frac = pos - (float) i0;
        return buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * frac;
    }

    float filter (float x, float& lp, float& hp, float& hpIn, float lpA) const
    {
        lp += (x - lp) * lpA;
        const float y = hpA * (hp + lp - hpIn);
        hpIn = lp;
        hp = y;
        return y;
    }

    DelayParams params;
    float fs = 44100.0f;
    int size = 0, write = 0;
    std::vector<float> bufL, bufR;
    float targetDelay = 1000.0f, delaySmooth = -1.0f, wetGain = 0.0f;
    float lpL = 0, lpR = 0, hpL = 0, hpR = 0, hpInL = 0, hpInR = 0, hpA = 0.99f;
    bool cleared = true;
};

} // namespace squelch
