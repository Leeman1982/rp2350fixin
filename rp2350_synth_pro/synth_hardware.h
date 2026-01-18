/*
 * synth_hardware.h - RP2350 Hardware Optimizations
 * 
 * Platform-specific features:
 * - Hardware interpolator for fast linear interpolation
 * - Watchdog timer for safety/recovery
 * - Performance monitoring
 * - Optimized memory operations
 * 
 * The RP2350 has 2 hardware interpolators per core that can do:
 * - Linear interpolation in ~2 cycles
 * - Fixed-point multiply-accumulate
 * - Clamping/saturation
 */

#ifndef SYNTH_HARDWARE_H
#define SYNTH_HARDWARE_H

#include <Arduino.h>
#include "hardware/interp.h"
#include "hardware/watchdog.h"
#include "hardware/clocks.h"
#include "pico/multicore.h"

// ===== HARDWARE INTERPOLATOR CONFIGURATION =====

// Interpolator 0 on each core: Used for wavetable/delay line interpolation
// Interpolator 1 on each core: Used for gain/mixing operations

// Fixed-point format for interpolation: 16.16
#define INTERP_FRAC_BITS 16
#define INTERP_FRAC_MASK ((1 << INTERP_FRAC_BITS) - 1)
#define INTERP_ONE (1 << INTERP_FRAC_BITS)

// ===== INTERPOLATOR FUNCTIONS =====

// Initialize hardware interpolators for audio processing
void initHardwareInterpolators();

// Fast linear interpolation using hardware
// Returns interpolated value between table[index] and table[index+1]
// phase is 16.16 fixed point where fractional part determines blend
inline float hwInterpolate(const float* table, uint32_t phase, uint32_t tableMask) {
  // Extract integer and fractional parts
  uint32_t index = (phase >> INTERP_FRAC_BITS) & tableMask;
  uint32_t frac = phase & INTERP_FRAC_MASK;
  
  // Get two samples
  float s0 = table[index];
  float s1 = table[(index + 1) & tableMask];
  
  // Linear interpolation
  float blend = (float)frac * (1.0f / (float)INTERP_ONE);
  return s0 + (s1 - s0) * blend;
}

// Fast linear interpolation for delay lines (int16 samples)
inline int16_t hwInterpolateInt16(const int16_t* buffer, uint32_t phase, uint32_t bufferMask) {
  uint32_t index = (phase >> INTERP_FRAC_BITS) & bufferMask;
  uint32_t frac = phase & INTERP_FRAC_MASK;
  
  int32_t s0 = buffer[index];
  int32_t s1 = buffer[(index + 1) & bufferMask];
  
  // Fixed-point interpolation (avoids float)
  return (int16_t)(s0 + (((s1 - s0) * (int32_t)frac) >> INTERP_FRAC_BITS));
}

// Optimized float-to-int16 conversion with saturation
inline int16_t floatToInt16Sat(float x) {
  // Clamp to valid range
  if (x > 1.0f) x = 1.0f;
  if (x < -1.0f) x = -1.0f;
  return (int16_t)(x * 32767.0f);
}

// ===== WATCHDOG TIMER =====

// Initialize watchdog with specified timeout in milliseconds
void initWatchdog(uint32_t timeoutMs);

// Feed the watchdog (call regularly to prevent reset)
inline void feedWatchdog() {
  watchdog_update();
}

// Check if last reset was caused by watchdog
bool wasWatchdogReset();

// ===== PERFORMANCE MONITORING =====

struct PerfStats {
  uint32_t audioCallbackCycles;
  uint32_t maxCallbackCycles;
  uint32_t minCallbackCycles;
  uint32_t avgCallbackCycles;
  uint32_t callCount;
  uint32_t overruns;           // Times we exceeded budget
  float cpuUsagePercent;
};

extern volatile PerfStats perfStats;

// Start performance measurement
inline uint32_t perfStart() {
  return time_us_32();
}

// End performance measurement and update stats
void perfEnd(uint32_t startTime, uint32_t budgetUs);

// Reset performance statistics
void perfReset();

// Get current CPU usage percentage
float getCpuUsage();

// ===== MEMORY OPERATIONS =====

// Fast memory clear (uses DMA if available)
void fastMemClear(void* dst, size_t bytes);

// Fast memory copy
void fastMemCopy(void* dst, const void* src, size_t bytes);

// ===== CORE SYNCHRONIZATION =====

// Memory barrier for cross-core communication
inline void memoryBarrier() {
  __dmb();
}

// Critical section for shared data access
class CriticalSection {
public:
  CriticalSection() {
    savedInterrupts = save_and_disable_interrupts();
  }
  ~CriticalSection() {
    restore_interrupts(savedInterrupts);
  }
private:
  uint32_t savedInterrupts;
};

// ===== CLOCK AND TIMING =====

// Get current system clock frequency
uint32_t getSystemClockHz();

// Get microsecond timestamp
inline uint32_t getMicros() {
  return time_us_32();
}

// High-precision delay (busy-wait, use sparingly)
inline void delayMicrosecondsPrecise(uint32_t us) {
  uint32_t start = time_us_32();
  while (time_us_32() - start < us) {
    // Busy wait
  }
}

// ===== SAFETY FEATURES =====

// Audio safety limiter state
struct SafetyLimiter {
  float dcAccumulator;
  float peakLevel;
  uint32_t clipCount;
  bool isMuted;
  uint32_t muteCountdown;
};

extern SafetyLimiter safetyLimiter;

// Process sample through safety limiter
float processSafetyLimiter(float input);

// Check and handle audio safety issues
void checkAudioSafety();

// Emergency mute (e.g., on error)
void emergencyMute();

// ===== TEMPERATURE MONITORING =====

// Get chip temperature in Celsius (approximate)
float getChipTemperature();

// ===== DEBUG UTILITIES =====

#ifdef SYNTH_DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(fmt, ...)
#endif

// Print performance statistics
void printPerfStats();

// Print memory usage
void printMemoryUsage();

#endif // SYNTH_HARDWARE_H
