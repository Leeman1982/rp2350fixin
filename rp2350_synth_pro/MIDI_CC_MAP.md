# MIDI CC Mapping - RP2350 Professional Synthesizer v2.0

## Overview

This synthesizer responds to 60+ MIDI Continuous Controllers for real-time parameter control.
All parameters can be automated from a DAW or controlled via hardware MIDI controllers.

---

## Standard MIDI CCs

| CC# | Parameter | Range | Notes |
|-----|-----------|-------|-------|
| 1 | Mod Wheel | 0-127 | Adds to LFO depth for vibrato/filter mod |
| 7 | Master Volume | 0-127 | Overall output level |
| 10 | Pan | 0-127 | Not implemented (stereo effects handle this) |
| 11 | Expression | 0-127 | Maps to filter cutoff |
| 64 | Sustain Pedal | 0-63=off, 64-127=on | Hold notes |
| 65 | Portamento | 0-63=off, 64-127=on | Enable/disable glide |

---

## Oscillator Section (CC 14-25)

| CC# | Parameter | Range | Description |
|-----|-----------|-------|-------------|
| 14 | OSC1 Range | 0-127 | Footage: 16', 8', 4', 2', 1', 1/2' |
| 15 | OSC2 Range | 0-127 | Footage: 16', 8', 4', 2', 1', 1/2' |
| 16 | OSC3 Range | 0-127 | Footage: 16', 8', 4', 2', 1', 1/2' |
| 17 | OSC1 Fine | 0-127 | ±100 cents (64 = center) |
| 18 | OSC2 Fine | 0-127 | ±100 cents (64 = center) |
| 19 | OSC3 Fine | 0-127 | ±100 cents (64 = center) |
| 20 | OSC1 Wave | 0-127 | Triangle/Saw/Pulse/Narrow/Wide/Sine |
| 21 | OSC2 Wave | 0-127 | Triangle/Saw/Pulse/Narrow/Wide/Sine |
| 22 | OSC3 Wave | 0-127 | Triangle/Saw/Pulse/Narrow/Wide/Sine |
| 23 | OSC1 Volume | 0-127 | Mix level for oscillator 1 |
| 24 | OSC2 Volume | 0-127 | Mix level for oscillator 2 |
| 25 | OSC3 Volume | 0-127 | Mix level for oscillator 3 |

### Waveform Values:
- 0-21: Triangle
- 22-42: Sawtooth
- 43-63: Pulse (50%)
- 64-84: Narrow Pulse (25%)
- 85-105: Wide Pulse (75%)
- 106-127: Sine

---

## Sub-Oscillator Section (CC 26-29)

| CC# | Parameter | Range | Description |
|-----|-----------|-------|-------------|
| 26 | Sub Enable | 0-63=off, 64-127=on | Enable sub-oscillator |
| 27 | Sub Wave | 0-127 | Square/Sine/Pulse |
| 28 | Sub Octave | 0-63=-1oct, 64-127=-2oct | Octave below main |
| 29 | Sub Volume | 0-127 | Sub-oscillator level |

---

## Ring Modulator Section (CC 30-31)

| CC# | Parameter | Range | Description |
|-----|-----------|-------|-------------|
| 30 | Ring Enable | 0-63=off, 64-127=on | Enable ring modulation |
| 31 | Ring Amount | 0-127 | Blend with original signal |

---

## Filter Section (CC 70-79)

| CC# | Parameter | Range | Description |
|-----|-----------|-------|-------------|
| 70 | Cutoff | 0-127 | 20Hz - 20kHz (exponential) |
| 71 | Resonance | 0-127 | 0-3.9 (self-oscillates near max) |
| 72 | Filter Attack | 0-127 | 0.5ms - 5s |
| 73 | Filter Decay | 0-127 | 5ms - 10s |
| 74 | Filter Sustain | 0-127 | 0-100% |
| 75 | Filter Release | 0-127 | 5ms - 10s |
| 76 | Filter Env Strength | 0-127 | How much envelope affects cutoff |
| 77 | Filter Key Track | 0-127 | -100% to +100% (64 = 0%) |
| 78 | Drive | 0-127 | 1x to 8x pre-filter saturation |
| 79 | Velocity→Filter | 0-127 | Velocity modulation depth |

---

## Amp Envelope Section (CC 80-84)

| CC# | Parameter | Range | Description |
|-----|-----------|-------|-------------|
| 80 | Amp Attack | 0-127 | 0.5ms - 5s |
| 81 | Amp Decay | 0-127 | 5ms - 10s |
| 82 | Amp Sustain | 0-127 | 0-100% |
| 83 | Amp Release | 0-127 | 5ms - 10s |
| 84 | Velocity→Amp | 0-127 | Velocity sensitivity |

---

## LFO Section (CC 85-89)

| CC# | Parameter | Range | Description |
|-----|-----------|-------|-------------|
| 85 | LFO Rate | 0-127 | 0.05Hz - 30Hz |
| 86 | LFO Depth | 0-127 | Modulation intensity |
| 87 | LFO Wave | 0-127 | Sine/Triangle/Saw/Square |
| 88 | LFO Target | 0-127 | 0-42=Pitch, 43-84=Filter, 85-127=Amp |
| 89 | LFO Enable | 0-63=off, 64-127=on | Master LFO on/off |

### LFO Waveform Values:
- 0-31: Sine
- 32-63: Triangle
- 64-95: Sawtooth
- 96-127: Square

---

## Unison Section (CC 90-92)

| CC# | Parameter | Range | Description |
|-----|-----------|-------|-------------|
| 90 | Unison Count | 0-127 | 1-4 unison voices |
| 91 | Unison Detune | 0-127 | 0-50 cents spread |
| 92 | Unison Spread | 0-127 | Stereo width |

---

## Chorus Section (CC 102-105)

Symphonic chorus inspired by Roland Dimension D / Jupiter-8 ensemble.

| CC# | Parameter | Range | Description |
|-----|-----------|-------|-------------|
| 102 | Chorus Rate | 0-127 | LFO speed (0.1-5Hz) |
| 103 | Chorus Depth | 0-127 | Modulation depth |
| 104 | Chorus Feedback | 0-127 | Regeneration (max 70%) |
| 105 | Chorus Mix | 0-127 | Dry/Wet blend |

---

## Reverb Section (CC 106-110)

FreeVerb-style algorithmic reverb.

| CC# | Parameter | Range | Description |
|-----|-----------|-------|-------------|
| 106 | Reverb Size | 0-127 | Room size (small→large) |
| 107 | Reverb Damp | 0-127 | High frequency damping |
| 108 | Reverb Width | 0-127 | Stereo spread |
| 109 | Reverb Mix | 0-127 | Dry/Wet blend |
| 110 | Reverb Predelay | 0-127 | 0-100ms initial delay |

---

## Noise Section (CC 111-112)

| CC# | Parameter | Range | Description |
|-----|-----------|-------|-------------|
| 111 | Noise Volume | 0-127 | Noise generator level |
| 112 | Noise Type | 0-63=White, 64-127=Pink | Noise color |

---

## Global Section (CC 113-115)

| CC# | Parameter | Range | Description |
|-----|-----------|-------|-------------|
| 113 | Play Mode | 0-127 | 0-42=Poly, 43-84=Mono, 85-127=Legato |
| 114 | Glide Time | 0-127 | Portamento time (0=off, max=2s) |
| 115 | Master Tune | 0-127 | ±100 cents (64 = A440) |

---

## Arpeggiator Section (CC 116-122)

| CC# | Parameter | Range | Description |
|-----|-----------|-------|-------------|
| 116 | Arp Enable | 0-63=off, 64-127=on | Arpeggiator on/off |
| 117 | Arp Pattern | 0-127 | 50 patterns available |
| 118 | Arp Rate | 0-127 | 40-300 BPM |
| 119 | Arp Gate | 0-127 | Note length (10-100%) |
| 121 | Arp Swing | 0-127 | Swing amount (0-60%) |
| 122 | Arp Octaves | 0-127 | 1-4 octave range |

### Arp Direction (via CC 117 pattern selection):
- Patterns 0-9: Up patterns
- Patterns 10-19: Down patterns
- Patterns 20-29: Up/Down patterns
- Patterns 30-39: Random patterns
- Patterns 40-49: Chord patterns

---

## System Messages

| CC# | Function | Description |
|-----|----------|-------------|
| 120 | All Sound Off | Immediately silence all audio |
| 123 | All Notes Off | Release all notes |

---

## Program Change

Program Change messages 0-11 load the built-in presets:

| PC# | Preset Name |
|-----|-------------|
| 0 | Classic Lead |
| 1 | Fat Bass |
| 2 | Analog Pad |
| 3 | Acid Bass |
| 4 | Supersaw |
| 5 | Moog Brass |
| 6 | Wobble Bass |
| 7 | Plucked String |
| 8 | Warm Strings |
| 9 | Ring Mod Lead |
| 10 | Resonant Sweep |
| 11 | Mono Lead |

---

## Tips for MIDI Controllers

### Recommended CC Assignments for 8-knob Controller:
1. CC 70 - Filter Cutoff
2. CC 71 - Resonance
3. CC 76 - Filter Env Amount
4. CC 78 - Drive
5. CC 80 - Amp Attack
6. CC 83 - Amp Release
7. CC 85 - LFO Rate
8. CC 86 - LFO Depth

### For Performance:
- Use Mod Wheel (CC 1) for expressive vibrato/filter sweeps
- Use Expression (CC 11) as secondary filter control
- Map a button to CC 116 for arp toggle
- Map a button to CC 89 for LFO toggle

### Parameter Smoothing:
All parameters use 10ms smoothing to prevent zipper noise.
Cutoff and volume have dedicated smoothers for extra-clean modulation.
