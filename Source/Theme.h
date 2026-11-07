// Squelch — colour themes and shared drawing helpers for the interface.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace squelch::ui
{

struct Theme
{
    juce::Colour header, background, pattern, patternFeature, patternOutline;
    juce::Colour panel, panelOutline, shadow;
    juce::Colour text, valueText, titleText;
    juce::Colour accent;          // curve, LED, preset name
    juce::Colour ring;            // big knob ring, pill titles, selected buttons
    juce::Colour onRing;          // text drawn on top of `ring`
    juce::Colour knob, knobOutline, knobDot, bigKnobDot;
    juce::Colour button, buttonOutline, buttonText;
    juce::Colour display, displayOutline;
    juce::Colour logoFace, logoFeature;
    juce::Colour presetButton, presetIcon, presetDisplay;

    static Theme red()
    {
        Theme t;
        t.header = juce::Colour (0xff8e0000);    t.background = juce::Colour (0xff1c0505);
        t.pattern = juce::Colour (0xff4a0909);   t.patternFeature = juce::Colour (0xff1c0505);
        t.patternOutline = juce::Colour (0xff4a0909);
        t.panel = juce::Colour (0xff121212);     t.panelOutline = juce::Colour (0xff8e0000);
        t.shadow = juce::Colour (0xff000000);
        t.text = juce::Colours::white;           t.valueText = juce::Colour (0xffb8b8b8);
        t.titleText = juce::Colours::white;
        t.accent = juce::Colour (0xffe8303a);    t.ring = juce::Colour (0xff8e0000);
        t.onRing = juce::Colours::white;
        t.knob = juce::Colour (0xff1e1e1e);      t.knobOutline = juce::Colour (0xff5a0a0a);
        t.knobDot = juce::Colour (0xfff0f0f0);   t.bigKnobDot = juce::Colour (0xffe8303a);
        t.button = juce::Colour (0xff262626);    t.buttonOutline = juce::Colour (0xff262626);
        t.buttonText = juce::Colours::white;
        t.display = juce::Colours::black;        t.displayOutline = juce::Colour (0xffb5121b);
        t.logoFace = juce::Colour (0xff111111);  t.logoFeature = juce::Colour (0xffe8303a);
        t.presetButton = juce::Colour (0xff161616); t.presetIcon = juce::Colours::white;
        t.presetDisplay = juce::Colour (0xff111111);
        return t;
    }

    static Theme green()
    {
        Theme t;
        t.header = juce::Colour (0xff00f59b);    t.background = juce::Colour (0xffeafff4);
        t.pattern = juce::Colour (0xffffc83d);   t.patternFeature = juce::Colour (0xff10241c);
        t.patternOutline = juce::Colour (0xff10241c);
        t.panel = juce::Colours::white;          t.panelOutline = juce::Colour (0xff10241c);
        t.shadow = juce::Colour (0xff10241c);
        t.text = juce::Colour (0xff10241c);      t.valueText = juce::Colour (0xff3d5a4e);
        t.titleText = juce::Colours::white;
        t.accent = juce::Colour (0xff00f59b);    t.ring = juce::Colour (0xff00f59b);
        t.onRing = juce::Colour (0xff10241c);
        t.knob = juce::Colours::white;           t.knobOutline = juce::Colour (0xff10241c);
        t.knobDot = juce::Colour (0xff10241c);   t.bigKnobDot = juce::Colour (0xff10241c);
        t.button = juce::Colours::white;         t.buttonOutline = juce::Colour (0xff10241c);
        t.buttonText = juce::Colour (0xff10241c);
        t.display = juce::Colour (0xff10241c);   t.displayOutline = juce::Colour (0xff10241c);
        t.logoFace = juce::Colour (0xffffc83d);  t.logoFeature = juce::Colour (0xff10241c);
        t.presetButton = juce::Colours::white;   t.presetIcon = juce::Colour (0xff10241c);
        t.presetDisplay = juce::Colour (0xff111a16);
        return t;
    }
};

// Shared look: current theme + the embedded rounded font.
struct Skin
{
    Theme theme = Theme::red();
    juce::Typeface::Ptr bold, medium;

    juce::Font font (float height, bool isBold = true) const
    {
        auto tf = isBold ? bold : medium;
        if (tf == nullptr) return juce::Font (juce::FontOptions (height, isBold ? juce::Font::bold : juce::Font::plain));
        return juce::Font (juce::FontOptions (tf).withHeight (height));
    }
};

// A smiley face: filled circle, two oval eyes, curved mouth.
inline void drawSmiley (juce::Graphics& g, juce::Point<float> c, float r,
                        juce::Colour face, juce::Colour feature, juce::Colour outline, float outlineW)
{
    g.setColour (face);
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    if (outlineW > 0.0f)
    {
        g.setColour (outline);
        g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, outlineW);
    }
    g.setColour (feature);
    const float ew = r * 0.17f, eh = r * 0.34f;
    g.fillEllipse (c.x - r * 0.36f - ew / 2, c.y - r * 0.38f, ew, eh);
    g.fillEllipse (c.x + r * 0.36f - ew / 2, c.y - r * 0.38f, ew, eh);
    juce::Path mouth;
    mouth.addCentredArc (c.x, c.y + r * 0.02f, r * 0.55f, r * 0.5f, 0.0f,
                         juce::MathConstants<float>::pi * 0.62f, juce::MathConstants<float>::pi * 1.38f, true);
    g.strokePath (mouth, juce::PathStrokeType (std::max (1.5f, r * 0.13f), juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

// Rounded box with the offset "sticker" shadow used everywhere in the design.
inline void drawStickerBox (juce::Graphics& g, juce::Rectangle<float> r, float radius, juce::Colour fill,
                            juce::Colour outline, float outlineW, juce::Colour shadow, float shadowOffset)
{
    if (shadowOffset > 0.0f)
    {
        g.setColour (shadow);
        g.fillRoundedRectangle (r.translated (shadowOffset, shadowOffset), radius);
    }
    g.setColour (fill);
    g.fillRoundedRectangle (r, radius);
    if (outlineW > 0.0f)
    {
        g.setColour (outline);
        g.drawRoundedRectangle (r.reduced (outlineW * 0.5f), radius, outlineW);
    }
}

} // namespace squelch::ui
