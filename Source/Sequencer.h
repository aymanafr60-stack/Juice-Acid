// Squelch — 16-step sequencer and arpeggiator.
// Pure C++ (no JUCE) so it can be tested offline.
//
// The processor calls, per audio block:
//   beginBlock (host info)  ->  for each sample: [midiNote(...) at its position]  tick (sink)
// and the sequencer sends note on/off to the sink (the synth voice) sample-accurately.
//
// Timing: when the host (FL) is playing, steps lock to the host's beat position, so
// step 1 always lands on the bar. When the host is stopped (or in the standalone app),
// an internal clock runs at the BPM setting.
//
// Slides work like a 303: a step with SLIDE holds its note into the next step and the
// voice glides to it (same pitch = tied note).

#pragma once

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>

namespace squelch
{

constexpr int maxSteps = 16;
constexpr int seqBaseNote = 36;      // the pattern's middle octave starts here
constexpr int arpMaxNotes = 64;

enum SeqMode { seqOff = 0, seqPattern = 1, seqArp = 2 };

struct Step
{
    int  note = 0;      // 0..11 semitone within the octave
    int  oct = 0;       // -1, 0, +1
    bool accent = false;
    bool slide = false;
    bool on = true;     // false = rest

    int encode() const
    {
        return (note & 15) | ((oct + 1) & 3) << 4 | (accent ? 1 << 6 : 0) | (slide ? 1 << 7 : 0) | (on ? 1 << 8 : 0);
    }
    static Step decode (int v)
    {
        Step s;
        s.note = std::clamp (v & 15, 0, 11);
        s.oct = std::clamp (((v >> 4) & 3) - 1, -1, 1);
        s.accent = (v >> 6) & 1;
        s.slide = (v >> 7) & 1;
        s.on = (v >> 8) & 1;
        return s;
    }
    int midiNote() const { return seqBaseNote + 12 * oct + note; }
};

struct SeqSettings
{
    int   mode = seqOff;
    int   length = 16;    // 1..16
    int   rate = 1;       // 0 = 1/8, 1 = 1/16, 2 = 1/32, 3 = 1/8T, 4 = 1/16T
    float swing = 0.0f;   // 0..1  (delays every second step by up to half a step)
    float gate = 0.5f;    // 0.05..1 of a step
    float bpm = 130.0f;   // internal clock
    bool  play = false;   // run the pattern on the internal clock when the host isn't playing

    int   arpMode = 0;    // 0 Up, 1 Down, 2 Up-Down, 3 Random, 4 As Played
    int   arpOct = 1;     // 1..4
    int   arpAccent = 0;  // 0 Off, 1 Every 2, 2 Every 3, 3 Every 4, 4 Every 5, 5 Every 7, 6 Auto
    float arpSlide = 0.0f;// 0..1 probability
    bool  latch = false;
    bool  chord = false;

    int   key = 0;        // 0 = C .. 11 = B
    int   scale = 0;      // see scales below
};

struct Scale { const char* name; int size; int steps[7]; };
inline const Scale scales[] =
{
    { "Minor",      7, { 0, 2, 3, 5, 7, 8, 10 } },
    { "Phrygian",   7, { 0, 1, 3, 5, 7, 8, 10 } },
    { "Dorian",     7, { 0, 2, 3, 5, 7, 9, 10 } },
    { "Harm Minor", 7, { 0, 2, 3, 5, 7, 8, 11 } },
    { "Major",      7, { 0, 2, 4, 5, 7, 9, 11 } },
    { "Minor Pent", 5, { 0, 3, 5, 7, 10, 0, 0 } },
};
constexpr int numScales = (int) (sizeof (scales) / sizeof (scales[0]));

struct NoteSink
{
    virtual ~NoteSink() = default;
    virtual void seqNoteOn (int note, int velocity) = 0;
    virtual void seqNoteOff (int note) = 0;
};

class Sequencer
{
public:
    // ---------------------------------------------------------- shared with the UI
    std::array<std::atomic<int>, maxSteps> pattern;
    std::atomic<int> currentStep { -1 };   // step index being played, -1 when stopped
    std::atomic<int> patternVersion { 0 }; // bumps when the audio thread rewrites the pattern
    std::atomic<bool> printRequest { false };
    std::atomic<int> printOk { 0 }, printFail { 0 };
    std::atomic<bool> hostClock { false };   // true when following the host's tempo

    Sequencer()
    {
        // a starter acid line in C, so PLAY makes sound right away
        static const Step starter[maxSteps] =
        {
            { 0, 0, true,  false, true }, { 0, 0, false, false, true }, { 0, 1, false, true,  true }, { 3, 0, false, false, true },
            { 0, 0, true,  false, true }, { 0, 0, false, false, false },{ 7, 0, false, true,  true }, { 5, 0, false, false, true },
            { 0, 0, true,  false, true }, { 10,-1, false, false, true }, { 0, 0, false, true,  true }, { 0, 1, true,  false, true },
            { 3, 0, false, false, true }, { 0, 0, false, false, false },{ 5, 0, true,  true,  true }, { 7, 0, false, false, true },
        };
        for (int i = 0; i < maxSteps; ++i) pattern[(size_t) i].store (starter[i].encode());
        held.reserve (arpMaxNotes);
        latched.reserve (arpMaxNotes);
        work.reserve (arpMaxNotes * 4);
        work2.reserve (arpMaxNotes * 4);
    }

    void setStep (int i, const Step& s)
    {
        if (i >= 0 && i < maxSteps) { pattern[(size_t) i].store (s.encode()); patternVersion.fetch_add (1); }
    }
    Step getStep (int i) const { return Step::decode (pattern[(size_t) std::clamp (i, 0, maxSteps - 1)].load()); }

    // ---------------------------------------------------------- audio thread
    void prepare (double sampleRate) { sr = sampleRate; reset(); }

    void reset()
    {
        running = false; sounding = -1; offPending = false; prevSlide = false;
        held.clear(); latched.clear(); physicalHeld = 0;
        internalPpq = 0.0; lastPpq = -1.0;
        currentStep.store (-1);
    }

    // Call once per block, before any tick(). Pass the sink so mode changes can silence notes.
    void beginBlock (const SeqSettings& s, bool hostPlaying, double hostPpq, double hostBpm, NoteSink& sink)
    {
        if (s.mode != settings.mode)
        {
            stopSounding (sink);
            held.clear(); latched.clear(); physicalHeld = 0;
            running = false;
        }
        if (! s.latch) latched.clear();

        const bool rateChanged = s.rate != settings.rate;
        settings = s;
        settings.length = std::clamp (settings.length, 1, maxSteps);
        settings.rate = std::clamp (settings.rate, 0, 4);
        stepPpq = rateToPpq (settings.rate);

        // clock source for this block
        useHost = hostPlaying;
        hostClock.store (hostPlaying);
        const double bpm = useHost ? (hostBpm > 1.0 ? hostBpm : 120.0) : std::clamp ((double) settings.bpm, 20.0, 300.0);
        ppqPerSample = bpm / 60.0 / sr;

        // should the sequencer be running?
        bool shouldRun = false;
        if (settings.mode == seqPattern) shouldRun = useHost || settings.play || ! held.empty(); // a held key also plays it
        if (settings.mode == seqArp)     shouldRun = ! source().empty();

        if (shouldRun && (! running || rateChanged || useHost != lastUseHost))
        {
            if (! useHost && ! (running && lastUseHost == useHost)) internalPpq = 0.0;
            needResync = true;
        }
        if (! shouldRun && running)
        {
            stopSounding (sink);
            currentStep.store (-1);
        }
        running = shouldRun;
        lastUseHost = useHost;

        clockPpq = useHost ? hostPpq : internalPpq;

        if (printRequest.exchange (false))
            doPrint();
    }

    // Incoming MIDI note (call at its sample position, before that sample's tick()).
    void midiNote (int note, bool isOn, int velocity, NoteSink& sink)
    {
        if (settings.mode == seqOff)
        {
            if (isOn) sink.seqNoteOn (note, velocity); else sink.seqNoteOff (note);
            return;
        }

        if (isOn)
        {
            if (settings.latch && physicalHeld == 0) latched.clear(); // fresh chord replaces the latched one
            addUnique (held, note);
            if (settings.latch) addUnique (latched, note);
            ++physicalHeld;

            // arp, or pattern triggered by a key, starting from silence: start the grid now (step 1)
            if ((settings.mode == seqArp || settings.mode == seqPattern) && ! running)
            {
                running = true;
                if (! useHost) { internalPpq = 0.0; clockPpq = 0.0; }
                needResync = true;
            }
        }
        else
        {
            held.erase (std::remove (held.begin(), held.end(), note), held.end());
            physicalHeld = std::max (0, physicalHeld - 1);

            const bool patternKeyOnly = settings.mode == seqPattern && ! useHost && ! settings.play;
            if (((settings.mode == seqArp && source().empty()) || (patternKeyOnly && held.empty())) && running)
            {
                running = false;
                stopSounding (sink);
                currentStep.store (-1);
            }
        }
    }

    void allNotesOff (NoteSink& sink)
    {
        held.clear(); latched.clear(); physicalHeld = 0;
        stopSounding (sink);
    }

    // Once per sample.
    void tick (NoteSink& sink)
    {
        const double ppq = clockPpq;
        clockPpq += ppqPerSample;
        if (! useHost) internalPpq = clockPpq;

        if (! running || settings.mode == seqOff)
            return;

        // host jumped (loop, scrub) or we just started: line up with the grid again
        if (needResync || ppq < lastPpq - 1.0e-9 || ppq - lastPpq > stepPpq * 2.0)
        {
            resync (ppq, sink);
            needResync = false;
        }
        lastPpq = ppq;

        constexpr double eps = 1.0e-9; // absorbs floating-point rounding so events land on the exact sample
        if (offPending && ppq >= offPpq - eps)
        {
            sink.seqNoteOff (sounding);
            sounding = -1;
            offPending = false;
        }

        int guard = 0;
        while (ppq >= nextStartPpq - eps && guard++ < 4)
        {
            fireStep (nextIndex, nextStartPpq, sink);
            ++nextIndex;
            nextStartPpq = stepStart (nextIndex);
        }
    }

    // ---------------------------------------------------------- arp maths (also used by print)
    struct StepOut { bool on = false; int note = 0; bool accent = false; bool slide = false; };

    StepOut arpStep (int i)
    {
        StepOut out;
        const auto& src = source();
        if (src.empty()) return out;

        // 1. the notes to cycle through
        work.clear();
        if (settings.chord)
        {
            const int root = *std::min_element (src.begin(), src.end());
            buildChord (root, work);
        }
        else
        {
            work.assign (src.begin(), src.end());
            if (settings.arpMode != 4) std::sort (work.begin(), work.end());
        }

        // 2. spread over octaves
        const int octs = std::clamp (settings.arpOct, 1, 4);
        work2.clear();
        for (int o = 0; o < octs; ++o)
            for (int n : work)
                work2.push_back (n + 12 * o);

        const int n = (int) work2.size();
        int idx = 0;
        switch (settings.arpMode)
        {
            case 1:  idx = n - 1 - (i % n); break;                           // down
            case 2:  if (n > 1) { const int period = 2 * n - 2; const int p = i % period; idx = p < n ? p : period - p; } break;
            case 3:  idx = (int) (hash ((uint32_t) i * 2654435761u + 17u) % (uint32_t) n); break;
            default: idx = i % n; break;                                     // up, as played
        }

        out.on = true;
        out.note = std::clamp (work2[(size_t) idx], 0, 127);

        static constexpr int every[] = { 0, 2, 3, 4, 5, 7 };
        if (settings.arpAccent >= 1 && settings.arpAccent <= 5) out.accent = (i % every[settings.arpAccent]) == 0;
        else if (settings.arpAccent == 6)
            out.accent = (i % 4 == 0 && hash ((uint32_t) i + 11u) % 100 < 50) || hash ((uint32_t) i * 31u + 3u) % 100 < 22;

        out.slide = (float) (hash ((uint32_t) i * 7u + 5u) % 1000) < settings.arpSlide * 1000.0f;
        return out;
    }

    static double rateToPpq (int rate)
    {
        static constexpr double r[] = { 0.5, 0.25, 0.125, 1.0 / 3.0, 1.0 / 6.0 };
        return r[std::clamp (rate, 0, 4)];
    }

private:
    const std::vector<int>& source() const { return settings.latch ? latched : held; }

    static void addUnique (std::vector<int>& v, int n)
    {
        if (std::find (v.begin(), v.end(), n) == v.end() && (int) v.size() < arpMaxNotes) v.push_back (n);
    }

    static uint32_t hash (uint32_t x)
    {
        x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
        return x;
    }

    // Root, 3rd, 5th, 7th built from the chosen key and scale, starting at the held note
    // (snapped down to the nearest scale note).
    void buildChord (int root, std::vector<int>& out) const
    {
        const auto& sc = scales[std::clamp (settings.scale, 0, numScales - 1)];
        const int rel = ((root - settings.key) % 12 + 12) % 12;
        int degree = 0;
        for (int d = 0; d < sc.size; ++d) if (sc.steps[d] <= rel) degree = d;
        const int degreeBase = root - (rel - sc.steps[degree]); // the snapped root
        for (int j = 0; j < 4; ++j)
        {
            const int d = degree + 2 * j;
            const int semis = sc.steps[d % sc.size] + 12 * (d / sc.size) - sc.steps[degree];
            out.push_back (degreeBase + semis);
        }
    }

    double stepStart (long k) const
    {
        return (double) k * stepPpq + ((k & 1) ? settings.swing * 0.5 * stepPpq : 0.0);
    }

    void resync (double ppq, NoteSink& sink)
    {
        stopSounding (sink);
        nextIndex = (long) std::ceil (ppq / stepPpq - 1.0e-6);
        // if we're only just past a step start, play it now rather than waiting a whole step
        if (nextIndex > 0 && ppq - stepStart (nextIndex - 1) < stepPpq * 0.05) --nextIndex;
        nextStartPpq = stepStart (nextIndex);
    }

    void stopSounding (NoteSink& sink)
    {
        if (sounding >= 0) sink.seqNoteOff (sounding);
        sounding = -1;
        offPending = false;
        prevSlide = false;
    }

    void fireStep (long k, double startPpq, NoteSink& sink)
    {
        const int i = (int) (((k % settings.length) + settings.length) % settings.length);
        currentStep.store (i);

        StepOut s;
        if (settings.mode == seqPattern)
        {
            const Step st = getStep (i);
            s.on = st.on;
            s.note = std::clamp (st.midiNote() + transpose(), 0, 127);
            s.accent = st.accent;
            s.slide = st.slide;
        }
        else
        {
            s = arpStep (i);
        }

        if (! s.on)
        {
            stopSounding (sink);
            return;
        }

        const int vel = s.accent ? 120 : 80;
        if (sounding >= 0 && prevSlide)
        {
            if (sounding != s.note) { sink.seqNoteOn (s.note, vel); sink.seqNoteOff (sounding); }
            // same pitch + slide = tie: keep holding, no retrigger
        }
        else
        {
            if (sounding >= 0) sink.seqNoteOff (sounding);
            sink.seqNoteOn (s.note, vel);
        }

        sounding = s.note;
        prevSlide = s.slide;
        if (s.slide)
            offPending = false;
        else
        {
            offPending = true;
            offPpq = startPpq + std::clamp ((double) settings.gate, 0.05, 0.95) * stepPpq;
        }
    }

    // In PATTERN mode, holding a key transposes the pattern (C4 / note 60 = no change).
    int transpose() const
    {
        if (held.empty()) return 0;
        return std::clamp (held.back() - 60, -24, 24);
    }

    // Copy what the arp is playing into the step pattern (folded into the pattern's 3 octaves).
    void doPrint()
    {
        if (source().empty()) { printFail.fetch_add (1); return; }

        const int len = std::clamp (settings.length, 1, maxSteps);
        StepOut outs[maxSteps];
        int minNote = 127;
        for (int i = 0; i < len; ++i) { outs[i] = arpStep (i); minNote = std::min (minNote, outs[i].note); }

        const int shift = 12 * (int) std::ceil ((seqBaseNote - minNote) / 12.0);
        const int lo = seqBaseNote - 12, hi = seqBaseNote + 23;
        for (int i = 0; i < len; ++i)
        {
            int n = outs[i].note + shift;
            while (n > hi) n -= 12;
            while (n < lo) n += 12;
            const int rel = n - seqBaseNote;
            Step st;
            st.oct = rel < 0 ? -1 : (rel >= 12 ? 1 : 0);
            st.note = rel - 12 * st.oct;
            st.accent = outs[i].accent;
            st.slide = outs[i].slide;
            st.on = true;
            pattern[(size_t) i].store (st.encode());
        }
        patternVersion.fetch_add (1);
        printOk.fetch_add (1);
    }

    SeqSettings settings;
    double sr = 44100.0;
    double stepPpq = 0.25, ppqPerSample = 0.0;
    double clockPpq = 0.0, internalPpq = 0.0, lastPpq = -1.0;
    bool useHost = false, lastUseHost = false, running = false, needResync = false;

    long nextIndex = 0;
    double nextStartPpq = 0.0;
    int sounding = -1;
    bool offPending = false, prevSlide = false;
    double offPpq = 0.0;

    std::vector<int> held, latched, work, work2;
    int physicalHeld = 0;
};

} // namespace squelch
