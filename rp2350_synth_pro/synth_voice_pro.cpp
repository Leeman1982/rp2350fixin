/*
 * synth_voice_pro.cpp - PROFESSIONAL EDITION v2.0
 * 
 * High-quality DSP implementation with:
 * - PolyBLEP anti-aliased oscillators
 * - 2x oversampled Moog ladder filter
 * - Sub-oscillator with multiple waveforms
 * - Ring modulation
 * - Proper gain staging throughout
 */

#include "synth_voice_pro.h"
#include "synth_params_pro.h"
#include <math.h>

// ===== VOICE INITIALIZATION =====

void initVoice(Voice* v) {
  v->note = 0;
  v->active = false;
  v->velocity = 0;
  v->velocityFloat = 0.0f;
  v->noteOnTime = 0;
  v->stealPriority = 0;
  
  initOscillator(&v->osc1, 440.0f, WAVE_SAWTOOTH);
  initOscillator(&v->osc2, 440.0f, WAVE_SAWTOOTH);
  initOscillator(&v->osc3, 440.0f, WAVE_SAWTOOTH);
  
  initSubOscillator(&v->subOsc);
  
  v->ringMod.enabled = false;
  v->ringMod.amount = 0.0f;
  v->ringMod.sourceOsc = 1;
  
  // Initialize unison
  v->unisonCount = 1;
  v->unisonDetune = 0.0f;
  v->unisonSpread = 0.5f;
  for (int i = 0; i < MAX_UNISON; i++) {
    v->unisonVoices[i].phase[0] = 0.0f;
    v->unisonVoices[i].phase[1] = 0.0f;
    v->unisonVoices[i].phase[2] = 0.0f;
    v->unisonVoices[i].detuneAmount = 0.0f;
    v->unisonVoices[i].pan = 0.0f;
  }
  
  initEnvelope(&v->ampEnv);
  initEnvelope(&v->filtEnv);
  
  initMoogFilter(&v->filter);
  initDCBlocker(&v->dcBlock, 0.9975f);
  
  v->targetFreq = 440.0f;
  v->currentFreq = 440.0f;
  v->gliding = false;
  
  v->drive = 1.0f;
  v->filterKeyTrack = 0.0f;
  
  initSmoother(&v->cutoffMod, 1.0f, 5.0f);
  initSmoother(&v->ampMod, 1.0f, 5.0f);
}

void resetVoice(Voice* v) {
  v->osc1.phase = 0.0f;
  v->osc2.phase = 0.0f;
  v->osc3.phase = 0.0f;
  v->subOsc.phase = 0.0f;
  
  for (int i = 0; i < MAX_UNISON; i++) {
    // Random phase for unison to avoid phase cancellation
    v->unisonVoices[i].phase[0] = (float)(random(1000)) * 0.001f;
    v->unisonVoices[i].phase[1] = (float)(random(1000)) * 0.001f;
    v->unisonVoices[i].phase[2] = (float)(random(1000)) * 0.001f;
  }
  
  initMoogFilter(&v->filter);
  initDCBlocker(&v->dcBlock, 0.9975f);
}

// ===== OSCILLATOR FUNCTIONS =====

void initOscillator(Oscillator* osc, float freq, int waveform) {
  osc->phase = 0.0f;
  osc->frequency = freq;
  osc->waveform = waveform;
  osc->pulseWidth = 0.5f;
  osc->lastOutput = 0.0f;
  osc->blitIncrement = freq / SAMPLE_RATE_F;
}

float generateOscillator(Oscillator* osc) {
  float output = 0.0f;
  float dt = osc->frequency / SAMPLE_RATE_F;
  osc->blitIncrement = dt;
  
  switch (osc->waveform) {
    case WAVE_TRIANGLE:
      output = generateBandlimitedTriangle(osc);
      break;
      
    case WAVE_SAWTOOTH:
      output = generateBandlimitedSaw(osc);
      break;
      
    case WAVE_PULSE:
      output = generateBandlimitedPulse(osc, 0.5f);
      break;
      
    case WAVE_PULSE_NARROW:
      output = generateBandlimitedPulse(osc, 0.25f);
      break;
      
    case WAVE_PULSE_WIDE:
      output = generateBandlimitedPulse(osc, 0.75f);
      break;
      
    case WAVE_SINE:
      output = generateSine(osc);
      break;
  }
  
  // Advance phase
  osc->phase += dt;
  if (osc->phase >= 1.0f) osc->phase -= 1.0f;
  
  osc->lastOutput = output;
  return output;
}

float generateBandlimitedSaw(Oscillator* osc) {
  float t = osc->phase;
  float dt = osc->blitIncrement;
  
  // Naive sawtooth
  float output = 2.0f * t - 1.0f;
  
  // Apply PolyBLEP at discontinuity
  output -= polyBlep(t, dt);
  
  return output;
}

float generateBandlimitedPulse(Oscillator* osc, float pw) {
  float t = osc->phase;
  float dt = osc->blitIncrement;
  
  // Naive pulse
  float output = (t < pw) ? 1.0f : -1.0f;
  
  // PolyBLEP at rising edge (t = 0)
  output += polyBlep(t, dt);
  
  // PolyBLEP at falling edge (t = pw)
  float t2 = t - pw;
  if (t2 < 0.0f) t2 += 1.0f;
  output -= polyBlep(t2, dt);
  
  return output;
}

float generateBandlimitedTriangle(Oscillator* osc) {
  float t = osc->phase;
  float dt = osc->blitIncrement;
  
  // Generate from integrated square wave for anti-aliasing
  // Naive triangle
  float output;
  if (t < 0.5f) {
    output = 4.0f * t - 1.0f;
  } else {
    output = 3.0f - 4.0f * t;
  }
  
  // Leaky integrator approach would be better, but this is simpler
  // The triangle has fewer harmonics so aliasing is less severe
  
  return output;
}

float generateSine(Oscillator* osc) {
  return fastSin(osc->phase * 6.283185307f);
}

// ===== SUB-OSCILLATOR =====

void initSubOscillator(SubOscillator* sub) {
  sub->phase = 0.0f;
  sub->frequency = 220.0f;
  sub->waveform = 0;  // Square
  sub->level = 0.0f;
  sub->enabled = false;
  sub->octave = -1;
}

float generateSubOscillator(SubOscillator* sub, float mainFreq) {
  if (!sub->enabled || sub->level < 0.001f) return 0.0f;
  
  // Calculate sub frequency based on octave setting
  float subFreq = mainFreq * (sub->octave == -2 ? 0.25f : 0.5f);
  float dt = subFreq / SAMPLE_RATE_F;
  
  float output = 0.0f;
  
  switch (sub->waveform) {
    case 0: {  // Square - braces needed for local variable
      output = (sub->phase < 0.5f) ? 1.0f : -1.0f;
      // PolyBLEP
      output += polyBlep(sub->phase, dt);
      float t2 = sub->phase - 0.5f;
      if (t2 < 0.0f) t2 += 1.0f;
      output -= polyBlep(t2, dt);
      break;
    }
      
    case 1:  // Sine
      output = fastSin(sub->phase * 6.283185307f);
      break;
      
    case 2:  // Pulse (25%)
      output = (sub->phase < 0.25f) ? 1.0f : -1.0f;
      break;
  }
  
  // Advance phase
  sub->phase += dt;
  if (sub->phase >= 1.0f) sub->phase -= 1.0f;
  
  return output * sub->level;
}

// ===== ENVELOPE FUNCTIONS =====

void initEnvelope(Envelope* env) {
  env->state = ENV_IDLE;
  env->level = 0.0f;
  env->attackRate = 0.001f;
  env->decayRate = 0.001f;
  env->sustainLevel = 1.0f;
  env->releaseRate = 0.001f;
  env->attackCurve = 1.0f;
  env->decayCurve = 1.0f;
}

void triggerEnvelope(Envelope* env, float attack, float decay, 
                     float sustain, float release) {
  env->state = ENV_ATTACK;
  // Don't reset level - allows legato playing
  
  // Calculate rates
  env->attackRate = (attack > 0.0005f) ? (1.0f / (attack * SAMPLE_RATE_F)) : 1.0f;
  env->decayRate = (decay > 0.0005f) ? (1.0f / (decay * SAMPLE_RATE_F)) : 1.0f;
  env->sustainLevel = sustain;
  env->releaseRate = (release > 0.0005f) ? (1.0f / (release * SAMPLE_RATE_F)) : 1.0f;
}

void releaseEnvelope(Envelope* env) {
  if (env->state != ENV_IDLE) {
    env->state = ENV_RELEASE;
  }
}

float processEnvelope(Envelope* env) {
  switch (env->state) {
    case ENV_IDLE:
      return 0.0f;
      
    case ENV_ATTACK:
      env->level += env->attackRate;
      if (env->level >= 1.0f) {
        env->level = 1.0f;
        env->state = ENV_DECAY;
      }
      break;
      
    case ENV_DECAY:
      // Exponential decay
      env->level -= env->decayRate * (env->level - env->sustainLevel + 0.0001f);
      if (env->level <= env->sustainLevel + 0.001f) {
        env->level = env->sustainLevel;
        env->state = ENV_SUSTAIN;
      }
      break;
      
    case ENV_SUSTAIN:
      env->level = env->sustainLevel;
      break;
      
    case ENV_RELEASE:
      // Exponential release
      env->level -= env->releaseRate * (env->level + 0.0001f);
      if (env->level < 0.0001f) {
        env->level = 0.0f;
        env->state = ENV_IDLE;
      }
      break;
  }
  
  return env->level;
}

bool isEnvelopeIdle(Envelope* env) {
  return env->state == ENV_IDLE;
}

// ===== MOOG LADDER FILTER =====
// 2x oversampled for stability at high resonance

void initMoogFilter(MoogFilter* f) {
  for (int i = 0; i < 4; i++) {
    f->stage[i] = 0.0f;
    f->delay[i] = 0.0f;
  }
  
  f->cutoff = 1000.0f;
  f->resonance = 0.0f;
  f->g = 0.0f;
  f->gRes = 0.0f;
  f->gComp = 0.5f;
  f->oversampleState = 0.0f;
  f->lastCutoff = -1.0f;
  f->lastResonance = -1.0f;
}

void updateFilterCoeffs(MoogFilter* f, float cutoff, float resonance) {
  // Only update if parameters changed
  if (fabsf(cutoff - f->lastCutoff) < 1.0f && 
      fabsf(resonance - f->lastResonance) < 0.01f) {
    return;
  }
  
  f->lastCutoff = cutoff;
  f->lastResonance = resonance;
  f->cutoff = cutoff;
  f->resonance = resonance;
  
  // Clamp for stability
  cutoff = fmaxf(20.0f, fminf(cutoff, 20000.0f));
  resonance = fmaxf(0.0f, fminf(resonance, 3.95f));
  
  // Calculate normalized frequency (for oversampled rate)
  float fc = cutoff / INTERNAL_RATE_F;
  fc = fminf(fc, 0.45f);
  
  // Filter coefficient using tan approximation
  float g = 3.141592653f * fc;
  g = g / (1.0f + g);  // One-pole coefficient
  
  f->g = g;
  
  // Resonance with frequency compensation
  // Higher frequencies need less resonance to prevent instability
  float resComp = 1.0f - fc * 0.5f;
  f->gRes = resonance * resComp;
  
  // Gain compensation for resonance
  f->gComp = 1.0f + resonance * 0.25f;
}

// Process one oversampled sample through the filter
static inline float processMoogStage(MoogFilter* f, float input) {
  float g = f->g;
  
  // Feedback from 4th stage with limiting
  float fb = f->stage[3];
  fb = fmaxf(-1.5f, fminf(fb, 1.5f));
  
  // Input with resonance feedback
  float x = input - f->gRes * fb;
  
  // Soft saturation at input (Moog warmth)
  x = fastTanhApprox(x * 0.8f);
  
  // Process 4 one-pole stages with interleaved nonlinearity
  for (int i = 0; i < 4; i++) {
    float inp = (i == 0) ? x : f->stage[i - 1];
    
    // Trapezoidal integration
    float v = (inp - f->delay[i]) * g;
    float out = v + f->delay[i];
    
    // Soft saturation per stage
    out = fastTanhApprox(out);
    
    f->stage[i] = out;
    f->delay[i] = out + v;  // Store for next sample
  }
  
  return f->stage[3];
}

float processMoogFilter(MoogFilter* f, float input) {
  // SAFETY: sanitize input
  if (!isfinite(input)) input = 0.0f;
  
  // 2x oversampling: process twice with half-band filtering
  
  // First sample (with input)
  float out1 = processMoogStage(f, input);
  
  // Second sample (with interpolated input)
  float interp = (input + f->oversampleState) * 0.5f;
  float out2 = processMoogStage(f, interp);
  
  f->oversampleState = input;
  
  // Average (simple decimation filter)
  float output = (out1 + out2) * 0.5f * f->gComp;
  
  // SAFETY: check for NaN/Inf and reset filter if corrupted
  if (!isfinite(output)) {
    for (int i = 0; i < 4; i++) {
      f->stage[i] = 0.0f;
      f->delay[i] = 0.0f;
    }
    f->oversampleState = 0.0f;
    return 0.0f;
  }
  
  return output;
}

// ===== DRIVE/SATURATION =====

float processDrive(float input, float amount) {
  if (amount < 1.01f) return input;
  
  float driven = input * amount;
  
  // Soft clipping with asymmetry
  float output;
  if (driven > 0.0f) {
    output = 1.0f - fastExp(-driven);
  } else {
    output = -1.0f + fastExp(driven);
  }
  
  // Compensate gain increase
  output *= 1.0f / sqrtf(amount);
  
  return output;
}

// ===== NOISE GENERATION =====

void initNoiseGen(NoiseGen* gen) {
  gen->seed = 12345;
  for (int i = 0; i < 6; i++) {
    gen->pinkState[i] = 0.0f;
  }
  gen->brownState = 0.0f;
}

float generateWhiteNoise(NoiseGen* gen) {
  // Linear congruential generator
  gen->seed = gen->seed * 1664525 + 1013904223;
  return ((int32_t)gen->seed) * (1.0f / 2147483648.0f);
}

float generatePinkNoise(NoiseGen* gen) {
  // Voss-McCartney algorithm (improved)
  float white = generateWhiteNoise(gen);
  
  gen->pinkState[0] = 0.99886f * gen->pinkState[0] + white * 0.0555179f;
  gen->pinkState[1] = 0.99332f * gen->pinkState[1] + white * 0.0750759f;
  gen->pinkState[2] = 0.96900f * gen->pinkState[2] + white * 0.1538520f;
  gen->pinkState[3] = 0.86650f * gen->pinkState[3] + white * 0.3104856f;
  gen->pinkState[4] = 0.55000f * gen->pinkState[4] + white * 0.5329522f;
  gen->pinkState[5] = -0.7616f * gen->pinkState[5] - white * 0.0168980f;
  
  float pink = gen->pinkState[0] + gen->pinkState[1] + gen->pinkState[2] + 
               gen->pinkState[3] + gen->pinkState[4] + gen->pinkState[5] +
               white * 0.5362f;
  
  return pink * 0.11f;
}

float generateBrownNoise(NoiseGen* gen) {
  float white = generateWhiteNoise(gen);
  gen->brownState = (gen->brownState + 0.02f * white) * 0.998f;
  return gen->brownState * 3.5f;
}

// ===== UTILITY =====

float noteToFreq(int note) {
  return 440.0f * fastPow2((note - 69) * 0.0833333f);  // 1/12 = 0.0833333
}

float midiCCToNormalized(int value) {
  return (float)value * 0.007874016f;  // 1/127
}
