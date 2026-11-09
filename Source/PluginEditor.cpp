#include "PluginEditor.h"
#include "Presets.h"
#include "BinaryData.h"
#include "Generator.h"

using namespace squelch::ui;

// ------------------------------------------------------------------ layout constants
namespace layout
{
    const juce::Rectangle<float> oscPanel    { 30.0f, 103.0f, 202.0f, 368.0f };
    const juce::Rectangle<float> filterPanel { 251.0f, 103.0f, 480.0f, 368.0f };
    const juce::Rectangle<float> drivePanel  { 750.0f, 103.0f, 222.0f, 368.0f };
    const juce::Rectangle<float> stepsPanel  { 30.0f, 103.0f, 701.0f, 368.0f };
    const juce::Rectangle<float> arpPanel    { 750.0f, 103.0f, 222.0f, 368.0f };
    const juce::Rectangle<float> movePanel   { 30.0f, 103.0f, 342.0f, 368.0f };
    const juce::Rectangle<float> delayPanel  { 391.0f, 103.0f, 340.0f, 368.0f };
    const juce::Rectangle<float> reverbPanel { 750.0f, 103.0f, 222.0f, 368.0f };
    const juce::Point<float> logoCentre      { 52.0f, 40.0f };
    constexpr float logoRadius = 23.0f;
}

// ------------------------------------------------------------------ view
SquelchView::SquelchView (SquelchProcessor& p, Skin& s)
    : proc (p), skin (s),
      saw (s, "SAW"), square (s, "SQUARE"), pulse (s, "PULSE"), super (s, "SUPER"),
      tune (s, param ("tune"), "TUNE"), pw (s, param ("pw"), "PW"),
      sub (s, param ("sub"), "SUB"), slide (s, param ("slide"), "SLIDE"),
      display (s),
      envMod (s, param ("envmod"), "ENV MOD"), cutoff (s, param ("cutoff"), "CUTOFF", true),
      res (s, param ("res"), "RESONANCE", true), decay (s, param ("decay"), "DECAY"),
      accent (s, param ("accent"), "ACCENT"),
      od (s, "OD"), dist (s, "DIST"), fuzz (s, "FUZZ"),
      driveLed (s, param ("drive_on")),
      drive (s, param ("drive"), "DRIVE"), tone (s, param ("tone"), "TONE"),
      level (s, param ("level"), "LEVEL"), mix (s, param ("mix"), "MIX"),
      presetBar (s),
      meter (s, param ("volume")),
      synthTab (s, "SYNTH"), seqTab (s, "SEQ"), fxTab (s, "FX"),
      seqOff (s, "OFF"), seqPattern (s, "PATTERN"), seqArp (s, "ARP"),
      play (s, "PLAY", param ("seq_play")), clear (s, "CLEAR"),
      generate (s, "GENERATE"), mutate (s, "MUTATE"),
      dragMidi (s), grid (s, p.seq),
      length (s, param ("seq_len"), "LENGTH"), rate (s, param ("seq_rate"), "RATE"),
      swing (s, param ("seq_swing"), "SWING"), gate (s, param ("seq_gate"), "GATE"),
      bpm (s, param ("seq_bpm"), "BPM"), key (s, param ("key"), "KEY"), scale (s, param ("scale"), "SCALE"),
      latch (s, "LATCH", param ("arp_latch")), chord (s, "CHORD", param ("arp_chord")),
      arpMode (s, param ("arp_mode"), "MODE"), arpOct (s, param ("arp_oct"), "OCTAVE"),
      arpAcc (s, param ("arp_acc"), "ACCENT"), arpSlide (s, param ("arp_slide"), "SLIDE"),
      print (s, "PRINT TO STEPS"),
      softAtk (s, param ("soft_atk"), "SOFT ATK"), accDecay (s, param ("acc_decay"), "ACC DECAY"),
      tracking (s, param ("tracking"), "TRACKING"), filterFm (s, param ("filter_fm"), "FILTER FM"),
      sweep (s, param ("sweep"), "SWEEP"),
      delayLed (s, param ("dly_on")), reverbLed (s, param ("rev_on")),
      dlyTime (s, param ("dly_time"), "TIME"), dlyFb (s, param ("dly_fb"), "FEEDBACK"),
      dlyTone (s, param ("dly_tone"), "TONE"), dlyMix (s, param ("dly_mix"), "MIX"),
      revSize (s, param ("rev_size"), "SIZE"), revDamp (s, param ("rev_damp"), "DAMP"),
      revMix (s, param ("rev_mix"), "MIX"),
      waveGroup (param ("wave"), { &saw, &square, &pulse, &super }),
      modeGroup (param ("drive_mode"), { &od, &dist, &fuzz }),
      seqModeGroup (param ("seq_mode"), { &seqOff, &seqPattern, &seqArp })
{
    synthPage = { &saw, &square, &pulse, &super, &tune, &pw, &sub, &slide,
                  &display, &envMod, &cutoff, &res, &decay, &accent,
                  &od, &dist, &fuzz, &driveLed, &drive, &tone, &level, &mix };
    seqPage   = { &seqOff, &seqPattern, &seqArp, &play, &clear, &generate, &mutate, &dragMidi, &grid,
                  &length, &rate, &swing, &gate, &bpm, &key, &scale,
                  &latch, &chord, &arpMode, &arpOct, &arpAcc, &arpSlide, &print };
    fxPage    = { &softAtk, &accDecay, &tracking, &filterFm, &sweep,
                  &delayLed, &dlyTime, &dlyFb, &dlyTone, &dlyMix,
                  &reverbLed, &revSize, &revDamp, &revMix };
    for (auto* c : synthPage) addChildComponent (c);
    for (auto* c : seqPage)   addChildComponent (c);
    for (auto* c : fxPage)    addChildComponent (c);
    for (auto* c : std::initializer_list<juce::Component*> { &presetBar, &meter, &synthTab, &seqTab, &fxTab })
        addAndMakeVisible (c);

    for (auto* k : { &length, &rate, &swing, &gate, &bpm, &key, &scale }) k->knobSize = 40;
    for (auto* k : { &arpMode, &arpOct, &arpAcc, &arpSlide }) k->knobSize = 46;

    synthTab.onClick = [this] { setPage (0); };
    seqTab.onClick   = [this] { setPage (1); };
    fxTab.onClick    = [this] { setPage (2); };
    fxTab.setTooltip ("TD-3-MO style extras, delay and reverb");

    for (auto* k : { &softAtk, &accDecay, &tracking, &filterFm, &sweep }) k->knobSize = 56;
    for (auto* k : { &dlyTime, &dlyFb, &dlyTone, &dlyMix }) k->knobSize = 62;
    for (auto* k : { &revSize, &revDamp, &revMix }) k->knobSize = 46;
    softAtk.setTooltip ("Attack of normal (non-accented) notes: short = clicky, long = soft swell");
    accDecay.setTooltip ("How long the filter stays open on ACCENTED notes");
    tracking.setTooltip ("Cutoff follows the note: higher notes get brighter");
    filterFm.setTooltip ("The oscillator wobbles the filter: adds growl and grit");
    sweep.setTooltip ("How fast repeated accents wind the filter open");
    dlyTime.setTooltip ("Delay time, synced to tempo. 3/16 is the classic acid echo");
    dlyFb.setTooltip ("How many echoes");
    dlyTone.setTooltip ("Dark to bright echoes");
    dlyMix.setTooltip ("Echo level (the dry sound stays at full volume)");
    delayLed.setHelpText ("Delay on/off");
    reverbLed.setHelpText ("Reverb on/off");
    synthTab.setTooltip ("Synth controls");
    seqTab.setTooltip ("Step sequencer and arpeggiator");

    grid.getLength = [this] { return (int) std::round (proc.apvts.getRawParameterValue ("seq_len")->load()); };
    dragMidi.makeFile = [this] { return proc.writePatternMidi(); };
    clear.onClick = [this] { clearPattern(); };
    generate.onClick = [this] { generateLine (false); };
    mutate.onClick   = [this] { generateLine (true); };
    generate.setToggleState (true, juce::dontSendNotification); // always highlighted
    generate.setTooltip ("Write a new acid line in the chosen KEY and SCALE");
    mutate.setTooltip ("Change 2-3 steps of the current line, keep the rest");
    print.onClick = [this] { proc.seq.printRequest.store (true); };
    lastPrintOk = proc.seq.printOk.load();
    lastPrintFail = proc.seq.printFail.load();

    seqOff.setTooltip ("Sequencer off: your keyboard plays the synth directly");
    seqPattern.setTooltip ("Play the 16-step pattern. Follows FL's transport, or press PLAY. Hold a key to transpose (C4 = original)");
    seqArp.setTooltip ("Arpeggiator: hold notes on your keyboard");
    play.setTooltip ("Run the pattern on its own clock (BPM knob) when FL isn't playing");
    clear.setTooltip ("Reset all 16 steps to plain C notes");
    latch.setTooltip ("Keep the arp going after you let go of the keys");
    chord.setTooltip ("Hold one key: the arp plays a chord built from KEY and SCALE");
    print.setTooltip ("Copy what the arp is playing into the step pattern, so you can edit it or drag it into FL");
    key.setTooltip ("Key for CHORD mode (and the melody generator, coming later)");
    scale.setTooltip ("Scale for CHORD mode (and the melody generator, coming later)");

    // knob sizes
    for (auto* k : { &tune, &pw, &sub, &slide }) k->knobSize = 48;
    for (auto* k : { &envMod, &decay, &accent }) k->knobSize = 58;
    cutoff.knobSize = res.knobSize = 92;
    for (auto* k : { &drive, &tone, &level, &mix }) k->knobSize = 46;

    // the PW knob is "PW" for pulse, "DETUNE" for super, and unused for saw/square
    waveGroup.onChange = [this] (int w) { updateShapeKnob (w); };
    updateShapeKnob ((int) std::round (proc.apvts.getRawParameterValue ("wave")->load()));

    display.getCutoffHz  = [this] { return proc.displayCutoffHz.load(); };
    display.getResonance = [this] { return proc.apvts.getRawParameterValue ("res")->load(); };

    presetBar.getIndex = [this] { return proc.getPresetIndex(); };
    presetBar.getCount = [this] { return proc.getNumPresetsTotal(); };
    presetBar.getFactoryCount = [] { return squelch::numFactoryPresets; };
    presetBar.getName  = [this] (int i) { return proc.getPresetDisplayName (i); };
    presetBar.save     = [this] (const juce::String& name) { const bool ok = proc.saveUserPreset (name); presetBar.repaint(); return ok; };
    presetBar.openFolder = [this]
    {
        auto folder = proc.getUserPresetFolder();
        folder.createDirectory();
        folder.startAsProcess();
    };
    presetBar.load     = [this] (int i) { proc.loadPreset (i); presetBar.repaint(); };

    saw.setTooltip ("Saw: the classic acid buzz");
    square.setTooltip ("Square: hollow, rubbery");
    pulse.setTooltip ("Pulse: square with adjustable width (PW knob)");
    super.setTooltip ("Super: five detuned saws (DETUNE knob)");
    od.setTooltip ("Overdrive: warm, gentle");
    dist.setTooltip ("Distortion: dense, compressed");
    fuzz.setTooltip ("Fuzz: harsh, broken, octave-up grit");
    driveLed.setHelpText ("Drive on/off");

    setSize (canvasW, canvasH);
    setPage (proc.getPage());
}

void SquelchView::setPage (int newPage)
{
    page = juce::jlimit (0, 2, newPage);
    proc.setPage (page);
    for (auto* c : synthPage) c->setVisible (page == 0);
    for (auto* c : seqPage)   c->setVisible (page == 1);
    for (auto* c : fxPage)    c->setVisible (page == 2);
    synthTab.setToggleState (page == 0, juce::dontSendNotification);
    seqTab.setToggleState (page == 1, juce::dontSendNotification);
    fxTab.setToggleState (page == 2, juce::dontSendNotification);
    repaint();
}

void SquelchView::clearPattern()
{
    for (int i = 0; i < squelch::maxSteps; ++i)
        proc.seq.setStep (i, squelch::Step {});
    grid.repaint();
}

void SquelchView::generateLine (bool small)
{
    const int keyIndex   = (int) proc.apvts.getRawParameterValue ("key")->load();
    const int scaleIndex = (int) proc.apvts.getRawParameterValue ("scale")->load();
    const auto seed = (uint32_t) juce::Random::getSystemRandom().nextInt();

    if (small)
        squelch::AcidGenerator::mutate (proc.seq, keyIndex, scaleIndex,
                                        (int) std::round (proc.apvts.getRawParameterValue ("seq_len")->load()), seed);
    else
        squelch::AcidGenerator::generate (proc.seq, keyIndex, scaleIndex, seed);

    // a new line is only useful if the pattern plays it
    if (auto* m = proc.apvts.getParameter ("seq_mode"))
        if ((int) std::round (m->convertFrom0to1 (m->getValue())) != squelch::seqPattern)
            m->setValueNotifyingHost (m->convertTo0to1 ((float) squelch::seqPattern));

    grid.repaint();
}

juce::RangedAudioParameter& SquelchView::param (const char* id)
{
    auto* p = proc.apvts.getParameter (id);
    jassert (p != nullptr);
    return *p;
}

void SquelchView::updateShapeKnob (int wave)
{
    currentWave = wave;
    pw.setLabel (wave == 3 ? "DETUNE" : "PW");
    pw.setAlpha (wave >= 2 ? 1.0f : 0.35f);
    pw.valueText = [this]
    {
        const float v = proc.apvts.getRawParameterValue ("pw")->load();
        if (currentWave == 3) return juce::String (juce::roundToInt (v * 35.0f)) + " ct";
        if (currentWave == 2) return juce::String (juce::roundToInt ((0.05f + 0.9f * v) * 100.0f)) + "%";
        return juce::String ("n/a");
    };
    pw.repaint();
}

void SquelchView::tick()
{
    const float peak = proc.outputPeak.exchange (0.0f);
    meterLevel = std::max (peak, meterLevel * 0.86f);
    meter.setLevel (meterLevel);
    display.repaint();

    const int idx = proc.getPresetIndex();
    if (idx != lastPresetShown) { lastPresetShown = idx; presetBar.repaint(); }

    if (page == 1)
    {
        grid.repaint();

        // arp controls only matter in ARP mode
        const int mode = (int) proc.apvts.getRawParameterValue ("seq_mode")->load();
        if (mode != seqModeShown)
        {
            seqModeShown = mode;
            for (auto* c : std::initializer_list<juce::Component*> { &latch, &chord, &arpMode, &arpOct, &arpAcc, &arpSlide, &print })
                c->setAlpha (mode == squelch::seqArp ? 1.0f : 0.4f);
            play.setAlpha (mode == squelch::seqPattern ? 1.0f : 0.4f);
        }

        const int ok = proc.seq.printOk.load(), fail = proc.seq.printFail.load();
        if (ok != lastPrintOk)     { lastPrintOk = ok;     status = "printed to steps";  statusUntil = juce::Time::getMillisecondCounter() + 2500; }
        if (fail != lastPrintFail) { lastPrintFail = fail; status = "hold notes first";  statusUntil = juce::Time::getMillisecondCounter() + 2500; }
        if (status.isNotEmpty() && juce::Time::getMillisecondCounter() > statusUntil) status = {};
        repaint (750, 110, 222, 40);
        repaint (27, 486, 320, 40);
    }
}

void SquelchView::mouseUp (const juce::MouseEvent& e)
{
    if (e.position.getDistanceFrom (layout::logoCentre) <= layout::logoRadius + 4.0f && onThemeToggle)
        onThemeToggle();
}

void SquelchView::resized()
{
    // oscillator
    saw.setBounds    (48, 156, 84, 42);
    square.setBounds (138, 156, 84, 42);
    pulse.setBounds  (48, 204, 84, 42);
    super.setBounds  (138, 204, 84, 42);
    tune.setBounds   (43, 258, 90, 100);
    pw.setBounds     (131, 258, 90, 100);
    sub.setBounds    (43, 362, 90, 100);
    slide.setBounds  (131, 362, 90, 100);

    // filter + envelope
    display.setBounds (268, 155, 447, 108);
    static constexpr int labelY = 377;                          // all filter labels share one line
    auto place = [] (Knob& k, int cx, int w)
    {
        const int top = labelY - k.knobSize - 10;
        k.setBounds (cx - w / 2, top, w, labelY + 42 - top);
    };
    place (envMod, 303, 86);
    place (cutoff, 398, 112);
    place (res,    510, 112);
    place (decay,  606, 86);
    place (accent, 684, 86);

    // drive fx
    od.setBounds   (766, 156, 60, 42);
    dist.setBounds (832, 156, 64, 42);
    fuzz.setBounds (902, 156, 62, 42);
    driveLed.setBounds (933, 117, 24, 24);
    drive.setBounds (765, 214, 90, 100);
    tone.setBounds  (863, 214, 90, 100);
    level.setBounds (765, 330, 90, 100);
    mix.setBounds   (863, 330, 90, 100);

    // seq page
    seqOff.setBounds     (46, 152, 66, 42);
    seqPattern.setBounds (118, 152, 108, 42);
    seqArp.setBounds     (232, 152, 66, 42);
    play.setBounds       (322, 152, 82, 42);
    clear.setBounds      (410, 152, 86, 42);
    dragMidi.setBounds   (572, 152, 145, 42);
    generate.setBounds   (494, 111, 124, 37);
    mutate.setBounds     (624, 111, 93, 37);
    grid.setBounds       (46, 204, 670, 168);
    {
        int x = 46;
        for (auto* k : { &length, &rate, &swing, &gate, &bpm, &key, &scale })
        {
            k->setBounds (x, 376, 95, 90);
            x += 95;
        }
    }
    latch.setBounds    (766, 152, 94, 42);
    chord.setBounds    (866, 152, 94, 42);
    arpMode.setBounds  (765, 204, 90, 98);
    arpOct.setBounds   (863, 204, 90, 98);
    arpAcc.setBounds   (765, 304, 90, 98);
    arpSlide.setBounds (863, 304, 90, 98);
    print.setBounds    (766, 410, 194, 44);

    // fx page
    softAtk.setBounds  (46, 158, 104, 106);
    accDecay.setBounds (150, 158, 104, 106);
    tracking.setBounds (254, 158, 104, 106);
    filterFm.setBounds (98, 284, 104, 106);
    sweep.setBounds    (202, 284, 104, 106);
    delayLed.setBounds (691, 117, 24, 24);
    dlyTime.setBounds  (420, 160, 130, 118);
    dlyFb.setBounds    (572, 160, 130, 118);
    dlyTone.setBounds  (420, 300, 130, 118);
    dlyMix.setBounds   (572, 300, 130, 118);
    reverbLed.setBounds (933, 117, 24, 24);
    revSize.setBounds  (765, 200, 90, 100);
    revDamp.setBounds  (863, 200, 90, 100);
    revMix.setBounds   (814, 320, 90, 100);

    // header / footer
    synthTab.setBounds (444, 20, 72, 42);
    seqTab.setBounds   (520, 20, 56, 42);
    fxTab.setBounds    (580, 20, 52, 42);
    presetBar.setBounds (640, 18, 334, 48);
    meter.setBounds (688, 486, 286, 38);
}

void SquelchView::paint (juce::Graphics& g)
{
    const auto& t = skin.theme;
    const float w = (float) canvasW;

    // --- background + smiley wallpaper
    g.fillAll (t.background);
    for (int row = 0; row < 8; ++row)
        for (int col = 0; col < 17; ++col)
        {
            const juce::Point<float> c (12.0f + 60.5f * (float) col, 90.0f + 60.0f * (float) row + (row == 7 ? -8.0f : 0.0f));
            drawSmiley (g, c, 16.5f, t.pattern, t.patternFeature, t.patternOutline,
                        t.patternOutline == t.pattern ? 0.0f : 2.0f);
        }

    // --- header
    if (t.chromeHeader)
    {
        // brushed-chrome look: bright top, darker band, light bottom edge, with a faint violet/cyan sheen
        juce::ColourGradient chrome (juce::Colour (0xfff6f8ff), 0.0f, 0.0f, juce::Colour (0xffe4e9fb), 0.0f, 78.0f, false);
        chrome.addColour (0.35, juce::Colour (0xffc3c9e6));
        chrome.addColour (0.55, juce::Colour (0xff8f97c4));
        chrome.addColour (0.78, juce::Colour (0xffcfd5f0));
        g.setGradientFill (chrome);
        g.fillRect (0.0f, 0.0f, w, 78.0f);
        juce::ColourGradient sheen (juce::Colour (0x337a5cff), 0.0f, 0.0f, juce::Colour (0x3300e5ff), w, 0.0f, false);
        sheen.addColour (0.5, juce::Colour (0x00ffffff));
        g.setGradientFill (sheen);
        g.fillRect (0.0f, 0.0f, w, 78.0f);
    }
    else
    {
        g.setColour (t.header);
        g.fillRect (0.0f, 0.0f, w, 78.0f);
    }
    g.setColour (t.shadow);
    g.fillRect (0.0f, 76.0f, w, 3.0f);

    drawSmiley (g, layout::logoCentre, layout::logoRadius, t.logoFace, t.logoFeature, t.shadow, 2.5f);

    {   // title with outline + offset shadow
        juce::GlyphArrangement ga;
        ga.addLineOfText (skin.font (48.0f), "SQUELCH", 88.0f, 56.0f);
        juce::Path title;
        ga.createPath (title);
        g.setColour (t.shadow);
        g.fillPath (title, juce::AffineTransform::translation (3.0f, 3.0f));
        g.strokePath (title, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colours::white);
        g.fillPath (title);
    }

    {   // version sticker, slightly tilted
        juce::Graphics::ScopedSaveState save (g);
        const juce::Rectangle<float> pill (302.0f, 25.0f, 132.0f, 30.0f);
        g.addTransform (juce::AffineTransform::rotation (0.035f, pill.getCentreX(), pill.getCentreY()));
        drawStickerBox (g, pill, 15.0f, t.presetButton, t.shadow, 2.0f, t.shadow, 3.0f);
        g.setColour (t.presetIcon);
        g.setFont (skin.font (13.5f));
        g.drawText (juce::String::fromUTF8 ("acid bass synth \xc2\xb7 v0.6.1"), pill, juce::Justification::centred, false);
    }

    // --- panels
    auto panel = [&] (juce::Rectangle<float> r, const juce::String& title)
    {
        drawStickerBox (g, r, 22.0f, t.panel, t.panelOutline, 3.0f, t.shadow, 6.0f);
        g.setFont (skin.font (15.5f));
        const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), title) + 30.0f;
        const juce::Rectangle<float> pill (r.getX() + 16.0f, r.getY() + 15.0f, tw, 27.0f);
        drawStickerBox (g, pill, 13.5f, t.ring, t.shadow, 2.0f, t.shadow, 2.5f);
        g.setColour (t.onRing);
        g.drawText (title, pill, juce::Justification::centred, false);
    };
    if (page == 2)
    {
        panel (layout::movePanel, "MOVEMENT");
        panel (layout::delayPanel, "DELAY");
        panel (layout::reverbPanel, "REVERB");

        g.setColour (t.valueText);
        g.setFont (skin.font (13.0f, false));
        g.drawFittedText ("TD-3-MO style extras: how notes start, how long accents\nstay open, and how the filter follows the notes.",
                          juce::Rectangle<int> (46, 404, 310, 50), juce::Justification::centred, 2);
        g.setFont (skin.font (13.5f, false));
        g.drawText (juce::String::fromUTF8 ("ping-pong \xc2\xb7 synced"), juce::Rectangle<float> (520.0f, 118.0f, 160.0f, 27.0f),
                    juce::Justification::centredRight, false);
    }
    else if (page == 0)
    {
        panel (layout::oscPanel, "OSCILLATOR");
        panel (layout::filterPanel, "FILTER + ENVELOPE");
        panel (layout::drivePanel, "DRIVE FX");

        g.setColour (t.valueText);
        g.setFont (skin.font (13.5f, false));
        g.drawText ("24 dB low-pass", juce::Rectangle<float> (560.0f, 118.0f, 155.0f, 27.0f),
                    juce::Justification::centredRight, false);
    }
    else
    {
        panel (layout::stepsPanel, "SEQUENCER");
        panel (layout::arpPanel, "ARP");

        if (status.isNotEmpty())
        {
            g.setColour (t.accent);
            g.setFont (skin.font (13.5f));
            g.drawText (status, juce::Rectangle<float> (820.0f, 118.0f, 140.0f, 27.0f), juce::Justification::centredRight, false);
        }
    }

    // --- footer: MIDI pill
    {
        const int mode = (int) proc.apvts.getRawParameterValue ("seq_mode")->load();
        const juce::Rectangle<float> pill (27.0f, 486.0f, mode == 0 ? 180.0f : 300.0f, 34.0f);
        drawStickerBox (g, pill, 17.0f, t.panel, t.panelOutline, 2.5f, t.shadow, 3.5f);
        g.setColour (t.text);
        g.setFont (skin.font (14.0f));
        const bool running = proc.seq.currentStep.load() >= 0;
        g.drawText (mode == 0 ? "MIDI" : "SEQ", pill.withTrimmedLeft (14.0f), juce::Justification::centredLeft, false);
        g.setColour (mode == 0 || running ? t.accent : t.valueText.withAlpha (0.5f));
        g.fillEllipse (pill.getX() + 56.0f, pill.getCentreY() - 3.5f, 7.0f, 7.0f);
        g.setColour (t.text);
        const juce::String clock = proc.seq.hostClock.load()
            ? juce::String ("FL tempo")
            : juce::String ((int) proc.apvts.getRawParameterValue ("seq_bpm")->load()) + " bpm";
        const juce::String dot = juce::String::fromUTF8 (" \xc2\xb7 ");
        const juce::String text = mode == 0 ? juce::String::fromUTF8 ("mono \xc2\xb7 legato slide")
                                : mode == 1 ? juce::String (running ? "pattern playing" : "pattern stopped") + dot + clock
                                            : juce::String (running ? "arp playing" : "arp: hold notes") + dot + clock;
        g.drawText (text, pill.withTrimmedLeft (70.0f), juce::Justification::centredLeft, false);
    }
}

// ------------------------------------------------------------------ editor
SquelchEditor::SquelchEditor (SquelchProcessor& p)
    : AudioProcessorEditor (p), proc (p), view (p, skin)
{
    skin.bold   = juce::Typeface::createSystemTypefaceFor (BinaryData::FredokaBold_ttf, BinaryData::FredokaBold_ttfSize);
    skin.medium = juce::Typeface::createSystemTypefaceFor (BinaryData::FredokaMedium_ttf, BinaryData::FredokaMedium_ttfSize);

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
    setResizeLimits (700, 375, 1600, 856);
    setSize (SquelchView::canvasW, SquelchView::canvasH);

    startTimerHz (30);
}

SquelchEditor::~SquelchEditor()
{
    stopTimer();
}

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
