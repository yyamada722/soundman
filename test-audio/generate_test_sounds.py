#!/usr/bin/env python3
"""
Generate test audio files for Soundman audio analysis tool

Creates various test signals for evaluating:
- Loudness meter (LUFS)
- Phase correlation meter
- True peak meter
- Level meters
"""

import numpy as np
import wave
import struct
import os

# Audio parameters
SAMPLE_RATE = 44100
DURATION = 10  # seconds
BIT_DEPTH = 16

def db_to_linear(db):
    """Convert dB to linear amplitude"""
    return 10.0 ** (db / 20.0)

def lufs_to_rms(lufs):
    """
    Approximate conversion from LUFS to RMS amplitude
    LUFS = -0.691 + 10 * log10(MS)
    MS = 10^((LUFS + 0.691) / 10)
    RMS = sqrt(MS)
    """
    ms = 10.0 ** ((lufs + 0.691) / 10.0)
    return np.sqrt(ms)

def write_wav(filename, data, sample_rate=SAMPLE_RATE):
    """Write audio data to WAV file"""
    # Ensure output directory exists
    os.makedirs(os.path.dirname(filename) if os.path.dirname(filename) else '.', exist_ok=True)

    # Normalize to 16-bit range
    data = np.clip(data, -1.0, 1.0)
    data_int = (data * 32767).astype(np.int16)

    with wave.open(filename, 'w') as wav_file:
        n_channels = 2 if len(data.shape) > 1 else 1
        wav_file.setnchannels(n_channels)
        wav_file.setsampwidth(2)  # 16-bit
        wav_file.setframerate(sample_rate)

        if n_channels == 2:
            # Interleave stereo channels
            interleaved = np.empty((data_int.shape[1] * 2,), dtype=np.int16)
            interleaved[0::2] = data_int[0]
            interleaved[1::2] = data_int[1]
            wav_file.writeframes(interleaved.tobytes())
        else:
            wav_file.writeframes(data_int.tobytes())

def generate_sine_tone(freq, amplitude, duration, sample_rate=SAMPLE_RATE):
    """Generate a sine wave tone"""
    t = np.linspace(0, duration, int(sample_rate * duration), endpoint=False)
    return amplitude * np.sin(2 * np.pi * freq * t)

def generate_pink_noise(amplitude, duration, sample_rate=SAMPLE_RATE):
    """Generate pink noise (1/f noise)"""
    # Simple pink noise using white noise and filtering
    num_samples = int(sample_rate * duration)
    white = np.random.randn(num_samples)

    # Simple pink filter (approximate)
    b = np.array([0.049922035, -0.095993537, 0.050612699, -0.004408786])
    a = np.array([1, -2.494956002, 2.017265875, -0.522189400])

    # Apply filter
    pink = np.zeros(num_samples)
    for i in range(3, num_samples):
        pink[i] = (b[0] * white[i] + b[1] * white[i-1] + b[2] * white[i-2] + b[3] * white[i-3]
                   - a[1] * pink[i-1] - a[2] * pink[i-2] - a[3] * pink[i-3])

    # Normalize and scale
    pink = pink / np.max(np.abs(pink)) * amplitude
    return pink

def main():
    print("Generating test audio files for Soundman...")

    # Create output directory
    output_dir = "output"
    os.makedirs(output_dir, exist_ok=True)

    # 1. Loudness Test Files
    print("\n1. Generating loudness test files...")

    # -23 LUFS (EBU R128 target)
    print("  - Generating 1kHz tone at -23 LUFS...")
    amplitude_23 = lufs_to_rms(-23.0)
    tone_1k = generate_sine_tone(1000, amplitude_23, DURATION)
    stereo_23 = np.array([tone_1k, tone_1k])
    write_wav(f"{output_dir}/01_tone_1kHz_minus23LUFS.wav", stereo_23)

    # -18 LUFS (Maximum short-term for EBU R128)
    print("  - Generating 1kHz tone at -18 LUFS...")
    amplitude_18 = lufs_to_rms(-18.0)
    tone_1k = generate_sine_tone(1000, amplitude_18, DURATION)
    stereo_18 = np.array([tone_1k, tone_1k])
    write_wav(f"{output_dir}/02_tone_1kHz_minus18LUFS.wav", stereo_18)

    # -14 LUFS (Over limit)
    print("  - Generating 1kHz tone at -14 LUFS...")
    amplitude_14 = lufs_to_rms(-14.0)
    tone_1k = generate_sine_tone(1000, amplitude_14, DURATION)
    stereo_14 = np.array([tone_1k, tone_1k])
    write_wav(f"{output_dir}/03_tone_1kHz_minus14LUFS.wav", stereo_14)

    # Pink noise at -23 LUFS
    print("  - Generating pink noise at -23 LUFS...")
    pink = generate_pink_noise(amplitude_23, DURATION)
    stereo_pink = np.array([pink, pink])
    write_wav(f"{output_dir}/04_pink_noise_minus23LUFS.wav", stereo_pink)

    # 2. Phase Correlation Test Files
    print("\n2. Generating phase correlation test files...")

    # Perfect in-phase (correlation = +1.0)
    print("  - Generating in-phase stereo signal (L=R)...")
    tone_1k = generate_sine_tone(1000, amplitude_23, DURATION)
    stereo_in_phase = np.array([tone_1k, tone_1k])
    write_wav(f"{output_dir}/05_phase_in_phase_L_equals_R.wav", stereo_in_phase)

    # Perfect out-of-phase (correlation = -1.0)
    print("  - Generating out-of-phase stereo signal (L=-R)...")
    stereo_out_phase = np.array([tone_1k, -tone_1k])
    write_wav(f"{output_dir}/06_phase_out_of_phase_L_equals_minusR.wav", stereo_out_phase)

    # Wide stereo (low correlation ~0.0)
    print("  - Generating wide stereo signal (independent L/R)...")
    left = generate_pink_noise(amplitude_23, DURATION)
    right = generate_pink_noise(amplitude_23, DURATION)
    stereo_wide = np.array([left, right])
    write_wav(f"{output_dir}/07_phase_wide_stereo_independent.wav", stereo_wide)

    # Mono (correlation = 1.0)
    print("  - Generating mono signal...")
    mono = generate_sine_tone(1000, amplitude_23, DURATION)
    stereo_mono = np.array([mono, mono])
    write_wav(f"{output_dir}/08_phase_mono.wav", stereo_mono)

    # 3. True Peak Test Files
    print("\n3. Generating true peak test files...")

    # Peak at -6 dBFS
    print("  - Generating tone with peak at -6 dBFS...")
    amplitude_6db = db_to_linear(-6.0)
    tone_1k = generate_sine_tone(1000, amplitude_6db, DURATION)
    stereo_6db = np.array([tone_1k, tone_1k])
    write_wav(f"{output_dir}/09_peak_minus6dBFS.wav", stereo_6db)

    # Peak at -3 dBFS
    print("  - Generating tone with peak at -3 dBFS...")
    amplitude_3db = db_to_linear(-3.0)
    tone_1k = generate_sine_tone(1000, amplitude_3db, DURATION)
    stereo_3db = np.array([tone_1k, tone_1k])
    write_wav(f"{output_dir}/10_peak_minus3dBFS.wav", stereo_3db)

    # Peak at -1 dBFS
    print("  - Generating tone with peak at -1 dBFS...")
    amplitude_1db = db_to_linear(-1.0)
    tone_1k = generate_sine_tone(1000, amplitude_1db, DURATION)
    stereo_1db = np.array([tone_1k, tone_1k])
    write_wav(f"{output_dir}/11_peak_minus1dBFS.wav", stereo_1db)

    # 4. Complex Test Signal
    print("\n4. Generating complex test signal...")
    print("  - Generating multi-tone complex signal...")

    # Multi-tone signal with various frequencies
    t = np.linspace(0, DURATION, int(SAMPLE_RATE * DURATION), endpoint=False)
    complex_signal = (
        0.3 * np.sin(2 * np.pi * 100 * t) +   # Bass
        0.4 * np.sin(2 * np.pi * 1000 * t) +  # Mid
        0.2 * np.sin(2 * np.pi * 5000 * t) +  # High
        0.1 * np.sin(2 * np.pi * 10000 * t)   # Very high
    )
    # Normalize to -23 LUFS
    complex_signal = complex_signal / np.max(np.abs(complex_signal)) * amplitude_23

    # Create stereo with slight phase difference
    left = complex_signal
    right = complex_signal * 0.8 + generate_pink_noise(amplitude_23 * 0.2, DURATION)
    stereo_complex = np.array([left, right])
    write_wav(f"{output_dir}/12_complex_multi_tone.wav", stereo_complex)

    print(f"\n✓ All test files generated successfully in '{output_dir}/' directory!")
    print("\nTest files created:")
    print("  Loudness tests: 01-04")
    print("  Phase tests: 05-08")
    print("  Peak tests: 09-11")
    print("  Complex signal: 12")

if __name__ == "__main__":
    main()
