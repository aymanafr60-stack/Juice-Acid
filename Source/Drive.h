// Squelch — drive section: Distortion -> Fuzz -> output volume/soft clip.
// Pure C++ (no JUCE). In the plugin it runs at 4x oversampling so heavy
// settings don't turn into digital fizz (aliasing).
//
// Distortion: smooth, symmetric tanh saturation. Thickens and compresses,
//             keeps the squelch readable. +0..30 dB of drive.
// Fuzz:       harsher, asymmetric clipping with a rectified (octave-up) edge.
//             Gets buzzy and broken, like an overdriven pedal. +0..40 dB.
// Both have automatic make-up gain so turning them up doesn't just get louder.

#pragma once

#include <cmath>
#include <algorithm>

namespace squelch
{

class DriveStage
{
public:
    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        dcA = std::exp (-2.0f * 3.14159265f * 20.0f / fs);
        reset();
    }

    void reset() { dcState = dcPrevIn = lpState = 0.0f; sDist = sFuzz = -1.0f; }

    void setParams (float distortion, float fuzz, float volume)
    {
        dist = std::clamp (distortion, 0.0f, 1.0f);
        fuzzAmt = std::clamp (fuzz, 0.0f, 1.0f);
        vol = volume;
    }

    float process (float x)
    {
        // smooth knob moves (~10 ms) so turning drive doesn't click
        const float sm = 1.0f - std::exp (-1.0f / (0.01f * fs));
        if (sDist < 0.0f) { sDist = dist; sFuzz = fuzzAmt; }
        sDist += (dist - sDist) * sm;
        sFuzz += (fuzzAmt - sFuzz) * sm;

        // --- Distortion
        if (sDist > 0.0005f)
        {
            const float g = std::pow (10.0f, sDist * 30.0f / 20.0f);
            const float wet = std::tanh (x * g) / (1.0f + 2.2f * sDist);
            x = x + (wet - x) * std::min (1.0f, sDist * 8.0f); // fade in over the first bit of the knob
        }

        // --- Fuzz
        if (sFuzz > 0.0005f)
        {
            const float g = std::pow (10.0f, sFuzz * 40.0f / 20.0f);
            float u = x * g + 0.25f * sFuzz;                    // bias -> asymmetry
            float clipped = u > 0.0f ? std::tanh (u)
                                     : -0.7f * (1.0f - std::exp (std::max (u * 1.6f, -30.0f)));
            clipped = std::clamp (clipped * 1.3f, -0.8f, 0.9f); // hard edge on top
            const float rect = std::abs (clipped) - 0.45f;       // octave-up grit
            float wet = (1.0f - 0.35f * sFuzz) * clipped + 0.35f * sFuzz * rect;

            // DC blocker (the bias and rectifier create offset)
            const float dcOut = dcA * (dcState + wet - dcPrevIn);
            dcPrevIn = wet;
            dcState = dcOut;
            wet = dcOut / (1.0f + 1.6f * sFuzz);

            // tame the top end as fuzz increases (~12 kHz down to ~5 kHz)
            const float lpHz = 12000.0f - 7000.0f * sFuzz;
            const float a = 1.0f - std::exp (-2.0f * 3.14159265f * lpHz / fs);
            lpState += (wet - lpState) * a;

            x = x + (lpState - x) * std::min (1.0f, sFuzz * 8.0f);
        }
        else
        {
            dcState = dcPrevIn = 0.0f;
            lpState = x;
        }

        // --- output
        return std::tanh (x * vol * 1.6f);
    }

private:
    float fs = 44100.0f;
    float dist = 0.0f, fuzzAmt = 0.0f, vol = 0.7f;
    float sDist = -1.0f, sFuzz = -1.0f;
    float dcA = 0.999f, dcState = 0.0f, dcPrevIn = 0.0f, lpState = 0.0f;
};

} // namespace squelch
