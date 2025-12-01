# Secure Remote Vehicle Communication System (Pratt & Whitney SDP 2024–2025)

This repository contains the code and design artifacts for **SDP-PW-25**, a Pratt & Whitney–sponsored UConn Senior Design Project.  
The goal of the project is to build a **secure, remotely controlled robotic platform** backed by a **verified boot chain** and an **encrypted command channel**.

The system focuses on defending against realistic lab attackers who may have physical access to the board and non-physical access to the command interface, while still keeping the user control experience simple and responsive.

---

## High-Level Overview

At a high level, the project includes:

- **Secure Boot on FPGA/SoC board (e.g., Xilinx ZCU102)**
  - Root of trust anchored in ROM / immutable storage.
  - Signed and versioned boot stages stored in external flash.
  - Verification of each stage before execution (chain of trust).
  - Protection against booting unauthenticated or rolled-back images.

- **Application Layer**
  - Firmware that runs on the board after secure boot completes.
  - Receives motion / control commands from a host over a defined interface.
  - Enforces simple command protocol (e.g., front/back/left/right/stop).

- **User Control Interface (Web App / Host Tools)**
  - Front-end UI for issuing directional commands.
  - Support for **continuous** directional input while a key/button is held.
  - Toggle for **encrypted vs. unencrypted** command mode.
  - Status/log view to show recent commands and board responses.

---

## Features

### Secure Boot

- **Root public key hash** baked into the immutable boot stage.
- **Signature verification** of the next boot image (e.g., second-stage bootloader or application).
- **Versioning / anti-rollback** checks to refuse known-old images.
- Clear **separation of trust zones**:
  - ROM / immutable code (root of trust)
  - Verified bootloader
  - Verified application firmware

### Application & Control Path

- Application firmware exposes a simple, well-defined **command API** for:
  - Directional movement (forward, backward, left, right)
  - Stop / idle
  - Optional status queries (health, mode, last error, etc.)
- Commands can be **plaintext** or **encrypted**, depending on configuration.
- Support for **continuous commands** while input is held down.

### Web / Host UI

- Keyboard and on-screen controls for sending movement commands.
- **Encryption toggle** with clear indication of current mode.
- A **log panel** displaying:
  - Timestamped commands sent
  - Responses / acknowledgements from the board
  - Errors (e.g., verification failures, invalid commands)

---

## Repository Structure

> ⚠️ Folder names here are examples — update this section to match the actual repo layout.

- `firmware/` – Board-side code:
  - Secure boot stages
  - Application firmware
  - Build scripts / linker scripts
- `host/` – Host-side tools:
  - Command-line utilities
  - Test harnesses
- `web/` or `client/` – Front-end UI:
  - React (or other) web app for directional control
  - WebSocket / HTTP client for talking to the board or a bridge service
- `docs/` – Design documentation:
  - Threat model
  - Boot flow diagrams
  - Protocol specs
  - Slides / reports
- `scripts/` – Helper scripts:
  - Flashing, signing, packaging boot images
  - Local testing utilities

---

## Getting Started

### Prerequisites

Hardware (example):

- Development board (e.g., **Xilinx ZCU102** or equivalent)
- Robot dog

Software:

- Vitis 2025.1
- **Python 3.x** (for scripts / host tools)
- **Node.js & npm*
- Git

### Clone the Repository

```bash
git clone https://github.com/M-moore1/SDP-PW-25.git
cd SDP-PW-25
