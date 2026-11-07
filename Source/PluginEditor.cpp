#include "PluginEditor.h"

namespace colours
{
    const juce::Colour bg      { 0xff141414 };
    const juce::Colour panel   { 0xff1f1f1f };
    const juce::Colour accent  { 0xffd4ff3a }; // acid yellow-green
    const juce::Colour text    { 0xffe6e6e6 };
    const juce::Colour dim     { 0xff8a8a8a };
}

// ------------------------------------------------------------------ look and feel
SquelchLookAndFeel::SquelchLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, colours::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, colours::text);
    setColour (juce::ComboBox::backgroundColourId, colours::panel);
    setColour (juce::ComboBox::outlineColourId, colours::dim);
    setColour (juce::ComboBox::textColourId, colours::text);
    setColour (juce::ComboBox::arrowColourId, colours::accent);
    setColour (juce::PopupMenu::backgroundColourId, colours::panel);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::accent.withAlpha (0.3f));
}

void SquelchLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                           float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float arcR = radius - 3.0f;

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (colours::panel.brighter (0.15f));
    g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
    g.setColour (colours::accent);
    g.strokePath (value, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float knobR = radius - 10.0f;
    g.setColour (colours::panel.brighter (0.05f));
    g.fillEllipse (centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -knobR, 3.0f, knobR * 0.55f, 1.5f);
    g.setColour (colours::text);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
}

// ------------------------------------------------------------------ editor
SquelchEditor::SquelchEditor (SquelchProcessor& p)
    : AudioProcessorEditor (p), processor (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);

    addKnob (cutoff, "cutoff", "CUTOFF");
    addKnob (res,    "res",    "RESONANCE");
    addKnob (envMod, "envmod", "ENV MOD");
    addKnob (decay,  "decay",  "DECAY");
    addKnob (accent, "accent", "ACCENT");
    addKnob (tune,   "tune",   "TUNE");
    addKnob (slide,  "slide",  "SLIDE TIME");
    addKnob (sub,    "sub",    "SUB OSC");
    addKnob (dist,   "dist",   "DISTORTION");
    addKnob (fuzz,   "fuzz",   "FUZZ");
    addKnob (volume, "volume", "VOLUME");

    waveBox.addItemList ({ "Saw", "Square" }, 1);
    addAndMakeVisible (waveBox);
    waveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "wave", waveBox);

    waveLabel.setText ("WAVE", juce::dontSendNotification);
    waveLabel.setJustificationType (juce::Justification::centred);
    waveLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    addAndMakeVisible (waveLabel);

    // On-screen keys: lower half of each key = normal, upper part = harder (accent)
    keyboard.setAvailableRange (24, 72);
    keyboard.setLowestVisibleKey (36);
    keyboard.setVelocity (0.6f, true);
    addAndMakeVisible (keyboard);

    setSize (700, 430);
}

SquelchEditor::~SquelchEditor()
{
    setLookAndFeel (nullptr);
}

void SquelchEditor::addKnob (Knob& k, const juce::String& paramId, const juce::String& name)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
    k.slider.setDoubleClickReturnValue (true, k.slider.getDoubleClickReturnValue());
    addAndMakeVisible (k.slider);
    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, paramId, k.slider);

    k.label.setText (name, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    addAndMakeVisible (k.label);
}

void SquelchEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::bg);

    g.setColour (colours::accent);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("SQUELCH", 20, 10, 200, 30, juce::Justification::centredLeft);

    g.setColour (colours::dim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("acid bass voice  ·  v0.2  ·  overlap notes = slide  ·  velocity 100+ = accent",
                220, 10, getWidth() - 240, 30, juce::Justification::centredRight);

    g.setColour (colours::panel);
    g.fillRoundedRectangle (12.0f, 48.0f, (float) getWidth() - 24.0f, 140.0f, 8.0f);
    g.fillRoundedRectangle (12.0f, 196.0f, (float) getWidth() - 24.0f, 140.0f, 8.0f);
}

void SquelchEditor::resized()
{
    auto layoutRow = [] (juce::Rectangle<int> row, std::initializer_list<juce::Component*> sliders,
                         std::initializer_list<juce::Label*> labels)
    {
        const int n = (int) sliders.size();
        const int w = row.getWidth() / n;
        auto s = sliders.begin();
        auto l = labels.begin();
        for (int i = 0; i < n; ++i, ++s, ++l)
        {
            auto cell = row.removeFromLeft (w).reduced (6, 0);
            (*l)->setBounds (cell.removeFromTop (20));
            (*s)->setBounds (cell);
        }
    };

    auto row1 = juce::Rectangle<int> (12, 56, getWidth() - 24, 126);
    layoutRow (row1, { &cutoff.slider, &res.slider, &envMod.slider, &decay.slider, &accent.slider },
                     { &cutoff.label,  &res.label,  &envMod.label,  &decay.label,  &accent.label });

    auto row2 = juce::Rectangle<int> (12, 204, getWidth() - 24, 126);
    const int w = row2.getWidth() / 7;
    auto waveCell = row2.removeFromLeft (w).reduced (6, 0);
    waveLabel.setBounds (waveCell.removeFromTop (20));
    waveBox.setBounds (waveCell.withSizeKeepingCentre (waveCell.getWidth(), 28).translated (0, -10));

    layoutRow (row2, { &tune.slider, &slide.slider, &sub.slider, &dist.slider, &fuzz.slider, &volume.slider },
                     { &tune.label,  &slide.label,  &sub.label,  &dist.label,  &fuzz.label,  &volume.label });

    keyboard.setBounds (12, 346, getWidth() - 24, getHeight() - 358);
}
