# Safety Audit - RP2350 Professional Synthesizer v2.0

## Summary

| Category | Issues Found | Severity |
|----------|-------------|----------|
| Memory Safety | 3 | Medium |
| Real-Time Safety | 2 | **HIGH** |
| Thread Safety | 2 | Medium |
| Edge Cases | 5 | Low-Medium |

---

## 1. MEMORY SAFETY ISSUES

### 1.1 Buffer Bounds - Chorus Delay Read (Medium)
**File:** `synth_effects_pro.cpp:71-76`
**Issue:** Linear interpolation reads `readIdx2` which could wrap incorrectly
```cpp
int readIdx2 = (readIdx + 1) % CHORUS_BUFFER_SIZE;  // OK
```
**Status:** ✅ Already safe - modulo handles wraparound

### 1.2 Buffer Bounds - Arpeggiator Access (Medium)
**File:** `rp2350_synth_pro.ino:918`
**Issue:** `arpNoteBuffer[idx]` accessed without bounds check on `idx`
```cpp
return arpNoteBuffer[idx] + oct * 12;
```
**Fix Required:** Add bounds check:
```cpp
idx = constrain(idx, 0, arpNoteCount - 1);
return arpNoteBuffer[idx] + oct * 12;
```

### 1.3 Buffer Bounds - Reverb Predelay (Medium)
**File:** `synth_effects_pro.cpp:251`
**Issue:** `predelayIndex` modulo with dynamic `predelaySize`
**Status:** ✅ Safe - modulo handles wraparound

### 1.4 Array Overflow - Unison Voices (Low)
**File:** `rp2350_synth_pro.ino:248`
**Issue:** Loop uses `MAX_UNISON` but accesses based on `unisonCount`
**Status:** ✅ Safe - loop checks `u < v->unisonCount`

---

## 2. REAL-TIME SAFETY ISSUES (CRITICAL)

### 2.1 ❌ Dynamic Allocation in Init - FreeVerb
**File:** `synth_effects_pro.cpp:123-126, 133-136, 160-162`
**Issue:** `malloc()` used for reverb buffers
```cpp
comb->buffer = (float*)malloc(size * sizeof(float));
ap->buffer = (float*)malloc(size * sizeof(float));
rv->predelayBuffer = (float*)malloc(rv->predelaySize * sizeof(float));
```
**Severity:** HIGH - malloc can block, fragment heap
**Fix:** Use static allocation with fixed maximum sizes

### 2.2 ❌ Potential Division by Zero
**File:** `synth_voice_pro.cpp:278`
**Issue:** Exponential decay can create very small values
```cpp
env->level -= env->decayRate * (env->level - env->sustainLevel + 0.0001f);
```
**Status:** ✅ Mitigated with +0.0001f offset

### 2.3 ✅ No Serial.print in Audio Path
**Status:** Verified - no debug output in audio callback

### 2.4 ✅ No blocking delays in Audio Path  
**Status:** Verified - no delay() or delayMicroseconds() in audio

---

## 3. THREAD SAFETY ISSUES

### 3.1 Volatile Struct Copy (Fixed)
**File:** `rp2350_synth_pro.ino:200`
**Issue:** Direct assignment from volatile struct
**Status:** ✅ Fixed with field-by-field copy

### 3.2 Parameter Access Without Barriers (Medium)
**File:** `rp2350_synth_pro.ino:322-332`
**Issue:** Global parameters read in audio thread without `volatile`
```cpp
const float modWheel = modWheelValue;  // modWheelValue is volatile - OK
```
**Status:** ✅ modWheelValue, pitchBendValue declared volatile

### 3.3 Missing Barriers on Parameter Changes (Low)
**File:** `synth_params_pro.cpp`
**Issue:** Parameter changes from Core 0 may not be immediately visible to Core 1
**Recommendation:** Add `__dmb()` after parameter batch updates

---

## 4. EDGE CASE ISSUES

### 4.1 Division by Zero - Arpeggiator (Low)
**File:** `rp2350_synth_pro.ino:887-888`
**Issue:** Modulo by `arpNoteCount` when count could be 0
```cpp
idx = arpCurrentStep % arpNoteCount;
```
**Status:** ✅ Protected by early return: `if (arpNoteCount == 0) return 60;`

### 4.2 Log of Zero - Glide (Medium)
**File:** `rp2350_synth_pro.ino:419`
**Issue:** `logf(v->currentFreq)` when freq could theoretically be 0
```cpp
float logCurrent = logf(v->currentFreq);
```
**Fix Required:** Add protection:
```cpp
float logCurrent = logf(fmaxf(v->currentFreq, 1.0f));
```

### 4.3 Float Overflow - Filter Coefficients (Low)
**File:** `synth_voice_pro.cpp:338-339`
**Status:** ✅ Already clamped with fmaxf/fminf

### 4.4 NaN Propagation - Filter Feedback (Medium)
**File:** `synth_voice_pro.cpp:366`
**Issue:** If stage[3] becomes NaN, it propagates
```cpp
float fb = f->stage[3];
```
**Fix Required:** Add NaN check:
```cpp
float fb = f->stage[3];
if (!isfinite(fb)) fb = 0.0f;
```

### 4.5 Missing Note Bounds Check (Low)
**File:** `rp2350_synth_pro.ino:936`
**Issue:** `noteToFreq(note)` should validate note range
**Status:** ✅ Note already constrained earlier in handleNoteOn

---

## 5. RECOMMENDED FIXES

### Fix 1: Static Reverb Allocation
Replace malloc-based reverb with fixed-size static buffers:

```cpp
// In synth_effects_pro.h
#define MAX_COMB_SIZE 1760    // Largest comb buffer needed
#define MAX_ALLPASS_SIZE 600  // Largest allpass buffer needed
#define MAX_PREDELAY_SIZE 4800

struct ReverbComb {
  float buffer[MAX_COMB_SIZE];  // Static allocation
  int bufferSize;
  int index;
  float filterStore;
};
```

### Fix 2: Add Safety Checks in Filter

```cpp
float processMoogFilter(MoogFilter* f, float input) {
  // Sanitize input
  if (!isfinite(input)) input = 0.0f;
  
  // ... existing code ...
  
  // Sanitize output
  float output = f->stage[3];
  if (!isfinite(output)) {
    // Reset filter state on NaN
    for (int i = 0; i < 4; i++) {
      f->stage[i] = 0.0f;
      f->delay[i] = 0.0f;
    }
    return 0.0f;
  }
  return output;
}
```

### Fix 3: Add Glide Frequency Protection

```cpp
if (v->gliding && glideTime > 0.005f) {
  float safeFreq = fmaxf(v->currentFreq, 1.0f);
  float safeTarget = fmaxf(v->targetFreq, 1.0f);
  float logCurrent = logf(safeFreq);
  float logTarget = logf(safeTarget);
  // ...
}
```

### Fix 4: Add Arpeggiator Index Safety

```cpp
int calculateArpNote() {
  if (arpNoteCount <= 0) return 60;
  
  // ... existing code ...
  
  // Final safety clamp
  idx = constrain(idx, 0, arpNoteCount - 1);
  oct = constrain(oct, 0, arpOctaves - 1);
  return constrain(arpNoteBuffer[idx] + oct * 12, 0, 127);
}
```

---

## 6. VERIFICATION CHECKLIST

### Memory Safety
- [ ] All array accesses bounds-checked
- [ ] No dynamic allocation after init
- [ ] Buffer sizes compile-time constants
- [ ] Stack usage within limits

### Real-Time Safety
- [ ] No malloc/free in audio path
- [ ] No Serial output in audio path
- [ ] No blocking calls in audio path
- [ ] No unbounded loops

### Thread Safety
- [ ] Shared variables marked volatile
- [ ] Memory barriers at sync points
- [ ] Lock-free queue for commands
- [ ] Atomic parameter updates

### Edge Cases
- [ ] Division by zero protected
- [ ] Log/exp input ranges validated
- [ ] NaN/Inf detection and recovery
- [ ] MIDI note bounds enforced

---

## 7. TEST CASES

1. **Stress Test**: 8 voices, max resonance, rapid note on/off for 1 hour
2. **Parameter Sweep**: All CCs from 0→127→0 while playing
3. **Edge Values**: Cutoff=0, Resonance=max, Attack=0
4. **Arp Stress**: Enable arp, add/remove notes rapidly
5. **Recovery**: Verify filter recovers from instability
