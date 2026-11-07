// Squelch — acid bass voice (Phase 1)
// Pure C++ (no JUCE) so the engine can be tested offline.
//
// Signal path, modelled on how a 303-style box behaves rather than on its schematic:
//   Oscillator (saw / square, PolyBLEP anti-aliased) + sub-oscillator (square, -1 oct)
//   -> 4-pole resonant ladder filter with saturation (the "squelch")
//   -> VCA -> gentle high-pass (removes sub-rumble / DC) -> soft clip
//
// Behaviour that makes it "acid":
//   * Monophonic, last-note priority.
//   * Overlapping notes = SLIDE: pitch glides, envelopes are NOT retriggered.
//   * Velocity >= accentThreshold = ACCENT: louder, filter opens harder, and the
//     filter envelope decay is forced short.
//   * Accent sweep: accent energy is smoothed through a "capacitor" whose charge
//     time grows with resonance, so a run of accented notes winds the filter
//     further open each time (the classic accent build-up).

#pragma once

#include <cmath>
#include <algorithm>
#include <vector>

namespace squelch
{

struct VoiceParams
{
    int   waveform     = 0;     // 0 = saw, 1 = square, 2 = pulse, 3 = super (5 detuned saws)
    float shape        = 0.5f;  // pulse: width 0..1 (0.5 = square) / super: detune 0..1
    float tuneSemis    = 0.0f;  // -12 .. +12
    float cutoff       = 0.35f; // 0 .. 1  (mapped exponentially)
    float resonance    = 0.6f;  // 0 .. 1
    float envMod       = 0.5f;  // 0 .. 1
    float decay        = 0.4f;  // 0 .. 1  (normal filter-env decay)
    float accent       = 0.6f;  // 0 .. 1
    float slideTimeMs  = 60.0f; // 10 .. 500
    float subLevel     = 0.0f;  // 0 .. 1
    int   accentThreshold = 100; // MIDI velocity at/above which a note is accented
};

class AcidVoice
{
public:
    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        heldNotes.reserve (128); // no allocation on the audio thread later
        reset();
    }

    void reset()
    {
        heldNotes.clear();
        gate = false;
        phase = subPhase = 0.0f;
        static constexpr float starts[5] = { 0.0f, 0.31f, 0.57f, 0.13f, 0.79f }; // spread so the saws don't start in phase
        for (int i = 0; i < 5; ++i) superPhase[i] = starts[i];
        s1 = s2 = s3 = s4 = 0.0f;
        filterEnv = ampEnv = accentEnv = accentSweep = 0.0f;
        ampStage = AmpStage::idle;
        hpState = hpPrevIn = 0.0f;
        smoothCutoff = smoothRes = smoothEnvMod = -1.0f; // re-init on first sample
    }

    void setParams (const VoiceParams& p) { params = p; }

    // ---------------------------------------------------------------- MIDI
    void noteOn (int note, int velocity)
    {
        const bool legato = ! heldNotes.empty();
        heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
        heldNotes.push_back (note);

        targetPitch = (float) note;
        accented = velocity >= params.accentThreshold;

        if (legato)
        {
            sliding = true;            // glide, no retrigger
            if (accented) accentEnv = 1.0f;
        }
        else
        {
            sliding = false;
            currentPitch = targetPitch; // jump straight to the note
            trigger();
        }
        gate = true;
    }

    void noteOff (int note)
    {
        heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());

        if (heldNotes.empty())
        {
            gate = false;
            ampStage = AmpStage::release;
        }
        else
        {
            targetPitch = (float) heldNotes.back(); // slide back to the still-held note
            sliding = true;
        }
    }

    void allNotesOff()
    {
        heldNotes.clear();
        gate = false;
        ampStage = AmpStage::release;
    }

    // ---------------------------------------------------------------- audio
    float processSample()
    {
        if (ampStage == AmpStage::idle)
            return 0.0f;

        // --- parameter smoothing (avoids zipper noise when knobs move)
        const float sm = 1.0f - std::exp (-1.0f / (0.005f * fs)); // ~5 ms
        if (smoothCutoff < 0.0f) { smoothCutoff = params.cutoff; smoothRes = params.resonance; smoothEnvMod = params.envMod; }
        smoothCutoff += (params.cutoff    - smoothCutoff) * sm;
        smoothRes    += (params.resonance - smoothRes)    * sm;
        smoothEnvMod += (params.envMod    - smoothEnvMod) * sm;

        // --- pitch / slide
        if (sliding)
        {
            const float slideCoeff = 1.0f - std::exp (-1.0f / (params.slideTimeMs * 0.001f * fs / 3.0f));
            currentPitch += (targetPitch - currentPitch) * slideCoeff;
        }
        else
        {
            currentPitch = targetPitch;
        }

        const float pitch = currentPitch + params.tuneSemis;
        const float freq  = 440.0f * std::exp2 ((pitch - 69.0f) / 12.0f);

        // --- oscillators
        const float dt = std::min (freq / fs, 0.45f);
        float osc = 0.0f;
        switch (params.waveform)
        {
            case 0: // saw
                osc = 2.0f * phase - 1.0f;
                osc -= polyBlep (phase, dt);
                break;

            case 1: // square
                osc = squareish (phase, 0.5f, dt) * 0.7f; // level-matched to the saw
                break;

            case 2: // pulse with adjustable width (5% .. 95%)
            {
                const float pw = 0.05f + 0.9f * std::clamp (params.shape, 0.0f, 1.0f);
                osc = squareish (phase, pw, dt) * 0.7f;
                osc -= 0.7f * (2.0f * pw - 1.0f); // remove the DC offset narrow pulses create
                break;
            }

            case 3: // super: five saws spread up to +-35 cents
            {
                static constexpr float spread[5] = { 0.0f, -1.0f, 1.0f, -0.45f, 0.45f };
                const float cents = 35.0f * std::clamp (params.shape, 0.0f, 1.0f);
                for (int i = 0; i < 5; ++i)
                {
                    const float d = std::min (dt * std::exp2 (spread[i] * cents / 1200.0f), 0.45f);
                    float v = 2.0f * superPhase[i] - 1.0f;
                    v -= polyBlep (superPhase[i], d);
                    osc += v;
                    superPhase[i] += d;
                    if (superPhase[i] >= 1.0f) superPhase[i] -= 1.0f;
                }
                osc *= 0.5f; // ~ level-match five saws to one
                break;
            }

            default: break;
        }
        phase += dt;
        if (phase >= 1.0f) phase -= 1.0f;

        const float subDt = dt * 0.5f;
        float sub = subPhase < 0.5f ? 1.0f : -1.0f;
        sub += polyBlep (subPhase, subDt);
        sub -= polyBlep (std::fmod (subPhase + 0.5f, 1.0f), subDt);
        subPhase += subDt;
        if (subPhase >= 1.0f) subPhase -= 1.0f;

        const float oscMix = osc + sub * params.subLevel * 0.8f;

        // --- envelopes
        // Filter env: exponential decay. Accented notes use a fixed short decay.
        const float normalDecaySec = 0.2f * std::pow (12.5f, params.decay); // 0.2 s .. 2.5 s
        const float decaySec = accented ? 0.2f : normalDecaySec;
        filterEnv *= std::exp (-4.6f / (decaySec * fs)); // ~-40 dB over decaySec

        // Accent env always short; feeds the accent-sweep "capacitor".
        accentEnv *= std::exp (-4.6f / (0.2f * fs));
        const float accentTarget = accented ? accentEnv * params.accent : 0.0f;
        // Higher resonance = slower charge = stronger build-up on consecutive accents.
        const float sweepTime = 0.008f + 0.07f * smoothRes;
        accentSweep += (accentTarget - accentSweep) * (1.0f - std::exp (-1.0f / (sweepTime * fs)));

        // Amp env: fast attack, long natural decay while held, quick release.
        switch (ampStage)
        {
            case AmpStage::attack:
                ampEnv += 1.0f / (0.003f * fs);
                if (ampEnv >= 1.0f) { ampEnv = 1.0f; ampStage = AmpStage::hold; }
                break;
            case AmpStage::hold:
                ampEnv *= std::exp (-1.0f / (3.0f * fs));
                break;
            case AmpStage::release:
                ampEnv *= std::exp (-1.0f / (0.008f * fs));
                if (ampEnv < 1.0e-5f) { ampEnv = 0.0f; ampStage = AmpStage::idle; }
                break;
            case AmpStage::idle: break;
        }

        // --- filter cutoff
        // Knob: ~50 Hz .. ~5 kHz, exponential. Env mod adds up to ~4.5 octaves.
        const float baseHz   = 50.0f * std::pow (100.0f, smoothCutoff);
        const float envOct   = (accented ? 0.0f : smoothEnvMod * 4.5f * filterEnv)
                             + accentSweep * 4.0f
                             + (accented ? smoothEnvMod * 2.5f * filterEnv : 0.0f);
        float fc = baseHz * std::exp2 (envOct);
        fc = std::clamp (fc, 20.0f, 0.42f * fs);
        lastCutoffHz = fc;

        float y = ladder (oscMix * 0.5f, fc, smoothRes);

        // --- VCA (accent makes it louder)
        const float accentGain = accented ? 1.0f + 0.7f * params.accent : 1.0f;
        y *= ampEnv * accentGain;

        // --- gentle 25 Hz high-pass (DC and rumble)
        const float hpA = std::exp (-2.0f * 3.14159265f * 25.0f / fs);
        const float hpOut = hpA * (hpState + y - hpPrevIn);
        hpPrevIn = y;
        hpState = hpOut;

        // --- output (raw; distortion, fuzz, volume and clipping happen in DriveStage)
        return hpOut;
    }

    bool isActive() const { return ampStage != AmpStage::idle; }

    // Current (modulated) filter cutoff, for the on-screen filter display.
    float getLastCutoffHz() const { return lastCutoffHz; }

private:
    enum class AmpStage { idle, attack, hold, release };

    void trigger()
    {
        filterEnv = 1.0f;
        accentEnv = accented ? 1.0f : 0.0f;
        ampStage  = AmpStage::attack;
    }

    static float squareish (float ph, float pw, float dt)
    {
        float v = ph < pw ? 1.0f : -1.0f;
        v += polyBlep (ph, dt);
        v -= polyBlep (std::fmod (ph + 1.0f - pw, 1.0f), dt);
        return v;
    }

    static float polyBlep (float t, float dt)
    {
        if (t < dt)        { t /= dt;               return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt;   return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    // Zero-delay-feedback 4-pole ladder (Zavalishin topology) with a saturating
    // feedback path. resonance 0..1 maps to feedback k 0..~3.95 (edge of self-oscillation).
    float ladder (float x, float fc, float res)
    {
        const float g  = std::tan (3.14159265f * fc / fs);
        const float G  = g / (1.0f + g);
        const float inv = 1.0f / (1.0f + g);
        const float k  = 3.95f * res;

        const float G2 = G * G, G3 = G2 * G, G4 = G3 * G;
        const float sigma = G3 * s1 * inv + G2 * s2 * inv + G * s3 * inv + s4 * inv;

        // Partial bass compensation so high resonance doesn't hollow out the low end.
        const float xin = x * (1.0f + 0.6f * k);

        const float y4est = (G4 * xin + sigma) / (1.0f + k * G4);
        const float u = std::tanh (xin - k * y4est);

        const float y1 = onePole (u,  s1, G);
        const float y2 = onePole (y1, s2, G);
        const float y3 = onePole (y2, s3, G);
        const float y4 = onePole (y3, s4, G);

        // guard against any numerical blow-up
        if (! std::isfinite (y4)) { s1 = s2 = s3 = s4 = 0.0f; return 0.0f; }
        return y4;
    }

    static float onePole (float x, float& s, float G)
    {
        const float v = (x - s) * G;
        const float y = v + s;
        s = y + v;
        return y;
    }

    VoiceParams params;
    float fs = 44100.0f;

    std::vector<int> heldNotes;
    bool gate = false, sliding = false, accented = false;
    float currentPitch = 48.0f, targetPitch = 48.0f;

    float phase = 0.0f, subPhase = 0.0f;
    float superPhase[5] = {};
    float lastCutoffHz = 1000.0f;
    float s1 = 0, s2 = 0, s3 = 0, s4 = 0;
    float filterEnv = 0, ampEnv = 0, accentEnv = 0, accentSweep = 0;
    AmpStage ampStage = AmpStage::idle;
    float hpState = 0, hpPrevIn = 0;
    float smoothCutoff = -1.0f, smoothRes = -1.0f, smoothEnvMod = -1.0f;
};

} // namespace squelch
