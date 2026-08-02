#!/usr/bin/env python3
import sys
import os
import subprocess
from pathlib import Path

def convert_flac_to_wav(input_folder, output_folder):
    input_path = Path(input_folder)
    output_path = Path(output_folder)
    output_path.mkdir(parents=True, exist_ok=True)

    # Search for all .flac files in the input folder
    flac_files = sorted(list(input_path.glob("**/*.flac")) + list(input_path.glob("**/*.FLAC")))
    
    if not flac_files:
        print(f"Error: No .flac files found in {input_folder}")
        return

    print(f"====================================================")
    print(f"   FLAC TO WAV OFFLINE CONVERTER (afconvert)        ")
    print(f"====================================================")
    print(f"Found {len(flac_files)} FLAC files to convert.\n")

    for i, flac_file in enumerate(flac_files, 1):
        wav_filename = f"{flac_file.stem}.wav"
        output_wav_path = output_path / wav_filename
        
        print(f"[{i}/{len(flac_files)}] Converting: {flac_file.name} --> {wav_filename}")

        # Call Apple's built-in afconvert: -f WAVE (WAV format), -d LEI24 (24-bit Little Endian PCM)
        cmd = ["afconvert", "-f", "WAVE", "-d", "LEI24", str(flac_file), str(output_wav_path)]
        subprocess.run(cmd, check=True)

    print(f"\n====================================================")
    print(f"SUCCESS: Converted {len(flac_files)} files into {output_folder}")
    print(f"====================================================")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 tools/convert_flac_to_wav.py <input_flac_folder> <output_wav_folder>")
        sys.exit(1)
    
    convert_flac_to_wav(sys.argv[1], sys.argv[2])
