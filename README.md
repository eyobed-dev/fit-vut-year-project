# Year Project 2 (YP2) - Side Channel Analysis

This project focuses on Side Channel Analysis (SCA) of cryptographic implementations (AES/ECC) using ChipWhisperer and Deep Learning methods.

## Directory Structure

- `src/`: Source code and Jupyter notebooks (8-bit and 32-bit).
- `data/`: Measurement data (8-bit and 32-bit).
- `scripts/`: Utility scripts.

## Environment

The project uses a Python virtual environment located at `/home/eyobed/.cwvenv/`.
Key libraries: `chipwhisperer`, `pycryptodome`, `numpy`, `scipy`, `matplotlib`.

## Usage

Refer to `src/8-bit/analysis/yearproj.ipynb` and `src/32-bit/analysis/yearproj_ARM.ipynb` for acquisition and TVLA leakage assessment workflows.
