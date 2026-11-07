// Squelch — starter presets.
// These are placeholders so the preset bar works. They'll be retuned once the
// core sound is dialled in, because engine changes shift what every setting sounds like.
//
// Values are in real units (what the knob shows), not 0..1:
//   wave: 0 Saw, 1 Square, 2 Pulse, 3 Super     drive_mode: 0 OD, 1 DIST, 2 FUZZ
//   drive_on: 0/1   tune: semitones   slide: ms   everything else 0..1

#pragma once

namespace squelch
{

// Order of the values in every preset below.
inline const char* const presetParamIds[] =
{
    "wave", "pw", "tune", "cutoff", "res", "envmod", "decay", "accent", "slide", "sub",
    "drive_on", "drive_mode", "drive", "tone", "level", "mix", "volume"
};
inline constexpr int numPresetParams = (int) (sizeof (presetParamIds) / sizeof (presetParamIds[0]));

struct Preset
{
    const char* name;
    float values[numPresetParams];
};

inline const Preset factoryPresets[] =
{
    //                            wave  pw  tune cut  res  env  dec  acc  slide sub  on mode drv tone lvl  mix  vol
    { "INIT",            { 0, 0.5f, 0, 0.35f, 0.6f, 0.5f, 0.4f, 0.6f, 60, 0, 0, 1, 0.4f, 0.7f, 0.6667f, 1, 0.7f } },
    { "CHICAGO SQUELCH", { 0, 0.5f, 0, 0.25f, 0.82f, 0.6f, 0.35f, 0.7f, 60, 0, 0, 1, 0.4f, 0.7f, 0.6667f, 1, 0.7f } },
    { "RUBBER BAND",     { 1, 0.5f, 0, 0.3f, 0.7f, 0.55f, 0.3f, 0.55f, 90, 0, 1, 0, 0.3f, 0.6f, 0.6667f, 0.6f, 0.7f } },
    { "ACID SCREAM",     { 0, 0.5f, 0, 0.38f, 0.95f, 0.7f, 0.45f, 0.8f, 50, 0, 1, 1, 0.6f, 0.75f, 0.6f, 1, 0.7f } },
    { "FUZZ RAVE",       { 0, 0.5f, 0, 0.45f, 0.75f, 0.5f, 0.3f, 0.8f, 40, 0.3f, 1, 2, 0.7f, 0.55f, 0.6f, 0.9f, 0.7f } },
    { "DEEP SUB ACID",   { 1, 0.5f, 0, 0.2f, 0.6f, 0.4f, 0.5f, 0.5f, 80, 0.8f, 1, 0, 0.2f, 0.4f, 0.6667f, 0.5f, 0.7f } },
    { "TRANCE PLUCK",    { 3, 0.4f, 0, 0.3f, 0.5f, 0.7f, 0.15f, 0.4f, 30, 0.2f, 1, 1, 0.2f, 0.8f, 0.6667f, 0.4f, 0.7f } },
    { "SUPER ACID",      { 3, 0.8f, 0, 0.4f, 0.8f, 0.6f, 0.5f, 0.7f, 120, 0.3f, 1, 1, 0.5f, 0.65f, 0.6f, 0.8f, 0.7f } },
    { "NARROW BITE",     { 2, 0.15f, 0, 0.35f, 0.75f, 0.55f, 0.3f, 0.75f, 60, 0, 1, 0, 0.4f, 0.7f, 0.6667f, 0.7f, 0.7f } },
    { "LONG SLIDE",      { 0, 0.5f, 0, 0.3f, 0.78f, 0.45f, 0.6f, 0.5f, 250, 0.2f, 0, 1, 0.4f, 0.7f, 0.6667f, 1, 0.7f } },
};

inline constexpr int numFactoryPresets = (int) (sizeof (factoryPresets) / sizeof (factoryPresets[0]));

} // namespace squelch
