#include "PluginEditor.h"
#include "Presets.h"
#include "BinaryData.h"

using namespace squelch::ui;

// ------------------------------------------------------------------ layout constants
namespace layout
{
    const juce::Rectangle<float> oscPanel    { 30.0f, 103.0f, 202.0f, 368.0f };
    const juce::Rectangle<float> filterPanel { 251.0f, 103.0f, 480.0f, 368.0f };
    const juce::Rectangle<float> drivePanel  { 750.0f, 103.0f, 222.0f, 368.0f };
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
      waveGroup (param ("wave"), { &saw, &square, &pulse, &super }),
      modeGroup (param ("drive_mode"), { &od, &dist, &fuzz })
{
    for (auto* c : std::initializer_list<juce::Component*> {
             &saw, &square, &pulse, &super, &tune, &pw, &sub, &slide,
             &display, &envMod, &cutoff, &res, &decay, &accent,
             &od, &dist, &fuzz, &driveLed, &drive, &tone, &level, &mix,
             &presetBar, &meter })
        addAndMakeVisible (c);

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
    presetBar.getCount = [] { return squelch::numFactoryPresets; };
    presetBar.getName  = [] (int i) { return juce::String (squelch::factoryPresets[juce::jlimit (0, squelch::numFactoryPresets - 1, i)].name); };
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

    // header / footer
    presetBar.setBounds (612, 18, 362, 48);
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
    g.setColour (t.header);
    g.fillRect (0.0f, 0.0f, w, 78.0f);
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
        g.drawText (juce::String::fromUTF8 ("acid bass synth \xc2\xb7 v0.3.1"), pill, juce::Justification::centred, false);
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
    panel (layout::oscPanel, "OSCILLATOR");
    panel (layout::filterPanel, "FILTER + ENVELOPE");
    panel (layout::drivePanel, "DRIVE FX");

    g.setColour (t.valueText);
    g.setFont (skin.font (13.5f, false));
    g.drawText ("24 dB low-pass", juce::Rectangle<float> (560.0f, 118.0f, 155.0f, 27.0f),
                juce::Justification::centredRight, false);

    // --- footer: MIDI pill
    {
        const juce::Rectangle<float> pill (27.0f, 486.0f, 180.0f, 34.0f);
        drawStickerBox (g, pill, 17.0f, t.panel, t.panelOutline, 2.5f, t.shadow, 3.5f);
        g.setColour (t.text);
        g.setFont (skin.font (14.0f));
        g.drawText ("MIDI", pill.withTrimmedLeft (14.0f), juce::Justification::centredLeft, false);
        g.setColour (t.accent);
        g.fillEllipse (pill.getX() + 56.0f, pill.getCentreY() - 3.5f, 7.0f, 7.0f);
        g.setColour (t.text);
        g.drawText (juce::String::fromUTF8 ("mono \xc2\xb7 legato slide"), pill.withTrimmedLeft (70.0f), juce::Justification::centredLeft, false);
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
        proc.setTheme (proc.getTheme() == 0 ? 1 : 0);
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
    skin.theme = proc.getTheme() == 1 ? Theme::green() : Theme::red();
    view.repaint();
}

void SquelchEditor::resized()
{
    const float scale = (float) getWidth() / (float) SquelchView::canvasW;
    view.setBounds (0, 0, SquelchView::canvasW, SquelchView::canvasH);
    view.setTransform (juce::AffineTransform::scale (scale));
}
