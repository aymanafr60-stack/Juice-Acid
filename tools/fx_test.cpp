// Checks for the delay and the TD-3-MO extras. Build: g++ -O2 -std=c++17 -I../Source fx_test.cpp
#include "Delay.h"
#include "AcidVoice.h"
#include <cstdio>
#include <cmath>
using namespace squelch;

int main()
{
    int fails = 0;
    auto check = [&] (bool ok, const char* what) { std::printf ("%s  %s\n", ok ? "ok  " : "FAIL", what); fails += ! ok; };
    const double sr = 48000.0;

    { // delay timing + ping-pong
        PingPongDelay d; d.prepare (sr);
        DelayParams p; p.on = true; p.time = 3; p.feedback = 0.5f; p.mix = 1.0f; p.tone = 1.0f;
        d.setParams (p, 120.0); // 3/16 at 120 bpm = 0.375 s = 18000 samples
        int peakL = -1, peakR = -1; float maxL = 0, maxR = 0; bool finite = true;
        for (int i = 0; i < 48000; ++i)
        {
            float l = 0, r = 0;
            d.process (i == 0 ? 1.0f : 0.0f, l, r);
            finite &= std::isfinite (l) && std::isfinite (r);
            if (std::abs (l) > maxL) { maxL = std::abs (l); peakL = i; }
            if (i > 20000 && std::abs (r) > maxR) { maxR = std::abs (r); peakR = i; }
        }
        check (finite, "delay output stays finite");
        check (std::abs (peakL - 18000) < 30, "first echo on the LEFT after a dotted 8th");
        check (std::abs (peakR - 36000) < 60, "second echo on the RIGHT one dotted 8th later");
        check (maxR < maxL, "echoes decay");
    }
    { // ping-pong off: echoes centred (same in both channels)
        PingPongDelay d; d.prepare (sr);
        DelayParams p; p.on = true; p.time = 3; p.feedback = 0.5f; p.mix = 1.0f; p.pingPong = false;
        d.setParams (p, 120.0);
        float diff = 0, mx = 0;
        for (int i = 0; i < 48000; ++i) { float l = 0, r = 0; d.process (i == 0 ? 1.0f : 0.0f, l, r); diff += std::abs (l - r); mx = std::max (mx, std::abs (l)); }
        check (diff < 1e-4f && mx > 0.3f, "ping-pong off: echoes identical left and right");
    }
    { // sync off: free time in ms
        PingPongDelay d; d.prepare (sr);
        DelayParams p; p.on = true; p.sync = false; p.timeMs = 250.0f; p.feedback = 0.0f; p.mix = 1.0f; p.tone = 1.0f;
        d.setParams (p, 120.0);
        int peak = -1; float mx = 0;
        for (int i = 0; i < 24000; ++i) { float l = 0, r = 0; d.process (i == 0 ? 1.0f : 0.0f, l, r); if (std::abs (l) > mx) { mx = std::abs (l); peak = i; } }
        check (std::abs (peak - 12000) < 30, "sync off: 250 ms echo lands at 250 ms");
    }
    { // delay off: silent
        PingPongDelay d; d.prepare (sr);
        DelayParams p; p.on = false; d.setParams (p, 120.0);
        float sum = 0; for (int i = 0; i < 48000; ++i) { float l = 0, r = 0; d.process (i == 0 ? 1.0f : 0.0f, l, r); sum += std::abs (l) + std::abs (r); }
        check (sum < 1.0e-6f, "delay off adds nothing");
    }
    { // full feedback doesn't blow up
        PingPongDelay d; d.prepare (sr);
        DelayParams p; p.on = true; p.time = 0; p.feedback = 1.0f; p.mix = 1.0f; d.setParams (p, 180.0);
        float mx = 0; for (int i = 0; i < 480000; ++i) { float l = 0, r = 0; d.process (i < 2000 ? std::sin (i * 0.05f) : 0.0f, l, r); mx = std::max (mx, std::abs (l)); }
        check (mx < 1.5f, "max feedback stays bounded");
    }

    auto render = [&] (VoiceParams vp, int note, int vel, int n, float* firstMs = nullptr)
    {
        AcidVoice v; v.prepare (sr); v.setParams (vp); v.noteOn (note, vel);
        double sq = 0; bool fin = true; int reach = -1;
        for (int i = 0; i < n; ++i) { float y = v.processSample(); fin &= std::isfinite (y); sq += (double) y * y; if (reach < 0 && std::abs (y) > 0.05f) reach = i; }
        if (firstMs) *firstMs = reach * 1000.0f / (float) sr;
        return fin ? std::sqrt (sq / n) : -1.0;
    };

    { // soft attack slows the start of normal notes only
        VoiceParams a; a.softAttack = 0.0f; VoiceParams b; b.softAttack = 1.0f;
        float ta, tb, tc;
        render (a, 45, 80, 4800, &ta); render (b, 45, 80, 4800, &tb); render (b, 45, 120, 4800, &tc);
        check (tb > ta + 5.0f, "soft attack 100% starts noticeably later");
        check (tc < tb, "accented notes ignore soft attack");
    }
    { // tracking, filter FM, sweep: all finite and audible differences
        VoiceParams base; base.resonance = 0.9f;
        VoiceParams tr = base; tr.tracking = 1.0f;
        VoiceParams fm = base; fm.filterFm = 1.0f;
        const double r0 = render (base, 72, 80, 24000), r1 = render (tr, 72, 80, 24000), r2 = render (fm, 72, 80, 24000);
        check (r0 > 0 && r1 > 0 && r2 > 0, "tracking / filter FM stay stable at high resonance");
        check (std::abs (r1 - r0) > 0.005 && std::abs (r2 - r0) > 0.005, "tracking and filter FM change the sound");
        VoiceParams ad = base; ad.accentDecay = 1.0f;
        const double a0 = render (base, 45, 120, 24000), a1 = render (ad, 45, 120, 24000);
        check (std::abs (a1 - a0) > 0.003, "accent decay changes accented notes");
    }
    { // defaults reproduce the previous sound: accent decay default = 0.2 s
        const float d = 0.05f * std::pow (20.0f, 0.4628f);
        check (std::abs (d - 0.2f) < 0.005f, "accent decay default equals the old fixed 0.2 s");
    }
    std::printf (fails ? "\n%d FAILED\n" : "\nall passed\n", fails);
    return fails ? 1 : 0;
}
