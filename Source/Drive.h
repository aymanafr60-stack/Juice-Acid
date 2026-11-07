// Squelch — Drive FX: one drive section with three characters, then output volume.
// Pure C++ (no JUCE). In the plugin it runs at 4x oversampling so heavy
// settings don't turn into digital fizz (aliasing).
//
//   OD    overdrive: gentle, asymmetric (warm, even harmonics), up to +24 dB.
//   DIST  distortion: smooth symmetric saturation, up to +36 dB. Dense and compressed.
//   FUZZ  harsh asymmetric clipping with a rectified octave-up edge, up to +40 dB.
//
// DRIVE  how hard it's pushed       TONE  dark (800 Hz) .. bright (16 kHz)
// LEVEL  wet output, -12 .. +6 dB   MIX   dry .. wet
// Each mode has automatic make-up gain so DRIVE changes character more than loudness.

#pragma once

#include <cmath>
#include <algorithm>

namespace squelch
{

struct DriveParams
{
    bool  on     = false;
    int   mode   = 1;     // 0 = OD, 1 = DIST, 2 = FUZZ
    float drive  = 0.4f;  // 0..1
    float tone   = 0.7f;  // 0..1
    float level  = 0.6667f; // 0..1  (2/3 = 0 dB)
    float mix    = 1.0f;  // 0..1
    float volume = 0.7f;  // 0..1  master output
};

class DriveStage
{
public:
    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        dcA = std::exp (-2.0f * 3.14159265f * 20.0f / fs);
        reset();
    }

    void reset()
    {
        dcState = dcPrevIn = lp1 = lp2 = 0.0f;
        first = true;
    }

    void setParams (const DriveParams& p) { params = p; }

    float process (float x)
    {
        // smooth knob moves (~10 ms) so turning knobs doesn't click
        const float targetWet = params.on ? params.mix : 0.0f;
        if (first)
        {
            sDrive = params.drive; sTone = params.tone; sLevel = params.level; sWet = targetWet; first = false;
        }
        const float sm = 1.0f - std::exp (-1.0f / (0.01f * fs));
        sDrive += (params.drive - sDrive) * sm;
        sTone  += (params.tone  - sTone)  * sm;
        sLevel += (params.level - sLevel) * sm;
        sWet   += (targetWet    - sWet)   * sm;

        float out = x;

        if (sWet > 0.0001f)
        {
            float wet = shape (x, params.mode, sDrive);

            // DC blocker (asymmetric modes create offset)
            const float dcOut = dcA * (dcState + wet - dcPrevIn);
            dcPrevIn = wet;
            dcState = dcOut;
            wet = dcOut;

            // TONE: 12 dB/oct low-pass, 800 Hz .. 16 kHz
            const float lpHz = 800.0f * std::pow (20.0f, sTone);
            const float a = 1.0f - std::exp (-2.0f * 3.14159265f * std::min (lpHz, 0.45f * fs) / fs);
            lp1 += (wet - lp1) * a;
            lp2 += (lp1 - lp2) * a;
            wet = lp2;

            // LEVEL: -12 .. +6 dB
            wet *= std::pow (10.0f, (-12.0f + 18.0f * sLevel) / 20.0f);

            out = x + (wet - x) * sWet;
        }
        else
        {
            dcState = dcPrevIn = 0.0f;
            lp1 = lp2 = x; // keeps the filters ready, so switching on doesn't thump
        }

        return std::tanh (out * params.volume * 1.6f);
    }

private:
    static float shape (float x, int mode, float d)
    {
        switch (mode)
        {
            case 0: // OD
            {
                const float g = std::pow (10.0f, d * 24.0f / 20.0f);
                const float bias = 0.2f * d;
                return (std::tanh (x * g + bias) - std::tanh (bias)) / (1.0f + 1.4f * d);
            }
            case 2: // FUZZ
            {
                const float g = std::pow (10.0f, d * 40.0f / 20.0f);
                const float u = x * g + 0.25f * d;
                float c = u > 0.0f ? std::tanh (u)
                                   : -0.7f * (1.0f - std::exp (std::max (u * 1.6f, -30.0f)));
                c = std::clamp (c * 1.3f, -0.8f, 0.9f);
                const float rect = std::abs (c) - 0.45f;
                return ((1.0f - 0.35f * d) * c + 0.35f * d * rect) / (1.0f + 1.6f * d);
            }
            default: // DIST
            {
                const float g = std::pow (10.0f, d * 36.0f / 20.0f);
                return std::tanh (x * g) / (1.0f + 2.4f * d);
            }
        }
    }

    DriveParams params;
    float fs = 44100.0f;
    bool first = true;
    float sDrive = 0, sTone = 0, sLevel = 0, sWet = 0;
    float dcA = 0.999f, dcState = 0, dcPrevIn = 0, lp1 = 0, lp2 = 0;
};

} // namespace squelch
