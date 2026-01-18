/*
 * synth_effects_pro.cpp - PROFESSIONAL EDITION v2.0
 * 
 * High-quality effects implementation
 * SAFETY: All buffers statically allocated - no malloc in real-time path
 */

#include "synth_effects_pro.h"
#include <math.h>

// ===== SYMPHONIC CHORUS =====

void initSymphonicChorus(SymphonicChorus* ch) {
  for (int i = 0; i < CHORUS_BUFFER_SIZE; i++) {
    ch->delayBuffer[i] = 0.0f;
  }
  ch->writePos = 0;
  
  // Initialize LFO phases with 120° separation (triplet)
  ch->lfoPhase[0] = 0.0f;
  ch->lfoPhase[1] = 2.0943951f;  // 120°
  ch->lfoPhase[2] = 4.1887902f;  // 240°
  
  ch->lfoRate = 0.5f;
  ch->baseDelay = 600.0f;   // ~12.5ms at 48kHz
  ch->depth = 300.0f;       // ~6.25ms modulation
  ch->feedback = 0.2f;
  ch->mix = 0.5f;
  ch->hpfState = 0.0f;
  ch->enabled = false;
}

void processSymphonicChorus(SymphonicChorus* ch, float* left, float* right,
                            float rate, float depth, float feedback, float mix) {
  if (!ch->enabled || mix < 0.001f) return;
  
  float input = (*left + *right) * 0.5f;
  
  // High-pass filter the input to remove DC
  float hpfCoeff = 0.995f;
  ch->hpfState = hpfCoeff * ch->hpfState + (1.0f - hpfCoeff) * input;
  input -= ch->hpfState;
  
  // Write to delay buffer
  ch->delayBuffer[ch->writePos] = input + ch->delayBuffer[ch->writePos] * feedback * 0.3f;
  
  // LFO increment
  float lfoInc = rate * 6.283185f / SAMPLE_RATE_F;
  
  // Process each chorus voice
  float wetL = 0.0f;
  float wetR = 0.0f;
  
  for (int v = 0; v < CHORUS_VOICES; v++) {
    // Update LFO
    ch->lfoPhase[v] += lfoInc;
    if (ch->lfoPhase[v] >= 6.283185f) ch->lfoPhase[v] -= 6.283185f;
    
    // Calculate delay time with modulation
    float lfoVal = fastSin(ch->lfoPhase[v]);
    float delayTime = ch->baseDelay + depth * 400.0f * lfoVal;
    
    // Clamp delay time
    delayTime = fmaxf(4.0f, fminf(delayTime, CHORUS_BUFFER_SIZE - 2.0f));
    
    // Calculate read position with wraparound
    float readPos = (float)ch->writePos - delayTime;
    if (readPos < 0.0f) readPos += CHORUS_BUFFER_SIZE;
    
    // Linear interpolation
    int readIdx = (int)readPos;
    float frac = readPos - (float)readIdx;
    int readIdx2 = (readIdx + 1) % CHORUS_BUFFER_SIZE;
    
    float delayed = ch->delayBuffer[readIdx] * (1.0f - frac) + 
                    ch->delayBuffer[readIdx2] * frac;
    
    // Pan each voice differently for stereo spread
    // Voice 0: center, Voice 1: left, Voice 2: right
    float panL, panR;
    if (v == 0) {
      panL = 0.7f; panR = 0.7f;
    } else if (v == 1) {
      panL = 0.9f; panR = 0.3f;
    } else {
      panL = 0.3f; panR = 0.9f;
    }
    
    wetL += delayed * panL;
    wetR += delayed * panR;
  }
  
  // Normalize
  wetL *= 0.5f;
  wetR *= 0.5f;
  
  // Mix dry and wet
  *left = *left * (1.0f - mix) + wetL * mix;
  *right = *right * (1.0f - mix) + wetR * mix;
  
  // Advance write position
  ch->writePos = (ch->writePos + 1) % CHORUS_BUFFER_SIZE;
}

// ===== FREEVERB IMPLEMENTATION =====

// Comb filter tunings (in samples at 44.1kHz, scaled for 48kHz)
static const int combTuningL[REVERB_COMB_COUNT] = {
  1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617
};
static const int combTuningR[REVERB_COMB_COUNT] = {
  1116 + 23, 1188 + 23, 1277 + 23, 1356 + 23, 
  1422 + 23, 1491 + 23, 1557 + 23, 1617 + 23
};

static const int allpassTuning[REVERB_ALLPASS_COUNT] = {
  556, 441, 341, 225
};

// Scale factor for 48kHz
#define REVERB_SCALE_FACTOR 1.0884f  // 48000/44100

static void initComb(ReverbComb* comb, int size) {
  // SAFETY: Use static buffer, clamp size to maximum
  comb->bufferSize = (size < MAX_COMB_SIZE) ? size : MAX_COMB_SIZE;
  // Clear static buffer
  for (int i = 0; i < comb->bufferSize; i++) {
    comb->buffer[i] = 0.0f;
  }
  comb->index = 0;
  comb->filterStore = 0.0f;
}

static void initAllpass(ReverbAllpass* ap, int size) {
  // SAFETY: Use static buffer, clamp size to maximum
  ap->bufferSize = (size < MAX_ALLPASS_SIZE) ? size : MAX_ALLPASS_SIZE;
  // Clear static buffer
  for (int i = 0; i < ap->bufferSize; i++) {
    ap->buffer[i] = 0.0f;
  }
  ap->index = 0;
}

void initFreeVerb(FreeVerb* rv) {
  // Initialize comb filters (static allocation)
  for (int i = 0; i < REVERB_COMB_COUNT; i++) {
    int sizeL = (int)(combTuningL[i] * REVERB_SCALE_FACTOR);
    int sizeR = (int)(combTuningR[i] * REVERB_SCALE_FACTOR);
    initComb(&rv->combL[i], sizeL);
    initComb(&rv->combR[i], sizeR);
  }
  
  // Initialize allpass filters (static allocation)
  for (int i = 0; i < REVERB_ALLPASS_COUNT; i++) {
    int size = (int)(allpassTuning[i] * REVERB_SCALE_FACTOR);
    initAllpass(&rv->allpassL[i], size);
    initAllpass(&rv->allpassR[i], size + 23);  // Slight stereo offset
  }
  
  // Predelay - STATIC allocation, just clear buffer
  rv->predelaySize = MAX_PREDELAY_SIZE;
  for (int i = 0; i < rv->predelaySize; i++) {
    rv->predelayBuffer[i] = 0.0f;
  }
  rv->predelayIndex = 0;
  rv->predelaySamples = 0;
  
  // Default parameters
  rv->roomSize = 0.5f;
  rv->damp = 0.5f;
  rv->width = 1.0f;
  rv->wet = 0.3f;
  rv->dry = 0.7f;
  
  rv->enabled = false;
  
  updateFreeVerbParams(rv, rv->roomSize, rv->damp, rv->width, rv->wet);
}

// No longer needed - static allocation
void freeFreeVerb(FreeVerb* rv) {
  // Nothing to free - all buffers are statically allocated
  (void)rv;  // Suppress unused parameter warning
}

void updateFreeVerbParams(FreeVerb* rv, float roomSize, float damp, float width, float mix) {
  rv->roomSize = roomSize;
  rv->damp = damp;
  rv->width = width;
  rv->wet = mix;
  rv->dry = 1.0f - mix;
  
  // Calculate feedback from room size
  rv->feedback = roomSize * 0.28f + 0.7f;  // Range: 0.7 - 0.98
  
  // Damping coefficients
  rv->damp1 = damp * 0.4f;
  rv->damp2 = 1.0f - rv->damp1;
}

static inline float processComb(ReverbComb* comb, float input, float feedback, float damp1, float damp2) {
  // SAFETY: bounds check index
  if (comb->index >= comb->bufferSize) comb->index = 0;
  
  float output = comb->buffer[comb->index];
  
  // One-pole lowpass (damping)
  comb->filterStore = output * damp2 + comb->filterStore * damp1;
  
  // Write back with feedback
  comb->buffer[comb->index] = input + comb->filterStore * feedback;
  
  // Advance index with bounds check
  comb->index = (comb->index + 1) % comb->bufferSize;
  
  return output;
}

static inline float processAllpass(ReverbAllpass* ap, float input) {
  // SAFETY: bounds check index
  if (ap->index >= ap->bufferSize) ap->index = 0;
  
  float bufOut = ap->buffer[ap->index];
  float output = -input + bufOut;
  
  ap->buffer[ap->index] = input + bufOut * 0.5f;
  
  // Advance index with bounds check
  ap->index = (ap->index + 1) % ap->bufferSize;
  
  return output;
}

void processFreeVerb(FreeVerb* rv, float inputL, float inputR, float* outL, float* outR) {
  if (!rv->enabled) {
    *outL = inputL;
    *outR = inputR;
    return;
  }
  
  // Mono input for reverb
  float input = (inputL + inputR) * 0.5f;
  
  // Predelay (static buffer)
  float predelayed = input;
  if (rv->predelaySamples > 0) {
    // SAFETY: bounds check
    int safeDelay = (rv->predelaySamples < rv->predelaySize) ? 
                     rv->predelaySamples : rv->predelaySize - 1;
    if (rv->predelayIndex >= rv->predelaySize) rv->predelayIndex = 0;
    
    predelayed = rv->predelayBuffer[rv->predelayIndex];
    rv->predelayBuffer[rv->predelayIndex] = input;
    rv->predelayIndex = (rv->predelayIndex + 1) % rv->predelaySize;
  }
  
  input = predelayed * 0.015f;  // Attenuate input
  
  // Parallel comb filters
  float outCombL = 0.0f;
  float outCombR = 0.0f;
  
  for (int i = 0; i < REVERB_COMB_COUNT; i++) {
    outCombL += processComb(&rv->combL[i], input, rv->feedback, rv->damp1, rv->damp2);
    outCombR += processComb(&rv->combR[i], input, rv->feedback, rv->damp1, rv->damp2);
  }
  
  // Series allpass filters
  for (int i = 0; i < REVERB_ALLPASS_COUNT; i++) {
    outCombL = processAllpass(&rv->allpassL[i], outCombL);
    outCombR = processAllpass(&rv->allpassR[i], outCombR);
  }
  
  // Stereo width processing
  float wet1 = rv->wet * (rv->width * 0.5f + 0.5f);
  float wet2 = rv->wet * ((1.0f - rv->width) * 0.5f);
  
  // Mix
  *outL = inputL * rv->dry + outCombL * wet1 + outCombR * wet2;
  *outR = inputR * rv->dry + outCombR * wet1 + outCombL * wet2;
}

// ===== STEREO WIDENER =====

void initStereoWidener(StereoWidener* sw) {
  sw->width = 1.0f;
  sw->lastMid = 0.0f;
  sw->lastSide = 0.0f;
}

void processStereoWidener(StereoWidener* sw, float* left, float* right, float width) {
  // M/S processing
  float mid = (*left + *right) * 0.5f;
  float side = (*left - *right) * 0.5f;
  
  // Apply width
  side *= width;
  
  // Convert back to L/R
  *left = mid + side;
  *right = mid - side;
}

// ===== GLOBAL LFO =====

void initGlobalLFO(GlobalLFO* lfo) {
  lfo->phase = 0.0f;
  lfo->rate = 1.0f;
  lfo->waveform = 0;
  lfo->output = 0.0f;
  lfo->smoothedOutput = 0.0f;
}

float processGlobalLFO(GlobalLFO* lfo, float rate, int waveform) {
  lfo->rate = rate;
  lfo->waveform = waveform;
  
  // Update phase
  lfo->phase += rate / SAMPLE_RATE_F;
  if (lfo->phase >= 1.0f) lfo->phase -= 1.0f;
  
  float output = 0.0f;
  
  switch (waveform) {
    case 0:  // Sine
      output = fastSin(lfo->phase * 6.283185f);
      break;
      
    case 1:  // Triangle
      if (lfo->phase < 0.5f) {
        output = 4.0f * lfo->phase - 1.0f;
      } else {
        output = 3.0f - 4.0f * lfo->phase;
      }
      break;
      
    case 2:  // Sawtooth
      output = 2.0f * lfo->phase - 1.0f;
      break;
      
    case 3:  // Square
      output = (lfo->phase < 0.5f) ? 1.0f : -1.0f;
      break;
  }
  
  // Smooth for parameter modulation
  lfo->smoothedOutput += (output - lfo->smoothedOutput) * 0.01f;
  
  lfo->output = output;
  return output;
}

// ===== LIMITER =====

void initLimiter(Limiter* lim, float threshold) {
  lim->envelope = 0.0f;
  lim->attack = 0.001f * SAMPLE_RATE_F;    // 1ms
  lim->release = 0.1f * SAMPLE_RATE_F;     // 100ms
  lim->threshold = threshold;
}

float processLimiter(Limiter* lim, float input) {
  float absInput = fabsf(input);
  
  // Envelope follower
  if (absInput > lim->envelope) {
    lim->envelope += (absInput - lim->envelope) / lim->attack;
  } else {
    lim->envelope += (absInput - lim->envelope) / lim->release;
  }
  
  // Calculate gain reduction
  float gain = 1.0f;
  if (lim->envelope > lim->threshold) {
    gain = lim->threshold / lim->envelope;
  }
  
  return input * gain;
}
