# RP2350 Professional Synthesizer v2.0

A professional-quality, 8-voice polyphonic MIDI synthesizer for the Raspberry Pi Pico 2 (RP2350).

## Features

### Sound Generation
- **8-voice polyphony** with intelligent voice stealing
- **3 oscillators per voice** with 6 waveforms each (Triangle, Saw, Pulse, Narrow Pulse, Wide Pulse, Sine)
- **PolyBLEP anti-aliasing** for clean, alias-free sound
- **Sub-oscillator** per voice (Jupiter-8 style) with -1 or -2 octave
- **Ring modulation** between oscillators for metallic tones
- **Unison mode** (1-4 voices) with detune spread for massive sounds
- **White and pink noise** generators

### Filter
- **Authentic Moog ladder filter** based on Huovilainen model
- **2x oversampling** for stability at high resonance
- Full **ADSR envelope** with adjustable modulation depth
- **Keyboard tracking** (-100% to +100%)
- **Drive/saturation** stage before filter for analog warmth
- **Velocity modulation** of cutoff

### Modulation
- **Global LFO** with 4 waveforms (Sine, Triangle, Saw, Square)
- Routes to **pitch, filter, or amplitude**
- **Mod wheel** adds to LFO depth
- **Pitch bend** (±2 semitones default)
- **Aftertouch** support

### Effects
- **Symphonic Chorus** - Roland Dimension D / Jupiter-8 style ensemble
  - 3-voice stereo chorus with quadrature LFOs
  - Rate, depth, feedback, mix controls
- **FreeVerb Reverb** - Schroeder algorithm
  - Room size, damping, width, mix, predelay
  - 8 parallel comb filters + 4 allpass filters

### Performance Features
- **Arpeggiator** with 50 patterns
  - Up, Down, Up/Down, Random, Pattern modes
  - 1-4 octave range
  - Swing timing
  - Latch mode
- **Portamento/Glide** (mono and legato modes)
- **Master tune** (±100 cents)
- **12 professional presets**

### Technical
- **48kHz / 16-bit** audio
- **I2S output** for high-quality DACs
- **Dual-core architecture**:
  - Core 0: MIDI, UI, arpeggiator
  - Core 1: Audio synthesis (uninterrupted)
- **Parameter smoothing** eliminates zipper noise
- **DC blocking** prevents offset buildup
- **Safety limiter** protects speakers/ears
- **Watchdog timer** for reliability
- **60+ MIDI CCs** for complete control

---

## Hardware Requirements

### Microcontroller
- Raspberry Pi Pico 2 (RP2350)
- 200MHz overclock recommended

### Audio Output
- Any I2S DAC (PCM5102, UDA1334, MAX98357, etc.)
- Connections:
  - BCK (Bit Clock): GPIO 20
  - WS (Word Select): GPIO 21
  - DATA: GPIO 22

### MIDI Input
- Standard 5-pin DIN or 3.5mm TRS MIDI
- Optocoupler circuit recommended
- Connections:
  - RX: GPIO 1

### Wiring Diagram

```
RP2350 Pico 2          I2S DAC
─────────────          ───────
GPIO 20 (BCK)  ───────► BCK/BCLK
GPIO 21 (WS)   ───────► LCK/LRCLK
GPIO 22 (DATA) ───────► DIN/DATA
3.3V           ───────► VIN (if 3.3V tolerant)
GND            ───────► GND

RP2350 Pico 2          MIDI IN (via optocoupler)
─────────────          ────────────────────────
GPIO 1 (RX)    ◄─────── Optocoupler output
```

---

## Installation

### Using PlatformIO (Recommended)

1. Install [PlatformIO](https://platformio.org/)

2. Clone or download this repository

3. Open the folder in VS Code with PlatformIO extension

4. Select build environment:
   - `pico2` - Standard build (150MHz)
   - `pico2_fast` - Overclocked build (200MHz) **recommended**
   - `pico2_debug` - Debug build with serial output

5. Build and upload:
   ```
   pio run -e pico2_fast -t upload
   ```

### Using Arduino IDE

1. Install [arduino-pico](https://github.com/earlephilhower/arduino-pico) board support

2. Install required libraries:
   - MIDI Library
   - arduino-audio-tools

3. Select board: "Raspberry Pi Pico 2"

4. Set CPU Speed to 200MHz (overclock)

5. Upload the sketch

---

## Quick Start

1. Connect your I2S DAC and MIDI input
2. Upload the firmware
3. Connect a MIDI keyboard or controller
4. Play notes!

### Default Settings
- Polyphonic mode (8 voices)
- Preset 0: "Classic Lead"
- Filter cutoff: 4kHz
- No effects

### Changing Presets
- Send **Program Change** 0-11 from your MIDI controller
- Or use MIDI CC controller with SysEx

---

## MIDI Control

See [MIDI_CC_MAP.md](MIDI_CC_MAP.md) for complete CC mapping.

### Essential CCs
| CC | Parameter |
|----|-----------|
| 1 | Mod Wheel |
| 7 | Master Volume |
| 70 | Filter Cutoff |
| 71 | Resonance |
| 78 | Drive |
| 105 | Chorus Mix |
| 109 | Reverb Mix |

---

## Presets

| # | Name | Description |
|---|------|-------------|
| 0 | Classic Lead | Bright, cutting mono lead |
| 1 | Fat Bass | Deep bass with sub-oscillator |
| 2 | Analog Pad | Warm evolving pad with chorus |
| 3 | Acid Bass | 303-style squelch |
| 4 | Supersaw | Massive detuned unison |
| 5 | Moog Brass | Classic brass stab |
| 6 | Wobble Bass | LFO-modulated dubstep bass |
| 7 | Plucked String | Acoustic-like pluck |
| 8 | Warm Strings | Lush string ensemble |
| 9 | Ring Mod Lead | Metallic bell tones |
| 10 | Resonant Sweep | Filter sweep showcase |
| 11 | Mono Lead | Portamento lead with vibrato |

---

## Performance Tips

### Getting the Best Sound
- Use **200MHz overclock** for maximum polyphony headroom
- Set **amp release** > 50ms to avoid clicks on note-off
- Use **sub-oscillator** for bass patches (adds weight without muddiness)
- **Chorus** adds width to pads and strings
- **Reverb predelay** of 20-30ms adds depth without washing out

### Avoiding Problems
- Keep **resonance** < 3.5 for stability at low cutoff
- Use **drive** sparingly (1.5-2.0) for warmth without harshness
- **DC blocker** is always active - no need to worry about offset
- If audio glitches occur, reduce voice count or effects

### CPU Usage
At 200MHz with 8 voices + chorus + reverb:
- Typical: 60-70%
- Maximum (all voices, high resonance): 85%

---

## Troubleshooting

### No Audio
1. Check I2S wiring (BCK, WS, DATA, GND)
2. Verify DAC power supply
3. Check MIDI connection with LED blinking on note

### Clicks/Pops
1. Increase buffer size in code (costs latency)
2. Reduce voice count to 6
3. Disable reverb when not needed

### Unstable Filter
1. Reduce resonance
2. Check that cutoff isn't extremely low with high resonance
3. The filter has self-oscillation protection but extreme settings may still cause issues

### MIDI Not Working
1. Check RX pin connection
2. Verify optocoupler circuit if using 5-pin DIN
3. Ensure MIDI channel is correct (default: OMNI)

---

## Files

```
synth_pro/
├── rp2350_synth_pro.ino    # Main sketch
├── synth_voice_pro.h       # Voice structures
├── synth_voice_pro.cpp     # Oscillators, filter, envelopes
├── synth_params_pro.h      # Parameter definitions
├── synth_params_pro.cpp    # Parameter handling, presets
├── synth_effects_pro.h     # Effects structures
├── synth_effects_pro.cpp   # Chorus, reverb
├── synth_hardware.h        # RP2350 optimizations
├── synth_hardware.cpp      # Hardware features
├── platformio.ini          # Build configuration
├── CLAUDE.md               # Project specification
├── MIDI_CC_MAP.md          # Complete CC reference
└── README.md               # This file
```

---

## Credits & References

- **Moog Filter**: Based on Antti Huovilainen's improved model
- **FreeVerb**: Jezar's public domain reverb algorithm
- **PolyBLEP**: Välimäki anti-aliasing technique
- **Pink Noise**: Voss-McCartney algorithm
- **arduino-audio-tools**: Phil Schatzmann's audio library

---

## License

MIT License - Feel free to use, modify, and distribute.

---

## Version History

### v2.0 (Current)
- 8-voice polyphony (up from 6)
- Added sub-oscillator
- Added ring modulation
- Added unison mode
- 2x oversampled filter
- Symphonic chorus
- FreeVerb reverb
- Parameter smoothing
- Safety features
- 60+ MIDI CCs

### v1.0
- Initial release
- 6-voice polyphony
- Basic chorus
- 40 parameters
