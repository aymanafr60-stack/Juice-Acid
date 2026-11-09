// Squelch — acid line generator.
// Pure C++ (no JUCE). Writes a 16-step pattern that follows the habits of classic acid lines
// rather than picking notes at random:
//
//   * a short motif (4 or 8 steps) that repeats, with small variations each time round
//   * the root dominates and anchors step 1; repeated "pedal" notes are common
//   * other notes come from the chosen scale, weighted towards the 5th, b3 and b7,
//     with the b2 / 4th / 6th used sparingly for tension
//   * octave jumps (mostly on the root), a few rests, never on step 1
//   * accents on a handful of steps (often the motif start and off-beats)
//   * slides between different pitches, never into a rest
//   * the last quarter of the bar often gets a "turnaround" variation

#pragma once

#include "Sequencer.h"
#include <random>

namespace squelch
{

class AcidGenerator
{
public:
    // Fills all 16 steps of `seq`.
    static void generate (Sequencer& seq, int key, int scaleIndex, uint32_t seed)
    {
        std::mt19937 rng (seed);
        const Ctx ctx { key, scales[std::clamp (scaleIndex, 0, numScales - 1)] };

        Step out[maxSteps];
        for (int attempt = 0; attempt < 20; ++attempt)
        {
            build (ctx, rng, out);
            if (isGood (out)) break;
        }
        for (int i = 0; i < maxSteps; ++i) seq.setStep (i, out[i]);
    }

    // Small changes to the current pattern: 2-3 steps get a new note, accent, slide or rest.
    static void mutate (Sequencer& seq, int key, int scaleIndex, int length, uint32_t seed)
    {
        std::mt19937 rng (seed);
        const Ctx ctx { key, scales[std::clamp (scaleIndex, 0, numScales - 1)] };
        const int len = std::clamp (length, 1, maxSteps);
        if (len < 2) return;

        std::uniform_int_distribution<int> stepDist (1, len - 1); // keep step 1 as the anchor
        const int changes = 2 + (int) (rng() % 2);
        for (int c = 0; c < changes; ++c)
        {
            const int i = stepDist (rng);
            Step s = seq.getStep (i);
            const float r = uni (rng);
            if (r < 0.5f)       { pickNote (ctx, rng, s, false); s.on = true; }
            else if (r < 0.7f)  s.accent = ! s.accent;
            else if (r < 0.85f) s.slide = ! s.slide;
            else                s.on = ! s.on;
            seq.setStep (i, s);
        }

        // no slides into rests
        for (int i = 0; i < len; ++i)
        {
            Step s = seq.getStep (i);
            if (s.slide && ! seq.getStep ((i + 1) % len).on) { s.slide = false; seq.setStep (i, s); }
        }
    }

    // ---------------------------------------------------------- helpers (public for tests)
    struct Ctx { int key; const Scale& scale; };

    // Absolute semitone (relative to the pattern's C) -> step note/oct, kept inside the 3-octave range.
    static void setPitch (Step& s, int semis)
    {
        while (semis > 23) semis -= 12;
        while (semis < -12) semis += 12;
        s.oct = semis < 0 ? -1 : (semis >= 12 ? 1 : 0);
        s.note = semis - 12 * s.oct;
    }

    static int pitchOf (const Step& s) { return s.oct * 12 + s.note; }

private:
    static float uni (std::mt19937& rng) { return std::uniform_real_distribution<float> (0.0f, 1.0f) (rng); }

    // Pick a scale degree with acid-typical weights and place it.
    static void pickNote (const Ctx& ctx, std::mt19937& rng, Step& s, bool rootOnly)
    {
        const auto& sc = ctx.scale;
        int degree = 0;
        if (! rootOnly)
        {
            // weights per degree for 7-note scales: root, 2nd, 3rd, 4th, 5th, 6th, 7th
            static constexpr float w7[7] = { 5.0f, 0.9f, 2.0f, 0.9f, 2.2f, 0.6f, 1.6f };
            // pentatonic: root, b3, 4, 5, b7
            static constexpr float w5[5] = { 5.0f, 2.0f, 1.0f, 2.2f, 1.6f };
            const float* w = sc.size == 7 ? w7 : w5;
            float total = 0.0f;
            for (int d = 0; d < sc.size; ++d) total += w[d];
            float r = uni (rng) * total;
            for (int d = 0; d < sc.size; ++d) { r -= w[d]; if (r <= 0.0f) { degree = d; break; } }
        }

        // the root is placed in the bottom octave of the pattern's middle row
        const int rootSemis = ctx.key;               // 0..11
        int semis = rootSemis + sc.steps[degree];

        // octave choice: roots jump up/down most, other notes occasionally go up
        const float r = uni (rng);
        if (degree == 0)
        {
            if (r < 0.22f) semis += 12;
            else if (r < 0.32f) semis -= 12;
        }
        else if (r < 0.15f) semis += 12;
        else if (r < 0.2f && (degree == sc.size - 1 || sc.steps[degree] >= 7)) semis -= 12; // low 5th / b7 under the root

        setPitch (s, semis);
    }

    static void build (const Ctx& ctx, std::mt19937& rng, Step* out)
    {
        const int motifLen = uni (rng) < 0.55f ? 4 : 8;
        Step motif[8];

        for (int i = 0; i < motifLen; ++i)
        {
            Step s;
            s.on = i == 0 || uni (rng) > 0.14f;
            if (i == 0)
                pickNote (ctx, rng, s, true);
            else if (uni (rng) < 0.33f && motif[i - 1].on)
                s.note = motif[i - 1].note, s.oct = motif[i - 1].oct;  // pedal: repeat the last note
            else
                pickNote (ctx, rng, s, false);

            const bool offbeat = (i % 2) == 1;
            s.accent = i == 0 ? uni (rng) < 0.6f : uni (rng) < (offbeat ? 0.3f : 0.18f);
            motif[i] = s;
        }

        // repeat the motif across 16 steps, varying later passes
        for (int i = 0; i < maxSteps; ++i)
        {
            Step s = motif[i % motifLen];
            const int pass = i / motifLen;
            if (pass > 0 && i % motifLen != 0)
            {
                const float r = uni (rng);
                if (r < 0.14f)      { pickNote (ctx, rng, s, false); s.on = true; }
                else if (r < 0.2f)  s.accent = ! s.accent;
            }
            out[i] = s;
        }

        // turnaround: the last 4 steps often get new, higher-energy notes
        if (uni (rng) < 0.5f)
            for (int i = 12; i < maxSteps; ++i)
                if (uni (rng) < 0.55f)
                {
                    pickNote (ctx, rng, out[i], false);
                    out[i].on = true;
                    if (uni (rng) < 0.3f) out[i].accent = true;
                }

        // slides: between different pitches, never into a rest
        for (int i = 0; i < maxSteps; ++i)
        {
            const Step& next = out[(i + 1) % maxSteps];
            out[i].slide = out[i].on && next.on && pitchOf (next) != pitchOf (out[i]) && uni (rng) < 0.22f;
        }

        out[0].on = true;
    }

    // Reject lines that don't sound like acid lines.
    static bool isGood (const Step* p)
    {
        int ons = 0, accents = 0, slides = 0, distinct = 0;
        int seen[36] = {};
        for (int i = 0; i < maxSteps; ++i)
        {
            if (! p[i].on) continue;
            ++ons;
            accents += p[i].accent;
            slides += p[i].slide;
            const int k = pitchOf (p[i]) + 12;
            if (seen[k]++ == 0) ++distinct;
        }
        return ons >= 11 && accents >= 2 && accents <= 7 && slides >= 1 && slides <= 6 && distinct >= 3;
    }
};

} // namespace squelch
