# Secure Remote Vehicle Communication System

**Pratt & Whitney SDP 2024–2025**

A Pratt & Whitney–sponsored UConn Senior Design Project. The goal is to build a **secure, remotely controlled robotic platform** with encrypted command channels.

---

## Project Structure

| Directory          | Description                                                                                            |
|--------------------|--------------------------------------------------------------------------------------------------------|
| **controller-ui/** | User frontend (React + Tailwind). Sends encrypted and unencrypted commands to the robot via WebSocket. |
| **encryption/**    | AES-GCM encrypt/decrypt tools and JSON serialization in C. Used on the server and robot.               |
| **archive/**       | Archived server code (Express WebSocket server, test server).                                          |
| **server/**        |  WebSocket server and C code for Bluetooth communication between server and robot.                     |

### Planned

- **attacker-ui/** — Attacker frontend for sniffing Bluetooth packets and performing man-in-the-middle attacks on user–robot communication.


## Quick Start

### Controller UI

```bash
cd controller-ui
npm install
npm run dev
```

### Encryption (C)

```bash
cd encryption
make all
make test   # Run AES-GCM tests
```

Dependencies in [encryption/DEPENDENCIES.md](encryption/DEPENDENCIES.md).