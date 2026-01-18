/*
 * synth_voice_pro.h - PROFESSIONAL EDITION v2.0
 * 
 * ENHANCEMENTS:
 * - 8-voice polyphony with intelligent voice stealing
 * - Sub-oscillator per voice (Jupiter-8 style)
 * - 2x oversampled Moog ladder filter for stability
 * - Ring modulation between oscillators
 * - Unison mode with detune spread
 * - Parameter smoothing (anti-zipper)
 * - DC blocking filter
 * - Improved fast math functions
 */

#ifndef SYNTH_VOICE_PRO_H
#define SYNTH_VOICE_PRO_H

#include <Arduino.h>

// ===== AUDIO CONFIGURATION =====
#define SAMPLE_RATE 48000
#define SAMPLE_RATE_F 48000.0f
#define INV_SAMPLE_RATE (1.0f / 48000.0f)
#define OVERSAMPLING 2
#define INTERNAL_RATE (SAMPLE_RATE * OVERSAMPLING)
#define INTERNAL_RATE_F (SAMPLE_RATE_F * OVERSAMPLING)

// ===== VOICE CONFIGURATION =====
#define MAX_VOICES 8
#define MAX_UNISON 4

// ===== WAVEFORMS =====
enum Waveform {
  WAVE_TRIANGLE = 0,
  WAVE_SAWTOOTH = 1,
  WAVE_PULSE = 2,
  WAVE_PULSE_NARROW = 3,
  WAVE_PULSE_WIDE = 4,
  WAVE_SINE = 5
};

// ===== ENVELOPE STATES =====
enum EnvelopeState {
  ENV_IDLE = 0,
  ENV_ATTACK,
  ENV_DECAY,
  ENV_SUSTAIN,
  ENV_RELEASE
};

// ===== PARAMETER SMOOTHER =====
// Prevents zipper noise on CC changes
struct ParamSmoother {
  float current;
  float target;
  float coeff;  // Smoothing coefficient (higher = faster)
};

inline void initSmoother(ParamSmoother* s, float initial, float timeMs) {
  s->current = initial;
  s->target = initial;
  // Time constant for ~63% convergence
  s->coeff = 1.0f - expf(-1000.0f / (timeMs * SAMPLE_RATE_F));
}

inline float processSmoother(ParamSmoother* s) {
  s->current += (s->target - s->current) * s->coeff;
  return s->current;
}

inline void setSmootherTarget(ParamSmoother* s, float target) {
  s->target = target;
}

inline void setSmootherImmediate(ParamSmoother* s, float value) {
  s->current = value;
  s->target = value;
}

// ===== DC BLOCKING FILTER =====
struct DCBlocker {
  float x1;
  float y1;
  float R;  // Pole position (0.995-0.9999)
};

inline void initDCBlocker(DCBlocker* dc, float R = 0.9975f) {
  dc->x1 = 0.0f;
  dc->y1 = 0.0f;
  dc->R = R;
}

inline float processDCBlocker(DCBlocker* dc, float input) {
  // y[n] = x[n] - x[n-1] + R * y[n-1]
  float output = input - dc->x1 + dc->R * dc->y1;
  dc->x1 = input;
  dc->y1 = output;
  return output;
}

// ===== ENVELOPE =====
struct Envelope {
  EnvelopeState state;
  float level;
  float attackRate;
  float decayRate;
  float sustainLevel;
  float releaseRate;
  float attackCurve;   // Curvature for attack (1.0 = linear)
  float decayCurve;    // Curvature for decay/release
};

// ===== OSCILLATOR =====
struct Oscillator {
  float phase;
  float frequency;
  int waveform;
  float pulseWidth;
  float lastOutput;
  float blitIncrement;  // For BLIT anti-aliasing
};

// ===== SUB-OSCILLATOR =====
struct SubOscillator {
  float phase;
  float frequency;
  int waveform;     // 0=square, 1=sine, 2=pulse
  float level;
  bool enabled;
  int octave;       // -1 or -2 octaves below
};

// ===== IMPROVED MOOG LADDER FILTER =====
// 2x oversampled for better stability at high resonance
struct MoogFilter {
  float stage[4];
  float delay[4];
  
  float cutoff;
  float resonance;
  
  // Cached coefficients
  float g;
  float gRes;
  float gComp;
  
  // Oversampling anti-aliasing
  float oversampleState;
  
  // Coefficient update optimization
  float lastCutoff;
  float lastResonance;
};

// ===== RING MODULATOR =====
struct RingMod {
  bool enabled;
  float amount;     // 0-1 blend with original
  int sourceOsc;    // Which oscillator to use as modulator (1 or 2)
};

// ===== UNISON VOICE =====
struct UnisonVoice {
  float phase[3];       // Phase for each oscillator
  float detuneAmount;   // Detune for this unison voice
  float pan;            // Stereo pan position
};

// ===== MAIN VOICE STRUCTURE =====
struct Voice {
  // State
  int note;
  bool active;
  int velocity;
  float velocityFloat;
  unsigned long noteOnTime;
  uint8_t stealPriority;  // For intelligent voice stealing
  
  // Main oscillators
  Oscillator osc1;
  Oscillator osc2;
  Oscillator osc3;
  
  // Sub-oscillator
  SubOscillator subOsc;
  
  // Ring modulation
  RingMod ringMod;
  
  // Unison
  UnisonVoice unisonVoices[MAX_UNISON];
  int unisonCount;
  float unisonDetune;
  float unisonSpread;
  
  // Envelopes
  Envelope ampEnv;
  Envelope filtEnv;
  
  // Filter
  MoogFilter filter;
  
  // DC blocker per voice
  DCBlocker dcBlock;
  
  // Portamento
  float targetFreq;
  float currentFreq;
  bool gliding;
  
  // Per-voice parameters
  float drive;
  float filterKeyTrack;
  
  // Parameter smoothers for per-voice modulation
  ParamSmoother cutoffMod;
  ParamSmoother ampMod;
};

// ===== FAST MATH FUNCTIONS =====

// High-quality tanh approximation (Pade approximant)
inline float fastTanh(float x) {
  if (x < -3.0f) return -1.0f;
  if (x > 3.0f) return 1.0f;
  float x2 = x * x;
  return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// Even faster tanh for filter stages
inline float fastTanhApprox(float x) {
  x = fmaxf(-3.0f, fminf(3.0f, x));
  return x * (1.0f - x * x * 0.037037f);  // Quick approximation
}

// Soft clipping with smooth saturation
inline float softClip(float x) {
  if (x > 1.0f) return 0.666667f + (x - 1.0f) * 0.333333f;
  if (x < -1.0f) return -0.666667f + (x + 1.0f) * 0.333333f;
  return x - x * x * x * 0.333333f;
}

// Asymmetric soft clipping for tube-like character
inline float tubeClip(float x) {
  if (x > 0.0f) {
    return 1.0f - expf(-x);
  } else {
    return -1.0f + expf(x);
  }
}

// Fast sine approximation (Bhaskara I)
inline float fastSin(float x) {
  // Normalize to [-PI, PI]
  const float invTwoPi = 0.159154943f;
  x = x - 6.283185307f * floorf(x * invTwoPi + 0.5f);
  
  // Bhaskara approximation
  const float B = 1.27323954f;
  const float C = -0.405284735f;
  float y = B * x + C * x * fabsf(x);
  
  // Extra precision
  const float P = 0.225f;
  return P * (y * fabsf(y) - y) + y;
}

// Fast cosine
inline float fastCos(float x) {
  return fastSin(x + 1.5707963f);
}

// Fast power of 2 (for pitch calculations)
inline float fastPow2(float x) {
  if (x < -10.0f) return 0.0009765625f;
  if (x > 10.0f) return 1024.0f;
  
  int i = (int)(x >= 0 ? x : x - 1);
  float f = x - (float)i;
  
  // Cubic approximation for 2^f where f is in [0,1]
  float p = 1.0f + f * (0.6931472f + f * (0.2402265f + f * 0.0558f));
  
  // Apply integer part
  union { float f; uint32_t i; } u;
  u.f = p;
  u.i += (uint32_t)(i) << 23;
  return u.f;
}

// Fast exp (based on pow2)
inline float fastExp(float x) {
  return fastPow2(x * 1.442695f);  // log2(e) = 1.442695
}

// PolyBLEP for anti-aliased oscillators
inline float polyBlep(float t, float dt) {
  if (t < dt) {
    t /= dt;
    return t + t - t * t - 1.0f;
  } else if (t > 1.0f - dt) {
    t = (t - 1.0f) / dt;
    return t * t + t + t + 1.0f;
  }
  return 0.0f;
}

// ===== FUNCTION DECLARATIONS =====

// Voice management
void initVoice(Voice* v);
void resetVoice(Voice* v);
float processVoice(Voice* v, float noiseInput, float lfoValue, 
                   float pitchMod, float filterMod, float ampMod);

// Oscillators
void initOscillator(Oscillator* osc, float freq, int waveform);
float generateOscillator(Oscillator* osc);
float generateBandlimitedSaw(Oscillator* osc);
float generateBandlimitedPulse(Oscillator* osc, float pw);
float generateBandlimitedTriangle(Oscillator* osc);
float generateSine(Oscillator* osc);

// Sub-oscillator
void initSubOscillator(SubOscillator* sub);
float generateSubOscillator(SubOscillator* sub, float mainFreq);

// Envelopes
void initEnvelope(Envelope* env);
void triggerEnvelope(Envelope* env, float attack, float decay, 
                     float sustain, float release);
void releaseEnvelope(Envelope* env);
float processEnvelope(Envelope* env);
bool isEnvelopeIdle(Envelope* env);

// Filter
void initMoogFilter(MoogFilter* f);
void updateFilterCoeffs(MoogFilter* f, float cutoff, float resonance);
float processMoogFilter(MoogFilter* f, float input);

// Drive
float processDrive(float input, float amount);

// Noise
struct NoiseGen {
  uint32_t seed;
  float pinkState[6];  // Extended pink noise
  float brownState;
};

void initNoiseGen(NoiseGen* gen);
float generateWhiteNoise(NoiseGen* gen);
float generatePinkNoise(NoiseGen* gen);
float generateBrownNoise(NoiseGen* gen);

// Utility
float noteToFreq(int note);
float midiCCToNormalized(int value);

#endif // SYNTH_VOICE_PRO_H
