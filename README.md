# DSP SDR Learning Project

This repository is a small C-based SDR learning project built around rtl-sdr and raylib. The goal is to explore how software-defined radio signals can be captured, demodulated, and visualized from a real RTL-SDR dongle.

## What this project is for

This is meant as a hands-on learning project for:
- working with RTL-SDR devices from C
- reading and interpreting I/Q samples
- tuning frequency and sample rate settings
- building simple FM demodulation
- visualizing signal data and audio output in a window

## Current feature list

- device discovery and basic USB string reporting
- configurable center frequency and sample rate
- FM demodulation from incoming I/Q data
- simple audio stream output
- live drawing of I/Q samples and demodulated signal traces

## Build

From the project root:

```bash
cmake -S . -B build
cmake --build build
```

## Notes

- An RTL-SDR device is required to run the application.
- On Windows, the dongle may need a compatible WinUSB driver installed with Zadig.
- The build uses the vendored rtl-sdr and raylib sources in this repository.
