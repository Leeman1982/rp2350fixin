/*
 * synth_hardware.cpp - RP2350 Hardware Optimizations
 * 
 * Implementation of platform-specific features
 */

#include "synth_hardware.h"
#include <hardware/adc.h>

// ===== GLOBAL STATE =====

volatile PerfStats perfStats = {0};
SafetyLimiter safetyLimiter = {0};

// ===== HARDWARE INTERPOLATOR INITIALIZATION =====

void initHardwareInterpolators() {
  // Configure Interpolator 0 for linear interpolation
  // This can interpolate between two values based on a blend factor
  
  interp_config cfg = interp_default_config();
  
  // Lane 0: accumulator for index
  interp_config_set_shift(&cfg, INTERP_FRAC_BITS);
  interp_config_set_mask(&cfg, 0, 15);  // 16-bit index
  interp_config_set_signed(&cfg, false);
  interp_set_config(interp0, 0, &cfg);
  
  // Lane 1: accumulator for fractional blend
  cfg = interp_default_config();
  interp_config_set_mask(&cfg, 0, INTERP_FRAC_BITS - 1);
  interp_config_set_signed(&cfg, false);
  interp_set_config(interp0, 1, &cfg);
  
  // Interpolator 1 can be used for gain calculations
  cfg = interp_default_config();
  interp_config_set_signed(&cfg, true);
  interp_set_config(interp1, 0, &cfg);
  interp_set_config(interp1, 1, &cfg);
}

// ===== WATCHDOG TIMER =====

void initWatchdog(uint32_t timeoutMs) {
  // Enable watchdog with specified timeout
  // The watchdog will reset the chip if not fed within timeout
  watchdog_enable(timeoutMs, true);
}

bool wasWatchdogReset() {
  return watchdog_caused_reboot();
}

// ===== PERFORMANCE MONITORING =====

void perfEnd(uint32_t startTime, uint32_t budgetUs) {
  uint32_t elapsed = time_us_32() - startTime;
  
  perfStats.audioCallbackCycles = elapsed;
  perfStats.callCount++;
  
  if (elapsed > perfStats.maxCallbackCycles) {
    perfStats.maxCallbackCycles = elapsed;
  }
  
  if (perfStats.minCallbackCycles == 0 || elapsed < perfStats.minCallbackCycles) {
    perfStats.minCallbackCycles = elapsed;
  }
  
  // Running average (simple IIR filter)
  perfStats.avgCallbackCycles = (perfStats.avgCallbackCycles * 7 + elapsed) / 8;
  
  // Check for overruns
  if (elapsed > budgetUs) {
    perfStats.overruns++;
  }
  
  // Calculate CPU usage
  perfStats.cpuUsagePercent = (float)elapsed / (float)budgetUs * 100.0f;
}

void perfReset() {
  perfStats.audioCallbackCycles = 0;
  perfStats.maxCallbackCycles = 0;
  perfStats.minCallbackCycles = 0;
  perfStats.avgCallbackCycles = 0;
  perfStats.callCount = 0;
  perfStats.overruns = 0;
  perfStats.cpuUsagePercent = 0.0f;
}

float getCpuUsage() {
  return perfStats.cpuUsagePercent;
}

// ===== MEMORY OPERATIONS =====

void fastMemClear(void* dst, size_t bytes) {
  // For small buffers, use simple loop
  // For larger buffers, could use DMA
  uint32_t* dst32 = (uint32_t*)dst;
  size_t words = bytes / 4;
  
  // Unrolled loop for speed
  while (words >= 8) {
    dst32[0] = 0;
    dst32[1] = 0;
    dst32[2] = 0;
    dst32[3] = 0;
    dst32[4] = 0;
    dst32[5] = 0;
    dst32[6] = 0;
    dst32[7] = 0;
    dst32 += 8;
    words -= 8;
  }
  
  while (words > 0) {
    *dst32++ = 0;
    words--;
  }
  
  // Handle remaining bytes
  uint8_t* dst8 = (uint8_t*)dst32;
  size_t remaining = bytes & 3;
  while (remaining > 0) {
    *dst8++ = 0;
    remaining--;
  }
}

void fastMemCopy(void* dst, const void* src, size_t bytes) {
  // Use word-aligned copy when possible
  if (((uintptr_t)dst & 3) == 0 && ((uintptr_t)src & 3) == 0) {
    uint32_t* dst32 = (uint32_t*)dst;
    const uint32_t* src32 = (const uint32_t*)src;
    size_t words = bytes / 4;
    
    while (words >= 4) {
      dst32[0] = src32[0];
      dst32[1] = src32[1];
      dst32[2] = src32[2];
      dst32[3] = src32[3];
      dst32 += 4;
      src32 += 4;
      words -= 4;
    }
    
    while (words > 0) {
      *dst32++ = *src32++;
      words--;
    }
    
    // Remaining bytes
    uint8_t* dst8 = (uint8_t*)dst32;
    const uint8_t* src8 = (const uint8_t*)src32;
    size_t remaining = bytes & 3;
    while (remaining > 0) {
      *dst8++ = *src8++;
      remaining--;
    }
  } else {
    // Fallback to byte copy
    memcpy(dst, src, bytes);
  }
}

// ===== CLOCK AND TIMING =====

uint32_t getSystemClockHz() {
  return clock_get_hz(clk_sys);
}

// ===== SAFETY FEATURES =====

float processSafetyLimiter(float input) {
  // Check for NaN or Inf
  if (isnan(input) || isinf(input)) {
    safetyLimiter.clipCount++;
    return 0.0f;
  }
  
  // Update DC accumulator (high-pass filter for DC detection)
  safetyLimiter.dcAccumulator = safetyLimiter.dcAccumulator * 0.9999f + input * 0.0001f;
  
  // Remove DC offset if it builds up
  float output = input - safetyLimiter.dcAccumulator;
  
  // Track peak level
  float absVal = fabsf(output);
  if (absVal > safetyLimiter.peakLevel) {
    safetyLimiter.peakLevel = absVal;
  } else {
    safetyLimiter.peakLevel *= 0.99999f;  // Slow decay
  }
  
  // Hard clip as last resort
  if (output > 1.0f) {
    output = 1.0f;
    safetyLimiter.clipCount++;
  } else if (output < -1.0f) {
    output = -1.0f;
    safetyLimiter.clipCount++;
  }
  
  // Handle muting
  if (safetyLimiter.isMuted) {
    if (safetyLimiter.muteCountdown > 0) {
      safetyLimiter.muteCountdown--;
    } else {
      safetyLimiter.isMuted = false;
    }
    return 0.0f;
  }
  
  return output;
}

void checkAudioSafety() {
  // Check for excessive clipping
  if (safetyLimiter.clipCount > 1000) {
    // Too much clipping - might be feedback loop
    emergencyMute();
    safetyLimiter.clipCount = 0;
  }
  
  // Check for excessive DC offset
  if (fabsf(safetyLimiter.dcAccumulator) > 0.5f) {
    // DC offset too high
    emergencyMute();
    safetyLimiter.dcAccumulator = 0.0f;
  }
  
  // Check for sustained high level (potential feedback)
  if (safetyLimiter.peakLevel > 0.99f) {
    static uint32_t highLevelCount = 0;
    highLevelCount++;
    if (highLevelCount > 48000) {  // 1 second at 48kHz
      emergencyMute();
      highLevelCount = 0;
    }
  }
}

void emergencyMute() {
  safetyLimiter.isMuted = true;
  safetyLimiter.muteCountdown = 24000;  // 500ms mute at 48kHz
  
  #ifdef SYNTH_DEBUG
  Serial.println("EMERGENCY MUTE TRIGGERED!");
  #endif
}

// ===== TEMPERATURE MONITORING =====

float getChipTemperature() {
  // Read from internal temperature sensor
  // Note: ADC must be initialized first
  static bool adcInitialized = false;
  
  if (!adcInitialized) {
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adcInitialized = true;
  }
  
  adc_select_input(4);  // Temperature sensor is on input 4
  uint16_t raw = adc_read();
  
  // Convert to temperature (approximate formula for RP2040/RP2350)
  // T = 27 - (ADC_voltage - 0.706) / 0.001721
  float voltage = raw * 3.3f / 4096.0f;
  float temperature = 27.0f - (voltage - 0.706f) / 0.001721f;
  
  return temperature;
}

// ===== DEBUG UTILITIES =====

void printPerfStats() {
  #ifdef SYNTH_DEBUG
  Serial.println("\n=== Performance Stats ===");
  Serial.printf("Callback time: %lu us (min: %lu, max: %lu, avg: %lu)\n",
                perfStats.audioCallbackCycles,
                perfStats.minCallbackCycles,
                perfStats.maxCallbackCycles,
                perfStats.avgCallbackCycles);
  Serial.printf("CPU usage: %.1f%%\n", perfStats.cpuUsagePercent);
  Serial.printf("Overruns: %lu (of %lu calls)\n", 
                perfStats.overruns, perfStats.callCount);
  Serial.printf("Clip count: %lu\n", safetyLimiter.clipCount);
  Serial.printf("Peak level: %.3f\n", safetyLimiter.peakLevel);
  Serial.printf("DC offset: %.6f\n", safetyLimiter.dcAccumulator);
  Serial.printf("Chip temp: %.1f C\n", getChipTemperature());
  #endif
}

void printMemoryUsage() {
  #ifdef SYNTH_DEBUG
  extern char __StackLimit, __bss_end__;
  
  // This is an approximation
  uint32_t totalRam = 520 * 1024;  // RP2350 has 520KB
  uint32_t usedStack = (uint32_t)&__StackLimit - (uint32_t)&__bss_end__;
  
  Serial.println("\n=== Memory Usage ===");
  Serial.printf("Total RAM: %lu KB\n", totalRam / 1024);
  Serial.printf("Free heap: %lu bytes\n", rp2040.getFreeHeap());
  Serial.printf("System clock: %lu MHz\n", getSystemClockHz() / 1000000);
  #endif
}

// ===== OPTIMIZED DSP HELPERS =====

// These use the hardware interpolators when set up properly

// Fast 16-bit stereo interleave
void interleaveStereo(const int16_t* left, const int16_t* right, 
                      int16_t* output, size_t samples) {
  for (size_t i = 0; i < samples; i++) {
    output[i * 2] = left[i];
    output[i * 2 + 1] = right[i];
  }
}

// Fast gain application (in-place)
void applyGain(float* buffer, size_t samples, float gain) {
  // Unrolled for speed
  size_t i = 0;
  for (; i + 4 <= samples; i += 4) {
    buffer[i] *= gain;
    buffer[i + 1] *= gain;
    buffer[i + 2] *= gain;
    buffer[i + 3] *= gain;
  }
  for (; i < samples; i++) {
    buffer[i] *= gain;
  }
}

// Fast mix of two buffers
void mixBuffers(const float* src1, const float* src2, 
                float* dst, size_t samples, float mix) {
  float mix1 = 1.0f - mix;
  float mix2 = mix;
  
  for (size_t i = 0; i < samples; i++) {
    dst[i] = src1[i] * mix1 + src2[i] * mix2;
  }
}

// Soft saturation (approximation of tanh)
void applySoftSat(float* buffer, size_t samples, float drive) {
  if (drive <= 1.0f) return;
  
  float invDrive = 1.0f / drive;
  
  for (size_t i = 0; i < samples; i++) {
    float x = buffer[i] * drive;
    // Fast tanh approximation
    if (x > 3.0f) {
      buffer[i] = invDrive;
    } else if (x < -3.0f) {
      buffer[i] = -invDrive;
    } else {
      float x2 = x * x;
      buffer[i] = (x * (27.0f + x2) / (27.0f + 9.0f * x2)) * invDrive;
    }
  }
}
