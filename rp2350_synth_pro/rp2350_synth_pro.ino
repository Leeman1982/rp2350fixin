/*
 * RP2350 MIDI Synthesizer - PROFESSIONAL EDITION v2.0
 * 
 * MAJOR ENHANCEMENTS:
 * - 8-voice polyphony with intelligent voice stealing
 * - Sub-oscillator per voice (Jupiter-8 style)
 * - 2x oversampled Moog ladder filter
 * - Ring modulation between oscillators
 * - Unison mode with detune spread (up to 4 voices)
 * - Symphonic chorus (Roland Dimension D style)
 * - FreeVerb algorithmic reverb
 * - Parameter smoothing (no zipper noise)
 * - DC blocking filters
 * - Comprehensive MIDI CC mapping (60+ CCs)
 * - 12 professional presets
 * - Dual-core architecture (Core 1 = Audio, Core 0 = MIDI/UI)
 * 
 * Hardware:
 * - RP2350 @ 200MHz (overclock recommended)
 * - I2S DAC on pins 20/21/22
 * - MIDI input on GPIO1
 * 
 * CC MAPPING - See bottom of file for complete list
 */

#include <MIDI.h>
#include "AudioTools.h"
#include "synth_voice_pro.h"
#include "synth_params_pro.h"
#include "synth_effects_pro.h"
#include <pico/multicore.h>

// ===== AUDIO CONFIGURATION =====
const int CHANNELS = 2;
const int BITS_PER_SAMPLE = 16;
const int DMA_BUFFER_SIZE = 128;  // Smaller for lower latency

I2SStream i2s;
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

// ===== VOICES =====
#define VOICES 8
Voice voices[VOICES];
NoiseGen noiseGen;

// ===== EFFECTS =====
SymphonicChorus chorus;
FreeVerb reverb;
GlobalLFO globalLFO;
Limiter outputLimiter;

// ===== VOICE COMMAND QUEUE =====
#define CMD_QUEUE_SIZE 64

enum VoiceCommand {
  CMD_NOTE_ON,
  CMD_NOTE_OFF,
  CMD_RELEASE_ALL,
  CMD_PARAM_CHANGE
};

struct VoiceCommandMsg {
  VoiceCommand cmd;
  int voiceIndex;
  int note;
  int velocity;
  float frequency;
  bool triggerEnv;
  int paramIndex;
  float paramValue;
};

volatile VoiceCommandMsg cmdQueue[CMD_QUEUE_SIZE];
volatile int cmdQueueHead = 0;
volatile int cmdQueueTail = 0;

// ===== MONO NOTE STACK =====
int monoNoteStack[16];
int monoStackSize = 0;

// ===== MODULATION =====
volatile float modWheelValue = 0.0f;
volatile float pitchBendValue = 0.0f;
volatile float aftertouch = 0.0f;
const float PITCH_BEND_RANGE = 2.0f;  // Semitones

// ===== PARAMETER SMOOTHERS (global) =====
ParamSmoother smoothCutoff;
ParamSmoother smoothResonance;
ParamSmoother smoothVolume;

// ===== ARPEGGIATOR =====
#define ARP_MAX_NOTES 16
#define ARP_NUM_PATTERNS 50

bool arpEnabled = false;
int arpPattern = 0;
float arpRate = 120.0f;
float arpGate = 0.8f;
float arpSwing = 0.0f;
int arpOctaves = 1;
int arpDirection = 0;
bool arpLatch = false;

int arpNoteBuffer[ARP_MAX_NOTES];
int arpNoteCount = 0;
int arpCurrentStep = 0;
bool arpNoteActive = false;
unsigned long arpStepStartTime = 0;
int arpPlayingNote = -1;

// Classic arpeggio patterns
const int8_t arpPatterns[ARP_NUM_PATTERNS][16] = {
  {0, 4, 7, 12, 7, 4, 0, 0, 0, 4, 7, 12, 7, 4, 0, 0},   // Major arp
  {0, 3, 7, 12, 7, 3, 0, 0, 0, 3, 7, 12, 7, 3, 0, 0},   // Minor arp
  {0, 5, 7, 12, 7, 5, 0, 0, 0, 5, 7, 12, 7, 5, 0, 0},   // Sus4 arp
  {0, 12, 0, 12, 0, 12, 0, 12, 0, 12, 0, 12, 0, 12, 0, 12}, // Octave
  {0, 7, 12, 7, 0, 7, 12, 7, 0, 7, 12, 7, 0, 7, 12, 7},  // Fifth
  {0, 4, 7, 4, 0, 4, 7, 4, 0, 4, 7, 4, 0, 4, 7, 4},     // Major triad
  {0, 3, 7, 3, 0, 3, 7, 3, 0, 3, 7, 3, 0, 3, 7, 3},     // Minor triad
  {0, 0, 12, 12, 7, 7, 12, 12, 0, 0, 12, 12, 7, 7, 12, 12},
  {0, 0, 0, 12, 0, 0, 0, 12, 0, 0, 0, 12, 0, 0, 0, 12},
  {0, 7, 0, 5, 0, 7, 0, 5, 0, 7, 0, 5, 0, 7, 0, 5},
  // ... more patterns
  {0, 12, 24, 12, 0, 12, 24, 12, 0, 12, 24, 12, 0, 12, 24, 12},
  {0, 4, 7, 12, 16, 12, 7, 4, 0, 4, 7, 12, 16, 12, 7, 4},
  {0, 3, 7, 10, 12, 10, 7, 3, 0, 3, 7, 10, 12, 10, 7, 3},
  {0, 12, 7, 19, 12, 24, 19, 12, 7, 12, 0, 7, 12, 7, 0, 0},
  {0, 0, 7, 7, 12, 12, 7, 7, 0, 0, 7, 7, 12, 12, 7, 7},
  {0, 4, 0, 7, 0, 12, 0, 7, 0, 4, 0, 7, 0, 12, 0, 7},
  {0, 7, 12, 7, 0, -5, 0, -5, 0, 7, 12, 7, 0, -5, 0, -5},
  {0, 0, 12, 0, 7, 0, 12, 0, 0, 0, 12, 0, 7, 0, 12, 0},
  {0, 12, 7, 0, 12, 7, 0, 12, 7, 0, 12, 7, 0, 12, 7, 0},
  {0, 0, 0, 12, 7, 7, 7, 12, 0, 0, 0, 12, 7, 7, 7, 12},
  {0, 12, 24, 36, 24, 12, 0, 12, 24, 36, 24, 12, 0, 0, 0, 0},
  {0, 5, 12, 5, 0, 5, 12, 5, 0, 5, 12, 5, 0, 5, 12, 5},
  {0, 7, 14, 21, 14, 7, 0, 7, 14, 21, 14, 7, 0, 0, 0, 0},
  {0, 3, 6, 9, 12, 9, 6, 3, 0, 3, 6, 9, 12, 9, 6, 3},
  {0, 2, 4, 7, 9, 11, 12, 14, 12, 11, 9, 7, 4, 2, 0, 0},
  {0, 0, 0, 0, 12, 12, 12, 12, 7, 7, 7, 7, 12, 12, 12, 12},
  {0, 12, 0, 7, 0, 12, 0, 5, 0, 12, 0, 7, 0, 12, 0, 5},
  {0, 4, 7, 0, 4, 7, 12, 7, 4, 0, 7, 4, 0, 7, 12, 7},
  {12, 12, 0, 0, 7, 7, 0, 0, 12, 12, 0, 0, 7, 7, 0, 0},
  {0, 0, 12, 7, 0, 0, 12, 7, 0, 0, 12, 7, 0, 0, 12, 7},
  {0, 7, 12, 19, 12, 7, 0, 7, 12, 19, 12, 7, 0, 0, 0, 0},
  {0, 12, 7, 19, 0, 12, 7, 19, 24, 12, 19, 7, 24, 12, 19, 7},
  {0, 12, 7, 5, 0, 12, 7, 5, 0, 12, 7, 5, 0, 12, 7, 5},
  {0, 0, 7, 12, 0, 0, 7, 12, 0, 0, 7, 12, 0, 0, 7, 12},
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 11, 10, 9},
  {0, 12, 0, 12, 7, 12, 7, 12, 0, 12, 0, 12, 7, 12, 7, 12},
  {0, 4, 7, 12, 7, 4, 0, -5, 0, 4, 7, 12, 7, 4, 0, -5},
  {0, 7, 0, 12, 0, 7, 0, 19, 0, 7, 0, 12, 0, 7, 0, 19},
  {0, 12, 7, 12, 0, 12, 7, 12, 5, 12, 7, 12, 5, 12, 7, 12},
  {0, 2, 3, 5, 7, 8, 10, 12, 10, 8, 7, 5, 3, 2, 0, 0},
  {0, 0, 0, 0, 12, 12, 12, 12, 24, 24, 24, 24, 12, 12, 0, 0},
  {0, 0, 12, 0, 7, 0, 12, 0, 5, 0, 12, 0, 7, 0, 12, 0},
  {0, 12, 24, 12, 0, 0, 12, 0, 0, 12, 24, 12, 0, 0, 12, 0},
  {0, 3, 7, 12, 15, 12, 7, 3, 0, 3, 7, 12, 15, 12, 7, 3},
  {0, 4, 7, 11, 12, 11, 7, 4, 0, 4, 7, 11, 12, 11, 7, 4},
  {0, 12, 7, 19, 24, 19, 7, 12, 0, 12, 7, 19, 24, 19, 7, 12},
  {0, 5, 7, 12, 7, 5, 0, 5, 7, 12, 7, 5, 0, 5, 7, 12},
  {0, 12, 7, 0, 0, 12, 7, 0, 0, 12, 7, 0, 0, 12, 7, 0},
  {0, 0, 12, 12, 0, 0, 7, 7, 0, 0, 12, 12, 0, 0, 7, 7},
  {0, 12, 0, 7, 0, 5, 0, 7, 0, 12, 0, 7, 0, 5, 0, 7}
};

volatile bool core1Running = false;

// ===== FUNCTION DECLARATIONS =====
void generateAudioBlock(int16_t* buffer, int samples);
float processVoiceSample(Voice* v, float noiseInput, float lfoValue, float pitchMod);
int allocateVoice();
int findVoice(int note);
void noteOn(int note, int velocity);
void noteOff(int note);

// ===== COMMAND QUEUE =====
bool pushCommand(VoiceCommand cmd, int voiceIndex, int note, int velocity, 
                 float freq, bool trigEnv, int paramIdx = 0, float paramVal = 0.0f) {
  int nextHead = (cmdQueueHead + 1) % CMD_QUEUE_SIZE;
  if (nextHead == cmdQueueTail) return false;  // Queue full
  
  cmdQueue[cmdQueueHead].cmd = cmd;
  cmdQueue[cmdQueueHead].voiceIndex = voiceIndex;
  cmdQueue[cmdQueueHead].note = note;
  cmdQueue[cmdQueueHead].velocity = velocity;
  cmdQueue[cmdQueueHead].frequency = freq;
  cmdQueue[cmdQueueHead].triggerEnv = trigEnv;
  cmdQueue[cmdQueueHead].paramIndex = paramIdx;
  cmdQueue[cmdQueueHead].paramValue = paramVal;
  
  __dmb();
  cmdQueueHead = nextHead;
  return true;
}

bool popCommand(VoiceCommandMsg* msg) {
  if (cmdQueueTail == cmdQueueHead) return false;
  
  // Copy field by field from volatile struct to avoid compiler error
  const volatile VoiceCommandMsg* src = &cmdQueue[cmdQueueTail];
  msg->cmd = src->cmd;
  msg->voiceIndex = src->voiceIndex;
  msg->note = src->note;
  msg->velocity = src->velocity;
  msg->frequency = src->frequency;
  msg->triggerEnv = src->triggerEnv;
  msg->paramIndex = src->paramIndex;
  msg->paramValue = src->paramValue;
  
  __dmb();
  cmdQueueTail = (cmdQueueTail + 1) % CMD_QUEUE_SIZE;
  return true;
}

// ===== CORE 1: AUDIO PROCESSING =====
void core1AudioLoop() {
  core1Running = true;
  int16_t audioBuffer[DMA_BUFFER_SIZE * 2];
  
  while (true) {
    // Process up to 16 commands per audio block
    VoiceCommandMsg cmd;
    int cmdCount = 0;
    
    while (popCommand(&cmd) && cmdCount < 16) {
      cmdCount++;
      
      switch (cmd.cmd) {
        case CMD_NOTE_ON: {
          Voice* v = &voices[cmd.voiceIndex];
          v->note = cmd.note;
          v->velocity = cmd.velocity;
          v->velocityFloat = cmd.velocity / 127.0f;
          v->active = true;
          v->noteOnTime = millis();
          v->stealPriority = cmd.velocity;
          v->targetFreq = cmd.frequency;
          v->drive = drive;
          v->filterKeyTrack = filterKeyTrack;
          
          // Sub-oscillator setup
          v->subOsc.enabled = subEnabled;
          v->subOsc.waveform = subWave;
          v->subOsc.octave = subOctave;
          v->subOsc.level = subVol;
          
          // Ring mod setup
          v->ringMod.enabled = ringEnabled;
          v->ringMod.amount = ringAmount;
          
          // Unison setup
          v->unisonCount = unisonCount;
          v->unisonDetune = unisonDetune;
          v->unisonSpread = unisonSpread;
          
          // Calculate unison detune amounts
          for (int u = 0; u < MAX_UNISON; u++) {
            if (u < v->unisonCount) {
              float spread = (float)(u - (v->unisonCount - 1) / 2.0f) / 
                            fmaxf(1.0f, (float)(v->unisonCount - 1));
              v->unisonVoices[u].detuneAmount = spread * v->unisonDetune;
              v->unisonVoices[u].pan = spread * v->unisonSpread;
            }
          }
          
          // Portamento
          if (glideTime > 0.005f && v->currentFreq > 20.0f) {
            v->gliding = true;
          } else {
            v->currentFreq = cmd.frequency;
            v->gliding = false;
          }
          
          // Update oscillator frequencies
          v->osc1.frequency = v->currentFreq * osc1Range * osc1Fine;
          v->osc2.frequency = v->currentFreq * osc2Range * osc2Fine;
          v->osc3.frequency = v->currentFreq * osc3Range * osc3Fine;
          v->osc1.waveform = osc1Wave;
          v->osc2.waveform = osc2Wave;
          v->osc3.waveform = osc3Wave;
          
          // Trigger envelopes
          if (cmd.triggerEnv) {
            triggerEnvelope(&v->ampEnv, ampAttack, ampDecay, ampSustain, ampRelease);
            triggerEnvelope(&v->filtEnv, filtAttack, filtDecay, filtSustain, filtRelease);
            resetVoice(v);
          }
          break;
        }
        
        case CMD_NOTE_OFF: {
          Voice* v = &voices[cmd.voiceIndex];
          releaseEnvelope(&v->ampEnv);
          releaseEnvelope(&v->filtEnv);
          v->active = false;
          break;
        }
        
        case CMD_RELEASE_ALL:
          for (int i = 0; i < VOICES; i++) {
            releaseEnvelope(&voices[i].ampEnv);
            releaseEnvelope(&voices[i].filtEnv);
            voices[i].active = false;
          }
          break;
          
        case CMD_PARAM_CHANGE:
          // Handle real-time parameter changes here if needed
          break;
      }
    }
    
    // Generate audio block
    generateAudioBlock(audioBuffer, DMA_BUFFER_SIZE);
    i2s.write((uint8_t*)audioBuffer, DMA_BUFFER_SIZE * 2 * sizeof(int16_t));
  }
}

void generateAudioBlock(int16_t* buffer, int samples) {
  // Get current modulation values
  const float modWheel = modWheelValue;
  const float pitchBend = pitchBendValue;
  
  // Master tune + pitch bend
  const float masterTuneMult = fastPow2(masterTune / 1200.0f);
  const float pitchMult = fastPow2(pitchBend * PITCH_BEND_RANGE / 12.0f) * masterTuneMult;
  
  // Process parameter smoothers
  float smoothedCutoff = processSmoother(&smoothCutoff);
  float smoothedResonance = processSmoother(&smoothResonance);
  float smoothedVolume = processSmoother(&smoothVolume);
  
  for (int i = 0; i < samples; i++) {
    float mixL = 0.0f;
    float mixR = 0.0f;
    
    // Process global LFO
    float lfoValue = processGlobalLFO(&globalLFO, lfoRate, lfoWave);
    
    // Generate noise
    float noiseOut = 0.0f;
    if (noiseVol > 0.001f) {
      noiseOut = (noiseType == 0) ? 
        generateWhiteNoise(&noiseGen) : generatePinkNoise(&noiseGen);
      noiseOut *= noiseVol;
    }
    
    // Calculate LFO modulations
    float lfoMod = 0.0f;
    if (lfoEnabled && lfoDepth > 0.01f) {
      lfoMod = lfoValue * lfoDepth;
    }
    // Add mod wheel contribution
    lfoMod += modWheel * 0.5f;
    
    // Process all voices
    int activeVoices = 0;
    for (int vi = 0; vi < VOICES; vi++) {
      Voice* v = &voices[vi];
      
      // Skip completely idle voices
      if (!v->active && isEnvelopeIdle(&v->ampEnv)) continue;
      
      float voiceOut = processVoiceSample(v, noiseOut, lfoMod, pitchMult);
      
      // Apply per-voice panning from unison
      if (v->unisonCount > 1 && v->unisonSpread > 0.01f) {
        // Slight stereo variation based on voice index
        float panMod = (float)(vi % 4 - 1.5f) * 0.1f;
        mixL += voiceOut * (0.5f - panMod * 0.3f);
        mixR += voiceOut * (0.5f + panMod * 0.3f);
      } else {
        mixL += voiceOut;
        mixR += voiceOut;
      }
      activeVoices++;
    }
    
    // Voice scaling
    if (activeVoices > 0) {
      float scale = 0.7f / sqrtf((float)activeVoices);
      mixL *= scale;
      mixR *= scale;
    }
    
    // Apply chorus
    if (chorusMix > 0.01f) {
      chorus.enabled = true;
      processSymphonicChorus(&chorus, &mixL, &mixR, 
                              chorusRate, chorusDepth, chorusFeedback, chorusMix);
    }
    
    // Apply reverb
    if (reverbMix > 0.01f) {
      reverb.enabled = true;
      reverb.predelaySamples = (int)(reverbPredelay * SAMPLE_RATE_F / 1000.0f);
      processFreeVerb(&reverb, mixL, mixR, &mixL, &mixR);
    }
    
    // Apply master volume
    mixL *= smoothedVolume;
    mixR *= smoothedVolume;
    
    // Output limiting
    mixL = processLimiter(&outputLimiter, mixL);
    mixR = fastTanh(mixR * 0.95f);  // Simple soft clip for R
    
    // Convert to 16-bit
    buffer[i * 2] = (int16_t)(mixL * 32000.0f);
    buffer[i * 2 + 1] = (int16_t)(mixR * 32000.0f);
  }
}

float processVoiceSample(Voice* v, float noiseInput, float lfoMod, float pitchMult) {
  // Exponential glide with SAFETY checks
  if (v->gliding && glideTime > 0.005f) {
    float glideSpeed = 10.0f / (glideTime * SAMPLE_RATE_F);
    // SAFETY: prevent log(0) by clamping minimum frequency
    float safeCurrentFreq = fmaxf(v->currentFreq, 1.0f);
    float safeTargetFreq = fmaxf(v->targetFreq, 1.0f);
    float logCurrent = logf(safeCurrentFreq);
    float logTarget = logf(safeTargetFreq);
    float diff = logTarget - logCurrent;
    
    if (fabsf(diff) < 0.001f) {
      v->currentFreq = v->targetFreq;
      v->gliding = false;
    } else {
      logCurrent += diff * glideSpeed;
      v->currentFreq = expf(logCurrent);
      // SAFETY: clamp to audible range
      v->currentFreq = fmaxf(1.0f, fminf(v->currentFreq, 20000.0f));
    }
  }
  
  // LFO modulation destinations
  float pitchLFO = 1.0f;
  float filterLFO = 1.0f;
  float ampLFO = 1.0f;
  
  if (lfoMod > 0.001f) {
    switch (lfoTarget) {
      case 0:  // Pitch (vibrato)
        pitchLFO = 1.0f + globalLFO.smoothedOutput * lfoMod * 0.05f;
        break;
      case 1:  // Filter
        filterLFO = fastPow2(globalLFO.smoothedOutput * lfoMod * 2.0f);
        break;
      case 2:  // Amplitude (tremolo)
        ampLFO = 1.0f - globalLFO.smoothedOutput * lfoMod * 0.4f;
        break;
    }
  }
  
  // Calculate oscillator frequencies with modulation
  float basePitch = pitchMult * pitchLFO;
  v->osc1.frequency = v->currentFreq * osc1Range * osc1Fine * basePitch;
  v->osc2.frequency = v->currentFreq * osc2Range * osc2Fine * basePitch;
  v->osc3.frequency = v->currentFreq * osc3Range * osc3Fine * basePitch;
  
  // Generate oscillators
  float osc1Out = 0.0f, osc2Out = 0.0f, osc3Out = 0.0f;
  
  if (v->unisonCount > 1) {
    // Unison mode - stack multiple slightly detuned oscillators
    for (int u = 0; u < v->unisonCount; u++) {
      float detuneMult = fastPow2(v->unisonVoices[u].detuneAmount / 1200.0f);
      
      // Detune and generate each unison voice
      v->osc1.frequency = v->currentFreq * osc1Range * osc1Fine * basePitch * detuneMult;
      osc1Out += generateOscillator(&v->osc1) * vol1 / (float)v->unisonCount;
      
      v->osc2.frequency = v->currentFreq * osc2Range * osc2Fine * basePitch * detuneMult;
      osc2Out += generateOscillator(&v->osc2) * vol2 / (float)v->unisonCount;
    }
    // OSC3 without unison for differentiation
    osc3Out = generateOscillator(&v->osc3) * vol3;
  } else {
    osc1Out = generateOscillator(&v->osc1) * vol1;
    osc2Out = generateOscillator(&v->osc2) * vol2;
    osc3Out = generateOscillator(&v->osc3) * vol3;
  }
  
  // Ring modulation
  float oscMix;
  if (v->ringMod.enabled && v->ringMod.amount > 0.01f) {
    float modulator = (v->ringMod.sourceOsc == 1) ? osc1Out : osc2Out;
    float carrier = osc1Out + osc2Out + osc3Out;
    float ringOut = carrier * modulator * 4.0f;  // Boost ring mod
    oscMix = carrier * (1.0f - v->ringMod.amount) + ringOut * v->ringMod.amount;
  } else {
    oscMix = osc1Out + osc2Out + osc3Out;
  }
  
  // Sub-oscillator
  oscMix += generateSubOscillator(&v->subOsc, v->currentFreq);
  
  // Add noise
  oscMix += noiseInput;
  
  // Apply drive/saturation BEFORE filter
  if (v->drive > 1.01f) {
    oscMix = processDrive(oscMix, v->drive);
  }
  
  // Process envelopes
  float ampEnvOut = processEnvelope(&v->ampEnv);
  if (ampEnvOut < 0.0001f && isEnvelopeIdle(&v->ampEnv)) return 0.0f;
  
  float filtEnvOut = processEnvelope(&v->filtEnv);
  
  // Calculate filter cutoff with modulations
  float filterCutoff = cutoff;
  
  // Filter envelope
  float envOctaves = filtEnvOut * filterStrength * 5.0f;
  filterCutoff *= fastPow2(envOctaves);
  
  // LFO modulation
  filterCutoff *= filterLFO;
  
  // Keyboard tracking
  if (fabsf(v->filterKeyTrack) > 0.01f) {
    float noteOffset = (v->note - 60) / 12.0f;
    filterCutoff *= fastPow2(noteOffset * v->filterKeyTrack);
  }
  
  // Velocity modulation
  if (velToFilter > 0.01f) {
    float velMod = 1.0f + (v->velocityFloat - 0.5f) * velToFilter * 2.0f;
    filterCutoff *= velMod;
  }
  
  // Clamp filter
  filterCutoff = fmaxf(20.0f, fminf(filterCutoff, 20000.0f));
  
  // Update filter coefficients
  updateFilterCoeffs(&v->filter, filterCutoff, resonance);
  
  // Process through Moog filter
  float voiceOut = processMoogFilter(&v->filter, oscMix);
  
  // Apply amplitude envelope
  float velAmpMod = 1.0f - velToAmp + velToAmp * v->velocityFloat;
  voiceOut *= ampEnvOut * velAmpMod;
  
  // LFO amplitude modulation
  voiceOut *= ampLFO;
  
  // DC blocking
  voiceOut = processDCBlocker(&v->dcBlock, voiceOut);
  
  return voiceOut;
}

// ===== CORE 0: MIDI & CONTROL =====

void setup() {
  Serial.begin(115200);
  delay(1500);
  
  Serial.println("\n========================================");
  Serial.println("  RP2350 PRO SYNTH v2.0");
  Serial.println("  8-Voice Moog-Style Polyphonic Synth");
  Serial.println("========================================");
  Serial.println("Features: Sub-Osc, Ring Mod, Unison,");
  Serial.println("          Symphonic Chorus, FreeVerb");
  Serial.println("========================================\n");
  
  // Initialize MIDI
  Serial1.setRX(1);
  Serial1.setTX(0);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
  
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handleControlChange);
  MIDI.setHandlePitchBend(handlePitchBend);
  MIDI.setHandleProgramChange(handleProgramChange);
  MIDI.setHandleAfterTouchChannel(handleAftertouch);
  
  // Initialize I2S
  auto config = i2s.defaultConfig(TX_MODE);
  config.sample_rate = SAMPLE_RATE;
  config.channels = CHANNELS;
  config.bits_per_sample = BITS_PER_SAMPLE;
  config.pin_bck = 20;
  config.pin_ws = 21;
  config.pin_data = 22;
  config.i2s_format = I2S_STD_FORMAT;
  config.buffer_size = 256;
  config.buffer_count = 4;
  
  if (!i2s.begin(config)) {
    Serial.println("I2S INIT FAILED!");
    while(1) delay(1000);
  }
  
  // Initialize voices
  for (int i = 0; i < VOICES; i++) {
    initVoice(&voices[i]);
    voices[i].currentFreq = 440.0f;
  }
  
  // Initialize noise generator
  initNoiseGen(&noiseGen);
  
  // Initialize effects
  initSymphonicChorus(&chorus);
  initFreeVerb(&reverb);
  initGlobalLFO(&globalLFO);
  initLimiter(&outputLimiter, 0.95f);
  
  // Initialize parameter smoothers
  initSmoother(&smoothCutoff, cutoff, 10.0f);
  initSmoother(&smoothResonance, resonance, 10.0f);
  initSmoother(&smoothVolume, masterVol, 20.0f);
  
  // Initialize parameters
  initParameters();
  loadPreset(0);
  
  // Update reverb parameters
  updateFreeVerbParams(&reverb, reverbSize, reverbDamp, reverbWidth, reverbMix);
  
  // Launch audio core
  multicore_launch_core1(core1AudioLoop);
  while (!core1Running) delay(10);
  
  Serial.println("Synth ready! Play some notes.");
  Serial.println("Program Change 0-11 for presets");
}

void loop() {
  MIDI.read();
  arpProcess();
  delayMicroseconds(100);
}

// ===== MIDI HANDLERS =====

void handleNoteOn(byte channel, byte note, byte velocity) {
  if (velocity == 0) {
    handleNoteOff(channel, note, 0);
    return;
  }
  
  if (arpEnabled) {
    arpAddNote(note, velocity);
  } else {
    noteOn(note, velocity);
  }
}

void handleNoteOff(byte channel, byte note, byte velocity) {
  if (arpEnabled) {
    arpRemoveNote(note);
  } else {
    noteOff(note);
  }
}

void handleAftertouch(byte channel, byte pressure) {
  aftertouch = pressure / 127.0f;
}

void handlePitchBend(byte channel, int bend) {
  pitchBendValue = (bend - 8192) * 0.000122070312f;  // -1 to +1
}

void handleProgramChange(byte channel, byte program) {
  if (program < NUM_PRESETS) {
    loadPreset(program);
    updateFreeVerbParams(&reverb, reverbSize, reverbDamp, reverbWidth, reverbMix);
    setSmootherTarget(&smoothVolume, masterVol);
  }
}

void handleControlChange(byte channel, byte cc, byte value) {
  float n = midiCCToNormalized(value);
  
  switch (cc) {
    // Standard CCs
    case 1:  modWheelValue = n; break;
    case 7:  setParameter(PARAM_MASTER_VOL, n); setSmootherTarget(&smoothVolume, masterVol); break;
    case 10: /* Pan - could implement */ break;
    case 11: setParameter(PARAM_CUTOFF, n); setSmootherTarget(&smoothCutoff, cutoff); break;
    
    // Oscillator section (14-25)
    case 14: setParameter(PARAM_OSC1_RANGE, n); break;
    case 15: setParameter(PARAM_OSC2_RANGE, n); break;
    case 16: setParameter(PARAM_OSC3_RANGE, n); break;
    case 17: setParameter(PARAM_OSC1_FINE, n); break;
    case 18: setParameter(PARAM_OSC2_FINE, n); break;
    case 19: setParameter(PARAM_OSC3_FINE, n); break;
    case 20: setParameter(PARAM_OSC1_WAVE, n); break;
    case 21: setParameter(PARAM_OSC2_WAVE, n); break;
    case 22: setParameter(PARAM_OSC3_WAVE, n); break;
    case 23: setParameter(PARAM_OSC1_VOL, n); break;
    case 24: setParameter(PARAM_OSC2_VOL, n); break;
    case 25: setParameter(PARAM_OSC3_VOL, n); break;
    
    // Sub-oscillator (26-29)
    case 26: setParameter(PARAM_SUB_ENABLED, n); break;
    case 27: setParameter(PARAM_SUB_WAVE, n); break;
    case 28: setParameter(PARAM_SUB_OCTAVE, n); break;
    case 29: setParameter(PARAM_SUB_VOL, n); break;
    
    // Ring mod (30-31)
    case 30: setParameter(PARAM_RING_ENABLED, n); break;
    case 31: setParameter(PARAM_RING_AMOUNT, n); break;
    
    // Filter section (70-79)
    case 70: setParameter(PARAM_CUTOFF, n); setSmootherTarget(&smoothCutoff, cutoff); break;
    case 71: setParameter(PARAM_RESONANCE, n); setSmootherTarget(&smoothResonance, resonance); break;
    case 72: setParameter(PARAM_FILT_ATTACK, n); break;
    case 73: setParameter(PARAM_FILT_DECAY, n); break;
    case 74: setParameter(PARAM_FILT_SUSTAIN, n); break;
    case 75: setParameter(PARAM_FILT_RELEASE, n); break;
    case 76: setParameter(PARAM_FILT_STRENGTH, n); break;
    case 77: setParameter(PARAM_FILT_KEY_TRACK, n); break;
    case 78: setParameter(PARAM_DRIVE, n); break;
    case 79: setParameter(PARAM_VEL_TO_FILT, n); break;
    
    // Amp envelope (80-84)
    case 80: setParameter(PARAM_AMP_ATTACK, n); break;
    case 81: setParameter(PARAM_AMP_DECAY, n); break;
    case 82: setParameter(PARAM_AMP_SUSTAIN, n); break;
    case 83: setParameter(PARAM_AMP_RELEASE, n); break;
    case 84: setParameter(PARAM_VEL_TO_AMP, n); break;
    
    // LFO (85-89)
    case 85: setParameter(PARAM_LFO_RATE, n); break;
    case 86: setParameter(PARAM_LFO_DEPTH, n); break;
    case 87: setParameter(PARAM_LFO_WAVE, n); break;
    case 88: setParameter(PARAM_LFO_TARGET, n); break;
    case 89: setParameter(PARAM_LFO_ENABLED, n); break;
    
    // Unison (90-92)
    case 90: setParameter(PARAM_UNISON_COUNT, n); break;
    case 91: setParameter(PARAM_UNISON_DETUNE, n); break;
    case 92: setParameter(PARAM_UNISON_SPREAD, n); break;
    
    // Chorus (102-105)
    case 102: setParameter(PARAM_CHORUS_RATE, n); break;
    case 103: setParameter(PARAM_CHORUS_DEPTH, n); break;
    case 104: setParameter(PARAM_CHORUS_FEEDBACK, n); break;
    case 105: setParameter(PARAM_CHORUS_MIX, n); break;
    
    // Reverb (106-110)
    case 106: 
      setParameter(PARAM_REVERB_SIZE, n); 
      updateFreeVerbParams(&reverb, reverbSize, reverbDamp, reverbWidth, reverbMix);
      break;
    case 107: 
      setParameter(PARAM_REVERB_DAMP, n); 
      updateFreeVerbParams(&reverb, reverbSize, reverbDamp, reverbWidth, reverbMix);
      break;
    case 108: 
      setParameter(PARAM_REVERB_WIDTH, n); 
      updateFreeVerbParams(&reverb, reverbSize, reverbDamp, reverbWidth, reverbMix);
      break;
    case 109: 
      setParameter(PARAM_REVERB_MIX, n); 
      updateFreeVerbParams(&reverb, reverbSize, reverbDamp, reverbWidth, reverbMix);
      break;
    case 110: setParameter(PARAM_REVERB_PREDELAY, n); break;
    
    // Noise (111-112)
    case 111: setParameter(PARAM_NOISE_VOL, n); break;
    case 112: setParameter(PARAM_NOISE_TYPE, n); break;
    
    // Global (113-115)
    case 113: setParameter(PARAM_PLAY_MODE, n); break;
    case 114: setParameter(PARAM_GLIDE_TIME, n); break;
    case 115: setParameter(PARAM_MASTER_TUNE, n); break;
    
    // Arpeggiator (116-123)
    case 116:
      arpEnabled = (value >= 64);
      if (!arpEnabled && arpNoteActive) {
        noteOff(arpPlayingNote);
        arpNoteActive = false;
        arpPlayingNote = -1;
      }
      break;
    case 117: arpPattern = constrain((int)(n * ARP_NUM_PATTERNS), 0, ARP_NUM_PATTERNS - 1); break;
    case 118: arpRate = 40.0f + n * 260.0f; break;
    case 119: arpGate = 0.1f + n * 0.9f; break;
    case 120: /* All sound off */ pushCommand(CMD_RELEASE_ALL, 0, 0, 0, 0, false); break;
    case 121: arpSwing = n * 0.6f; break;
    case 122: arpOctaves = 1 + (int)(n * 3.99f); break;
    case 123: /* All notes off */ pushCommand(CMD_RELEASE_ALL, 0, 0, 0, 0, false); break;
    
    // Sustain and portamento
    case 64: /* Sustain pedal - could implement */ break;
    case 65: /* Portamento on/off */ 
      if (value >= 64) setParameter(PARAM_GLIDE_TIME, 0.3f);
      else setParameter(PARAM_GLIDE_TIME, 0.0f);
      break;
  }
}

// ===== ARPEGGIATOR =====

void arpAddNote(int note, int velocity) {
  // Check if already in buffer
  for (int i = 0; i < arpNoteCount; i++) {
    if (arpNoteBuffer[i] == note) return;
  }
  
  if (arpNoteCount < ARP_MAX_NOTES) {
    arpNoteBuffer[arpNoteCount++] = note;
    
    // Sort notes
    for (int i = 0; i < arpNoteCount - 1; i++) {
      for (int j = i + 1; j < arpNoteCount; j++) {
        if (arpNoteBuffer[i] > arpNoteBuffer[j]) {
          int t = arpNoteBuffer[i];
          arpNoteBuffer[i] = arpNoteBuffer[j];
          arpNoteBuffer[j] = t;
        }
      }
    }
    
    if (arpNoteCount == 1) {
      arpCurrentStep = 0;
      arpStepStartTime = millis();
    }
  }
}

void arpRemoveNote(int note) {
  if (arpLatch) return;
  
  for (int i = 0; i < arpNoteCount; i++) {
    if (arpNoteBuffer[i] == note) {
      for (int j = i; j < arpNoteCount - 1; j++) {
        arpNoteBuffer[j] = arpNoteBuffer[j + 1];
      }
      arpNoteCount--;
      break;
    }
  }
}

void arpProcess() {
  if (!arpEnabled || arpNoteCount == 0) {
    if (arpNoteActive) {
      noteOff(arpPlayingNote);
      arpNoteActive = false;
      arpPlayingNote = -1;
    }
    return;
  }
  
  unsigned long now = millis();
  float stepMs = 15000.0f / arpRate;
  if (arpCurrentStep & 1) stepMs *= (1.0f + arpSwing);
  
  unsigned long elapsed = now - arpStepStartTime;
  
  // Note off at gate end
  if (arpNoteActive && elapsed >= (unsigned long)(stepMs * arpGate)) {
    noteOff(arpPlayingNote);
    arpNoteActive = false;
  }
  
  // Next step
  if (elapsed >= (unsigned long)stepMs) {
    arpCurrentStep = (arpCurrentStep + 1) & 15;
    
    int noteToPlay = calculateArpNote();
    noteToPlay = constrain(noteToPlay, 0, 127);
    
    if (arpNoteActive) noteOff(arpPlayingNote);
    
    arpPlayingNote = noteToPlay;
    noteOn(noteToPlay, 100);
    arpNoteActive = true;
    arpStepStartTime = now;
  }
}

int calculateArpNote() {
  if (arpNoteCount <= 0) return 60;  // SAFETY: return middle C if empty
  
  int base = arpNoteBuffer[0];
  
  // Pattern mode
  if (arpDirection == 4) {
    // SAFETY: bounds check pattern index
    int safePattern = constrain(arpPattern, 0, ARP_NUM_PATTERNS - 1);
    int safeStep = arpCurrentStep & 15;  // Already masked but explicit
    return constrain(base + arpPatterns[safePattern][safeStep], 0, 127);
  }
  
  int total = arpNoteCount * arpOctaves;
  if (total <= 0) return 60;  // SAFETY
  
  int idx = 0, oct = 0;
  
  switch (arpDirection) {
    case 0:  // Up
      idx = arpCurrentStep % arpNoteCount;
      oct = (arpCurrentStep / arpNoteCount) % arpOctaves;
      break;
      
    case 1: {  // Down
      int pos = total - 1 - (arpCurrentStep % total);
      idx = pos % arpNoteCount;
      oct = pos / arpNoteCount;
      break;
    }
    
    case 2: {  // Up/Down
      int cycle = (total > 1) ? (total * 2 - 2) : 1;
      int pos = arpCurrentStep % cycle;
      if (pos < total) {
        idx = pos % arpNoteCount;
        oct = pos / arpNoteCount;
      } else {
        int dp = cycle - pos;
        idx = dp % arpNoteCount;
        oct = dp / arpNoteCount;
      }
      break;
    }
    
    case 3:  // Random
      idx = random(arpNoteCount);
      oct = random(arpOctaves);
      break;
  }
  
  // SAFETY: final bounds checks
  idx = constrain(idx, 0, arpNoteCount - 1);
  oct = constrain(oct, 0, arpOctaves - 1);
  return constrain(arpNoteBuffer[idx] + oct * 12, 0, 127);
}

// ===== VOICE ALLOCATION =====

void noteOn(int note, int velocity) {
  int vi = -1;
  bool trigEnv = true;
  
  if (playMode == 0 || playMode == 2) {  // Mono or Legato
    if (monoStackSize < 16) monoNoteStack[monoStackSize++] = note;
    vi = 0;
    if (playMode == 2 && monoStackSize > 1) trigEnv = false;  // Legato
  } else {  // Poly
    vi = allocateVoice();
  }
  
  if (vi >= 0) {
    pushCommand(CMD_NOTE_ON, vi, note, velocity, noteToFreq(note), trigEnv);
  }
}

void noteOff(int note) {
  if (playMode == 0 || playMode == 2) {  // Mono/Legato
    // Remove from stack
    for (int i = 0; i < monoStackSize; i++) {
      if (monoNoteStack[i] == note) {
        for (int j = i; j < monoStackSize - 1; j++) {
          monoNoteStack[j] = monoNoteStack[j + 1];
        }
        monoStackSize--;
        break;
      }
    }
    
    if (monoStackSize > 0) {
      // Go back to previous note
      int prev = monoNoteStack[monoStackSize - 1];
      pushCommand(CMD_NOTE_ON, 0, prev, 100, noteToFreq(prev), false);
    } else {
      pushCommand(CMD_NOTE_OFF, 0, note, 0, 0, false);
    }
  } else {
    int vi = findVoice(note);
    if (vi >= 0) {
      pushCommand(CMD_NOTE_OFF, vi, note, 0, 0, false);
    }
  }
}

int allocateVoice() {
  // First: find completely idle voice
  for (int i = 0; i < VOICES; i++) {
    if (!voices[i].active && isEnvelopeIdle(&voices[i].ampEnv)) {
      return i;
    }
  }
  
  // Second: find voice in release state (steal it)
  for (int i = 0; i < VOICES; i++) {
    if (!voices[i].active) {
      return i;
    }
  }
  
  // Third: steal oldest active voice
  int oldest = 0;
  unsigned long oldestT = voices[0].noteOnTime;
  for (int i = 1; i < VOICES; i++) {
    if (voices[i].noteOnTime < oldestT) {
      oldestT = voices[i].noteOnTime;
      oldest = i;
    }
  }
  return oldest;
}

int findVoice(int note) {
  for (int i = 0; i < VOICES; i++) {
    if (voices[i].note == note && (voices[i].active || !isEnvelopeIdle(&voices[i].ampEnv))) {
      return i;
    }
  }
  return -1;
}

/*
 * ========================================
 * COMPLETE MIDI CC MAPPING
 * ========================================
 * 
 * STANDARD CCs:
 *   1  = Mod Wheel
 *   7  = Master Volume
 *   11 = Filter Cutoff (Expression)
 *   64 = Sustain Pedal
 *   65 = Portamento On/Off
 * 
 * OSCILLATORS (14-25):
 *   14 = OSC1 Range (16'/8'/4'/2')
 *   15 = OSC2 Range
 *   16 = OSC3 Range
 *   17 = OSC1 Fine Tune
 *   18 = OSC2 Fine Tune
 *   19 = OSC3 Fine Tune
 *   20 = OSC1 Waveform
 *   21 = OSC2 Waveform
 *   22 = OSC3 Waveform
 *   23 = OSC1 Volume
 *   24 = OSC2 Volume
 *   25 = OSC3 Volume
 * 
 * SUB-OSCILLATOR (26-29):
 *   26 = Sub Enable
 *   27 = Sub Waveform
 *   28 = Sub Octave (-1/-2)
 *   29 = Sub Volume
 * 
 * RING MOD (30-31):
 *   30 = Ring Mod Enable
 *   31 = Ring Mod Amount
 * 
 * FILTER (70-79):
 *   70 = Cutoff
 *   71 = Resonance
 *   72 = Filter Attack
 *   73 = Filter Decay
 *   74 = Filter Sustain
 *   75 = Filter Release
 *   76 = Filter Env Strength
 *   77 = Filter Key Track
 *   78 = Drive
 *   79 = Velocity to Filter
 * 
 * AMP ENVELOPE (80-84):
 *   80 = Amp Attack
 *   81 = Amp Decay
 *   82 = Amp Sustain
 *   83 = Amp Release
 *   84 = Velocity to Amp
 * 
 * LFO (85-89):
 *   85 = LFO Rate
 *   86 = LFO Depth
 *   87 = LFO Waveform
 *   88 = LFO Target (Pitch/Filter/Amp)
 *   89 = LFO Enable
 * 
 * UNISON (90-92):
 *   90 = Unison Count (1-4)
 *   91 = Unison Detune
 *   92 = Unison Spread
 * 
 * CHORUS (102-105):
 *   102 = Chorus Rate
 *   103 = Chorus Depth
 *   104 = Chorus Feedback
 *   105 = Chorus Mix
 * 
 * REVERB (106-110):
 *   106 = Reverb Size
 *   107 = Reverb Damping
 *   108 = Reverb Width
 *   109 = Reverb Mix
 *   110 = Reverb Predelay
 * 
 * NOISE (111-112):
 *   111 = Noise Volume
 *   112 = Noise Type (White/Pink)
 * 
 * GLOBAL (113-115):
 *   113 = Play Mode (Poly/Mono/Legato)
 *   114 = Glide Time
 *   115 = Master Tune
 * 
 * ARPEGGIATOR (116-122):
 *   116 = Arp Enable
 *   117 = Arp Pattern
 *   118 = Arp Rate
 *   119 = Arp Gate
 *   121 = Arp Swing
 *   122 = Arp Octaves
 * 
 * SYSTEM:
 *   120 = All Sound Off
 *   123 = All Notes Off
 * 
 * ========================================
 */
