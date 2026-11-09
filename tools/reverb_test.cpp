// Checks ReverbFx: pre-delay, low cut, ducking. Built as a JUCE console app (see tools/CMakeLists in test build).
#include "ReverbFx.h"
#include <cstdio>
using namespace squelch;

static int fails = 0;
static void check (bool ok, const char* what) { std::printf ("%s  %s\n", ok ? "ok  " : "FAIL", what); fails += ! ok; }

// impulse in, returns first sample index where the added reverb exceeds a threshold, and energy
static void run (ReverbParams p, int& onset, double& energy, bool sustainDry = false)
{
    const double sr = 48000.0; const int n = 48000, bs = 480;
    ReverbFx r; r.prepare (sr, bs);
    std::vector<float> L (bs), R (bs);
    onset = -1; energy = 0;
    for (int b = 0; b < n / bs; ++b)
    {
        for (int i = 0; i < bs; ++i)
        {
            const int t = b * bs + i;
            const float dry = sustainDry ? 0.3f * std::sin (t * 0.05f) : (t < 64 ? std::sin (t * 0.3f) : 0.0f);
            L[(size_t) i] = R[(size_t) i] = dry;
        }
        std::vector<float> dryL = L;
        r.process (p, L.data(), R.data(), bs);
        for (int i = 0; i < bs; ++i)
        {
            const float wet = L[(size_t) i] - dryL[(size_t) i];
            if (onset < 0 && std::abs (wet) > 1e-4f) onset = b * bs + i;
            energy += (double) wet * wet;
        }
    }
}

int main()
{
    int o0, o1; double e0, e1;
    ReverbParams p; p.on = true; p.mix = 1.0f; p.preDelayMs = 0.0f;
    run (p, o0, e0);
    p.preDelayMs = 100.0f;
    run (p, o1, e1);
    std::printf ("      onset %d -> %d samples\n", o0, o1);
    // juce::Reverb's first comb adds 1116 samples @44.1k (1215 @48k) on its own
    check (std::abs (o1 - (4800 + 1215)) < 40, "pre-delay 100 ms: tail starts 100 ms + the reverb's own 25 ms later");

    ReverbParams lo; lo.on = true; lo.mix = 1.0f; lo.lowCutHz = 20.0f;
    ReverbParams hi = lo; hi.lowCutHz = 1000.0f;
    int a, b; double ea, eb;
    run (lo, a, ea); run (hi, b, eb);
    check (eb < ea * 0.8, "low cut 1 kHz removes energy from the reverb");

    ReverbParams nd; nd.on = true; nd.mix = 1.0f; nd.ducking = 0.0f;
    ReverbParams dk = nd; dk.ducking = 1.0f;
    double en, ed;
    run (nd, a, en, true); run (dk, b, ed, true);
    check (ed < en * 0.3, "ducking 100% pushes the reverb down under a playing note");

    ReverbParams off; off.on = false;
    double eo; run (off, a, eo);
    check (eo == 0.0, "reverb off adds nothing");

    std::printf (fails ? "\n%d FAILED\n" : "\nall passed\n", fails);
    return fails ? 1 : 0;
}
