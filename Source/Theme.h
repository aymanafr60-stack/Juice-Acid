// Squelch — themes (Phosphor Lab, Matrix, Kawaii), fonts and shared drawing helpers (v0.8).
// Colours are taken from the approved HTML mockups in design/.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

namespace squelch::ui
{

using juce::Colour;

struct Theme
{
    enum class Kind { phosphor, matrix, kawaii };
    Kind kind = Kind::phosphor;

    // text
    Colour ink, ink2;                    // labels / secondary labels
    Colour title, titleShadow, titleStroke;
    bool titleGlow = false, roundedFont = false;

    // push buttons ("keys")
    Colour keyTop, keyMid, keyBottom, keyHighlight, keyInner, keyEdge, keyShadow, keyText;
    Colour keyOnTop, keyOnMid, keyOnBottom, keyOnText, keyOnEdge, keyOnGlow;
    float keyRadius = 5.0f;
    Colour ledOff, ledOn;                // indicator light on keys
    bool roundLed = false;

    // screens
    Colour screen, grid, gridTick, phos, phos2, glare, glareLine, readoutBg;
    Colour ghostA, ghostB; float ghostAlphaA = .6f, ghostAlphaB = .6f;
    Colour bezelTop, bezelMid, bezelBottom, bezelHighlight, screwTop, screwBottom, screenRing;
    float bezelRadius = 14.0f;

    // knobs (bodies are pre-rendered images; the pointer is drawn live)
    Colour pointer;
    juce::String knobPrefix;             // knob_<prefix>_<size>.png

    // sections, tags
    Colour sectionBorder, sectionFill, sectionGlow, sectionHighlight, tagBg, tagText, tagBorder, tagShadow;
    float sectionBorderW = 1.5f, sectionRadius = 10.0f, tagRadius = 3.0f;

    // header / footer
    Colour divider, dividerHighlight, logoScreen, logoRing;
    Colour footTop, footBottom, footBorder, footText, meterOn, meterHot, meterOff, barcode;

    // toggle switch drawing
    Colour switchBody, switchStroke, switchInner, switchBat, switchBatStroke;

    // background image + extras
    const char* bgData = nullptr; int bgSize = 0;
    bool stickers = false;

    // ------------------------------------------------------------------ Phosphor Lab (pink lab light, green CRT)
    static Theme phosphor()
    {
        Theme t; t.kind = Kind::phosphor;
        t.ink = Colour (0xff3a1236); t.ink2 = Colour (0xff6b3a64);
        t.title = t.ink; t.titleShadow = Colour (0xb3ffdcf5);
        t.keyTop = Colour (0xffffe6f8); t.keyMid = Colour (0xfff2c2e6); t.keyBottom = Colour (0xffd9a0d0);
        t.keyHighlight = Colour (0xf2ffffff); t.keyInner = Colour (0x40963288); t.keyEdge = Colour (0xff8a3f83); t.keyShadow = Colour (0x735a005a);
        t.keyText = t.ink;
        t.keyOnTop = Colour (0xfffff6d0); t.keyOnMid = Colour (0xfffbd97e); t.keyOnBottom = Colour (0xffeeb85a);
        t.keyOnText = t.ink; t.keyOnEdge = Colour (0xff8a3f83); t.keyOnGlow = Colour (0x00000000);
        t.ledOff = Colour (0xff6d4a3a); t.ledOn = Colour (0xffffe24a);
        t.screen = Colour (0xff071419); t.grid = Colour (0xff1f4a52); t.gridTick = Colour (0xff2f6e78);
        t.phos = Colour (0xff7dff8e); t.phos2 = Colour (0xff5aa8ff);
        t.glare = Colour (0x33ff6edc); t.glareLine = Colour (0x8cff8ce6); t.readoutBg = Colour (0xff061114);
        t.ghostA = Colour (0xffff4fd2); t.ghostB = Colour (0xff5ad8ff); t.ghostAlphaA = .55f; t.ghostAlphaB = .55f;
        t.bezelTop = Colour (0xff6a3a63); t.bezelMid = Colour (0xff3a2338); t.bezelBottom = Colour (0xff241522);
        t.bezelHighlight = Colour (0xccffb4f0); t.screwTop = Colour (0xffc58bbd); t.screwBottom = Colour (0xff7a5a76);
        t.screenRing = Colour (0x99ffaaeb);
        t.pointer = Colour (0xfffff4fb); t.knobPrefix = "phosphor";
        t.sectionBorder = Colour (0x8c3a1236); t.sectionFill = Colour (0x00000000); t.sectionGlow = Colour (0x00000000);
        t.sectionHighlight = Colour (0x80ffdcf5);
        t.tagBg = t.ink; t.tagText = Colour (0xfff7d8ef); t.tagBorder = Colour (0x00000000); t.tagShadow = Colour (0x00000000);
        t.divider = Colour (0x733a1236); t.dividerHighlight = Colour (0x99ffc8f0);
        t.logoScreen = Colour (0xff071419); t.logoRing = Colour (0xb3ffaaeb);
        t.footTop = Colour (0xff2a0f29); t.footBottom = Colour (0xff1a0719); t.footBorder = Colour (0xffb44aa6);
        t.footText = Colour (0xfff2c9e8); t.meterOn = t.phos; t.meterHot = Colour (0xffff4fd2); t.meterOff = Colour (0xff3b1638);
        t.barcode = Colour (0xbff2c9e8);
        t.switchBody = Colour (0xffe9d2e3); t.switchStroke = Colour (0xff6b3a64); t.switchInner = Colour (0xffb48fae);
        t.switchBat = Colour (0xfffff0fa); t.switchBatStroke = Colour (0xff6b3a64);
        t.bgData = BinaryData::bg_phosphor_jpg; t.bgSize = BinaryData::bg_phosphor_jpgSize;
        return t;
    }

    // ------------------------------------------------------------------ Matrix (black + neon green)
    static Theme matrix()
    {
        Theme t; t.kind = Kind::matrix;
        t.ink = Colour (0xff5dff84); t.ink2 = Colour (0xff2f9a4c);
        t.title = t.ink; t.titleShadow = Colour (0x8c3cff78); t.titleGlow = true;
        t.keyTop = Colour (0xff16241a); t.keyMid = Colour (0xff0d150f); t.keyBottom = Colour (0xff070b08);
        t.keyHighlight = Colour (0x736eff96); t.keyInner = Colour (0x2e3cff78); t.keyEdge = Colour (0xff000000); t.keyShadow = Colour (0xcc000000);
        t.keyText = t.ink;
        t.keyOnTop = Colour (0xff1d5a2c); t.keyOnMid = Colour (0xff0f3a1b); t.keyOnBottom = Colour (0xff0a2612);
        t.keyOnText = Colour (0xffb9ffcb); t.keyOnEdge = Colour (0xff000000); t.keyOnGlow = Colour (0x593cff78);
        t.ledOff = Colour (0xff0f2a17); t.ledOn = Colour (0xff3dff6e);
        t.screen = Colour (0xff071419); t.grid = Colour (0xff1f4a52); t.gridTick = Colour (0xff2f6e78);
        t.phos = Colour (0xff7dff8e); t.phos2 = Colour (0xffc8ffd6);
        t.glare = Colour (0x1478ffa0); t.glareLine = Colour (0x40a0ffbe); t.readoutBg = Colour (0xff061114);
        t.ghostA = Colour (0xffd8ffe2); t.ghostB = Colour (0xff0f9a3c); t.ghostAlphaA = .35f; t.ghostAlphaB = .7f;
        t.bezelTop = Colour (0xff14281a); t.bezelMid = Colour (0xff0b150e); t.bezelBottom = Colour (0xff050906);
        t.bezelHighlight = Colour (0x8078ffa0); t.screwTop = Colour (0xff1f5a2f); t.screwBottom = Colour (0xff14361e);
        t.screenRing = Colour (0x593cff78);
        t.pointer = Colour (0xffb9ffcb); t.knobPrefix = "matrix";
        t.sectionBorder = Colour (0x733cff78); t.sectionFill = Colour (0xd1020603); t.sectionGlow = Colour (0x1a3cff78);
        t.sectionHighlight = Colour (0x2e78ffa0); t.sectionBorderW = 1.0f;
        t.tagBg = Colour (0xff0b1c10); t.tagText = Colour (0xff7dff8e); t.tagBorder = Colour (0x993cff78); t.tagShadow = Colour (0x00000000);
        t.divider = Colour (0x733cff78); t.dividerHighlight = Colour (0x00000000);
        t.logoScreen = Colour (0xff071419); t.logoRing = Colour (0x8c3cff78);
        t.footTop = Colour (0xff030705); t.footBottom = Colour (0xff000000); t.footBorder = Colour (0x993cff78);
        t.footText = Colour (0xff7dff8e); t.meterOn = Colour (0xff3dff6e); t.meterHot = Colour (0xffd8ffe2); t.meterOff = Colour (0xff0d2414);
        t.barcode = Colour (0x993dff6e);
        t.switchBody = Colour (0xff0d1a11); t.switchStroke = Colour (0xff3dff6e); t.switchInner = Colour (0xff1b3322);
        t.switchBat = Colour (0xff9dffb5); t.switchBatStroke = Colour (0xff0b1c10);
        t.bgData = BinaryData::bg_matrix_jpg; t.bgSize = BinaryData::bg_matrix_jpgSize;
        return t;
    }

    // ------------------------------------------------------------------ Kawaii (white + pink, stickers)
    static Theme kawaii()
    {
        Theme t; t.kind = Kind::kawaii; t.roundedFont = true;
        t.ink = Colour (0xffb02f6e); t.ink2 = Colour (0xffe07aa8);
        t.title = Colour (0xffff4f9a); t.titleShadow = Colour (0xffffd0e4); t.titleStroke = Colour (0xffb02f6e);
        t.keyTop = juce::Colours::white; t.keyMid = Colour (0xfffff0f6); t.keyBottom = Colour (0xffffdcea);
        t.keyHighlight = juce::Colours::white; t.keyInner = Colour (0x40ff78b4); t.keyEdge = Colour (0xfff2a6c8); t.keyShadow = Colour (0x4dff78b4);
        t.keyText = t.ink; t.keyRadius = 12.0f;
        t.keyOnTop = Colour (0xffff9cc8); t.keyOnMid = Colour (0xffff6faa); t.keyOnBottom = Colour (0xfff2508f);
        t.keyOnText = juce::Colours::white; t.keyOnEdge = Colour (0xffc23472); t.keyOnGlow = Colour (0x73ff4f9a);
        t.ledOff = Colour (0xfff4c6da); t.ledOn = juce::Colours::white; t.roundLed = true;
        t.screen = Colour (0xff2c0a1d); t.grid = Colour (0xff5a1d3f); t.gridTick = Colour (0xff7a2c57);
        t.phos = Colour (0xffff8cc6); t.phos2 = juce::Colours::white;
        t.glare = Colour (0x1affffff); t.glareLine = Colour (0x8cff8ce6); t.readoutBg = Colour (0xff3a0f26);
        t.ghostA = juce::Colours::white; t.ghostB = Colour (0xffb77cff); t.ghostAlphaA = .45f; t.ghostAlphaB = .55f;
        t.bezelTop = juce::Colours::white; t.bezelMid = Colour (0xffffe3ef); t.bezelBottom = Colour (0xffffc9df);
        t.bezelHighlight = juce::Colours::white; t.screwTop = Colour (0xffff9cc8); t.screwBottom = Colour (0xffff9cc8);
        t.screenRing = juce::Colours::white; t.bezelRadius = 22.0f;
        t.pointer = Colour (0xffff4f9a); t.knobPrefix = "kawaii";
        t.sectionBorder = Colour (0xffffc3dc); t.sectionFill = Colour (0xbfffffff); t.sectionGlow = Colour (0x26ff78b4);
        t.sectionHighlight = Colour (0x00000000); t.sectionBorderW = 2.0f; t.sectionRadius = 18.0f;
        t.tagBg = Colour (0xffff6faa); t.tagText = juce::Colours::white; t.tagBorder = Colour (0x00000000); t.tagShadow = Colour (0xffd9407f);
        t.tagRadius = 11.0f;
        t.divider = Colour (0xffffc3dc); t.dividerHighlight = Colour (0x00000000);
        t.logoScreen = Colour (0xff3a0f26); t.logoRing = juce::Colours::white;
        t.footTop = Colour (0xffff8cbf); t.footBottom = Colour (0xffff6faa); t.footBorder = juce::Colours::white;
        t.footText = juce::Colours::white; t.meterOn = Colour (0xffffe3ef); t.meterHot = juce::Colours::white; t.meterOff = Colour (0xffe0498c);
        t.barcode = Colour (0xd9ffffff);
        t.switchBody = juce::Colours::white; t.switchStroke = Colour (0xffff8cc6); t.switchInner = Colour (0xffffd0e4);
        t.switchBat = Colour (0xffff6faa); t.switchBatStroke = juce::Colours::white;
        t.bgData = BinaryData::bg_kawaii_jpg; t.bgSize = BinaryData::bg_kawaii_jpgSize;
        t.stickers = true;
        return t;
    }

    static Theme byIndex (int i) { return i == 1 ? matrix() : (i == 2 ? kawaii() : phosphor()); }
};

// ============================================================ fonts
struct Skin
{
    Theme theme = Theme::phosphor();
    juce::Typeface::Ptr plex, plexBold, dot, fredoka, fredokaBold, manrope;

    void loadFonts()
    {
        auto tf = [] (const char* d, int n) { return juce::Typeface::createSystemTypefaceFor (d, (size_t) n); };
        plex        = tf (BinaryData::PlexMono500_ttf, BinaryData::PlexMono500_ttfSize);
        plexBold    = tf (BinaryData::PlexMono700_ttf, BinaryData::PlexMono700_ttfSize);
        dot         = tf (BinaryData::DotGothic16_ttf, BinaryData::DotGothic16_ttfSize);
        fredoka     = tf (BinaryData::Fredoka600_ttf, BinaryData::Fredoka600_ttfSize);
        fredokaBold = tf (BinaryData::Fredoka700_ttf, BinaryData::Fredoka700_ttfSize);
        manrope     = tf (BinaryData::Manrope800_ttf, BinaryData::Manrope800_ttfSize);
    }

    static juce::Font make (juce::Typeface::Ptr t, float h, float kern = 0.0f)
    {
        auto o = t != nullptr ? juce::FontOptions (t) : juce::FontOptions();
        // point height = CSS px as in the mockups; kerning is given relative to that size (like CSS letter-spacing / font-size)
        juce::Font f (o.withPointHeight (h));
        const float ratio = f.getHeight() / std::max (1.0f, h);
        return f.withExtraKerningFactor (kern / std::max (0.5f, ratio));
    }

    // captions and button text: mono bold, or rounded in the kawaii theme
    juce::Font label (float h = 10.0f, float kern = 0.16f) const
    {
        return theme.roundedFont ? make (fredoka, h * 1.12f, kern * 0.8f) : make (plexBold, h, kern);
    }
    juce::Font mono (float h, bool bold = false, float kern = 0.1f) const { return make (bold ? plexBold : plex, h, kern); }
    juce::Font readout (float h = 12.0f) const { return make (dot, h); }
    juce::Font title() const { return theme.roundedFont ? make (fredokaBold, 38.0f, 0.03f) : make (manrope, 34.0f, 0.015f); }
};

// ============================================================ drawing helpers
// a line drawn with a soft phosphor glow (wide faint strokes under a bright core)
inline void glowStroke (juce::Graphics& g, const juce::Path& p, Colour c, float width, float strength = 1.0f)
{
    const juce::PathStrokeType::JointStyle j = juce::PathStrokeType::curved;
    const auto cap = juce::PathStrokeType::rounded;
    const float widths[] = { 9.0f, 5.0f, 2.6f };
    const float alphas[] = { 0.06f, 0.12f, 0.24f };
    for (int i = 0; i < 3; ++i)
    {
        g.setColour (c.withMultipliedAlpha (alphas[i] * strength));
        g.strokePath (p, juce::PathStrokeType (width * widths[i] * 0.55f + 2.0f, j, cap));
    }
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (width, j, cap));
}

inline void glowFill (juce::Graphics& g, const juce::Path& p, Colour c, float strength = 1.0f)
{
    const float widths[] = { 8.0f, 4.0f, 2.0f };
    const float alphas[] = { 0.06f, 0.12f, 0.24f };
    for (int i = 0; i < 3; ++i)
    {
        g.setColour (c.withMultipliedAlpha (alphas[i] * strength));
        g.strokePath (p, juce::PathStrokeType (widths[i], juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour (c);
    g.fillPath (p);
}

// glitched horned smiley: wobbly outline, blob eyes, crooked smile, two shifted slices, colour-split ghosts
inline void drawGlitchSmiley (juce::Graphics& g, juce::Point<float> c, float r, const Theme& t, float strokeW, float alpha, bool glow = true)
{
    using P = juce::Point<float>;
    auto at = [&] (float a, float k = 1.0f) { return P (c.x + r * k * std::sin (a), c.y - r * k * std::cos (a)); };
    std::vector<P> pts;
    auto wob = [&] (int i) { return 1.0f + 0.012f * std::sin ((float) i * 1.7f) + 0.008f * std::sin ((float) i * 4.1f + 1.0f); };
    auto arc = [&] (float a0, float a1, int n) { for (int i = 0; i <= n; ++i) { const float a = a0 + (a1 - a0) * (float) i / (float) n; pts.push_back (at (a, wob ((int) pts.size()))); } };
    auto quad = [] (float u, P p0, P cc, P p1) { const float v = 1.0f - u; return P (v*v*p0.x + 2*v*u*cc.x + u*u*p1.x, v*v*p0.y + 2*v*u*cc.y + u*u*p1.y); };
    auto horn = [&] (float side)
    {
        const P outer = at (side * 0.98f), inner = at (side * 0.56f), tip (c.x + side * r * 0.9f, c.y - r * 1.36f);
        const P co (c.x + side * r * 1.16f, c.y - r * 0.98f), ci (c.x + side * r * 0.66f, c.y - r * 1.0f);
        std::vector<P> seq;
        for (int i = 0; i <= 8; ++i) seq.push_back (quad ((float) i / 8.0f, outer, co, tip));
        for (int i = 1; i <= 8; ++i) seq.push_back (quad ((float) i / 8.0f, tip, ci, inner));
        if (side > 0) std::reverse (seq.begin(), seq.end());
        pts.insert (pts.end(), seq.begin(), seq.end());
    };
    horn (-1.0f); arc (-0.56f, 0.56f, 10); horn (1.0f); arc (0.98f, juce::MathConstants<float>::twoPi - 0.98f, 34);

    juce::Path outline;
    outline.startNewSubPath (pts[0]);
    for (size_t i = 1; i < pts.size(); ++i) outline.lineTo (pts[i]);
    outline.closeSubPath();

    auto eye = [&] (float ex, float ey, float flip)
    {
        const float w = r * 0.1f, h = r * 0.2f;
        juce::Path e;
        e.startNewSubPath (ex, ey - h);
        e.cubicTo (ex + w * 1.3f * flip, ey - h, ex + w * 0.9f * flip, ey - h * 0.1f, ex + w * 1.25f * flip, ey + h * 0.25f);
        e.cubicTo (ex + w * 0.8f * flip, ey + h * 1.05f, ex - w * 1.1f * flip, ey + h * 1.05f, ex - w * 1.05f * flip, ey + h * 0.1f);
        e.cubicTo (ex - w * 1.0f * flip, ey - h * 0.7f, ex - w * 0.5f * flip, ey - h, ex, ey - h);
        e.closeSubPath();
        return e;
    };
    juce::Path eyes = eye (c.x - r * 0.32f, c.y - r * 0.16f, -1.0f);
    eyes.addPath (eye (c.x + r * 0.32f, c.y - r * 0.16f, 1.0f));

    juce::Path smile;
    smile.startNewSubPath (c.x - r * 0.62f, c.y + r * 0.22f);
    smile.quadraticTo (c.x - r * 0.45f, c.y + r * 0.5f, c.x - r * 0.28f, c.y + r * 0.48f);
    smile.lineTo (c.x - r * 0.25f, c.y + r * 0.56f);
    smile.quadraticTo (c.x + r * 0.1f, c.y + r * 0.72f, c.x + r * 0.42f, c.y + r * 0.5f);
    smile.lineTo (c.x + r * 0.46f, c.y + r * 0.42f);
    smile.quadraticTo (c.x + r * 0.58f, c.y + r * 0.36f, c.x + r * 0.64f, c.y + r * 0.2f);

    const juce::PathStrokeType st (strokeW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    auto drawSet = [&] (juce::Graphics& gg, Colour col, float w, bool fillEyes, bool withGlow)
    {
        if (withGlow) { glowStroke (gg, outline, col, w); glowStroke (gg, smile, col, w * 1.1f); }
        else
        {
            gg.setColour (col);
            gg.strokePath (outline, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            gg.strokePath (smile, juce::PathStrokeType (w * 1.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        gg.setColour (col);
        if (fillEyes) { if (withGlow) glowFill (gg, eyes, col); else gg.fillPath (eyes); }
        else gg.strokePath (eyes, juce::PathStrokeType (w * 0.6f));
    };
    juce::ignoreUnused (st);

    // slices: [y, height, x shift]; everything else is drawn unshifted
    struct Band { float y, h, dx; };
    const Band bands[] = { { c.y - r * 0.5f, r * 0.09f, -r * 0.08f }, { c.y + r * 0.62f, r * 0.1f, r * 0.1f } };
    const float off = std::max (1.0f, r * 0.045f);

    auto drawAll = [&] (float dx)
    {
        juce::Graphics::ScopedSaveState s1 (g);
        g.addTransform (juce::AffineTransform::translation (dx, 0.0f));
        {
            juce::Graphics::ScopedSaveState s2 (g);
            g.addTransform (juce::AffineTransform::translation (-off, -off * 0.3f));
            drawSet (g, t.ghostA.withAlpha (t.ghostAlphaA * alpha), strokeW * 0.9f, false, false);
        }
        {
            juce::Graphics::ScopedSaveState s2 (g);
            g.addTransform (juce::AffineTransform::translation (off, off * 0.3f));
            drawSet (g, t.ghostB.withAlpha (t.ghostAlphaB * alpha), strokeW * 0.9f, false, false);
        }
        drawSet (g, t.phos.withAlpha (alpha), strokeW, true, glow);
    };

    const float top = c.y - r * 2.0f, bottom = c.y + r * 2.0f, left = c.x - r * 2.0f, wide = r * 4.0f;
    float y = top;
    for (const auto& b : bands)
    {
        { juce::Graphics::ScopedSaveState s (g); g.reduceClipRegion (juce::Rectangle<float> (left, y, wide, b.y - y).toNearestInt()); drawAll (0.0f); }
        { juce::Graphics::ScopedSaveState s (g); g.reduceClipRegion (juce::Rectangle<float> (left, b.y, wide, b.h).toNearestIntEdges()); drawAll (b.dx); }
        y = b.y + b.h;
    }
    { juce::Graphics::ScopedSaveState s (g); g.reduceClipRegion (juce::Rectangle<float> (left, y, wide, bottom - y).toNearestInt()); drawAll (0.0f); }
}

// physical push button ("key") with an optional indicator light
inline void drawKey (juce::Graphics& g, juce::Rectangle<float> r, bool on, bool down, const Theme& t)
{
    const float rad = t.keyRadius;
    if (down) r = r.translated (0.0f, 1.0f);
    // drop shadow + bottom edge
    g.setColour (t.keyShadow);
    g.fillRoundedRectangle (r.translated (0.0f, 3.0f).expanded (1.0f, 0.5f), rad + 1.0f);
    g.setColour (on ? t.keyOnEdge : t.keyEdge);
    g.fillRoundedRectangle (r.translated (0.0f, down ? 1.0f : 2.0f), rad);
    if (on && ! t.keyOnGlow.isTransparent())
    {
        g.setColour (t.keyOnGlow.withMultipliedAlpha (0.6f));
        g.fillRoundedRectangle (r.expanded (3.0f), rad + 3.0f);
    }
    juce::ColourGradient grad (on ? t.keyOnTop : t.keyTop, 0.0f, r.getY(), on ? t.keyOnBottom : t.keyBottom, 0.0f, r.getBottom(), false);
    grad.addColour (0.4, on ? t.keyOnMid : t.keyMid);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, rad);
    // top highlight + inner shading
    g.setColour (t.keyHighlight.withMultipliedAlpha (on ? 0.7f : 1.0f));
    g.drawLine (r.getX() + rad * 0.6f, r.getY() + 0.8f, r.getRight() - rad * 0.6f, r.getY() + 0.8f, 1.0f);
    g.setColour (t.keyInner);
    g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
}

inline void drawLed (juce::Graphics& g, juce::Rectangle<float> r, bool on, const Theme& t, Colour onColour = {})
{
    const auto lit = onColour.isTransparent() ? t.ledOn : onColour;
    const float rad = t.roundLed ? r.getWidth() * 0.5f : 2.0f;
    if (on)
    {
        g.setColour (lit.withAlpha (0.25f)); g.fillRoundedRectangle (r.expanded (4.0f), rad + 4.0f);
        g.setColour (lit.withAlpha (0.45f)); g.fillRoundedRectangle (r.expanded (1.8f), rad + 1.8f);
        g.setColour (lit); g.fillRoundedRectangle (r, rad);
    }
    else
    {
        g.setColour (t.ledOff); g.fillRoundedRectangle (r, rad);
        g.setColour (juce::Colours::black.withAlpha (0.25f)); g.drawLine (r.getX() + 1, r.getY() + 1, r.getRight() - 1, r.getY() + 1, 1.0f);
    }
}

// dark readout window with phosphor-style text
inline void drawReadout (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, const Skin& s, float textH = 12.0f,
                         juce::Justification j = juce::Justification::centred)
{
    const auto& t = s.theme;
    g.setColour (t.readoutBg);
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
    g.setFont (s.readout (textH));
    g.setColour (t.phos.withAlpha (0.25f));
    g.drawText (text, r.translated (0.0f, 0.5f).expanded (1.0f, 0.0f), j, false);
    g.setColour (t.phos);
    g.drawText (text, r, j, false);
}

// silkscreen-style section frame with a title tag (and an optional small right-hand tag)
inline void drawSection (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title, const juce::String& tag2, const Skin& s)
{
    const auto& t = s.theme;
    if (! t.sectionGlow.isTransparent())
    {
        g.setColour (t.sectionGlow);
        g.drawRoundedRectangle (r.expanded (2.0f), t.sectionRadius + 2.0f, 4.0f);
    }
    if (t.kind == Theme::Kind::kawaii)
    {
        g.setColour (Colour (0xffffd9e9));
        g.fillRoundedRectangle (r.translated (0.0f, 3.0f), t.sectionRadius);
    }
    g.setColour (t.sectionFill);
    g.fillRoundedRectangle (r, t.sectionRadius);
    g.setColour (t.sectionBorder);
    g.drawRoundedRectangle (r, t.sectionRadius, t.sectionBorderW);
    if (! t.sectionHighlight.isTransparent())
    {
        g.setColour (t.sectionHighlight);
        g.drawRoundedRectangle (r.reduced (3.0f), t.sectionRadius - 3.0f, 1.0f);
    }

    // title tag across the top edge
    const bool round = t.roundedFont;
    auto f = round ? Skin::make (s.fredoka, 13.0f, 0.1f) : s.label (10.0f, 0.22f);
    const float tw = juce::GlyphArrangement::getStringWidth (f, title) + (round ? 24.0f : 16.0f);
    const float th = round ? 22.0f : 18.0f;
    auto tag = juce::Rectangle<float> (r.getX() + 16.0f, r.getY() - th * 0.5f, tw, th);
    if (! t.tagShadow.isTransparent()) { g.setColour (t.tagShadow); g.fillRoundedRectangle (tag.translated (0.0f, 2.0f), t.tagRadius); }
    g.setColour (t.tagBg);
    g.fillRoundedRectangle (tag, t.tagRadius);
    if (! t.tagBorder.isTransparent()) { g.setColour (t.tagBorder); g.drawRoundedRectangle (tag.reduced (0.5f), t.tagRadius, 1.0f); }
    g.setColour (t.tagText);
    g.setFont (f);
    g.drawText (title, tag, juce::Justification::centred, false);

    if (tag2.isNotEmpty())
    {
        g.setColour (t.ink2);
        g.setFont (s.label (8.5f, 0.1f));
        g.drawText (tag2, juce::Rectangle<float> (r.getRight() - 214.0f, r.getY() + 4.0f, 200.0f, 14.0f), juce::Justification::centredRight, false);
    }
}

// a hardware bat-toggle switch (used for DRIVE / DELAY / REVERB on-off)
inline void drawToggleSwitch (juce::Graphics& g, juce::Point<float> c, bool on, const Theme& t)
{
    g.setColour (t.switchBody);
    g.fillEllipse (c.x - 12, c.y - 12, 24, 24);
    g.setColour (t.switchStroke);
    g.drawEllipse (c.x - 12, c.y - 12, 24, 24, t.kind == Theme::Kind::kawaii ? 2.0f : 1.4f);
    g.setColour (t.switchInner);
    g.fillEllipse (c.x - 7, c.y - 7, 14, 14);
    const auto bat = on ? juce::Rectangle<float> (c.x - 2.5f, c.y - 15.0f, 5.0f, 16.0f) : juce::Rectangle<float> (c.x - 2.5f, c.y - 1.0f, 5.0f, 16.0f);
    g.setColour (t.switchBat);
    g.fillRoundedRectangle (bat, 2.5f);
    g.setColour (t.switchBatStroke);
    g.drawRoundedRectangle (bat, 2.5f, 1.0f);
}

} // namespace squelch::ui
