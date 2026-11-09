#!/usr/bin/env python3
"""Generates Source/Presets.h from the preset list below.
Each preset only lists what differs from DEFAULTS. Values are in knob units:
  wave 0 Saw 1 Square 2 Pulse 3 Super | drive_mode 0 OD 1 DIST 2 FUZZ | sweep 0 Slow 1 Normal 2 Fast
  dly_time 0 1/16 1 1/8T 2 1/8 3 3/16 4 1/4T 5 1/4 6 3/8 7 1/2 | tune semitones | slide ms | rest 0..1
Run: python3 tools/make_presets.py   (volume can be overridden by tools/preset_levels.json)
"""
import json, os, sys

ORDER = ["wave","pw","tune","cutoff","res","envmod","decay","accent","slide","sub",
         "drive_on","drive_mode","drive","tone","level","mix","volume",
         "soft_atk","acc_decay","tracking","filter_fm","sweep",
         "dly_on","dly_time","dly_fb","dly_tone","dly_mix",
         "rev_on","rev_size","rev_damp","rev_mix"]

DEFAULTS = dict(wave=0, pw=0.5, tune=0, cutoff=0.35, res=0.6, envmod=0.5, decay=0.4, accent=0.6, slide=60, sub=0,
                drive_on=0, drive_mode=1, drive=0.4, tone=0.7, level=0.6667, mix=1, volume=0.7,
                soft_atk=0.5, acc_decay=0.4628, tracking=0, filter_fm=0, sweep=1,
                dly_on=0, dly_time=3, dly_fb=0.45, dly_tone=0.5, dly_mix=0.3,
                rev_on=0, rev_size=0.5, rev_damp=0.5, rev_mix=0.25)

PRESETS = [
 # ---------------------------------------------------------------- starters
 ("INIT", {}),
 ("CHICAGO SQUELCH", dict(cutoff=0.25, res=0.82, envmod=0.6, decay=0.35, accent=0.7, tracking=0.2)),
 ("RUBBER BAND", dict(wave=1, cutoff=0.3, res=0.7, envmod=0.55, decay=0.3, accent=0.55, slide=90,
                      drive_on=1, drive_mode=0, drive=0.3, tone=0.6, mix=0.6, soft_atk=0.65, tracking=0.3,
                      dly_on=1, dly_time=2, dly_fb=0.3, dly_mix=0.2)),
 ("ACID SCREAM", dict(cutoff=0.38, res=0.95, envmod=0.7, decay=0.45, accent=0.8, slide=50,
                      drive_on=1, drive=0.6, tone=0.75, level=0.6, acc_decay=0.6, sweep=2,
                      dly_on=1, dly_fb=0.5, dly_tone=0.6, dly_mix=0.3, rev_on=1, rev_size=0.6, rev_mix=0.2)),
 ("FUZZ RAVE", dict(cutoff=0.45, res=0.75, decay=0.3, accent=0.8, slide=40, sub=0.3,
                    drive_on=1, drive_mode=2, drive=0.7, tone=0.55, level=0.6, mix=0.9,
                    acc_decay=0.35, filter_fm=0.25, sweep=2, rev_on=1, rev_size=0.3, rev_mix=0.15)),
 ("DEEP SUB ACID", dict(wave=1, cutoff=0.2, res=0.6, envmod=0.4, decay=0.5, accent=0.5, slide=80, sub=0.8,
                        drive_on=1, drive_mode=0, drive=0.2, tone=0.4, mix=0.5, soft_atk=0.7,
                        dly_on=1, dly_time=5, dly_fb=0.35, dly_tone=0.3, dly_mix=0.2, rev_on=1, rev_size=0.7, rev_mix=0.2)),
 ("TRANCE PLUCK", dict(wave=3, pw=0.4, cutoff=0.3, res=0.5, envmod=0.7, decay=0.15, accent=0.4, slide=30, sub=0.2,
                       drive_on=1, drive=0.2, tone=0.8, mix=0.4, soft_atk=0.3, tracking=0.5,
                       dly_on=1, dly_fb=0.55, dly_tone=0.65, dly_mix=0.35, rev_on=1, rev_size=0.8, rev_mix=0.3)),
 ("SUPER ACID", dict(wave=3, pw=0.8, cutoff=0.4, res=0.8, envmod=0.6, decay=0.5, accent=0.7, slide=120, sub=0.3,
                     drive_on=1, drive=0.5, tone=0.65, level=0.6, mix=0.8, tracking=0.3,
                     dly_on=1, dly_mix=0.25, rev_on=1, rev_size=0.7, rev_mix=0.25)),
 ("NARROW BITE", dict(wave=2, pw=0.15, cutoff=0.35, res=0.75, envmod=0.55, decay=0.3, accent=0.75,
                      drive_on=1, drive_mode=0, drive=0.4, mix=0.7, tracking=0.4, filter_fm=0.15,
                      dly_on=1, dly_time=0, dly_fb=0.3, dly_mix=0.2)),
 ("LONG SLIDE", dict(cutoff=0.3, res=0.78, envmod=0.45, decay=0.6, accent=0.5, slide=250, sub=0.2,
                     soft_atk=0.6, acc_decay=0.7, sweep=0,
                     dly_on=1, dly_time=6, dly_fb=0.5, dly_tone=0.4, dly_mix=0.3, rev_on=1, rev_size=0.75, rev_mix=0.25)),
 # ---------------------------------------------------------------- song-inspired
 ("ACCESS", dict(cutoff=0.3, res=0.88, envmod=0.65, decay=0.5, accent=0.75, slide=70,
                 drive_on=1, drive_mode=0, drive=0.25, tone=0.75, mix=0.5, acc_decay=0.55,
                 dly_on=1, dly_fb=0.55, dly_tone=0.6, dly_mix=0.3, rev_on=1, rev_size=0.75, rev_mix=0.25)),
 ("DON'T LAUGH", dict(cutoff=0.28, res=0.8, envmod=0.55, decay=0.55, accent=0.65, sub=0.1, tracking=0.2,
                      rev_on=1, rev_size=0.4, rev_mix=0.12)),
 ("LONDON ACID CITY", dict(cutoff=0.35, res=0.85, envmod=0.6, decay=0.3, accent=0.85, slide=45, sub=0.2,
                           drive_on=1, drive_mode=2, drive=0.65, tone=0.6, level=0.6,
                           acc_decay=0.4, filter_fm=0.2, sweep=2,
                           dly_on=1, dly_time=2, dly_fb=0.3, dly_tone=0.4, dly_mix=0.15)),
 ("TURKISH BAZAR", dict(wave=1, cutoff=0.33, res=0.86, envmod=0.6, decay=0.4, accent=0.7, slide=70,
                        drive_on=1, drive_mode=0, drive=0.35, mix=0.6, tracking=0.3,
                        dly_on=1, dly_fb=0.5, dly_mix=0.3)),
 ("ACPERIENCE", dict(wave=2, pw=0.278, cutoff=0.45, res=0.85, envmod=0.3, decay=0.32, accent=0.6,
                     drive_on=1, drive=0.43,
                     dly_on=1, dly_time=2, dly_fb=0.35, dly_mix=0.2, rev_on=1, rev_size=0.35, rev_mix=0.12)),
 ("999999999", dict(cutoff=0.4, res=0.92, envmod=0.7, decay=0.35, accent=0.9, slide=40, sub=0.25,
                    drive_on=1, drive_mode=2, drive=0.8, tone=0.5, level=0.55,
                    acc_decay=0.3, filter_fm=0.35, sweep=2,
                    dly_on=1, dly_time=0, dly_fb=0.25, dly_mix=0.12)),
 ("BLADE", dict(cutoff=0.38, res=0.82, envmod=0.6, decay=0.35, accent=0.8, slide=50, sub=0.4,
                drive_on=1, drive=0.65, tone=0.65, level=0.6, filter_fm=0.15, sweep=2,
                dly_on=1, dly_time=2, dly_fb=0.35, dly_mix=0.2, rev_on=1, rev_size=0.6, rev_mix=0.2)),
 # ---------------------------------------------------------------- new: built around the FX page
 ("DUB ECHO ACID", dict(wave=1, cutoff=0.25, res=0.75, envmod=0.5, decay=0.45, accent=0.6, slide=80, soft_atk=0.6,
                        dly_on=1, dly_time=6, dly_fb=0.7, dly_tone=0.35, dly_mix=0.45,
                        rev_on=1, rev_size=0.8, rev_damp=0.6, rev_mix=0.3)),
 ("GROWL FM", dict(cutoff=0.32, res=0.85, envmod=0.55, decay=0.35, accent=0.7, sub=0.3,
                   drive_on=1, drive=0.5, tone=0.6, level=0.6, tracking=0.3, filter_fm=0.5)),
 ("SOFT SWELL", dict(wave=3, pw=0.6, cutoff=0.28, res=0.6, envmod=0.4, decay=0.75, accent=0.4, slide=150, sub=0.3,
                     soft_atk=0.92, acc_decay=0.8, sweep=0,
                     dly_on=1, dly_time=5, dly_fb=0.5, dly_tone=0.45, dly_mix=0.3,
                     rev_on=1, rev_size=0.85, rev_damp=0.4, rev_mix=0.4)),
 ("WARPED PULSE", dict(wave=2, pw=0.1, cutoff=0.36, res=0.8, envmod=0.6, decay=0.3, accent=0.75,
                       drive_on=1, drive_mode=0, drive=0.4, filter_fm=0.3, sweep=2,
                       dly_on=1, dly_time=1, dly_fb=0.4, dly_mix=0.2)),
 ("WIDE TRANCE ACID", dict(wave=3, pw=0.7, cutoff=0.36, res=0.78, envmod=0.6, decay=0.4, accent=0.65, slide=70, sub=0.2,
                           drive_on=1, drive=0.3, mix=0.6, tracking=0.6,
                           dly_on=1, dly_fb=0.6, dly_tone=0.7, dly_mix=0.35, rev_on=1, rev_size=0.8, rev_mix=0.3)),
 ("BROKEN SPEAKER", dict(cutoff=0.4, res=0.8, envmod=0.6, decay=0.3, accent=0.8, slide=40,
                         drive_on=1, drive_mode=2, drive=0.9, tone=0.35, level=0.55, filter_fm=0.2, sweep=2)),
 ("HYPNO LOOP", dict(cutoff=0.3, res=0.85, envmod=0.3, decay=0.5, accent=0.7, acc_decay=0.9, sweep=0,
                     dly_on=1, dly_time=1, dly_fb=0.6, dly_tone=0.45, dly_mix=0.35, rev_on=1, rev_size=0.5, rev_mix=0.15)),
 ("DARK WAREHOUSE", dict(wave=1, cutoff=0.18, res=0.7, envmod=0.5, decay=0.45, accent=0.6, sub=0.5,
                         drive_on=1, drive_mode=0, drive=0.3, tone=0.3, mix=0.7,
                         rev_on=1, rev_size=0.9, rev_damp=0.8, rev_mix=0.25)),
 ("PIERCING 303", dict(cutoff=0.45, res=0.97, envmod=0.6, decay=0.35, accent=0.85, slide=50,
                       drive_on=1, drive=0.3, tone=0.8, tracking=0.5, acc_decay=0.25, sweep=2,
                       dly_on=1, dly_time=0, dly_fb=0.3, dly_mix=0.15)),
]

def main():
    here = os.path.dirname(os.path.abspath(__file__))
    levels_path = os.path.join(here, "preset_levels.json")
    levels = json.load(open(levels_path)) if os.path.exists(levels_path) else {}
    rows = []
    for name, vals in PRESETS:
        bad = set(vals) - set(DEFAULTS)
        if bad: sys.exit(f"{name}: unknown {bad}")
        v = dict(DEFAULTS); v.update(vals)
        if name in levels: v["volume"] = levels[name]
        def lit(x):
            t = f"{float(x):.4g}"
            return (t if ("." in t or "e" in t) else t + ".0") + "f"
        nums = ", ".join(lit(v[k]) for k in ORDER)
        rows.append(f'    {{ "{name}", {{ {nums} }} }},')
    if len(sys.argv) > 1 and sys.argv[1] == "--list":
        for name, vals in PRESETS:
            v = dict(DEFAULTS); v.update(vals); v["volume"] = levels.get(name, v["volume"])
            print(name + "|" + ",".join(str(float(v[k])) for k in ORDER))
        return
    out = f'''// Squelch — factory presets. GENERATED by tools/make_presets.py — edit that file, not this one.
// Values are in knob units, in the order of presetParamIds. Volume is level-matched by
// tools/preset_level_test.cpp so switching presets doesn't jump in loudness.

#pragma once

namespace squelch
{{

inline const char* const presetParamIds[] =
{{
    {", ".join('"' + k + '"' for k in ORDER)}
}};
inline constexpr int numPresetParams = {len(ORDER)};

struct Preset
{{
    const char* name;
    float values[numPresetParams];
}};

inline const Preset factoryPresets[] =
{{
{chr(10).join(rows)}
}};

inline constexpr int numFactoryPresets = (int) (sizeof (factoryPresets) / sizeof (factoryPresets[0]));

}} // namespace squelch
'''
    open(os.path.join(here, "..", "Source", "Presets.h"), "w").write(out)
    print(f"wrote {len(rows)} presets")

main()
