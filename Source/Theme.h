// Squelch — colour themes, fonts and shared drawing helpers (flat style, v0.7).
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace squelch::ui
{

struct Theme
{
    juce::Colour background;      // window
    juce::Colour divider;         // header / footer lines
    juce::Colour panel, panelBorder;
    juce::Colour text;            // main text and values
    juce::Colour label;           // small spaced captions
    juce::Colour selected;        // selected segment, accent / slide cells, tab
    juce::Colour onSelected;      // text on `selected`
    juce::Colour active;          // knob arcs, curve, lights, playing step, preset name
    juce::Colour onActive;        // text on `active` (GENERATE, step number)
    juce::Colour track;           // unlit knob arc
    juce::Colour knobBody, knobBorder, pointer;
    juce::Colour well;            // dark boxes: segment containers, steppers, note cells, display
    juce::Colour wellBorder;
    juce::Colour group;           // step group background
    juce::Colour octOn, octOnBorder;
    juce::Colour gateOn;          // gate bar of a step that plays
    juce::Colour toggleOnTrack, toggleOffTrack, toggleOffKnob;
    juce::Colour pillOnFill;      // PING-PONG / SYNC / LATCH when on
    juce::Colour playFill;        // STOP button background while playing
    juce::Colour displayFill, grid;
    juce::Colour logoFace, logoFeature;
    juce::Colour meterOff;

    // ---------------------------------------------------------------- Y2K / acid / rave
    static Theme y2k()
    {
        Theme t;
        t.background = juce::Colour (0xff17122f);  t.divider = juce::Colour (0xff2a2350);
        t.panel = juce::Colour (0xff1d1738);       t.panelBorder = juce::Colour (0xff2c2553);
        t.text = juce::Colour (0xfff3f1ff);        t.label = juce::Colour (0xffa69ed4);
        t.selected = juce::Colour (0xffff45c8);    t.onSelected = juce::Colour (0xff17122f);
        t.active = juce::Colour (0xffccff33);      t.onActive = juce::Colour (0xff17122f);
        t.track = juce::Colour (0xff2f2763);
        t.knobBody = juce::Colour (0xff221c45);    t.knobBorder = juce::Colour (0xff3d3375);
        t.pointer = juce::Colours::white;
        t.well = juce::Colour (0xff0e0b20);        t.wellBorder = juce::Colour (0xff2c2553);
        t.group = juce::Colour (0xff221a45);
        t.octOn = juce::Colour (0xff342b70);       t.octOnBorder = juce::Colour (0xff6a5cc8);
        t.gateOn = juce::Colour (0xff4b3f99);
        t.toggleOnTrack = juce::Colour (0xff3b4614); t.toggleOffTrack = juce::Colour (0xff2a2350);
        t.toggleOffKnob = juce::Colour (0xff6d65a0);
        t.pillOnFill = juce::Colour (0xff3a1240);  t.playFill = juce::Colour (0xff2b3312);
        t.displayFill = juce::Colour (0xff1d1c24); t.grid = juce::Colour (0xff22203a);
        t.logoFace = juce::Colour (0xffffd60a);    t.logoFeature = juce::Colour (0xff17122f);
        t.meterOff = juce::Colour (0xff2a2350);
        return t;
    }

    // ---------------------------------------------------------------- red (dark)
    static Theme red()
    {
        Theme t;
        t.background = juce::Colour (0xff120d0e);  t.divider = juce::Colour (0xff2b2022);
        t.panel = juce::Colour (0xff1a1415);       t.panelBorder = juce::Colour (0xff2e2325);
        t.text = juce::Colour (0xfff6efef);        t.label = juce::Colour (0xffb7a3a6);
        t.selected = juce::Colour (0xffe3242f);    t.onSelected = juce::Colours::white;
        t.active = juce::Colour (0xffff5a61);      t.onActive = juce::Colour (0xff120d0e);
        t.track = juce::Colour (0xff3a2a2d);
        t.knobBody = juce::Colour (0xff241b1d);    t.knobBorder = juce::Colour (0xff4a3538);
        t.pointer = juce::Colours::white;
        t.well = juce::Colour (0xff0b0809);        t.wellBorder = juce::Colour (0xff2e2325);
        t.group = juce::Colour (0xff221a1b);
        t.octOn = juce::Colour (0xff4a2226);       t.octOnBorder = juce::Colour (0xff9a3b42);
        t.gateOn = juce::Colour (0xff6b3036);
        t.toggleOnTrack = juce::Colour (0xff4a1a1e); t.toggleOffTrack = juce::Colour (0xff2b2022);
        t.toggleOffKnob = juce::Colour (0xff7a6669);
        t.pillOnFill = juce::Colour (0xff3d1216);  t.playFill = juce::Colour (0xff3a1418);
        t.displayFill = juce::Colour (0xff1f1a1b); t.grid = juce::Colour (0xff261e20);
        t.logoFace = juce::Colour (0xffe3242f);    t.logoFeature = juce::Colour (0xff120d0e);
        t.meterOff = juce::Colour (0xff2b2022);
        return t;
    }

    // ---------------------------------------------------------------- green (light)
    static Theme green()
    {
        Theme t;
        t.background = juce::Colour (0xffe6f8ee);  t.divider = juce::Colour (0xffc9e6d6);
        t.panel = juce::Colours::white;            t.panelBorder = juce::Colour (0xffcfe7da);
        t.text = juce::Colour (0xff10241c);        t.label = juce::Colour (0xff5a7a6b);
        t.selected = juce::Colour (0xff00e08f);    t.onSelected = juce::Colour (0xff10241c);
        t.active = juce::Colour (0xff00b06a);      t.onActive = juce::Colours::white;
        t.track = juce::Colour (0xffd6ebe0);
        t.knobBody = juce::Colour (0xfff3fbf7);    t.knobBorder = juce::Colour (0xffb5d6c6);
        t.pointer = juce::Colour (0xff10241c);
        t.well = juce::Colour (0xfff0f8f4);        t.wellBorder = juce::Colour (0xffcfe7da);
        t.group = juce::Colour (0xfff6fbf8);
        t.octOn = juce::Colour (0xffd2f5e4);       t.octOnBorder = juce::Colour (0xff4cc895);
        t.gateOn = juce::Colour (0xff9fdcc0);
        t.toggleOnTrack = juce::Colour (0xffbff0d9); t.toggleOffTrack = juce::Colour (0xffd9ece2);
        t.toggleOffKnob = juce::Colour (0xff9db7aa);
        t.pillOnFill = juce::Colour (0xffd9fbea);  t.playFill = juce::Colour (0xffd9fbea);
        t.displayFill = juce::Colour (0xff1b3a2d); t.grid = juce::Colour (0xff1f4334);
        t.logoFace = juce::Colour (0xffffd60a);    t.logoFeature = juce::Colour (0xff10241c);
        t.meterOff = juce::Colour (0xffd2e9dd);
        return t;
    }
};

// Display inside the filter box uses its own dark colours in every theme
inline juce::Colour displayBackground (const Theme& t)
{
    return t.background.getPerceivedBrightness() > 0.5f ? juce::Colour (0xff10241c) : t.well;
}

enum class Weight { medium, bold, extraBold };

struct Skin
{
    Theme theme = Theme::y2k();
    juce::Typeface::Ptr medium, bold, extraBold;

    juce::Font font (float height, Weight w = Weight::bold, float kerning = 0.0f) const
    {
        auto tf = w == Weight::medium ? medium : (w == Weight::bold ? bold : extraBold);
        auto opts = tf != nullptr ? juce::FontOptions (tf) : juce::FontOptions();
        return juce::Font (opts.withHeight (height).withKerningFactor (kerning));
    }

    // small spaced uppercase caption, e.g. "CUTOFF"
    juce::Font caption (float height = 11.5f) const { return font (height, Weight::bold, 0.14f); }
};

inline void drawSmiley (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour face, juce::Colour feature)
{
    g.setColour (face);
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
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

inline void fillBox (juce::Graphics& g, juce::Rectangle<float> r, float radius, juce::Colour fill,
                     juce::Colour border = juce::Colours::transparentBlack, float borderW = 1.0f)
{
    g.setColour (fill);
    g.fillRoundedRectangle (r, radius);
    if (! border.isTransparent())
    {
        g.setColour (border);
        g.drawRoundedRectangle (r.reduced (borderW * 0.5f), radius, borderW);
    }
}

} // namespace squelch::ui
