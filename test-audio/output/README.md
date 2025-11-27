# Soundman Test Audio Files

This directory contains professionally generated test audio files for evaluating the Soundman audio analysis tool.

## File List

### Loudness Meter Tests (LUFS)

1. **01_tone_1kHz_minus23LUFS.wav**
   - 1kHz sine tone at -23 LUFS
   - EBU R128 target level
   - Should display in **green zone** on loudness meter
   - Integrated: ~-23 LUFS

2. **02_tone_1kHz_minus18LUFS.wav**
   - 1kHz sine tone at -18 LUFS
   - Maximum short-term level for EBU R128
   - Should display in **yellow/orange zone**
   - Integrated: ~-18 LUFS

3. **03_tone_1kHz_minus14LUFS.wav**
   - 1kHz sine tone at -14 LUFS
   - Over the recommended limit
   - Should display in **red zone**
   - Integrated: ~-14 LUFS

4. **04_pink_noise_minus23LUFS.wav**
   - Pink noise (1/f spectrum) at -23 LUFS
   - More natural signal for testing
   - Should display around **-23 LUFS**
   - Good for testing loudness range (LRA)

### Phase Correlation Tests

5. **05_phase_in_phase_L_equals_R.wav**
   - Perfect in-phase stereo (L = R)
   - Phase correlation: **+1.0** (perfect correlation)
   - Should display **green** on phase meter
   - Status: "Good Stereo"

6. **06_phase_out_of_phase_L_equals_minusR.wav**
   - Perfect out-of-phase stereo (L = -R)
   - Phase correlation: **-1.0** (perfect anti-correlation)
   - Should display **red** on phase meter
   - Status: "Severe Phase Issues"
   - **Warning**: May cause mono compatibility issues

7. **07_phase_wide_stereo_independent.wav**
   - Wide stereo with independent L/R (pink noise)
   - Phase correlation: **~0.0** (uncorrelated)
   - Should display **yellow** on phase meter
   - Status: "Acceptable"

8. **08_phase_mono.wav**
   - Mono signal (L = R)
   - Phase correlation: **+1.0**
   - Should display **green** on phase meter

### True Peak Tests

9. **09_peak_minus6dBFS.wav**
   - 1kHz tone with peak at -6 dBFS
   - Should display **green** on true peak meter
   - True Peak: -6.0 dBTP

10. **10_peak_minus3dBFS.wav**
    - 1kHz tone with peak at -3 dBFS
    - Should display **yellow** on true peak meter
    - True Peak: -3.0 dBTP

11. **11_peak_minus1dBFS.wav**
    - 1kHz tone with peak at -1 dBFS
    - Should display **orange/red** on true peak meter
    - True Peak: -1.0 dBTP
    - Close to clipping threshold

### Complex Signal Test

12. **12_complex_multi_tone.wav**
    - Multi-frequency tone complex (100Hz, 1kHz, 5kHz, 10kHz)
    - Stereo with slight phase difference
    - Good for testing all meters simultaneously
    - Integrated loudness: ~-23 LUFS
    - Phase correlation: 0.6-0.8 (acceptable stereo)

## How to Use

1. Open Soundman Desktop application
2. Load any test file using "File > Open Audio File" or Cmd+O
3. Press **Space** to play
4. Observe the meters:
   - **Level Meter**: Shows RMS and peak levels in real-time
   - **True Peak Meter**: Shows dBTP levels with peak hold
   - **Phase Meter**: Shows stereo phase correlation (-1.0 to +1.0)
   - **Loudness Meter**: Shows LUFS (Integrated, Short-term, Momentary, LRA)

## Expected Results

### Loudness Tests
- File 01: Green zone, ~-23 LUFS (EBU R128 target)
- File 02: Yellow/orange zone, ~-18 LUFS
- File 03: Red zone, ~-14 LUFS (over limit)
- File 04: Similar to 01 but with dynamic variation

### Phase Tests
- File 05: Correlation = +1.0 (perfect mono compatibility)
- File 06: Correlation = -1.0 (severe phase issues - cancels in mono)
- File 07: Correlation ≈ 0.0 (wide stereo, acceptable)
- File 08: Correlation = +1.0 (mono)

### Peak Tests
- File 09: True Peak ≈ -6.0 dBTP (safe level)
- File 10: True Peak ≈ -3.0 dBTP (moderate level)
- File 11: True Peak ≈ -1.0 dBTP (close to limit)

## Technical Specifications

- **Format**: WAV (PCM 16-bit)
- **Sample Rate**: 44.1 kHz
- **Channels**: Stereo (2 channels)
- **Duration**: 10 seconds each
- **File Size**: ~1.7 MB per file

## Standards Reference

- **EBU R128**: European Broadcasting Union loudness standard
  - Target: -23 LUFS ± 1 LU
  - Maximum short-term: -18 LUFS
  - True peak limit: -1 dBTP

- **ITU-R BS.1770-4**: International loudness measurement standard
  - K-weighting filter
  - Gating at -70 LUFS (absolute) and -10 LU (relative)

## Notes

- These files are for testing and evaluation purposes only
- LUFS values are approximations based on simplified calculations
- For production use, professional calibration is recommended
- Phase issues in file 06 are intentional for testing

---

Generated with Python script `generate_test_sounds.py`
Part of the Soundman Audio Analysis Tool project
