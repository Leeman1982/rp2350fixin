/*
 * synth_effects_pro.h - PROFESSIONAL EDITION v2.0
 * 
 * Effects suite:
 * - Symphonic Chorus (based on Roland Dimension D)
 * - FreeVerb-style algorithmic reverb
 * - Stereo width processing
 */

#ifndef SYNTH_EFFECTS_PRO_H
#define SYNTH_EFFECTS_PRO_H

#include <Arduino.h>
#include "synth_voice_pro.h"

// ===== SYMPHONIC CHORUS =====
// Inspired by Roland Dimension D / Jupiter-8 ensemble
// Uses 3 delay lines with quadrature LFOs

#define CHORUS_BUFFER_SIZE 4096  // ~85ms at 48kHz
#define CHORUS_VOICES 3

struct SymphonicChorus {
  // Delay buffers (shared between L/R with different read positions)
  float delayBuffer[CHORUS_BUFFER_SIZE];
  int writePos;
  
  // LFO phases for each voice
  float lfoPhase[CHORUS_VOICES];
  float lfoRate;
  
  // Parameters
  float baseDelay;      // Center delay in samples
  float depth;          // Modulation depth in samples
  float feedback;       // Feedback amount
  float mix;            // Wet/dry mix
  
  // State for HPF (removes DC and low rumble)
  float hpfState;
  
  bool enabled;
};

void initSymphonicChorus(SymphonicChorus* ch);
void processSymphonicChorus(SymphonicChorus* ch, float* left, float* right,
                            float rate, float depth, float feedback, float mix);

// ===== FREEVERB REVERB =====
// Schroeder reverb with 8 parallel comb filters + 4 series allpass
// SAFETY: Static allocation - no malloc in real-time path

#define REVERB_COMB_COUNT 8
#define REVERB_ALLPASS_COUNT 4
#define MAX_COMB_SIZE 1760      // Largest comb buffer (scaled for 48kHz)
#define MAX_ALLPASS_SIZE 640    // Largest allpass buffer
#define MAX_PREDELAY_SIZE 4800  // 100ms at 48kHz

// Comb filter - STATIC allocation
struct ReverbComb {
  float buffer[MAX_COMB_SIZE];
  int bufferSize;
  int index;
  float filterStore;
};

// Allpass filter - STATIC allocation
struct ReverbAllpass {
  float buffer[MAX_ALLPASS_SIZE];
  int bufferSize;
  int index;
};

struct FreeVerb {
  // Comb filters (stereo - L and R have slightly different tuning)
  ReverbComb combL[REVERB_COMB_COUNT];
  ReverbComb combR[REVERB_COMB_COUNT];
  
  // Allpass filters
  ReverbAllpass allpassL[REVERB_ALLPASS_COUNT];
  ReverbAllpass allpassR[REVERB_ALLPASS_COUNT];
  
  // Predelay - STATIC allocation
  float predelayBuffer[MAX_PREDELAY_SIZE];
  int predelaySize;
  int predelayIndex;
  int predelaySamples;
  
  // Parameters
  float roomSize;       // 0-1
  float damp;           // High frequency damping 0-1
  float width;          // Stereo width 0-1
  float wet;            // Wet level
  float dry;            // Dry level
  
  // Derived coefficients
  float feedback;
  float damp1;
  float damp2;
  
  bool enabled;
};

void initFreeVerb(FreeVerb* rv);
void freeFreeVerb(FreeVerb* rv);
void updateFreeVerbParams(FreeVerb* rv, float roomSize, float damp, float width, float mix);
void processFreeVerb(FreeVerb* rv, float inputL, float inputR, float* outL, float* outR);

// ===== STEREO WIDENER =====
struct StereoWidener {
  float width;          // 0=mono, 1=stereo, >1=widened
  float lastMid;
  float lastSide;
};

void initStereoWidener(StereoWidener* sw);
void processStereoWidener(StereoWidener* sw, float* left, float* right, float width);

// ===== GLOBAL LFO =====
// Shared LFO for modulation

struct GlobalLFO {
  float phase;
  float rate;           // Hz
  int waveform;         // 0=sine, 1=triangle, 2=saw, 3=square
  float output;
  float smoothedOutput; // For parameter modulation
};

void initGlobalLFO(GlobalLFO* lfo);
float processGlobalLFO(GlobalLFO* lfo, float rate, int waveform);

// ===== LIMITER =====
// Simple soft limiter for output protection

struct Limiter {
  float envelope;
  float attack;
  float release;
  float threshold;
};

void initLimiter(Limiter* lim, float threshold);
float processLimiter(Limiter* lim, float input);

#endif // SYNTH_EFFECTS_PRO_H
