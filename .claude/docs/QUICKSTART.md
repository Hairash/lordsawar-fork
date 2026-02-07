# LordsAWar! Quick Start Guide

## What is LordsAWar!?

LordsAWar! is a free, open-source turn-based strategy game inspired by Warlords II. Command armies, capture cities, recruit heroes, and conquer your enemies!

## Running the Game

### On macOS

**First Time Setup:**
```bash
cd /path/to/lordsawar
./build-macos.sh
```

**Start the Game:**
```bash
./src/lordsawar
```

### On Linux

**Install Dependencies (Ubuntu/Debian):**
```bash
sudo apt-get install automake autoconf libtool m4 autotools-dev autopoint \
    g++ libsigc++-2.0-dev libglibmm-2.4-dev libgtkmm-3.0-dev gettext \
    libgstreamermm-1.0-dev intltool libarchive-dev libxslt1-dev libxml++2.6-dev
```

**Build:**
```bash
./autogen.sh
./configure
make -j$(nproc)
sudo make install
```

**Run:**
```bash
lordsawar
```

## Game Modes

### Single Player
Start a new game against AI opponents:
1. Launch the game
2. Select "New Game"
3. Choose a map
4. Configure players (Human vs AI)
5. Start playing!

### Hot-Seat Multiplayer
Play with friends on the same computer - players take turns.

### Network Multiplayer
Play with friends over the network:
1. One player hosts the game
2. Others connect using the host's IP address

### Scenario Editor
Create your own maps and scenarios:
```bash
./src/lordsawar --editor
```

## Basic Controls

| Action | How To |
|--------|--------|
| Select stack | Click on your army stack |
| Move stack | Right-click destination |
| Attack | Move onto enemy stack/city |
| Next stack | Press `N` or click "Next" |
| End turn | Click "End Turn" button |
| View city | Click on a city you own |
| Open menu | Press `Escape` |

## Gameplay Tips

### Starting Out
- **Explore early** - Send units to discover the map
- **Capture neutral cities** - They provide gold and can produce units
- **Protect your hero** - Heroes are powerful and can search ruins

### Cities
- Cities produce gold each turn
- Build army units in cities
- Vector units to send new armies to the front lines

### Heroes
- Search ruins for treasure and allies
- Collect items to boost stats
- Accept quests from temples for rewards

### Combat
- Stack up to 8 units together
- Terrain affects combat (cities give defense bonus)
- Stronger units fight better, but numbers matter too

## Command Line Options

```bash
./src/lordsawar [OPTIONS] [FILE]

Options:
  -t, --test          Start with a test scenario
  -s, --stress-test   AI vs AI automated test (no GUI)
  --editor            Launch the scenario editor
  -h, --help          Show all options
```

## Running Tests

### Combat System Unit Tests

Unit tests for the deterministic combat system are in `src/fight-test.cpp`.

**Compile and run:**
```bash
cd src
g++ -std=c++11 -o fight-test fight-test.cpp
./fight-test
```

**Expected output:**
```
========================================
Deterministic Combat System Unit Tests
========================================
Testing: combat-damage.h (real implementation)

Running single_hit_damage_calculation... PASSED
Running combat_ends_when_defender_dies... PASSED
Running defender_replacement_on_death... PASSED
Running attacker_replacement_on_death... PASSED
Running equal_strength_combat... PASSED
Running minimum_damage_threshold... PASSED
Running intense_combat_mode... PASSED
Running multiple_round_combat... PASSED
Running deterministic_results... PASSED
Running constants_verification... PASSED

========================================
Results: 10 passed, 0 failed
========================================
```

### Test Architecture

The tests use the **same code** as the game via a shared header:

```
combat-damage.h          <-- Shared damage calculation function
    |
    +-- fight.cpp        <-- Game uses it via Fight::calculateDeterministicDamage()
    |
    +-- fight-test.cpp   <-- Tests use it directly
```

This ensures tests verify the **real implementation**, not a copy.

### Test Cases

| Test | Description |
|------|-------------|
| `single_hit_damage_calculation` | Verifies damage formula: str 5 vs str 2 = 1.125 damage |
| `combat_ends_when_defender_dies` | Defender with 1 HP dies in one hit, combat ends |
| `defender_replacement_on_death` | When D1 dies, D2 takes over and gets next hit |
| `attacker_replacement_on_death` | When A1 dies, A2 takes over on attacker's turn |
| `equal_strength_combat` | Equal str 5 units deal 0.9375 damage each |
| `minimum_damage_threshold` | Weak attackers still deal at least 0.1 damage |
| `intense_combat_mode` | Verifies d24 produces less damage than d20 |
| `multiple_round_combat` | Full combat: str 5 vs str 2, both HP 2, verifies exact HP |
| `deterministic_results` | Same inputs always produce identical outputs |
| `constants_verification` | Verifies combat constants are correct |

## Troubleshooting

### Game won't start / Can't find data files
Edit `~/.config/lordsawar/lordsawarrc` and set the correct data path:
```xml
<d_datapath>/path/to/lordsawar/local_install/usr/local/share/lordsawar</d_datapath>
```

### No sound
Sound requires gstreamermm. Build with `--enable-sound` or install:
```bash
# macOS
brew install gstreamermm

# Ubuntu
sudo apt-get install libgstreamermm-1.0-dev
```

### GTK warnings on macOS
Minor GTK warnings about pixbuf loaders can be ignored - the game works fine.

## More Information

- **Full Manual:** http://www.nongnu.org/lordsawar/manual/
- **Report Bugs:** https://savannah.nongnu.org/bugs/?group=lordsawar
- **License:** GNU GPL v3 or later

---

*Enjoy conquering the realm!*
