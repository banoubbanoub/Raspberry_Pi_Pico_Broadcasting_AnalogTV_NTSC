# Raspberry Pi Pico RP2040 – NTSC Channel 3 Analog TV Broadcast

A Raspberry Pi Pico (RP2040) project that generates an NTSC-compatible analog television signal and broadcasts it on **VHF Channel 3**, allowing a vintage CRT television to display video directly.

The project uses the RP2040's PIO and DMA hardware to generate precisely timed video waveforms, while an Arduino Nano controls the PLL inside the CRT television's analog tuner to achieve reliable and stable channel reception.

## Project Overview

This project explores using the Raspberry Pi Pico RP2040 as a real-time video signal generator for analog television.

Instead of using an external video encoder IC, the RP2040 generates the video waveform using its programmable I/O (PIO), DMA controller, and carefully prepared waveform lookup tables.

The generated signal contains the timing and video information required for NTSC television reception.

The output is transmitted on analog TV Channel 3 and received by a vintage CRT television.

**One of the most important achievements of this project is obtaining a stable picture by controlling the PLL (Phase-Locked Loop) inside the CRT television's analog tuner using an Arduino Nano.**

## Hardware

- Raspberry Pi Pico (RP2040)
- Arduino Nano
- Analog CRT television with VHF Channel 3 reception
- Video signal output circuitry
- Power supply and interconnections

## System Architecture

The project uses two microcontrollers, each handling a different responsibility.

### 1. Raspberry Pi Pico – NTSC Video Generator

The RP2040 is responsible for generating the analog television video waveform.

Its main tasks include:

- Generating NTSC synchronization signals.
- Generating horizontal and vertical timing.
- Producing video and color-burst waveform samples.
- Reading pixel information from the framebuffer.
- Preparing waveform data using precomputed lookup tables.
- Transmitting waveform samples through PIO and DMA.

The RP2040 hardware peripherals handle the time-critical operations instead of relying on software timing loops.

### 2. Arduino Nano – CRT Tuner PLL Control

One of the challenges with analog television transmission is obtaining stable reception on the CRT television.

For this project, an Arduino Nano is used to control the Phase-Locked Loop (PLL) circuitry inside the CRT television's analog tuner.

This allows the tuner to lock onto the intended Channel 3 signal and maintain stable reception.

This was an important part of achieving a solid and stable picture on the CRT television.

The Arduino Nano handles tuner control, while the RP2040 independently generates the NTSC video waveform.

## RP2040 Video Generation Architecture

The project takes advantage of several powerful RP2040 hardware features.

### PIO (Programmable I/O)

The RP2040 PIO peripheral is used as a hardware serializer.

It outputs the prepared waveform samples at a controlled bit rate without requiring the CPU to toggle GPIO pins manually.

The current implementation uses:

- PIO0
- One state machine
- GPIO21 as the video output
- 80 MHz target video bit rate

### DMA (Direct Memory Access)

DMA transfers waveform data directly from memory to the PIO TX FIFO.

The implementation uses multiple DMA channels arranged in a chained configuration.

This allows one line buffer to be transmitted while other buffers are prepared for subsequent lines.

Benefits include:

- Continuous waveform transmission.
- Reduced CPU workload.
- Consistent output timing.
- Efficient use of RP2040 hardware peripherals.

### Waveform Lookup Tables

Instead of calculating every waveform sample in real time, the project uses precomputed waveform tables.

The waveform samples are retrieved using a lookup table and organized into line buffers before transmission.

This significantly reduces the amount of processing required during video generation.

## NTSC Signal Generation

The source code contains dedicated routines for generating different television synchronization and video line types.

These include:

| Function | Purpose |
|---|---|
| `FT_STA()` | Short synchronization pulse |
| `FT_STB()` | Long synchronization pulse |
| `FT_B()` | Black video line |
| `FT_SRA()` | Short-to-long synchronization transition |
| `FT_SRB()` | Long-to-short synchronization transition |
| `FT_LIN()` | Active video line |
| `FT_CLOSE_M()` | End-of-frame processing |

The line-generation routines construct the waveform using predefined signal levels:

- Sync level
- Black level
- White level
- Color burst level
- Gray level

The implementation also includes conditional PAL timing definitions, although this project is focused on NTSC Channel 3 operation.

## Framebuffer

The video generator reads pixel information from a framebuffer and converts the pixel values into waveform samples.

During active video generation, the framebuffer data is processed using the precomputed waveform table.

This approach makes it possible to generate television video directly from pixel data without a dedicated video encoder IC.

## Why the Picture Is Stable

A particularly interesting part of this project is the interaction between the video generator and the CRT television tuner.

Generating a correctly timed NTSC waveform is only one part of the problem. The television must also receive and tune to the intended RF signal correctly.

By using an Arduino Nano to control the PLL inside the CRT television's analog tuner, I was able to obtain stable Channel 3 reception.

This combination gives the project two independent hardware responsibilities:

- **RP2040:** Generates the NTSC video signal.
- **Arduino Nano:** Controls the CRT tuner PLL for stable Channel 3 reception.

The result is a solid and stable picture on a vintage CRT television.

## Project Highlights

- Direct NTSC waveform generation using RP2040.
- Analog TV Channel 3 operation.
- Hardware-based PIO waveform serialization.
- DMA-driven continuous video output.
- Precomputed waveform lookup tables.
- Framebuffer-based video generation.
- Arduino Nano-controlled CRT tuner PLL.
- Stable analog CRT television reception.
- No dedicated NTSC video encoder IC required.

## Conclusion

This project demonstrates how much can be accomplished with the Raspberry Pi Pico RP2040 by taking advantage of its hardware peripherals.

PIO, DMA, and carefully organized waveform lookup tables make the RP2040 a capable platform for generating real-time analog television signals.

Combining the Pico with an Arduino Nano for CRT tuner PLL control makes this an interesting experiment in microcontroller-based analog television technology.

It is another example of bringing modern microcontrollers together with vintage analog electronics.

---

**Platform:** Raspberry Pi Pico / RP2040  
**Video Standard:** NTSC  
**RF Channel:** VHF Channel 3  
**Video Generation:** PIO + DMA  
**Video Output GPIO:** GPIO21  
**Target Bit Rate:** 80 MHz  
**Tuner Control:** Arduino Nano / CRT tuner PLL
