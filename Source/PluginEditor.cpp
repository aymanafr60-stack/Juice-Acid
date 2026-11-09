#include "PluginEditor.h"
#include "Presets.h"
#include "BinaryData.h"
#include "Generator.h"

using namespace squelch::ui;
using Style = Btn::Style;

namespace layout
{
    // synth page
    const juce::Rectangle<float> oscPanel    { 20.0f, 91.0f, 233.0f, 446.0f };
    const juce::Rectangle<float> filterPanel { 268.0f, 91.0f, 454.0f, 446.0f };
    const juce::Rectangle<float> drivePanel  { 737.0f, 91.0f, 243.0f, 446.0f };
    // seq page
    const juce::Rectangle<float> seqPanel    { 20.0f, 91.0f, 960.0f, 446.0f };
    // fx page
    const juce::Rectangle<float> movePanel   { 20.0f, 91.0f, 333.0f, 446.0f };
    const juce::Rectangle<float> delayPanel  { 368.0f, 91.0f, 284.0f, 446.0f };
    const juce::Rectangle<float> reverbPanel { 667.0f, 91.0f, 313.0f, 446.0f };

    const juce::Point<float> logoCentre { 38.0f, 36.0f };
    constexpr float logoRadius = 17.0f;
}

// ------------------------------------------------------------------ construction
SquelchView::SquelchView (SquelchProcessor& p, Skin& s)
    : proc (p), skin (s),
      tabSynth (s, "SYNTH", Style::segment), tabSeq (s, "SEQ", Style::segment), tabFx (s, "FX", Style::segment),
      tabBox (s, { &tabSynth, &tabSeq, &tabFx }, 3),
      presetBar (s), meter (s, param ("volume")),
      saw (s, "SAW", Style::segment), square (s, "SQUARE", Style::segment),
      pulse (s, "PULSE", Style::segment), super (s, "SUPER", Style::segment),
      waveBox (s, { &saw, &square, &pulse, &super }, 2),
      tune (s, param ("tune"), "TUNE"), pw (s, param ("pw"), "PW"),
      sub (s, param ("sub"), "SUB"), slide (s, param ("slide"), "SLIDE"),
      display (s),
      envMod (s, param ("envmod"), "ENV MOD"), cutoff (s, param ("cutoff"), "CUTOFF", true),
      res (s, param ("res"), "RESONANCE", true), decay (s, param ("decay"), "DECAY"),
      accent (s, param ("accent"), "ACCENT"),
      driveSwitch (s, param ("drive_on")),
      od (s, "OD", Style::segment), dist (s, "DIST", Style::segment), fuzz (s, "FUZZ", Style::segment),
      driveBox (s, { &od, &dist, &fuzz }, 3),
      drive (s, param ("drive"), "DRIVE"), tone (s, param ("tone"), "TONE"),
      level (s, param ("level"), "LEVEL"), mix (s, param ("mix"), "MIX"),
      modeOff (s, "OFF", Style::segment), modePattern (s, "PATTERN", Style::segment), modeArp (s, "ARP", Style::segment),
      seqModeBox (s, { &modeOff, &modePattern, &modeArp }, 3),
      play (s, "PLAY", param ("seq_play"), Style::play),
      clear (s, "CLEAR"), mutate (s, "MUTATE"), generate (s, "GENERATE", Style::primary, Icon::sparkle),
      dragMidi (s), grid (s, p.seq),
      lengthStep (s, param ("seq_len")), rateStep (s, param ("seq_rate")), bpmStep (s, param ("seq_bpm")),
      keyStep (s, param ("key")), scaleStep (s, param ("scale")),
      swing (s, param ("seq_swing"), "SWING"), gate (s, param ("seq_gate"), "GATE"),
      arpUp (s, "UP", Style::segment), arpDown (s, "DOWN", Style::segment), arpUpDown (s, "UP-DOWN", Style::segment),
      arpRandom (s, "RANDOM", Style::segment), arpPlayed (s, "PLAYED", Style::segment),
      arpModeBox (s, { &arpUp, &arpDown, &arpUpDown, &arpRandom, &arpPlayed }, 5),
      latch (s, "LATCH", param ("arp_latch")), chord (s, "CHORD", param ("arp_chord")),
      print (s, "PRINT TO STEPS"),
      arpOctStep (s, param ("arp_oct")), arpAccStep (s, param ("arp_acc")),
      arpSlide (s, param ("arp_slide"), "SLIDE"),
      softAtk (s, param ("soft_atk"), "SOFT ATK"), accDecay (s, param ("acc_decay"), "ACC DECAY"),
      tracking (s, param ("tracking"), "TRACKING"), filterFm (s, param ("filter_fm"), "FILTER FM"),
      sweepSlow (s, "SLOW", Style::segment), sweepNormal (s, "NORM", Style::segment), sweepFast (s, "FAST", Style::segment),
      sweepBox (s, { &sweepSlow, &sweepNormal, &sweepFast }, 3),
      delaySwitch (s, param ("dly_on")), reverbSwitch (s, param ("rev_on")),
      dlyTimeStep (s, param ("dly_time")), dlyMsStep (s, param ("dly_ms")),
      pingPong (s, "PING-\nPONG", param ("dly_pingpong")), sync (s, "SYNC", param ("dly_sync")),
      dlyFb (s, param ("dly_fb"), "FEEDBACK"), dlyTone (s, param ("dly_tone"), "TONE"), dlyMix (s, param ("dly_mix"), "MIX"),
      revSize (s, param ("rev_size"), "SIZE"), revDamp (s, param ("rev_damp"), "DAMP"), revMix (s, param ("rev_mix"), "MIX"),
      revPre (s, param ("rev_predelay"), "PRE-DELAY"), revLowCut (s, param ("rev_lowcut"), "LOW CUT"),
      revDuck (s, param ("rev_duck"), "DUCKING"),
      waveGroup (param ("wave"), { &saw, &square, &pulse, &super }),
      driveModeGroup (param ("drive_mode"), { &od, &dist, &fuzz }),
      seqModeGroup (param ("seq_mode"), { &modeOff, &modePattern, &modeArp }),
      arpModeGroup (param ("arp_mode"), { &arpUp, &arpDown, &arpUpDown, &arpRandom, &arpPlayed }),
      sweepGroup (param ("sweep"), { &sweepSlow, &sweepNormal, &sweepFast })
{
    synthPage = { &waveBox, &tune, &pw, &sub, &slide, &display, &envMod, &cutoff, &res, &decay, &accent,
                  &driveSwitch, &driveBox, &drive, &tone, &level, &mix };
    seqPage   = { &seqModeBox, &play, &clear, &mutate, &generate, &dragMidi, &grid,
                  &lengthStep, &rateStep, &swing, &gate, &bpmStep, &keyStep, &scaleStep,
                  &arpModeBox, &latch, &chord, &print, &arpOctStep, &arpAccStep, &arpSlide };
    fxPage    = { &softAtk, &accDecay, &tracking, &filterFm, &sweepBox,
                  &delaySwitch, &dlyTimeStep, &dlyMsStep, &pingPong, &sync, &dlyFb, &dlyTone, &dlyMix,
                  &reverbSwitch, &revSize, &revDamp, &revMix, &revPre, &revLowCut, &revDuck };
    arpOnly     = { &arpModeBox, &latch, &chord, &print, &arpOctStep, &arpAccStep, &arpSlide };
    patternOnly = { &grid };

    for (auto* c : synthPage) addChildComponent (c);
    for (auto* c : seqPage)   addChildComponent (c);
    for (auto* c : fxPage)    addChildComponent (c);
    for (auto* c : std::initializer_list<juce::Component*> { &tabBox, &presetBar, &meter }) addAndMakeVisible (c);

    for (auto* k : { &cutoff, &res }) k->knobSize = 84;
    for (auto* k : { &dlyFb, &dlyTone, &dlyMix }) k->knobSize = 68;
    for (auto* k : { &revSize, &revDamp, &revMix, &revPre, &revLowCut, &revDuck }) k->knobSize = 54;

    tabSynth.onClick = [this] { setPage (0); };
    tabSeq.onClick   = [this] { setPage (1); };
    tabFx.onClick    = [this] { setPage (2); };

    // PW knob means pulse width (PULSE) or detune (SUPER)
    waveGroup.onChange = [this] (int w) { updateShapeKnob (w); };
    updateShapeKnob ((int) std::round (value ("wave")));

    display.getCutoffHz  = [this] { return proc.displayCutoffHz.load(); };
    display.getResonance = [this] { return value ("res"); };

    presetBar.getIndex = [this] { return proc.getPresetIndex(); };
    presetBar.getCount = [this] { return proc.getNumPresetsTotal(); };
    presetBar.getFactoryCount = [] { return squelch::numFactoryPresets; };
    presetBar.getName  = [this] (int i) { return proc.getPresetDisplayName (i); };
    presetBar.load     = [this] (int i) { proc.loadPreset (i); presetBar.repaint(); };
    presetBar.save     = [this] (const juce::String& name) { const bool ok = proc.saveUserPreset (name); presetBar.repaint(); return ok; };
    presetBar.openFolder = [this] { auto f = proc.getUserPresetFolder(); f.createDirectory(); f.startAsProcess(); };

    play.onToggled = [this]
    {
        const bool on = play.getToggleState();
        play.setButtonText (on ? "STOP" : "PLAY");
        play.setIcon (on ? Icon::stop : Icon::play);
    };
    play.init();
    grid.getLength = [this] { return (int) std::round (value ("seq_len")); };
    dragMidi.makeFile = [this] { return proc.writePatternMidi(); };
    clear.onClick    = [this] { clearPattern(); };
    generate.onClick = [this] { generateLine (false); };
    mutate.onClick   = [this] { generateLine (true); };
    print.onClick    = [this] { proc.seq.printRequest.store (true); };
    lastPrintOk = proc.seq.printOk.load();
    lastPrintFail = proc.seq.printFail.load();
    latch.init(); chord.init(); pingPong.init(); sync.init();

    play.setTooltip ("Run the pattern on its own clock (BPM) when FL isn't playing");
    clear.setTooltip ("Reset all 16 steps to plain C notes");
    generate.setTooltip ("Write a new acid line in the chosen KEY and SCALE");
    mutate.setTooltip ("Change 2-3 steps of the current line, keep the rest");
    modePattern.setTooltip ("Play the 16-step pattern: hold a key (middle C = as written, other keys transpose), press PLAY, or start FL");
    modeArp.setTooltip ("Arpeggiator: hold notes on your keyboard");
    latch.setTooltip ("Keep the arp going after you let go of the keys");
    chord.setTooltip ("Hold one key: the arp plays a chord built from KEY and SCALE");
    print.setTooltip ("Copy what the arp is playing into the step pattern");
    softAtk.setTooltip ("Attack of normal (non-accented) notes");
    accDecay.setTooltip ("How long the filter stays open on accented notes");
    tracking.setTooltip ("Cutoff follows the note: higher notes get brighter");
    filterFm.setTooltip ("The oscillator wobbles the filter: growl and grit");
    pingPong.setTooltip ("Echoes bounce left/right. Off: centred echoes");
    sync.setTooltip ("Delay time follows the tempo. Off: free time in milliseconds");
    revPre.setTooltip ("Gap before the reverb starts, keeps notes clear");
    revLowCut.setTooltip ("Removes bass from the reverb, so it doesn't get muddy");
    revDuck.setTooltip ("Turns the reverb down while notes play, lets it bloom in the gaps");
    driveSwitch.setTooltip ("Drive on/off");
    delaySwitch.setTooltip ("Delay on/off");
    reverbSwitch.setTooltip ("Reverb on/off");

    setSize (canvasW, canvasH);
    setPage (proc.getPage());
}

juce::RangedAudioParameter& SquelchView::param (const char* id)
{
    auto* p = proc.apvts.getParameter (id);
    jassert (p != nullptr);
    return *p;
}

float SquelchView::value (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }

// ------------------------------------------------------------------ behaviour
void SquelchView::setPage (int newPage)
{
    page = juce::jlimit (0, 2, newPage);
    proc.setPage (page);
    tabSynth.setToggleState (page == 0, juce::dontSendNotification);
    tabSeq.setToggleState (page == 1, juce::dontSendNotification);
    tabFx.setToggleState (page == 2, juce::dontSendNotification);
    for (auto* c : synthPage) c->setVisible (page == 0);
    for (auto* c : seqPage)   c->setVisible (page == 1);
    for (auto* c : fxPage)    c->setVisible (page == 2);
    shownMode = shownSync = -1;
    refreshVisibility();
    repaint();
}

void SquelchView::refreshVisibility()
{
    const int mode = (int) value ("seq_mode");
    const int syncOn = value ("dly_sync") > 0.5f ? 1 : 0;
    if (mode == shownMode && syncOn == shownSync) return;
    shownMode = mode; shownSync = syncOn;

    if (page == 1)
    {
        for (auto* c : arpOnly)     c->setVisible (mode == squelch::seqArp);
        for (auto* c : patternOnly) c->setVisible (mode != squelch::seqArp);
        play.setAlpha (mode == squelch::seqPattern ? 1.0f : 0.45f);
    }
    if (page == 2)
    {
        dlyTimeStep.setVisible (syncOn == 1);
        dlyMsStep.setVisible (syncOn == 0);
    }
    repaint();
}

void SquelchView::updateShapeKnob (int wave)
{
    currentWave = wave;
    pw.setLabel (wave == 3 ? "DETUNE" : "PW");
    pw.setAlpha (wave >= 2 ? 1.0f : 0.4f);
    pw.valueText = [this]
    {
        const float v = value ("pw");
        if (currentWave == 3) return juce::String (juce::roundToInt (v * 35.0f)) + " ct";
        if (currentWave == 2) return juce::String (juce::roundToInt ((0.05f + 0.9f * v) * 100.0f)) + "%";
        return juce::String ("n/a");
    };
    pw.repaint();
}

void SquelchView::clearPattern()
{
    for (int i = 0; i < squelch::maxSteps; ++i) proc.seq.setStep (i, squelch::Step {});
    grid.repaint();
}

void SquelchView::generateLine (bool small)
{
    const int keyIndex = (int) value ("key"), scaleIndex = (int) value ("scale");
    const auto seed = (uint32_t) juce::Random::getSystemRandom().nextInt();
    if (small) squelch::AcidGenerator::mutate (proc.seq, keyIndex, scaleIndex, (int) std::round (value ("seq_len")), seed);
    else       squelch::AcidGenerator::generate (proc.seq, keyIndex, scaleIndex, seed);

    if (auto* m = proc.apvts.getParameter ("seq_mode"))
        if ((int) std::round (m->convertFrom0to1 (m->getValue())) != squelch::seqPattern)
            m->setValueNotifyingHost (m->convertTo0to1 ((float) squelch::seqPattern));
    grid.repaint();
}

void SquelchView::tick()
{
    meterLevel = std::max (proc.outputPeak.exchange (0.0f), meterLevel * 0.86f);
    meter.setLevel (meterLevel);
    if (page == 0) display.repaint();

    const int idx = proc.getPresetIndex();
    if (idx != lastPresetShown) { lastPresetShown = idx; presetBar.repaint(); }

    refreshVisibility();
    if (page == 1) grid.repaint();

    const int ok = proc.seq.printOk.load(), fail = proc.seq.printFail.load();
    const auto now = juce::Time::getMillisecondCounter();
    if (ok != lastPrintOk)     { lastPrintOk = ok;     toast = "printed to steps";              toastUntil = now + 2500; }
    if (fail != lastPrintFail) { lastPrintFail = fail; toast = "hold notes first, then print"; toastUntil = now + 2500; }
    if (toast.isNotEmpty() && now > toastUntil) toast = {};

    repaint (0, 556, 700, 44); // footer status
}

void SquelchView::mouseUp (const juce::MouseEvent& e)
{
    if (e.position.getDistanceFrom (layout::logoCentre) <= layout::logoRadius + 4.0f && onThemeToggle)
        onThemeToggle();
}

// ------------------------------------------------------------------ layout
static juce::Rectangle<int> knobAt (int cx, int cy, int size, int width = 104)
{
    return { cx - width / 2, cy - size / 2, width, size + 44 };
}

void SquelchView::resized()
{
    tabBox.setBounds (263, 14, 219, 44);
    presetBar.setBounds (638, 16, 342, 40);
    meter.setBounds (711, 568, 269, 20);

    // synth
    waveBox.setBounds (37, 138, 200, 86);
    tune.setBounds (knobAt (87, 265, 50));
    pw.setBounds (knobAt (187, 265, 50));
    sub.setBounds (knobAt (87, 373, 50));
    slide.setBounds (knobAt (187, 373, 50));
    display.setBounds (284, 138, 421, 131);
    envMod.setBounds (knobAt (318, 343, 50, 90));
    cutoff.setBounds (knobAt (410, 328, 84, 112));
    res.setBounds (knobAt (515, 328, 84, 112));
    decay.setBounds (knobAt (603, 343, 50, 90));
    accent.setBounds (knobAt (674, 343, 50, 90));
    driveSwitch.setBounds (923, 107, 42, 24);
    driveBox.setBounds (753, 147, 210, 42);
    drive.setBounds (knobAt (806, 232, 50));
    tone.setBounds (knobAt (911, 232, 50));
    level.setBounds (knobAt (806, 339, 50));
    mix.setBounds (knobAt (911, 339, 50));

    // seq
    seqModeBox.setBounds (140, 107, 211, 40);
    play.setBounds (362, 108, 85, 39);
    clear.setBounds (458, 108, 72, 39);
    mutate.setBounds (607, 108, 84, 39);
    generate.setBounds (702, 108, 127, 39);
    dragMidi.setBounds (840, 108, 119, 39);
    grid.setBounds (40, 163, 898, 190);
    lengthStep.setBounds (45, 432, 103, 38);
    rateStep.setBounds (198, 432, 103, 38);
    swing.setBounds (knobAt (374, 410, 50));
    gate.setBounds (knobAt (473, 410, 50));
    bpmStep.setBounds (547, 432, 103, 38);
    keyStep.setBounds (699, 432, 103, 38);
    scaleStep.setBounds (852, 432, 103, 38);

    arpModeBox.setBounds (107, 186, 440, 42);
    latch.setBounds (565, 186, 88, 42);
    chord.setBounds (663, 186, 88, 42);
    print.setBounds (771, 186, 166, 42);
    arpOctStep.setBounds (107, 268, 110, 38);
    arpAccStep.setBounds (237, 268, 130, 38);
    arpSlide.setBounds (knobAt (420, 283, 50));

    // fx
    softAtk.setBounds (knobAt (86, 163, 50));
    accDecay.setBounds (knobAt (186, 163, 50));
    tracking.setBounds (knobAt (286, 163, 50));
    filterFm.setBounds (knobAt (86, 270, 50));
    sweepBox.setBounds (150, 251, 160, 38);
    delaySwitch.setBounds (596, 107, 42, 24);
    dlyTimeStep.setBounds (384, 166, 95, 39);
    dlyMsStep.setBounds (384, 166, 95, 39);
    pingPong.setBounds (490, 166, 75, 39);
    sync.setBounds (576, 166, 60, 39);
    dlyFb.setBounds (knobAt (426, 441, 68, 92));
    dlyTone.setBounds (knobAt (510, 441, 68, 92));
    dlyMix.setBounds (knobAt (594, 441, 68, 92));
    reverbSwitch.setBounds (923, 107, 42, 24);
    revSize.setBounds (knobAt (730, 334, 54, 96));
    revDamp.setBounds (knobAt (823, 334, 54, 96));
    revMix.setBounds (knobAt (916, 334, 54, 96));
    revPre.setBounds (knobAt (730, 447, 54, 96));
    revLowCut.setBounds (knobAt (823, 447, 54, 96));
    revDuck.setBounds (knobAt (916, 447, 54, 96));
}

// ------------------------------------------------------------------ painting
void SquelchView::paint (juce::Graphics& g)
{
    const auto& t = skin.theme;
    g.fillAll (t.background);

    // header
    drawSmiley (g, layout::logoCentre, layout::logoRadius, t.logoFace, t.logoFeature);
    g.setColour (t.text);
    g.setFont (skin.font (29.0f, Weight::extraBold, 0.01f));
    g.drawText ("SQUELCH", juce::Rectangle<float> (66.0f, 18.0f, 150.0f, 38.0f), juce::Justification::centredLeft, false);
    g.setColour (t.label);
    g.setFont (skin.font (12.0f, Weight::bold));
    g.drawText ("v0.7", juce::Rectangle<float> (208.0f, 30.0f, 45.0f, 18.0f), juce::Justification::centredLeft, false);
    g.setColour (t.divider);
    g.fillRect (0.0f, 72.0f, (float) canvasW, 1.0f);

    auto panel = [&] (juce::Rectangle<float> r, const juce::String& title)
    {
        fillBox (g, r, 16.0f, t.panel, t.panelBorder, 1.0f);
        g.setColour (t.label);
        g.setFont (skin.caption (12.0f));
        g.drawText (title, juce::Rectangle<float> (r.getX() + 17.0f, r.getY() + 14.0f, 250.0f, 18.0f),
                    juce::Justification::centredLeft, false);
    };
    auto caption = [&] (const juce::String& text, float x, float y, float w = 120.0f)
    {
        g.setColour (t.label);
        g.setFont (skin.caption());
        g.drawText (text, juce::Rectangle<float> (x, y, w, 16.0f), juce::Justification::centredLeft, false);
    };

    if (page == 0)
    {
        panel (layout::oscPanel, "OSCILLATOR");
        panel (layout::filterPanel, "FILTER + ENVELOPE");
        panel (layout::drivePanel, "DRIVE");
        g.setColour (t.label);
        g.setFont (skin.font (12.5f, Weight::bold));
        g.drawText ("24 dB low-pass", juce::Rectangle<float> (540.0f, 105.0f, 166.0f, 18.0f), juce::Justification::centredRight, false);
    }
    else if (page == 1)
    {
        fillBox (g, layout::seqPanel, 16.0f, t.panel, t.panelBorder, 1.0f);
        caption ("SEQUENCER", 40.0f, 119.0f, 100.0f);
        g.setColour (t.divider);
        g.fillRect (40.0f, 367.0f, 898.0f, 1.0f);
        caption ("LENGTH", 45.0f, 410.0f);
        caption ("RATE", 198.0f, 410.0f);
        caption ("BPM", 547.0f, 410.0f);
        caption ("KEY", 699.0f, 410.0f);
        caption ("SCALE", 852.0f, 410.0f);

        if ((int) value ("seq_mode") == squelch::seqArp)
        {
            caption ("MODE", 107.0f, 166.0f);
            caption ("OCTAVES", 107.0f, 248.0f);
            caption ("ACCENT", 237.0f, 248.0f);
            g.setColour (t.label);
            g.setFont (skin.font (13.0f, Weight::medium));
            g.drawFittedText ("Hold notes on your keyboard, or LATCH them. CHORD builds a chord from one key using "
                              "KEY + SCALE. PRINT TO STEPS copies what the arp plays into the pattern so you can edit it.",
                              juce::Rectangle<int> (490, 252, 447, 72), juce::Justification::centredLeft, 4);
        }
    }
    else
    {
        panel (layout::movePanel, "MOVEMENT");
        panel (layout::delayPanel, "DELAY");
        panel (layout::reverbPanel, "REVERB");
        g.setColour (t.label);
        g.setFont (skin.caption());
        g.drawText ("SWEEP", juce::Rectangle<float> (150.0f, 302.0f, 160.0f, 16.0f), juce::Justification::centred, false);
        caption ("TIME", 384.0f, 145.0f);
        g.setColour (t.label);
        g.setFont (skin.font (13.0f, Weight::medium));
        g.drawFittedText ("TD-3-MO style extras: how notes start, how long accents stay open, and how the filter follows the notes.",
                          juce::Rectangle<int> (37, 452, 300, 66), juce::Justification::topLeft, 3);
    }

    paintFooter (g);
}

void SquelchView::paintFooter (juce::Graphics& g)
{
    const auto& t = skin.theme;
    g.setColour (t.divider);
    g.fillRect (0.0f, 555.0f, (float) canvasW, 1.0f);

    const float cy = 578.0f;
    auto dot = [&] (float x, bool lit)
    {
        g.setColour (lit ? t.active : t.track);
        g.fillEllipse (x - 3.5f, cy - 3.5f, 7.0f, 7.0f);
    };
    auto text = [&] (const juce::String& s, float x, float w, bool strong)
    {
        g.setColour (t.text.withAlpha (strong ? 1.0f : 0.85f));
        g.setFont (strong ? skin.font (13.5f, Weight::extraBold) : skin.font (13.5f, Weight::medium));
        g.drawText (s, juce::Rectangle<float> (x, cy - 10.0f, w, 20.0f), juce::Justification::centredLeft, false);
    };
    const juce::String sep = juce::String::fromUTF8 (" \xc2\xb7 ");

    dot (23.0f, true);
    text ("MIDI", 35.0f, 40.0f, true);
    text ("mono" + sep + "legato slide", 69.0f, 130.0f, false);

    g.setColour (t.divider);
    g.fillRect (196.0f, cy - 10.0f, 1.0f, 20.0f);

    const int mode = (int) value ("seq_mode");
    const int step = proc.seq.currentStep.load();
    const bool running = step >= 0 && mode != squelch::seqOff;
    const juce::String clock = proc.seq.hostClock.load() ? juce::String ("FL tempo")
                                                         : juce::String ((int) value ("seq_bpm")) + " bpm";
    juce::String status;
    if (toast.isNotEmpty())               status = toast;
    else if (mode == squelch::seqOff)     status = "off";
    else if (mode == squelch::seqPattern) status = running ? "playing" + sep + "step " + juce::String (step + 1) + sep + clock
                                                           : "hold a key or press PLAY" + sep + clock;
    else                                  status = running ? "arp" + sep + "step " + juce::String (step + 1) + sep + clock
                                                           : "arp" + sep + "hold notes" + sep + clock;
    dot (218.0f, running);
    text ("SEQ", 230.0f, 40.0f, true);
    text (status, 262.0f, 420.0f, false);
}

// ------------------------------------------------------------------ editor
SquelchEditor::SquelchEditor (SquelchProcessor& p)
    : AudioProcessorEditor (p), proc (p), view (p, skin)
{
    skin.medium    = juce::Typeface::createSystemTypefaceFor (BinaryData::Manrope500_ttf, BinaryData::Manrope500_ttfSize);
    skin.bold      = juce::Typeface::createSystemTypefaceFor (BinaryData::Manrope700_ttf, BinaryData::Manrope700_ttfSize);
    skin.extraBold = juce::Typeface::createSystemTypefaceFor (BinaryData::Manrope800_ttf, BinaryData::Manrope800_ttfSize);

    view.onThemeToggle = [this]
    {
        proc.setTheme ((proc.getTheme() + 1) % 3);
        applyTheme();
    };
    applyTheme();
    addAndMakeVisible (view);

    setResizable (true, true);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) SquelchView::canvasW / (double) SquelchView::canvasH);
    setResizeLimits (700, 420, 1600, 960);
    setSize (SquelchView::canvasW, SquelchView::canvasH);

    startTimerHz (30);
}

SquelchEditor::~SquelchEditor() { stopTimer(); }

void SquelchEditor::applyTheme()
{
    const int th = proc.getTheme();
    skin.theme = th == 1 ? Theme::green() : (th == 2 ? Theme::y2k() : Theme::red());
    view.repaint();
}

void SquelchEditor::resized()
{
    const float scale = (float) getWidth() / (float) SquelchView::canvasW;
    view.setBounds (0, 0, SquelchView::canvasW, SquelchView::canvasH);
    view.setTransform (juce::AffineTransform::scale (scale));
}
