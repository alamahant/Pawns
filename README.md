# Pawns

![Pawns Screenshot](screenshots/full.png)

![Pawns Chess](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Qt](https://img.shields.io/badge/Qt-6.11-green.svg)
![License](https://img.shields.io/badge/License-GPLv3-red.svg)

A feature-rich chess application built with C++ and Qt6. Play against Stockfish, challenge friends locally or remotely, or watch engines battle it out.

**Download**

| Platform | Where to Get It |
| :--- | :--- |
| **Linux** | [Flathub](https://flathub.org/en/apps/search?q=alamahant) |
| **Windows and Mac** | [Buy on Gumroad](https://jnanadhakini.gumroad.com/) - Pre-compiled binary, no compilation needed |

---

## Features

### Game Modes
- **Human vs Human** – Local two-player chess on the same computer
- **Human vs Engine** – Play against Stockfish at adjustable difficulty (0-20)
- **Engine vs Engine (E2E)** – Watch two chess engines battle it out
- **Remote Play (P2P)** – Play over the network with SSL encryption

### Visual Customization
- **Multiple piece sets** – Celtic, Fantasy, Eyes, Skulls, Spatial, and more
- **Board color themes** – Classic, Wood, Green, Blue, Gray, Brown
- **Customizable background color** – Choose any color for the view
- **Zoom** – Ctrl+Mouse Wheel to zoom in/out
- **Board markings** – Toggle rank and file labels

### Time Controls
- Customizable time controls (1-60 minutes)
- Increment options (0-30 seconds)
- Visual progress bars with color changes
- Time-out detection (flag falls)
- Clock synchronization in remote play

### Tools
- **Scenario Builder** – Create and save custom board positions
- **Save/Load games** – Full state including clocks and difficulty in `.chess` format
- **PGN export** – Export games in standard PGN format
- **Move history** – Table view in the dock
- **Game replay** – Media-style controls to step through games
- **Engine console** – View engine output and send UCI commands

### Remote Play
- SSL-encrypted P2P connection
- Contact list with IP/port storage
- In-game chat
- DTLS encrypted push-to-talk audio
- LAN peer discovery via UDP broadcast
- UPnP port forwarding support
- Resign, draw offer, and stop game functionality

### Sound
- Move sounds
- Check notification
- Checkmate sound
- Toggle sounds on/off

## Requirements

- Qt 6.11 or higher
- Stockfish chess engine (bundled)
- CMake 3.16+
- C++17 compiler

### Linux (Debian/Ubuntu)

```bash
sudo apt install qt6-base-dev qt6-multimedia-dev cmake build-essential
```

## Building

### From Source

```bash
git clone https://github.com/alamahant/Pawns.git
cd Pawns
mkdir build && cd build
cmake ..
make -j$(nproc)
./Pawns
```

### Flatpak

```bash
flatpak-builder --user --install --force-clean build-dir io.github.alamahant.Pawns.yml
```

## Controls

| Action | Key / Button |
|--------|--------------|
| New Game | `Ctrl+N` |
| Save Game | `Ctrl+S` |
| Load Game | `Ctrl+O` |
| Undo Move | `Ctrl+Z` |
| Confirm Move | `Enter` or click **Confirm** |
| Zoom In/Out | `Ctrl + Mouse Wheel` |
| Show FEN | `Ctrl+F` |
| Show Remote Dock | `Ctrl+Shift+R` |

## Custom Piece Sets

Place your own piece sets in the `piecesets/` directory:

```
piecesets/
├── default/
│   ├── wk.png   # White King
│   ├── bk.png   # Black King
│   └── ...
└── my_set/
    ├── wk.png
    └── ...
```

**Naming:** `{color}{type}.png`  
- `w` / `b` – White / Black  
- `k` `q` `r` `b` `n` `p` – King, Queen, Rook, Bishop, Knight, Pawn

## File Formats

### `.chess` – Game Save
Stores full state: FEN, player colors, move history, clocks, difficulty, and more.

### `.pgn` – Portable Game Notation
Standard format for sharing games with other chess software.

##  Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Open a pull request

## License

GPLv3 – see [LICENSE](LICENSE) for details.

## Credits

- **[Stockfish](https://stockfishchess.org/)** – Chess engine
- **[Qt](https://www.qt.io/)** – Application framework
- **Maurizio Monge** – Chess piece sets (Celtic, Fantasy, Eyes, Skulls, Spatial, and more)  
  Piece sets used under permission with attribution.  
  https://poisson.phc.dm.unipi.it/~monge/chess_art.php

## Issues

Report bugs: [GitHub Issues](https://github.com/alamahant/Pawns/issues)

---
Copyright © 2026 Alamahant. All rights reserved.

