/*
 * synth_params_pro.h - PROFESSIONAL EDITION v2.0
 * 
 * Extended parameter set:
 * - Sub-oscillator controls
 * - Unison/detune settings
 * - Ring modulation
 * - Symphonic chorus
 * - FreeVerb reverb
 * - Extended velocity routing
 * - 55+ parameters with full MIDI CC mapping
 */

#ifndef SYNTH_PARAMS_PRO_H
#define SYNTH_PARAMS_PRO_H

#include <Arduino.h>

// ===== PARAMETER INDICES =====
// Oscillator section (0-19)
#define PARAM_OSC1_RANGE      0
#define PARAM_OSC2_RANGE      1
#define PARAM_OSC3_RANGE      2
#define PARAM_OSC1_FINE       3
#define PARAM_OSC2_FINE       4
#define PARAM_OSC3_FINE       5
#define PARAM_OSC1_WAVE       6
#define PARAM_OSC2_WAVE       7
#define PARAM_OSC3_WAVE       8
#define PARAM_OSC1_VOL        9
#define PARAM_OSC2_VOL        10
#define PARAM_OSC3_VOL        11
#define PARAM_NOISE_VOL       12
#define PARAM_NOISE_TYPE      13

// Sub-oscillator (14-17)
#define PARAM_SUB_ENABLED     14
#define PARAM_SUB_WAVE        15
#define PARAM_SUB_OCTAVE      16
#define PARAM_SUB_VOL         17

// Ring mod (18-19)
#define PARAM_RING_ENABLED    18
#define PARAM_RING_AMOUNT     19

// Filter section (20-30)
#define PARAM_CUTOFF          20
#define PARAM_RESONANCE       21
#define PARAM_FILT_ATTACK     22
#define PARAM_FILT_DECAY      23
#define PARAM_FILT_SUSTAIN    24
#define PARAM_FILT_RELEASE    25
#define PARAM_FILT_STRENGTH   26
#define PARAM_FILT_KEY_TRACK  27
#define PARAM_DRIVE           28
#define PARAM_VEL_TO_FILT     29
#define PARAM_VEL_TO_AMP      30

// Amp envelope (31-34)
#define PARAM_AMP_ATTACK      31
#define PARAM_AMP_DECAY       32
#define PARAM_AMP_SUSTAIN     33
#define PARAM_AMP_RELEASE     34

// LFO (35-39)
#define PARAM_LFO_RATE        35
#define PARAM_LFO_DEPTH       36
#define PARAM_LFO_WAVE        37
#define PARAM_LFO_TARGET      38
#define PARAM_LFO_ENABLED     39

// Unison (40-42)
#define PARAM_UNISON_COUNT    40
#define PARAM_UNISON_DETUNE   41
#define PARAM_UNISON_SPREAD   42

// Global (43-46)
#define PARAM_PLAY_MODE       43
#define PARAM_GLIDE_TIME      44
#define PARAM_MASTER_TUNE     45
#define PARAM_MASTER_VOL      46

// Chorus (47-50)
#define PARAM_CHORUS_RATE     47
#define PARAM_CHORUS_DEPTH    48
#define PARAM_CHORUS_FEEDBACK 49
#define PARAM_CHORUS_MIX      50

// Reverb (51-55)
#define PARAM_REVERB_SIZE     51
#define PARAM_REVERB_DAMP     52
#define PARAM_REVERB_WIDTH    53
#define PARAM_REVERB_MIX      54
#define PARAM_REVERB_PREDELAY 55

#define NUM_PARAMETERS 56

// ===== PARAMETER STORAGE =====
extern float allParameterValues[NUM_PARAMETERS];

// Oscillator parameters
extern float osc1Range, osc2Range, osc3Range;
extern float osc1Fine, osc2Fine, osc3Fine;
extern int osc1Wave, osc2Wave, osc3Wave;
extern float vol1, vol2, vol3, noiseVol;
extern int noiseType;

// Sub-oscillator
extern bool subEnabled;
extern int subWave;
extern int subOctave;
extern float subVol;

// Ring mod
extern bool ringEnabled;
extern float ringAmount;

// Envelope parameters
extern float ampAttack, ampDecay, ampSustain, ampRelease;
extern float filtAttack, filtDecay, filtSustain, filtRelease;

// Filter parameters
extern float cutoff, resonance;
extern float filterStrength;
extern float filterKeyTrack;
extern float drive;
extern float velToFilter, velToAmp;

// LFO parameters
extern float lfoRate, lfoDepth;
extern int lfoWave;
extern int lfoTarget;
extern bool lfoEnabled;

// Unison
extern int unisonCount;
extern float unisonDetune;
extern float unisonSpread;

// Global parameters
extern int playMode;
extern float glideTime;
extern float masterTune;
extern float masterVol;

// Chorus
extern float chorusRate, chorusDepth, chorusFeedback, chorusMix;

// Reverb
extern float reverbSize, reverbDamp, reverbWidth, reverbMix, reverbPredelay;

// ===== PRESET STRUCTURE =====
struct Preset {
  const char* name;
  float parameters[NUM_PARAMETERS];
};

extern const Preset presets[];
extern const int NUM_PRESETS;

// ===== FUNCTIONS =====
void setParameter(int paramIndex, float value);
void loadPreset(int presetIndex);
void initParameters();

// ===== CONVERSION FUNCTIONS =====

inline float rangeFromNormalized(float n) {
  // 0.5x, 1x, 2x, 4x, 8x, 16x
  int index = (int)(n * 5.99f);
  const float ranges[] = {0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f};
  return ranges[constrain(index, 0, 5)];
}

inline float fineFromNormalized(float n) {
  // ±100 cents
  return powf(2.0f, (n - 0.5f) * (100.0f / 1200.0f));
}

inline int waveformFromNormalized(float n) {
  return constrain((int)(n * 5.99f), 0, 5);
}

// Musical cutoff: 20Hz - 20kHz exponential
inline float cutoffFromNormalized(float n) {
  return 20.0f * powf(2.0f, n * 9.9658f);
}

// Resonance with quadratic curve
inline float resonanceFromNormalized(float n) {
  return n * n * 3.9f;
}

// Attack: 0.5ms to 5s
inline float attackFromNormalized(float n) {
  if (n < 0.01f) return 0.0005f;
  return 0.0005f * powf(10000.0f, n);
}

// Decay: 5ms to 10s
inline float decayFromNormalized(float n) {
  if (n < 0.01f) return 0.005f;
  return 0.005f * powf(2000.0f, n);
}

// Release: 5ms to 10s
inline float releaseFromNormalized(float n) {
  if (n < 0.01f) return 0.005f;
  return 0.005f * powf(2000.0f, n);
}

// LFO rate: 0.05Hz to 30Hz
inline float lfoRateFromNormalized(float n) {
  return 0.05f * powf(600.0f, n);
}

// Glide: 0 to 2s
inline float glideTimeFromNormalized(float n) {
  if (n < 0.02f) return 0.0f;
  return 0.005f * powf(400.0f, n);
}

// Drive: 1.0 to 8.0 (quadratic)
inline float driveFromNormalized(float n) {
  return 1.0f + n * n * 7.0f;
}

// Key track: -100% to +100%
inline float keyTrackFromNormalized(float n) {
  return (n - 0.5f) * 2.0f;
}

// Chorus rate: 0.1Hz to 5Hz
inline float chorusRateFromNormalized(float n) {
  return 0.1f + n * n * 4.9f;
}

// Unison count: 1 to 4
inline int unisonFromNormalized(float n) {
  return 1 + (int)(n * 3.99f);
}

// Unison detune: 0 to 50 cents
inline float unisonDetuneFromNormalized(float n) {
  return n * 50.0f;
}

// Master tune: -100 to +100 cents
inline float masterTuneFromNormalized(float n) {
  return (n - 0.5f) * 200.0f;
}

// Reverb size: 0.1 to 0.99
inline float reverbSizeFromNormalized(float n) {
  return 0.1f + n * 0.89f;
}

// Reverb predelay: 0 to 100ms
inline float reverbPredelayFromNormalized(float n) {
  return n * 100.0f;
}

#endif // SYNTH_PARAMS_PRO_H
