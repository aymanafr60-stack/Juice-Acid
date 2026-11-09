// Offline checks for Sequencer.h. Build: g++ -O1 -std=c++17 -I../Source seq_test.cpp
#include "Sequencer.h"
#include <cstdio>
#include <string>
using namespace squelch;

struct Rec : NoteSink
{
    struct E { long t; bool on; int n; int v; };
    std::vector<E> ev; long now = 0;
    void seqNoteOn (int n, int v) override { ev.push_back ({ now, true, n, v }); }
    void seqNoteOff (int n) override { ev.push_back ({ now, false, n, 0 }); }
};

static const double sr = 48000.0;

// run `blocks` blocks of 480 samples. host: if hostBpm>0 the host is playing from ppq0
static void run (Sequencer& s, SeqSettings st, Rec& r, int blocks, double hostBpm = 0, double ppq0 = 0,
                 std::vector<std::pair<long, std::pair<int,bool>>> midi = {})
{
    const int bs = 480;
    double ppq = ppq0;
    size_t mi = 0;
    for (int b = 0; b < blocks; ++b)
    {
        s.beginBlock (st, hostBpm > 0, ppq, hostBpm, r);
        for (int i = 0; i < bs; ++i)
        {
            while (mi < midi.size() && midi[mi].first == r.now) { s.midiNote (midi[mi].second.first, midi[mi].second.second, 100, r); ++mi; }
            s.tick (r);
            ++r.now;
        }
        if (hostBpm > 0) ppq += bs * hostBpm / 60.0 / sr;
    }
}

int main()
{
    int fails = 0;
    auto check = [&] (bool ok, const char* what) { std::printf ("%s  %s\n", ok ? "ok  " : "FAIL", what); if (! ok) ++fails; };

    { // 1. pattern on internal clock, 120 bpm, 1/16 -> a step every 6000 samples
        Sequencer s; s.prepare (sr); Rec r;
        SeqSettings st; st.mode = seqPattern; st.play = true; st.bpm = 120; st.gate = 0.5f;
        run (s, st, r, 200); // 96000 samples = 16 steps
        int ons = 0; for (auto& e : r.ev) if (e.on) ++ons;
        check (r.ev.size() > 0 && r.ev[0].on && r.ev[0].t == 0 && r.ev[0].n == 36 && r.ev[0].v == 120, "first step plays at once, C, accented");
        // step 1 (no slide): off at 3000, step 2 on at 6000
        check (r.ev[1].t == 3000 && ! r.ev[1].on, "gate 50% -> note off half a step later");
        check (r.ev[2].t == 6000 && r.ev[2].on, "next step one 16th later");
        // step 3 (index 2) has slide: next note-on before the old note-off at 18000
        bool slideOk = false;
        for (size_t i = 0; i + 1 < r.ev.size(); ++i)
            if (r.ev[i].t == 18000 && r.ev[i].on && r.ev[i].n == 39 && ! r.ev[i + 1].on && r.ev[i + 1].n == 48) slideOk = true;
        check (slideOk, "slide: new note on, then old note off (legato)");
        check (ons == 14, ("14 notes in 16 steps (2 rests), got " + std::to_string (ons)).c_str());
        check (s.currentStep.load() == 15, "current step reported");
    }
    { // 2. host sync: host starts at ppq 3.9 -> first step at ppq 4.0
        Sequencer s; s.prepare (sr); Rec r;
        SeqSettings st; st.mode = seqPattern;
        run (s, st, r, 20, 120.0, 3.9);
        const long expect = (long) std::ceil (0.1 / (120.0 / 60.0 / sr));
        check (! r.ev.empty() && std::labs (r.ev[0].t - expect) <= 1, "host sync: first note on the next 16th");
        check (s.currentStep.load() >= 0, "playing with host");
    }
    { // 3. swing: odd steps late
        Sequencer s; s.prepare (sr); Rec r;
        SeqSettings st; st.mode = seqPattern; st.play = true; st.bpm = 120; st.swing = 0.5f;
        for (int i = 0; i < 16; ++i) s.setStep (i, Step { 0, 0, false, false, true });
        run (s, st, r, 30);
        std::vector<long> ons; for (auto& e : r.ev) if (e.on) ons.push_back (e.t);
        check (ons.size() >= 3 && ons[1] == 6000 + 1500 && ons[2] == 12000, "swing 50% -> every 2nd step a quarter-step late");
    }
    { // 4. arp: hold C E G (60 64 67), up, 2 octaves, internal clock
        Sequencer s; s.prepare (sr); Rec r;
        SeqSettings st; st.mode = seqArp; st.bpm = 120; st.arpOct = 2; st.length = 16;
        run (s, st, r, 120, 0, 0, { {100, {60, true}}, {100, {64, true}}, {100, {67, true}} });
        std::vector<int> notes; for (auto& e : r.ev) if (e.on) notes.push_back (e.n);
        const std::vector<int> want = { 60, 64, 67, 72, 76, 79, 60, 64, 67 };
        bool ok = notes.size() >= want.size();
        for (size_t i = 0; ok && i < want.size(); ++i) ok = notes[i] == want[i];
        check (ok, "arp up over 2 octaves: C E G C' E' G' C ...");
        check (! r.ev.empty() && r.ev[0].t == 100, "arp starts the moment you press");
    }
    { // 5. arp stops on release (no latch), keeps going with latch
        Sequencer s; s.prepare (sr); Rec r;
        SeqSettings st; st.mode = seqArp; st.bpm = 120;
        run (s, st, r, 100, 0, 0, { {0, {60, true}}, {20000, {60, false}} });
        long lastOn = 0; for (auto& e : r.ev) if (e.on) lastOn = e.t;
        check (lastOn < 20000 && ! r.ev.back().on, "no latch: stops when released");

        Sequencer s2; s2.prepare (sr); Rec r2; st.latch = true;
        run (s2, st, r2, 100, 0, 0, { {0, {60, true}}, {20000, {60, false}} });
        long lastOn2 = 0; for (auto& e : r2.ev) if (e.on) lastOn2 = e.t;
        check (lastOn2 > 40000, "latch: keeps playing after release");
    }
    { // 6. chord: one key -> root 3rd 5th 7th in C minor
        Sequencer s; s.prepare (sr); Rec r;
        SeqSettings st; st.mode = seqArp; st.bpm = 120; st.chord = true; st.key = 0; st.scale = 0;
        run (s, st, r, 60, 0, 0, { {0, {60, true}} });
        std::vector<int> notes; for (auto& e : r.ev) if (e.on) notes.push_back (e.n);
        check (notes.size() >= 4 && notes[0] == 60 && notes[1] == 63 && notes[2] == 67 && notes[3] == 70, "chord from C in C minor: C Eb G Bb");
    }
    { // 7. print to steps
        Sequencer s; s.prepare (sr); Rec r;
        SeqSettings st; st.mode = seqArp; st.bpm = 120; st.length = 8; st.latch = true; st.arpAccent = 3;
        run (s, st, r, 5, 0, 0, { {0, {60, true}}, {0, {63, true}}, {0, {67, true}} });
        s.printRequest.store (true);
        run (s, st, r, 1);
        check (s.printOk.load() == 1, "print succeeded");
        const Step a = s.getStep (0), b = s.getStep (1), c = s.getStep (2);
        check (a.midiNote() == 36 && b.midiNote() == 39 && c.midiNote() == 43, "printed C Eb G folded into the bass range");
        check (a.accent && ! b.accent && s.getStep (4).accent, "accent every 4 printed");
    }
    { // 8. stopping the internal pattern releases the note
        Sequencer s; s.prepare (sr); Rec r;
        SeqSettings st; st.mode = seqPattern; st.play = true; st.bpm = 120; st.gate = 1.0f;
        for (int i = 0; i < 16; ++i) s.setStep (i, Step { 0, 0, false, true, true }); // all slides -> held
        run (s, st, r, 5);
        st.play = false;
        run (s, st, r, 1);
        check (! r.ev.empty() && ! r.ev.back().on, "PLAY off -> held note released");
    }

    { // 9. PATTERN mode: holding a key starts the pattern from step 1, transposed; release stops it
        Sequencer s; s.prepare (sr); Rec r;
        SeqSettings st; st.mode = seqPattern; st.bpm = 120;   // PLAY off, host stopped
        run (s, st, r, 200, 0, 0, { {1000, {72, true}}, {60000, {72, false}} });
        check (! r.ev.empty() && r.ev[0].on && r.ev[0].t == 1000 && r.ev[0].n == 36 + 12,
               "key press starts step 1 at once, transposed (+12 for C an octave up)");
        long lastOn = 0; for (auto& e : r.ev) if (e.on) lastOn = e.t;
        check (lastOn < 60000 && ! r.ev.back().on, "releasing the key stops the pattern");
    }
    { // 10. PATTERN with PLAY on: a key only transposes, doesn't restart
        Sequencer s; s.prepare (sr); Rec r;
        SeqSettings st; st.mode = seqPattern; st.bpm = 120; st.play = true;
        run (s, st, r, 100, 0, 0, { {30000, {60, true}}, {40000, {60, false}} });
        bool onGrid = true, playingAfter = false;
        for (auto& e : r.ev) { if (e.on && e.t % 6000 != 0) onGrid = false; if (e.on && e.t > 40000) playingAfter = true; }
        check (onGrid && playingAfter, "with PLAY on, a key doesn't restart the pattern, it keeps running in time");
    }

    std::printf (fails ? "\n%d FAILED\n" : "\nall passed\n", fails);
    return fails ? 1 : 0;
}
