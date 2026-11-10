#include "PluginEditor.h"
#include "Presets.h"
#include "BinaryData.h"
#include "Generator.h"
#include "Stickers.h"

using namespace squelch::ui;

namespace layout
{
    using R = juce::Rectangle<float>;
    // synth page
    const R oscPanel    { 18.0f, 98.0f, 232.0f, 440.0f };
    const R filterPanel { 266.0f, 98.0f, 458.0f, 440.0f };
    const R drivePanel  { 740.0f, 98.0f, 242.0f, 440.0f };
    // seq page
    const R seqPanel    { 18.0f, 98.0f, 964.0f, 440.0f };
    // fx page
    const R movePanel   { 18.0f, 98.0f, 342.0f, 440.0f };
    const R delayPanel  { 376.0f, 98.0f, 284.0f, 440.0f };
    const R reverbPanel { 676.0f, 98.0f, 306.0f, 440.0f };

    const juce::Point<float> logoCentre { 43.0f, 37.0f };
    constexpr float logoRadius = 25.0f;

    // live footer area (SEQ status)
    const juce::Rectangle<int> statusArea { 244, 558, 312, 40 };
}

static const juce::String sep = juce::String::fromUTF8 (" \xc2\xb7 ");

// ------------------------------------------------------------------ construction
SquelchView::SquelchView (SquelchProcessor& p, Skin& s)
    : proc (p), skin (s),
      tabSynth (s, "SYNTH"), tabSeq (s, "SEQ"), tabFx (s, "FX"),
      presetBar (s), meter (s, param ("volume")),
      saw (s, "SAW"), square (s, "SQUARE"), pulse (s, "PULSE"), super (s, "SUPER"),
      waveBox (s, { &saw, &square, &pulse, &super }, 2, 8, 10),
      tune (s, param ("tune"), "TUNE"), pw (s, param ("pw"), "PW"),
      sub (s, param ("sub"), "SUB"), slide (s, param ("slide"), "SLIDE"),
      display (s),
      envMod (s, param ("envmod"), "ENV MOD"), cutoff (s, param ("cutoff"), "CUTOFF"),
      res (s, param ("res"), "RESONANCE"), decay (s, param ("decay"), "DECAY"),
      accent (s, param ("accent"), "ACCENT"),
      driveSwitch (s, param ("drive_on")),
      od (s, "OD"), dist (s, "DIST"), fuzz (s, "FUZZ"),
      driveBox (s, { &od, &dist, &fuzz }, 3, 6),
      drive (s, param ("drive"), "DRIVE"), tone (s, param ("tone"), "TONE"),
      level (s, param ("level"), "LEVEL"), mix (s, param ("mix"), "MIX"),
      modeOff (s, "OFF"), modePattern (s, "PATTERN"), modeArp (s, "ARP"),
      seqModeBox (s, { &modeOff, &modePattern, &modeArp }, 3, 6),
      play (s, "PLAY", param ("seq_play"), false),
      clear (s, "CLEAR", false), mutate (s, "MUTATE", false), generate (s, "GENERATE", false, Icon::sparkle, true),
      dragMidi (s), grid (s, p.seq),
      lengthStep (s, param ("seq_len")), rateStep (s, param ("seq_rate")), bpmStep (s, param ("seq_bpm")),
      keyStep (s, param ("key")), scaleStep (s, param ("scale")),
      swing (s, param ("seq_swing"), "SWING"), gate (s, param ("seq_gate"), "GATE"),
      arpUp (s, "UP"), arpDown (s, "DOWN"), arpUpDown (s, "UP-DOWN"), arpRandom (s, "RANDOM"), arpPlayed (s, "PLAYED"),
      arpModeBox (s, { &arpUp, &arpDown, &arpUpDown, &arpRandom, &arpPlayed }, 5, 6),
      latch (s, "LATCH", param ("arp_latch")), chord (s, "CHORD", param ("arp_chord")),
      print (s, "PRINT TO STEPS", false),
      arpOctStep (s, param ("arp_oct")), arpAccStep (s, param ("arp_acc")),
      arpSlide (s, param ("arp_slide"), "SLIDE"),
      softAtk (s, param ("soft_atk"), "SOFT ATK"), accDecay (s, param ("acc_decay"), "ACC DECAY"),
      tracking (s, param ("tracking"), "TRACKING"), filterFm (s, param ("filter_fm"), "FILTER FM"),
      sweepSlow (s, "SLOW"), sweepNormal (s, "NORM"), sweepFast (s, "FAST"),
      sweepBox (s, { &sweepSlow, &sweepNormal, &sweepFast }, 3, 6),
      delaySwitch (s, param ("dly_on")), reverbSwitch (s, param ("rev_on")),
      dlyTimeStep (s, param ("dly_time")), dlyMsStep (s, param ("dly_ms")),
      pingPong (s, "PING-\nPONG", param ("dly_pingpong"), false), sync (s, "SYNC", param ("dly_sync"), false),
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
    for (auto* c : std::initializer_list<juce::Component*> { &tabSynth, &tabSeq, &tabFx, &presetBar, &meter }) addAndMakeVisible (c);

    for (auto* k : { &cutoff, &res }) k->setBodySize (76);
    for (auto* k : { &dlyFb, &dlyTone, &dlyMix }) k->setBodySize (56);

    tabSynth.onClick = [this] { setPage (0); };
    tabSeq.onClick   = [this] { setPage (1); };
    tabFx.onClick    = [this] { setPage (2); };

    // PW knob means pulse width (PULSE) or detune (SUPER)
    waveGroup.onChange = [this] (int w) { updateShapeKnob (w); };
    updateShapeKnob ((int) std::round (value ("wave")));

    display.getCutoffHz  = [this] { return proc.displayCutoffHz.load(); };
    display.getResonance = [this] { return value ("res"); };
    display.readScope    = [this] (float* d, int n) { proc.readScope (d, n); };

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
    // inside FL the tempo is FL's: show it, and dim the box (it only applies in the standalone app)
    bpmStep.valueText = [this]
    {
        const float host = proc.hostTempo.load();
        return host > 0.0f ? juce::String (host, std::abs (host - std::round (host)) < 0.05f ? 0 : 1) + " FL"
                           : juce::String ((int) value ("seq_bpm")) + " BPM";
    };
    bpmStep.setTooltip ("Inside FL the sequencer always follows FL's tempo. This box is only used in the standalone app");
    dragMidi.makeFile = [this] { return proc.writePatternMidi(); };
    clear.onClick    = [this] { clearPattern(); };
    generate.onClick = [this] { generateLine (false); };
    mutate.onClick   = [this] { generateLine (true); };
    print.onClick    = [this] { proc.seq.printRequest.store (true); };
    lastPrintOk = proc.seq.printOk.load();
    lastPrintFail = proc.seq.printFail.load();
    latch.init(); chord.init(); pingPong.init(); sync.init();
    sync.onToggled = [this] { refreshVisibility(); };

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
    background = {};
    refreshVisibility();
    repaint();
}

void SquelchView::themeChanged()
{
    background = {};
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
    background = {}; // captions depend on the mode
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
    if (page == 1)
    {
        grid.repaint();
        const bool hostTempo = proc.hostTempo.load() > 0.0f;
        bpmStep.setAlpha (hostTempo ? 0.5f : 1.0f);
        bpmStep.repaint();
    }

    const int ok = proc.seq.printOk.load(), fail = proc.seq.printFail.load();
    const auto now = juce::Time::getMillisecondCounter();
    if (ok != lastPrintOk)     { lastPrintOk = ok;     toast = "PRINTED TO STEPS";              toastUntil = now + 2500; }
    if (fail != lastPrintFail) { lastPrintFail = fail; toast = "HOLD NOTES FIRST, THEN PRINT"; toastUntil = now + 2500; }
    if (toast.isNotEmpty() && now > toastUntil) toast = {};

    repaint (layout::statusArea);
}

void SquelchView::mouseUp (const juce::MouseEvent& e)
{
    if (e.position.getDistanceFrom (layout::logoCentre) <= layout::logoRadius + 3.0f && onThemeToggle)
        onThemeToggle();
}

// ------------------------------------------------------------------ layout
// place a knob by the centre of its body
static void placeKnob (Knob& k, int cx, int cy, int body, int width = 0)
{
    k.setBodySize (body);
    const int f = k.frame();
    const int w = std::max (width > 0 ? width : f, 96);
    k.setBounds (cx - w / 2, cy - f / 2, w, f + 34);
}

void SquelchView::resized()
{
    tabSynth.setBounds (420, 18, 82, 36);
    tabSeq.setBounds (510, 18, 70, 36);
    tabFx.setBounds (588, 18, 58, 36);
    presetBar.setBounds (672, 18, 312, 36);
    meter.setBounds (748, 556, 234, 44);

    // synth
    waveBox.setBounds (36, 122, 196, 82);
    placeKnob (tune, 84, 262, 46);
    placeKnob (pw, 186, 262, 46);
    placeKnob (sub, 84, 392, 46);
    placeKnob (slide, 186, 392, 46);
    display.setBounds (284, 120, 422, 168);
    placeKnob (envMod, 314, 366, 46);
    placeKnob (cutoff, 414, 352, 76);
    placeKnob (res, 522, 352, 76);
    placeKnob (decay, 612, 366, 46);
    placeKnob (accent, 680, 366, 46);
    driveSwitch.setBounds (758, 118, 90, 34);
    driveBox.setBounds (758, 164, 208, 36);
    placeKnob (drive, 806, 258, 46);
    placeKnob (tone, 914, 258, 46);
    placeKnob (level, 806, 392, 46);
    placeKnob (mix, 914, 392, 46);

    // seq
    seqModeBox.setBounds (36, 116, 246, 36);
    play.setBounds (290, 116, 84, 36);
    clear.setBounds (382, 116, 80, 36);
    mutate.setBounds (600, 116, 90, 36);
    generate.setBounds (698, 116, 124, 36);
    dragMidi.setBounds (830, 116, 134, 36);
    grid.setBounds (36, 170, 926, 172);
    lengthStep.setBounds (40, 410, 112, 36);
    rateStep.setBounds (172, 410, 112, 36);
    placeKnob (swing, 350, 425, 46);
    placeKnob (gate, 450, 425, 46);
    bpmStep.setBounds (540, 410, 112, 36);
    keyStep.setBounds (680, 410, 112, 36);
    scaleStep.setBounds (820, 410, 140, 36);

    arpModeBox.setBounds (100, 188, 450, 36);
    latch.setBounds (570, 188, 90, 36);
    chord.setBounds (668, 188, 90, 36);
    print.setBounds (766, 188, 196, 36);
    arpOctStep.setBounds (100, 264, 120, 36);
    arpAccStep.setBounds (240, 264, 150, 36);
    placeKnob (arpSlide, 450, 282, 46);

    // fx
    placeKnob (softAtk, 84, 190, 46, 100);
    placeKnob (accDecay, 188, 190, 46, 100);
    placeKnob (tracking, 292, 190, 46, 100);
    placeKnob (filterFm, 84, 320, 46, 100);
    sweepBox.setBounds (150, 296, 190, 36);
    delaySwitch.setBounds (394, 118, 90, 34);
    dlyTimeStep.setBounds (392, 182, 100, 36);
    dlyMsStep.setBounds (392, 182, 100, 36);
    pingPong.setBounds (502, 182, 80, 36);
    sync.setBounds (590, 182, 56, 36);
    placeKnob (dlyFb, 430, 420, 56);
    placeKnob (dlyTone, 518, 420, 56);
    placeKnob (dlyMix, 606, 420, 56);
    reverbSwitch.setBounds (694, 118, 90, 34);
    placeKnob (revSize, 734, 300, 46);
    placeKnob (revDamp, 829, 300, 46);
    placeKnob (revMix, 924, 300, 46);
    placeKnob (revPre, 734, 430, 46);
    placeKnob (revLowCut, 829, 430, 46);
    placeKnob (revDuck, 924, 430, 46);

    background = {};
}

// ------------------------------------------------------------------ painting
void SquelchView::paint (juce::Graphics& g)
{
    const float scale = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (! background.isValid() || std::abs (scale - backgroundScale) > 0.01f)
    {
        backgroundScale = scale;
        background = juce::Image (juce::Image::ARGB, juce::roundToInt ((float) canvasW * scale), juce::roundToInt ((float) canvasH * scale), true);
        juce::Graphics bg (background);
        bg.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        bg.addTransform (juce::AffineTransform::scale (scale));
        paintStatic (bg);
    }
    g.drawImage (background, juce::Rectangle<float> (0.0f, 0.0f, (float) canvasW, (float) canvasH));
    paintStatus (g);
}

void SquelchView::paintStatic (juce::Graphics& g)
{
    const auto& t = skin.theme;

    // the lit plastic / black glass / pink laptop, rendered from the approved mockups
    if (t.bgData != nullptr)
        g.drawImage (juce::ImageCache::getFromMemory (t.bgData, t.bgSize), juce::Rectangle<float> (0.0f, 0.0f, (float) canvasW, (float) canvasH));
    else
        g.fillAll (juce::Colours::black);

    // ---- header: logo screen
    {
        const auto c = layout::logoCentre;
        const float r = layout::logoRadius;
        g.setColour (juce::Colours::black.withAlpha (t.kind == Theme::Kind::kawaii ? 0.12f : 0.4f));
        g.fillEllipse (c.x - r - 4.0f, c.y - r - 1.0f, (r + 4.0f) * 2.0f, (r + 4.0f) * 2.0f + 3.0f);
        g.setColour (t.logoRing);
        g.fillEllipse (c.x - r - 5.0f, c.y - r - 5.0f, (r + 5.0f) * 2.0f, (r + 5.0f) * 2.0f);
        g.setColour (t.kind == Theme::Kind::kawaii ? juce::Colour (0xffffc3dc) : t.bezelMid);
        g.fillEllipse (c.x - r - 4.0f, c.y - r - 4.0f, (r + 4.0f) * 2.0f, (r + 4.0f) * 2.0f);
        g.setColour (t.logoScreen);
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        juce::Graphics::ScopedSaveState s (g);
        juce::Path clip; clip.addEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.reduceClipRegion (clip);
        drawGlitchSmiley (g, { c.x, c.y + 3.0f }, 12.5f, t, 1.5f, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        for (float y = c.y - r; y < c.y + r; y += 3.0f) g.fillRect (c.x - r, y, r * 2.0f, 1.0f);
        g.setColour (t.glare.withMultipliedAlpha (1.5f));
        g.fillEllipse (c.x - r * 0.8f, c.y - r * 1.1f, r * 1.3f, r * 0.8f);
    }

    // ---- title, katakana, part code
    {
        const juce::String name ("SQUELCH");
        if (t.kind == Theme::Kind::kawaii)
        {
            const auto f = skin.title();
            juce::GlyphArrangement ga;
            ga.addLineOfText (f, name, 84.0f, 6.0f + f.getAscent());
            juce::Path p; ga.createPath (p);
            g.setColour (t.titleShadow); g.fillPath (p, juce::AffineTransform::translation (2.0f, 3.0f));
            g.setColour (t.titleStroke); g.strokePath (p, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (t.title); g.fillPath (p);
        }
        else
        {
            const auto f = skin.title();
            juce::GlyphArrangement ga;
            ga.addLineOfText (f, name, 84.0f, 9.0f + f.getAscent() + 2.0f);
            juce::Path p; ga.createPath (p);
            if (t.titleGlow) glowFill (g, p, t.title, 1.6f);
            else
            {
                g.setColour (t.titleShadow); g.fillPath (p, juce::AffineTransform::translation (0.0f, 1.0f));
                g.setColour (t.title); g.fillPath (p);
            }
        }
        g.setColour (t.ink2);
        g.setFont (juce::Font (skin.readout (13.0f)).withExtraKerningFactor (0.08f));
        g.drawText (juce::String::fromUTF8 ("\xe3\x82\xb9\xe3\x82\xaf\xe3\x82\xa8\xe3\x83\xab\xe3\x83\x81"), // スクエルチ
                    juce::Rectangle<float> (t.kind == Theme::Kind::kawaii ? 88.0f : 86.0f, 50.0f, 90.0f, 17.0f), juce::Justification::centredLeft, false);
        g.setFont (skin.mono (8.5f, true, 0.14f));
        g.drawText ("SQ-303 // ACID BASS OSCILLATOR // CH1", juce::Rectangle<float> (168.0f, 53.0f, 260.0f, 12.0f), juce::Justification::centredLeft, false);
    }

    // ---- divider under the header
    g.setColour (t.divider);
    g.fillRect (0.0f, 76.0f, (float) canvasW, 2.0f);
    if (! t.dividerHighlight.isTransparent()) { g.setColour (t.dividerHighlight); g.fillRect (0.0f, 78.0f, (float) canvasW, 1.0f); }

    // ---- page panels and captions
    auto caption = [&] (const juce::String& text, float x, float y, float w = 120.0f, juce::Justification j = juce::Justification::centredLeft)
    {
        g.setColour (t.ink);
        g.setFont (skin.label (10.0f));
        g.drawText (text, juce::Rectangle<float> (x, y, w, 14.0f), j, false);
    };
    auto body = [&] (const juce::String& text, juce::Rectangle<int> r, int lines)
    {
        g.setColour (t.ink2);
        g.setFont (t.roundedFont ? Skin::make (skin.fredoka, 12.5f, 0.02f) : skin.mono (10.5f, false, 0.02f));
        g.drawFittedText (text, r, juce::Justification::topLeft, lines, 1.0f);
    };

    if (page == 0)
    {
        drawSection (g, layout::oscPanel, "OSCILLATOR", "VCO-1", skin);
        drawSection (g, layout::filterPanel, "FILTER + ENVELOPE", "24 dB/OCT LPF", skin);
        drawSection (g, layout::drivePanel, "DRIVE", "OD" + sep + "DIST" + sep + "FUZZ", skin);
    }
    else if (page == 1)
    {
        drawSection (g, layout::seqPanel, "SEQUENCER", "16-STEP // ARP", skin);
        g.setColour (t.divider);
        g.fillRect (36.0f, 368.0f, 926.0f, 1.0f);
        caption ("LENGTH", 44.0f, 392.0f);
        caption ("RATE", 176.0f, 392.0f);
        caption ("BPM", 544.0f, 392.0f);
        caption ("KEY", 684.0f, 392.0f);
        caption ("SCALE", 824.0f, 392.0f);

        if ((int) value ("seq_mode") == squelch::seqArp)
        {
            caption ("MODE", 104.0f, 170.0f);
            caption ("OCTAVES", 104.0f, 246.0f);
            caption ("ACCENT", 244.0f, 246.0f);
            body ("Hold notes on your keyboard, or LATCH them. CHORD builds a chord from one key using "
                  "KEY + SCALE. PRINT TO STEPS copies what the arp plays into the pattern so you can edit it.",
                  { 520, 250, 440, 80 }, 5);
        }
    }
    else
    {
        drawSection (g, layout::movePanel, "MOVEMENT", "TD-3-MO EXTRAS", skin);
        drawSection (g, layout::delayPanel, "DELAY", "STEREO ECHO", skin);
        drawSection (g, layout::reverbPanel, "REVERB", "PRE" + sep + "LOW CUT" + sep + "DUCK", skin);
        caption ("SWEEP", 150.0f, 338.0f, 190.0f, juce::Justification::centred);
        caption (value ("dly_sync") > 0.5f ? "TIME (SYNC)" : "TIME (MS)", 396.0f, 164.0f);
        body ("How notes start, how long accents stay open, and how the filter follows the notes.",
              { 36, 440, 306, 60 }, 3);
    }

    // ---- footer strip (static parts)
    {
        const juce::Rectangle<float> f (0.0f, 556.0f, (float) canvasW, 44.0f);
        g.setGradientFill (juce::ColourGradient (t.footTop, 0.0f, f.getY(), t.footBottom, 0.0f, f.getBottom(), false));
        g.fillRect (f);
        if (t.kind == Theme::Kind::phosphor) { g.setColour (juce::Colour (0x99ff96e6)); g.fillRect (0.0f, 555.0f, (float) canvasW, 1.0f); }
        g.setColour (t.footBorder);
        g.fillRect (0.0f, 556.0f, (float) canvasW, 2.0f);

        const float cy = 579.0f;
        const auto ledCol = t.kind == Theme::Kind::kawaii ? juce::Colours::white : t.phos;
        drawLed (g, { 18.0f, cy - 4.0f, 8.0f, 8.0f }, true, t, ledCol);
        g.setColour (t.footText);
        g.setFont (skin.mono (10.5f, true, 0.12f));
        g.drawText ("MIDI", juce::Rectangle<float> (34.0f, cy - 8.0f, 40.0f, 16.0f), juce::Justification::centredLeft, false);
        g.setColour (t.footText.withMultipliedAlpha (0.7f));
        g.setFont (skin.mono (10.5f, false, 0.12f));
        g.drawText ("MONO / LEGATO SLIDE", juce::Rectangle<float> (76.0f, cy - 8.0f, 170.0f, 16.0f), juce::Justification::centredLeft, false);

        // barcode + serial
        float x = 560.0f;
        const int bars[] = { 2,1,1,3,1,2,1,1,2,3,1,1,2,1,3,1,2,2,1,1 };
        g.setColour (t.barcode);
        for (int i = 0; i < 20; ++i)
        {
            if (i % 2 == 0) g.fillRect (x, 568.0f, (float) bars[i] * 1.3f, 20.0f);
            x += (float) bars[i] * 1.3f + 0.6f;
        }
        g.setColour (t.footText.withMultipliedAlpha (0.6f));
        g.setFont (skin.mono (8.0f, false, 0.1f));
        g.drawText ("SN 0420-0303", juce::Rectangle<float> (636.0f, 572.0f, 100.0f, 12.0f), juce::Justification::centredLeft, false);
        g.drawText ("V0.8", juce::Rectangle<float> (636.0f, 583.0f, 100.0f, 10.0f), juce::Justification::centredLeft, false);
    }

    if (t.stickers) paintStickers (g);
}

void SquelchView::paintStickers (juce::Graphics& g)
{
    using S = Sticker;
    auto st = [&] (S k, float x, float y, float rot, float sc) { drawSticker (g, k, x, y, rot, sc, skin); };
    // the header one is on every page
    st (S::banner, 352.0f, 28.0f, -6.0f, 0.78f);

    if (page == 0)
    {
        st (S::bow, 980.0f, 102.0f, 16.0f, 0.8f);
        st (S::mascot, 64.0f, 516.0f, -12.0f, 0.98f);
        st (S::yellowSmiley, 205.0f, 512.0f, 12.0f, 0.85f);
        st (S::melting, 372.0f, 490.0f, -6.0f, 1.0f);
        st (S::acidPill, 505.0f, 513.0f, -4.0f, 1.0f);
        st (S::heart, 640.0f, 497.0f, 10.0f, 1.05f);
        st (S::sparkle, 705.0f, 470.0f, -14.0f, 0.8f);
        st (S::bolt, 780.0f, 514.0f, 18.0f, 0.8f);
        st (S::badge303, 862.0f, 510.0f, 8.0f, 0.9f);
        st (S::cherries, 952.0f, 522.0f, -10.0f, 0.8f);
    }
    else if (page == 1)
    {
        st (S::heart, 531.0f, 134.0f, 8.0f, 0.8f);
        st (S::mascot, 46.0f, 524.0f, -12.0f, 0.8f);
        st (S::cherries, 958.0f, 524.0f, -10.0f, 0.75f);
        if ((int) value ("seq_mode") != squelch::seqArp)
        {
            st (S::acidPill, 600.0f, 515.0f, -4.0f, 0.9f);
            st (S::sparkle, 520.0f, 500.0f, -14.0f, 0.7f);
        }
        else
        {
            st (S::melting, 300.0f, 330.0f, -6.0f, 0.9f);
            st (S::badge303, 600.0f, 515.0f, 8.0f, 0.8f);
        }
    }
    else
    {
        st (S::yellowSmiley, 216.0f, 400.0f, -8.0f, 0.8f);
        st (S::heart, 300.0f, 405.0f, 10.0f, 0.85f);
        st (S::melting, 450.0f, 292.0f, -6.0f, 1.0f);
        st (S::acidPill, 590.0f, 268.0f, -4.0f, 0.9f);
        st (S::badge303, 560.0f, 330.0f, 8.0f, 0.8f);
        st (S::mascot, 860.0f, 190.0f, -12.0f, 0.9f);
        st (S::sparkle, 812.0f, 140.0f, -14.0f, 0.7f);
        st (S::bolt, 940.0f, 200.0f, 18.0f, 0.8f);
        st (S::cherries, 958.0f, 524.0f, -10.0f, 0.75f);
    }
}

void SquelchView::paintStatus (juce::Graphics& g)
{
    const auto& t = skin.theme;
    const int mode = (int) value ("seq_mode");
    const int step = proc.seq.currentStep.load();
    const bool running = step >= 0 && mode != squelch::seqOff;
    const float host = proc.hostTempo.load();
    const juce::String clock = host > 0.0f ? juce::String (juce::roundToInt (host)) + " BPM (FL)"
                                           : juce::String ((int) value ("seq_bpm")) + " BPM";
    const juce::String stepText = "STEP " + juce::String (step + 1).paddedLeft ('0', 2);
    juce::String status;
    if (toast.isNotEmpty())               status = toast;
    else if (mode == squelch::seqOff)     status = "OFF";
    else if (mode == squelch::seqPattern) status = running ? "PLAYING" + sep + stepText + sep + clock : "HOLD A KEY OR PLAY" + sep + clock;
    else                                  status = running ? "ARP" + sep + stepText + sep + clock : "ARP" + sep + "HOLD NOTES" + sep + clock;

    // the cached background holds the footer gradient underneath
    const float cy = 579.0f;
    const auto ledCol = t.kind == Theme::Kind::kawaii ? juce::Colours::white : t.phos;
    drawLed (g, { 250.0f, cy - 4.0f, 8.0f, 8.0f }, running || toast.isNotEmpty(), t, ledCol);
    g.setColour (t.footText);
    g.setFont (skin.mono (10.5f, true, 0.12f));
    g.drawText ("SEQ", juce::Rectangle<float> (266.0f, cy - 8.0f, 30.0f, 16.0f), juce::Justification::centredLeft, false);
    g.setColour (t.footText.withMultipliedAlpha (toast.isNotEmpty() ? 1.0f : 0.7f));
    g.setFont (skin.mono (10.5f, false, 0.12f));
    g.drawFittedText (status, juce::Rectangle<float> (300.0f, cy - 8.0f, 252.0f, 16.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.7f);
}

// ------------------------------------------------------------------ editor
SquelchEditor::SquelchEditor (SquelchProcessor& p)
    : AudioProcessorEditor (p), proc (p), view (p, skin)
{
    skin.loadFonts();

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
    skin.theme = Theme::byIndex (proc.getTheme());
    view.themeChanged();
}

void SquelchEditor::resized()
{
    const float scale = (float) getWidth() / (float) SquelchView::canvasW;
    view.setBounds (0, 0, SquelchView::canvasW, SquelchView::canvasH);
    view.setTransform (juce::AffineTransform::scale (scale));
}
