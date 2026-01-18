# RP2350 Professional Synthesizer - Project Specification

## Hardware Configuration

### MCU: RP2350 (Raspberry Pi Pico 2)
- **Cores**: Dual ARM Cortex-M33 @ 150MHz (overclockable to 200MHz+)
- **FPU**: Yes - hardware floating point available
- **SRAM**: 520KB
- **Flash**: 4MB (typical)
- **Hardware Features**: 
  - 2x Hardware Interpolators per core (critical for audio!)
  - PIO state machines
  - DMA with 12 channels

### Audio Output
- **Interface**: I2S
- **Sample Rate**: 48000 Hz
- **Bit Depth**: 16-bit
- **Channels**: 2 (stereo)
- **Pins**: BCK=20, WS=21, DATA=22

### MIDI Input
- **Interface**: UART (Serial1)
- **Pins**: RX=1, TX=0
- **Baud Rate**: 31250

## Real-Time Constraints

### Critical Timing Requirements
- Audio buffer size: 128 samples
- Buffer time: 2.67ms @ 48kHz
- **Maximum audio callback time**: 2.5ms (with margin)
- Per-sample budget: ~20.8µs
- Per-sample cycles @ 150MHz: ~3125 cycles
- Per-sample cycles @ 200MHz: ~4166 cycles

### Core Assignment
- **Core 0**: MIDI parsing, UI, parameter changes, arpeggiator
- **Core 1**: Audio synthesis (NEVER interrupted)

## Memory Budget

### Stack Sizes
- Core 0 stack: 4KB
- Core 1 stack: 8KB (larger for audio processing)

### Heap Usage
- Reverb buffers: ~32KB
- Chorus buffer: ~16KB
- Voice state: ~8KB
- Total estimated: ~60KB

### Critical Buffers
- DMA audio buffers: 2 x 512 bytes (ping-pong)
- Command queue: 64 entries x 32 bytes = 2KB

## Forbidden Patterns

### NEVER do these in audio callback (Core 1):
- `malloc()` or `free()` - all allocation at init
- `Serial.print()` - uses interrupts, can block
- Blocking waits (`delay()`, `delayMicroseconds()`)
- Division by non-constant (use multiplication by reciprocal)
- Unbounded loops
- Access to SD card or flash
- USB operations

### NEVER do these anywhere:
- Shared variable access without `volatile` or memory barriers
- Float-to-int conversion without bounds checking
- Array access without bounds checking in production
- Recursive function calls in audio path

## Optimization Guidelines

### Use Hardware Interpolators
The RP2350 has 2 hardware interpolators per core. Use them for:
- Linear interpolation (wavetables, delay lines)
- Fixed-point multiplication
- Clamping operations

### Fixed-Point vs Floating-Point
- RP2350 HAS hardware FPU - floating point is acceptable
- Use fixed-point for: interpolation indices, phase accumulators
- Use floating-point for: filter coefficients, envelope calculations
- SIMD not available on M33, but unrolling helps

### Memory Access Patterns
- Keep hot data in first 256KB of SRAM (faster access)
- Align buffers to 4-byte boundaries
- Use `__attribute__((aligned(4)))` for DMA buffers

## Code Organization

```
synth_pro/
├── rp2350_synth_pro.ino    # Main file, setup, MIDI handlers
├── synth_voice_pro.h       # Voice structures and fast math
├── synth_voice_pro.cpp     # Oscillators, envelopes, filter
├── synth_params_pro.h      # Parameter definitions
├── synth_params_pro.cpp    # Parameter handling, presets
├── synth_effects_pro.h     # Effects structures
├── synth_effects_pro.cpp   # Chorus, reverb, limiter
├── synth_hardware.h        # Hardware-specific optimizations
├── synth_hardware.cpp      # Interpolator usage, DMA
├── platformio.ini          # Build configuration
└── CLAUDE.md               # This file
```

## Testing Checklist

### Per-Feature Tests
- [ ] Single voice produces clean sine wave
- [ ] All waveforms alias-free up to 5kHz fundamental
- [ ] Filter stable at max resonance
- [ ] Envelope timing matches settings
- [ ] No clicks on note on/off
- [ ] Portamento smooth across octaves
- [ ] 8 voices simultaneous without dropout
- [ ] Chorus adds width without artifacts
- [ ] Reverb tails clean

### Performance Tests
- [ ] CPU usage < 80% with 8 voices + effects
- [ ] No audio dropouts over 1 hour
- [ ] All MIDI CCs respond < 10ms
- [ ] Pitch bend smooth

### Safety Tests
- [ ] Watchdog recovers from hang
- [ ] No crash on rapid note on/off
- [ ] No crash on extreme parameter values
- [ ] DC blocker prevents offset buildup

## MIDI CC Quick Reference

| Range | Function |
|-------|----------|
| 1 | Mod Wheel |
| 7 | Master Volume |
| 14-25 | Oscillators |
| 26-29 | Sub-Oscillator |
| 30-31 | Ring Mod |
| 70-79 | Filter |
| 80-84 | Amp Envelope |
| 85-89 | LFO |
| 90-92 | Unison |
| 102-105 | Chorus |
| 106-110 | Reverb |
| 111-112 | Noise |
| 113-115 | Global |
| 116-122 | Arpeggiator |
