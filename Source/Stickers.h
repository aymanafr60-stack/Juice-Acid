// Squelch — stickers for the kawaii theme ("the back of a laptop"). All original designs.
#pragma once

#include "Theme.h"

namespace squelch::ui
{

enum class Sticker { mascot, yellowSmiley, melting, heart, sparkle, acidPill, badge303, cherries, bolt, bow, banner };

namespace stickerDetail
{
    using juce::Colour; using juce::Path;

    inline Path smileFace (float r = 20.0f) { Path p; p.addEllipse (-r, -r, r * 2, r * 2); return p; }

    inline void face (juce::Graphics& g, Colour fill, Colour ink, float r = 20.0f)
    {
        g.setColour (fill); g.fillEllipse (-r, -r, r * 2, r * 2);
        g.setColour (ink);  g.drawEllipse (-r, -r, r * 2, r * 2, 2.2f);
        g.fillEllipse (-7 - 2.6f, -5 - 4.6f, 5.2f, 9.2f);
        g.fillEllipse (7 - 2.6f, -5 - 4.6f, 5.2f, 9.2f);
        Path m; m.startNewSubPath (-11, 5); m.quadraticTo (0, 17, 11, 5);
        g.strokePath (m, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    inline Path hornPath (float s)
    {
        Path p; p.startNewSubPath (s * 9, -17); p.quadraticTo (s * 15, -27, s * 17, -33); p.quadraticTo (s * 21, -24, s * 18, -12); p.closeSubPath();
        return p;
    }

    // silhouette (for the white die-cut border and the shadow) and the art itself
    inline Path silhouette (Sticker k, const Skin& s)
    {
        Path p;
        switch (k)
        {
            case Sticker::mascot:       p = smileFace(); p.addPath (hornPath (-1)); p.addPath (hornPath (1)); break;
            case Sticker::yellowSmiley: p = smileFace(); break;
            case Sticker::melting:
                p.startNewSubPath (-20, 0); p.addCentredArc (0, 0, 20, 20, 0, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi);
                p.lineTo (20, 8); p.quadraticTo (18, 18, 14, 10); p.quadraticTo (10, 26, 6, 12); p.quadraticTo (2, 18, -3, 13);
                p.quadraticTo (-8, 30, -12, 12); p.quadraticTo (-16, 17, -20, 8); p.closeSubPath(); break;
            case Sticker::heart:
                p.startNewSubPath (0, 18); p.cubicTo (-26, 2, -22, -20, -8, -18); p.cubicTo (-3, -17, -1, -13, 0, -10);
                p.cubicTo (1, -13, 3, -17, 8, -18); p.cubicTo (22, -20, 26, 2, 0, 18); p.closeSubPath(); break;
            case Sticker::sparkle:
                p.startNewSubPath (0, -22); p.quadraticTo (4, -4, 22, 0); p.quadraticTo (4, 4, 0, 22); p.quadraticTo (-4, 4, -22, 0);
                p.quadraticTo (-4, -4, 0, -22); p.closeSubPath(); break;
            case Sticker::acidPill:     p.addRoundedRectangle (-34.0f, -13.0f, 68.0f, 26.0f, 13.0f); break;
            case Sticker::badge303:     p.addEllipse (-21, -21, 42, 42); break;
            case Sticker::cherries:
                p.addEllipse (-17, 3, 16, 16); p.addEllipse (1, 3, 16, 16);
                { Path st; st.startNewSubPath (-8, 8); st.quadraticTo (-6, -10, 6, -18); st.startNewSubPath (8, 8); st.quadraticTo (6, -6, 6, -18);
                  juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded).createStrokedPath (st, st); p.addPath (st); }
                break;
            case Sticker::bolt:
                p.startNewSubPath (4, -22); p.lineTo (-12, 2); p.lineTo (-1, 2); p.lineTo (-5, 22); p.lineTo (12, -4); p.lineTo (1, -4); p.closeSubPath(); break;
            case Sticker::bow:
                p.startNewSubPath (0, 0); p.lineTo (-20, -12); p.quadraticTo (-24, 0, -20, 12); p.closeSubPath();
                p.startNewSubPath (0, 0); p.lineTo (20, -12); p.quadraticTo (24, 0, 20, 12); p.closeSubPath();
                p.addEllipse (-5, -5, 10, 10); break;
            case Sticker::banner:       p.addRoundedRectangle (-42.0f, -12.0f, 84.0f, 24.0f, 5.0f); break;
        }
        juce::ignoreUnused (s);
        return p;
    }

    inline void art (juce::Graphics& g, Sticker k, const Skin& s)
    {
        switch (k)
        {
            case Sticker::mascot:
                for (float sd : { -1.0f, 1.0f })
                {
                    auto h = hornPath (sd);
                    g.setColour (Colour (0xffff4f9a)); g.fillPath (h);
                    g.setColour (Colour (0xffb02f6e)); g.strokePath (h, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));
                }
                face (g, Colour (0xffff8cc6), Colour (0xff7a1846)); break;
            case Sticker::yellowSmiley: face (g, Colour (0xffffd60a), Colour (0xff1a1a1a)); break;
            case Sticker::melting:
            {
                auto p = silhouette (k, s);
                g.setColour (Colour (0xffc9a7ff)); g.fillPath (p);
                g.setColour (Colour (0xff5b2a9a)); g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));
                g.setColour (Colour (0xff3a1466));
                g.fillEllipse (-7 - 2.4f, -6 - 4, 4.8f, 8); g.fillEllipse (7 - 2.4f, -6 - 4, 4.8f, 8);
                Path m; m.startNewSubPath (-9, 3); m.quadraticTo (0, 11, 9, 3);
                g.strokePath (m, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                break;
            }
            case Sticker::heart:
            {
                auto p = silhouette (k, s);
                g.setColour (Colour (0xffff4f9a)); g.fillPath (p);
                g.setColour (Colour (0xffa51f5d)); g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));
                g.setColour (juce::Colours::white);
                g.fillEllipse (-6 - 2.2f, -4 - 2.2f, 4.4f, 4.4f); g.fillEllipse (6 - 2.2f, -4 - 2.2f, 4.4f, 4.4f);
                Path m; m.startNewSubPath (-5, 3); m.quadraticTo (0, 8, 5, 3);
                g.strokePath (m, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                break;
            }
            case Sticker::sparkle:
            {
                auto p = silhouette (k, s);
                g.setColour (Colour (0xff9ee7ff)); g.fillPath (p);
                g.setColour (Colour (0xff2f7ea8)); g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));
                break;
            }
            case Sticker::acidPill:
                g.setColour (Colour (0xff1a1a1a)); g.fillRoundedRectangle (-34.0f, -13.0f, 68.0f, 26.0f, 13.0f);
                g.setColour (Colour (0xffffd60a)); g.setFont (Skin::make (s.fredokaBold, 19.0f, 0.12f));
                g.drawText ("ACID", juce::Rectangle<float> (-34.0f, -13.0f, 68.0f, 26.0f), juce::Justification::centred, false);
                break;
            case Sticker::badge303:
                g.setColour (Colour (0xffffe3ef)); g.fillEllipse (-21, -21, 42, 42);
                g.setColour (Colour (0xffff4f9a)); g.drawEllipse (-21, -21, 42, 42, 3.0f);
                {
                    Path ring; ring.addEllipse (-15, -15, 30, 30);
                    Path dashed; const float dash[] = { 2.0f, 2.0f };
                    juce::PathStrokeType (1.2f).createDashedStroke (dashed, ring, dash, 2);
                    g.setColour (Colour (0xffff8cc6)); g.fillPath (dashed);
                }
                g.setColour (Colour (0xffb02f6e)); g.setFont (Skin::make (s.fredokaBold, 17.0f));
                g.drawText ("303", juce::Rectangle<float> (-21, -21, 42, 42), juce::Justification::centred, false);
                break;
            case Sticker::cherries:
            {
                Path st; st.startNewSubPath (-8, 8); st.quadraticTo (-6, -10, 6, -18); st.startNewSubPath (8, 8); st.quadraticTo (6, -6, 6, -18);
                g.setColour (Colour (0xff2e8b57)); g.strokePath (st, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                for (float cx : { -9.0f, 9.0f })
                {
                    g.setColour (Colour (0xffff3b6b)); g.fillEllipse (cx - 8, 3, 16, 16);
                    g.setColour (Colour (0xffa5143d)); g.drawEllipse (cx - 8, 3, 16, 16, 2.0f);
                    g.setColour (juce::Colours::white); g.fillEllipse (cx - 4, 6, 4, 4);
                }
                break;
            }
            case Sticker::bolt:
            {
                auto p = silhouette (k, s);
                g.setColour (Colour (0xffffd60a)); g.fillPath (p);
                g.setColour (Colour (0xff1a1a1a)); g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));
                break;
            }
            case Sticker::bow:
            {
                auto p = silhouette (k, s);
                g.setColour (Colour (0xffff6faa)); g.fillPath (p);
                g.setColour (Colour (0xffb02f6e)); g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));
                break;
            }
            case Sticker::banner:
                g.setColour (Colour (0xffffd60a)); g.fillRoundedRectangle (-42.0f, -12.0f, 84.0f, 24.0f, 5.0f);
                g.setColour (Colour (0xff1a1a1a)); g.setFont (s.readout (16.0f));
                g.drawText (juce::String::fromUTF8 ("\xe3\x82\xa2\xe3\x82\xb7\xe3\x83\x83\xe3\x83\x89!"), // アシッド!
                            juce::Rectangle<float> (-42.0f, -12.0f, 84.0f, 24.0f), juce::Justification::centred, false);
                break;
        }
    }
}

// one die-cut sticker: soft shadow, thick white border, the art, and a shine streak
inline void drawSticker (juce::Graphics& g, Sticker k, float x, float y, float rotDeg, float scale, const Skin& s)
{
    juce::Graphics::ScopedSaveState save (g);
    g.addTransform (juce::AffineTransform::scale (scale).rotated (juce::degreesToRadians (rotDeg)).translated (x, y));
    auto sil = stickerDetail::silhouette (k, s);
    juce::Path border;
    juce::PathStrokeType (9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded).createStrokedPath (border, sil);
    border.addPath (sil);
    border.setUsingNonZeroWinding (true);
    juce::DropShadow (juce::Colour (0x59b0306e), 6, { 0, 3 }).drawForPath (g, border);
    g.setColour (juce::Colours::white);
    g.fillPath (border);
    stickerDetail::art (g, k, s);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.drawLine (-14.0f, -16.0f, 6.0f, -20.0f, 3.0f);
}

} // namespace squelch::ui
