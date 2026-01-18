/*
 * synth_params_pro.cpp - PROFESSIONAL EDITION v2.0
 * 
 * Parameter management with 12 professional presets
 */

#include "synth_params_pro.h"

float allParameterValues[NUM_PARAMETERS];

// ===== OSCILLATOR PARAMETERS =====
float osc1Range = 1.0f, osc2Range = 1.0f, osc3Range = 1.0f;
float osc1Fine = 1.0f, osc2Fine = 1.0f, osc3Fine = 1.0f;
int osc1Wave = 1, osc2Wave = 1, osc3Wave = 1;  // Sawtooth
float vol1 = 0.7f, vol2 = 0.7f, vol3 = 0.0f, noiseVol = 0.0f;
int noiseType = 0;

// ===== SUB-OSCILLATOR =====
bool subEnabled = false;
int subWave = 0;
int subOctave = -1;
float subVol = 0.0f;

// ===== RING MOD =====
bool ringEnabled = false;
float ringAmount = 0.0f;

// ===== ENVELOPE PARAMETERS =====
float ampAttack = 0.001f, ampDecay = 0.1f, ampSustain = 1.0f, ampRelease = 0.15f;
float filtAttack = 0.001f, filtDecay = 0.2f, filtSustain = 0.5f, filtRelease = 0.25f;

// ===== FILTER PARAMETERS =====
float cutoff = 4000.0f, resonance = 0.0f;
float filterStrength = 1.0f;
float filterKeyTrack = 0.0f;
float drive = 1.0f;
float velToFilter = 0.3f, velToAmp = 0.5f;

// ===== LFO PARAMETERS =====
float lfoRate = 5.0f, lfoDepth = 0.0f;
int lfoWave = 0;
int lfoTarget = 1;
bool lfoEnabled = false;

// ===== UNISON =====
int unisonCount = 1;
float unisonDetune = 0.0f;
float unisonSpread = 0.5f;

// ===== GLOBAL PARAMETERS =====
int playMode = 1;
float glideTime = 0.0f;
float masterTune = 0.0f;
float masterVol = 0.8f;

// ===== CHORUS =====
float chorusRate = 0.5f, chorusDepth = 0.3f, chorusFeedback = 0.2f, chorusMix = 0.0f;

// ===== REVERB =====
float reverbSize = 0.5f, reverbDamp = 0.5f, reverbWidth = 1.0f;
float reverbMix = 0.0f, reverbPredelay = 20.0f;

void initParameters() {
  // Initialize all to defaults
  for (int i = 0; i < NUM_PARAMETERS; i++) {
    allParameterValues[i] = 0.5f;
  }
}

void setParameter(int paramIndex, float value) {
  if (paramIndex < 0 || paramIndex >= NUM_PARAMETERS) return;
  
  value = constrain(value, 0.0f, 1.0f);
  allParameterValues[paramIndex] = value;
  
  switch (paramIndex) {
    // Oscillator
    case PARAM_OSC1_RANGE:  osc1Range = rangeFromNormalized(value); break;
    case PARAM_OSC2_RANGE:  osc2Range = rangeFromNormalized(value); break;
    case PARAM_OSC3_RANGE:  osc3Range = rangeFromNormalized(value); break;
    case PARAM_OSC1_FINE:   osc1Fine = fineFromNormalized(value); break;
    case PARAM_OSC2_FINE:   osc2Fine = fineFromNormalized(value); break;
    case PARAM_OSC3_FINE:   osc3Fine = fineFromNormalized(value); break;
    case PARAM_OSC1_WAVE:   osc1Wave = waveformFromNormalized(value); break;
    case PARAM_OSC2_WAVE:   osc2Wave = waveformFromNormalized(value); break;
    case PARAM_OSC3_WAVE:   osc3Wave = waveformFromNormalized(value); break;
    case PARAM_OSC1_VOL:    vol1 = value; break;
    case PARAM_OSC2_VOL:    vol2 = value; break;
    case PARAM_OSC3_VOL:    vol3 = value; break;
    case PARAM_NOISE_VOL:   noiseVol = value; break;
    case PARAM_NOISE_TYPE:  noiseType = (value > 0.5f) ? 1 : 0; break;
    
    // Sub-oscillator
    case PARAM_SUB_ENABLED: subEnabled = (value > 0.5f); break;
    case PARAM_SUB_WAVE:    subWave = constrain((int)(value * 2.99f), 0, 2); break;
    case PARAM_SUB_OCTAVE:  subOctave = (value > 0.5f) ? -2 : -1; break;
    case PARAM_SUB_VOL:     subVol = value; break;
    
    // Ring mod
    case PARAM_RING_ENABLED: ringEnabled = (value > 0.5f); break;
    case PARAM_RING_AMOUNT:  ringAmount = value; break;
    
    // Filter
    case PARAM_CUTOFF:      cutoff = cutoffFromNormalized(value); break;
    case PARAM_RESONANCE:   resonance = resonanceFromNormalized(value); break;
    case PARAM_FILT_ATTACK: filtAttack = attackFromNormalized(value); break;
    case PARAM_FILT_DECAY:  filtDecay = decayFromNormalized(value); break;
    case PARAM_FILT_SUSTAIN: filtSustain = value; break;
    case PARAM_FILT_RELEASE: filtRelease = releaseFromNormalized(value); break;
    case PARAM_FILT_STRENGTH: filterStrength = value; break;
    case PARAM_FILT_KEY_TRACK: filterKeyTrack = keyTrackFromNormalized(value); break;
    case PARAM_DRIVE:       drive = driveFromNormalized(value); break;
    case PARAM_VEL_TO_FILT: velToFilter = value; break;
    case PARAM_VEL_TO_AMP:  velToAmp = value; break;
    
    // Amp envelope
    case PARAM_AMP_ATTACK:  ampAttack = attackFromNormalized(value); break;
    case PARAM_AMP_DECAY:   ampDecay = decayFromNormalized(value); break;
    case PARAM_AMP_SUSTAIN: ampSustain = value; break;
    case PARAM_AMP_RELEASE: ampRelease = releaseFromNormalized(value); break;
    
    // LFO
    case PARAM_LFO_RATE:    lfoRate = lfoRateFromNormalized(value); break;
    case PARAM_LFO_DEPTH:   lfoDepth = value; break;
    case PARAM_LFO_WAVE:    lfoWave = constrain((int)(value * 3.99f), 0, 3); break;
    case PARAM_LFO_TARGET:  lfoTarget = constrain((int)(value * 2.99f), 0, 2); break;
    case PARAM_LFO_ENABLED: lfoEnabled = (value > 0.5f); break;
    
    // Unison
    case PARAM_UNISON_COUNT:  unisonCount = unisonFromNormalized(value); break;
    case PARAM_UNISON_DETUNE: unisonDetune = unisonDetuneFromNormalized(value); break;
    case PARAM_UNISON_SPREAD: unisonSpread = value; break;
    
    // Global
    case PARAM_PLAY_MODE:   playMode = constrain((int)(value * 2.99f), 0, 2); break;
    case PARAM_GLIDE_TIME:  glideTime = glideTimeFromNormalized(value); break;
    case PARAM_MASTER_TUNE: masterTune = masterTuneFromNormalized(value); break;
    case PARAM_MASTER_VOL:  masterVol = value; break;
    
    // Chorus
    case PARAM_CHORUS_RATE: chorusRate = chorusRateFromNormalized(value); break;
    case PARAM_CHORUS_DEPTH: chorusDepth = value; break;
    case PARAM_CHORUS_FEEDBACK: chorusFeedback = value * 0.7f; break;  // Max 70%
    case PARAM_CHORUS_MIX:  chorusMix = value; break;
    
    // Reverb
    case PARAM_REVERB_SIZE: reverbSize = reverbSizeFromNormalized(value); break;
    case PARAM_REVERB_DAMP: reverbDamp = value; break;
    case PARAM_REVERB_WIDTH: reverbWidth = value; break;
    case PARAM_REVERB_MIX:  reverbMix = value; break;
    case PARAM_REVERB_PREDELAY: reverbPredelay = reverbPredelayFromNormalized(value); break;
  }
}

// ===== PROFESSIONAL PRESETS =====
// Each preset is tuned for specific musical applications

const Preset presets[] = {
  // 0: Classic Lead - Bright, cutting lead sound
  {"Classic Lead", {
    0.33f, 0.33f, 0.33f,  // OSC ranges (8')
    0.52f, 0.48f, 0.5f,   // OSC fine
    0.2f, 0.2f, 0.2f,     // OSC waves (saw)
    0.7f, 0.6f, 0.0f,     // OSC volumes
    0.0f, 0.0f,           // Noise
    0.0f, 0.0f, 0.0f, 0.0f, // Sub off
    0.0f, 0.0f,           // Ring off
    0.65f, 0.35f,         // Filter cutoff, res
    0.0f, 0.2f, 0.3f, 0.2f, // Filt env ADSR
    0.5f, 0.5f,           // Filt strength, keytrack
    0.15f, 0.3f, 0.5f,    // Drive, vel->filt, vel->amp
    0.0f, 0.15f, 0.85f, 0.2f, // Amp ADSR
    0.25f, 0.0f, 0.0f, 1.0f, 0.0f, // LFO
    0.0f, 0.0f, 0.5f,     // Unison off
    1.0f, 0.0f, 0.5f, 0.8f, // Play mode poly, glide, tune, vol
    0.0f, 0.0f, 0.0f, 0.0f, // Chorus off
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f // Reverb off
  }},
  
  // 1: Fat Bass - Deep, warm bass with sub
  {"Fat Bass", {
    0.0f, 0.0f, 0.0f,     // OSC ranges (16')
    0.5f, 0.5f, 0.5f,
    0.2f, 0.2f, 0.4f,     // Saw, saw, pulse
    0.8f, 0.6f, 0.4f,
    0.15f, 0.0f,          // Some noise
    1.0f, 0.0f, 0.0f, 0.6f, // Sub ON, square, -1 oct
    0.0f, 0.0f,
    0.45f, 0.55f,
    0.0f, 0.08f, 0.0f, 0.15f,
    0.7f, 0.0f,
    0.25f, 0.2f, 0.6f,
    0.0f, 0.08f, 1.0f, 0.15f,
    0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.5f,
    0.0f, 0.0f, 0.5f, 0.85f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f
  }},
  
  // 2: Analog Pad - Lush, evolving pad with chorus
  {"Analog Pad", {
    0.33f, 0.33f, 0.5f,   // 8', 8', 4'
    0.51f, 0.49f, 0.52f,
    0.2f, 0.2f, 0.2f,
    0.5f, 0.5f, 0.4f,
    0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, // Sub off
    0.0f, 0.0f,
    0.55f, 0.3f,
    0.15f, 0.45f, 0.75f, 0.5f,
    0.4f, 0.5f,
    0.05f, 0.3f, 0.4f,
    0.2f, 0.35f, 0.8f, 0.5f,
    0.08f, 0.3f, 0.0f, 1.0f, 1.0f, // LFO on filter
    0.0f, 0.0f, 0.5f,
    1.0f, 0.0f, 0.5f, 0.75f,
    0.25f, 0.4f, 0.25f, 0.4f, // Chorus ON
    0.4f, 0.6f, 0.8f, 0.25f, 0.2f // Reverb
  }},
  
  // 3: Acid Bass - 303-style squelch
  {"Acid Bass", {
    0.33f, 0.0f, 0.0f,
    0.5f, 0.5f, 0.5f,
    0.2f, 0.2f, 0.2f,     // All saw
    0.85f, 0.0f, 0.0f,    // OSC1 only
    0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f,
    0.35f, 0.8f,          // Low cutoff, high res
    0.0f, 0.1f, 0.0f, 0.15f,
    0.9f, 0.0f,           // High filt env
    0.1f, 0.5f, 0.7f,
    0.0f, 0.08f, 0.9f, 0.15f,
    0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.5f,
    0.0f, 0.2f, 0.5f, 0.8f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f
  }},
  
  // 4: Supersaw - Massive detuned unison
  {"Supersaw", {
    0.33f, 0.33f, 0.33f,
    0.54f, 0.46f, 0.5f,
    0.2f, 0.2f, 0.2f,
    0.6f, 0.7f, 0.7f,
    0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f,
    0.7f, 0.2f,
    0.0f, 0.25f, 0.5f, 0.35f,
    0.5f, 0.5f,
    0.2f, 0.4f, 0.5f,
    0.0f, 0.2f, 0.85f, 0.3f,
    0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.75f, 0.4f, 0.7f,    // Unison 4, detune, spread
    1.0f, 0.0f, 0.5f, 0.7f,
    0.3f, 0.45f, 0.3f, 0.35f, // Chorus
    0.5f, 0.5f, 1.0f, 0.2f, 0.15f // Reverb
  }},
  
  // 5: Moog Brass - Classic brass stab
  {"Moog Brass", {
    0.33f, 0.33f, 0.33f,
    0.52f, 0.48f, 0.5f,
    0.2f, 0.2f, 0.2f,
    0.6f, 0.5f, 0.3f,
    0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.3f, // Sub on
    0.0f, 0.0f,
    0.55f, 0.45f,
    0.05f, 0.15f, 0.85f, 0.3f,
    0.65f, 0.5f,
    0.3f, 0.4f, 0.6f,
    0.05f, 0.15f, 0.85f, 0.25f,
    0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.5f,
    1.0f, 0.0f, 0.5f, 0.8f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.3f, 0.4f, 0.7f, 0.15f, 0.1f
  }},
  
  // 6: Wobble Bass - LFO modulated dubstep bass
  {"Wobble Bass", {
    0.17f, 0.17f, 0.0f,   // 32'
    0.5f, 0.5f, 0.5f,
    0.2f, 0.2f, 0.2f,
    0.75f, 0.6f, 0.0f,
    0.0f, 0.0f,
    1.0f, 0.0f, 1.0f, 0.5f, // Sub -2 oct
    0.0f, 0.0f,
    0.4f, 0.7f,
    0.0f, 0.2f, 0.0f, 0.2f,
    0.7f, 0.0f,
    0.35f, 0.3f, 0.6f,
    0.0f, 0.15f, 1.0f, 0.2f,
    0.4f, 0.7f, 0.0f, 1.0f, 1.0f, // LFO to filter
    0.0f, 0.0f, 0.5f,
    0.0f, 0.0f, 0.5f, 0.85f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f
  }},
  
  // 7: Plucked String
  {"Plucked String", {
    0.33f, 0.5f, 0.33f,
    0.5f, 0.5f, 0.5f,
    0.0f, 0.0f, 0.0f,     // Triangle
    0.7f, 0.4f, 0.2f,
    0.05f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f,
    0.75f, 0.1f,
    0.0f, 0.08f, 0.0f, 0.3f,
    0.75f, 0.6f,
    0.1f, 0.5f, 0.7f,
    0.0f, 0.08f, 0.0f, 0.35f,
    0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.5f,
    1.0f, 0.0f, 0.5f, 0.75f,
    0.15f, 0.25f, 0.2f, 0.2f,
    0.45f, 0.55f, 0.8f, 0.3f, 0.05f
  }},
  
  // 8: Warm Strings - Lush string ensemble
  {"Warm Strings", {
    0.33f, 0.33f, 0.5f,
    0.51f, 0.49f, 0.52f,
    0.2f, 0.2f, 0.2f,
    0.5f, 0.5f, 0.4f,
    0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f,
    0.6f, 0.15f,
    0.3f, 0.5f, 0.7f, 0.6f,
    0.3f, 0.4f,
    0.05f, 0.3f, 0.4f,
    0.35f, 0.5f, 0.75f, 0.55f,
    0.08f, 0.2f, 0.0f, 1.0f, 1.0f,
    0.5f, 0.15f, 0.6f,    // Unison 3
    1.0f, 0.0f, 0.5f, 0.7f,
    0.35f, 0.5f, 0.3f, 0.45f, // Symphonic chorus
    0.55f, 0.5f, 1.0f, 0.35f, 0.25f
  }},
  
  // 9: Ring Mod Lead - Metallic, bell-like
  {"Ring Mod Lead", {
    0.33f, 0.5f, 0.33f,
    0.5f, 0.58f, 0.5f,    // OSC2 detuned for ring
    0.1f, 0.1f, 0.1f,     // Sine/triangle
    0.6f, 0.5f, 0.0f,
    0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.5f,           // Ring mod ON
    0.7f, 0.25f,
    0.0f, 0.3f, 0.4f, 0.3f,
    0.5f, 0.5f,
    0.15f, 0.4f, 0.5f,
    0.0f, 0.2f, 0.7f, 0.35f,
    0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.5f,
    1.0f, 0.0f, 0.5f, 0.75f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.4f, 0.5f, 0.8f, 0.25f, 0.1f
  }},
  
  // 10: Resonant Sweep
  {"Resonant Sweep", {
    0.33f, 0.33f, 0.33f,
    0.51f, 0.49f, 0.5f,
    0.2f, 0.2f, 0.2f,
    0.7f, 0.7f, 0.7f,
    0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f,
    0.25f, 0.85f,         // Low cutoff, high res
    0.03f, 1.0f, 0.0f, 0.8f,
    1.0f, 0.3f,
    0.2f, 0.3f, 0.5f,
    0.05f, 0.1f, 1.0f, 0.35f,
    0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.5f,
    1.0f, 0.0f, 0.5f, 0.7f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.5f, 0.5f, 0.9f, 0.3f, 0.15f
  }},
  
  // 11: Mono Lead - Portamento lead
  {"Mono Lead", {
    0.33f, 0.33f, 0.0f,
    0.52f, 0.48f, 0.5f,
    0.2f, 0.4f, 0.2f,     // Saw, pulse, saw
    0.7f, 0.5f, 0.0f,
    0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.4f, // Sub on
    0.0f, 0.0f,
    0.6f, 0.4f,
    0.0f, 0.15f, 0.5f, 0.2f,
    0.6f, 0.5f,
    0.2f, 0.4f, 0.6f,
    0.0f, 0.12f, 0.8f, 0.2f,
    0.2f, 0.35f, 0.0f, 0.0f, 1.0f, // LFO vibrato
    0.0f, 0.0f, 0.5f,
    0.0f, 0.3f, 0.5f, 0.8f, // Mono with glide
    0.0f, 0.0f, 0.0f, 0.0f,
    0.3f, 0.5f, 0.7f, 0.15f, 0.1f
  }},
};

const int NUM_PRESETS = sizeof(presets) / sizeof(presets[0]);

void loadPreset(int presetIndex) {
  if (presetIndex < 0 || presetIndex >= NUM_PRESETS) return;
  
  Serial.print("Loading: ");
  Serial.println(presets[presetIndex].name);
  
  for (int i = 0; i < NUM_PARAMETERS; i++) {
    setParameter(i, presets[presetIndex].parameters[i]);
  }
}
