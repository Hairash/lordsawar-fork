# LordsAWar! Architecture Documentation

## Table of Contents

1. [Project Overview](#project-overview)
2. [Architecture](#architecture)
3. [Directory Structure](#directory-structure)
4. [Core Components](#core-components)
5. [Building the Project](#building-the-project)
6. [Game Mechanics](#game-mechanics)
7. [File Formats](#file-formats)
8. [Multiplayer System](#multiplayer-system)
9. [AI System](#ai-system)
10. [Asset System](#asset-system)
11. [Development Guide](#development-guide)

---

## Project Overview

**LordsAWar!** is a free and open-source turn-based strategy game that is a clone of Warlords II by the Strategic Studies Group (SSG). The project emphasizes software freedom and is licensed under the GNU General Public License version 3 or later.

### Key Features

- Turn-based strategic warfare simulation
- Single-player, hot-seat multiplayer, and networked multiplayer
- Full scenario editor for creating custom maps
- Multiple AI difficulty levels
- Quest system with hero units
- Fog of war
- Diplomacy system
- Internationalization support (multiple languages)

### Current Version

0.3.3-pre3

### Project Links

- Manual: http://www.nongnu.org/lordsawar/manual/

---

## Architecture

LordsAWar! follows a layered architecture with clear separation of concerns:

```
+--------------------------------------------------+
|                    GUI Layer                      |
|              (GTKmm 3.0+ / Glade UI)              |
+--------------------------------------------------+
|                 Graphics Layer                    |
|          (Cairo-based rendering, BigMap)          |
+--------------------------------------------------+
|                 Network Layer                     |
|         (TCP/IP multiplayer, game servers)        |
+--------------------------------------------------+
|                Game Engine Layer                  |
|    (Game logic, combat, AI, pathfinding, etc.)    |
+--------------------------------------------------+
|               Asset Management Layer              |
|       (TAR-based packages, XML configuration)     |
+--------------------------------------------------+
```

### Design Patterns Used

1. **Singleton Pattern** - `GameMap`, `Configuration`
2. **Signal/Slot Pattern** - Heavy use of `libsigc++` for event-driven architecture
3. **Observer Pattern** - GUI components observe game state changes
4. **Factory Pattern** - Player creation (real vs. AI), Army creation
5. **Strategy Pattern** - AI strategies (fast, smart, dummy)

### Technology Stack

| Component | Technology |
|-----------|------------|
| Language | C++ (C++11 standard) |
| GUI Toolkit | GTKmm 3.0+ |
| XML Processing | libxml++ 2.6+ |
| XSLT | libxslt 1.1.20+ |
| Archive Handling | libarchive 0.28+ |
| Sound | gstreamermm 1.0+ (optional) |
| Build System | GNU Autotools |
| Internationalization | GNU gettext |

---

## Directory Structure

```
lordsawar/
├── src/                    # Source code
│   ├── gui/                # GUI components (GTK+ based)
│   ├── editor/             # Scenario editor
│   ├── gls/                # Game List Server
│   ├── ghs/                # Game Host Server
│   └── utils/              # Utility tools
├── dat/                    # Game assets
│   ├── army/               # Army unit graphics and definitions
│   ├── tilesets/           # Map tile graphics
│   ├── shield/             # Player shield graphics
│   ├── citysets/           # City graphics
│   ├── map/                # Pre-built scenario maps
│   ├── music/              # Background music
│   ├── glade/              # GTK Glade UI definitions
│   └── various/            # Miscellaneous assets
├── po/                     # Internationalization files
├── doc/                    # Documentation
└── help/                   # User manual
```

---

## Core Components

### Libraries

LordsAWar! is built from several static libraries:

| Library | Description |
|---------|-------------|
| `liblordsawar.la` | Main game engine (~97K lines) |
| `liblordsawargfx.la` | Graphics rendering |
| `liblordsawarnet.la` | Network/multiplayer support |
| `liblordsawargamelist.la` | Game list server client |
| `liblordsawargamehost.la` | Game host server client |

### Applications

| Application | Description |
|-------------|-------------|
| `lordsawar` | Main game executable |
| `editor` | Scenario/map editor |
| `gls` | Game List Server |
| `ghs` | Game Host Server |

### Key Source Files

#### Game Engine
- `game.h/game.cpp` - Main Game controller class
- `GameMap.h/GameMap.cpp` - Game world map
- `GameScenario.h/GameScenario.cpp` - Scenario state management

#### Player System
- `player.h/player.cpp` - Player abstraction (abstract class)
- `real_player.h/real_player.cpp` - Human player implementation
- `ai_dummy.cpp` - Neutral player AI
- `ai_fast.cpp` - Easy AI player
- `ai_hard.cpp` - Hard AI player

#### Game Objects
- `hero.h/hero.cpp` - Hero unit class
- `army.h/army.cpp` - Army/unit class
- `city.h/city.cpp` - City class
- `stack.h/stack.cpp` - Stack of army units
- `Quest.h/Quest.cpp` - Quest system

#### Combat & Movement
- `fight.h/fight.cpp` - Combat system
- `PathCalculator.h/PathCalculator.cpp` - Movement pathfinding
- `FogMap.h/FogMap.cpp` - Fog of war implementation

---

## Building the Project

### Prerequisites

- C++ compiler with C++11 support
- GTKmm 3.0+
- GlibMM 2.4+
- libsigc++ 2.0+
- libxml++ 2.6+
- libxslt 1.1.20+
- libarchive 0.28+
- gstreamermm 1.0+ (optional, for sound)

### Build Steps

```bash
# Generate configure script
./autogen.sh

# Configure build
./configure [options]

# Compile
make

# Install
make install
```

### Configure Options

| Option | Description | Default |
|--------|-------------|---------|
| `--enable-gls` | Compile game list server | yes |
| `--enable-ghs` | Compile game host server | yes |
| `--enable-sound` | Enable sound/music | yes |
| `--with-resolution=RES` | Set default screen resolution | 800x600 |
| `--enable-optimizations` | Enable compiler optimizations | yes |

### Testing

Run automated AI vs AI tests:
```bash
./lordsawar --stress-test
```

### Building on macOS

A convenience script is provided for macOS builds:

```bash
./build-macos.sh
```

Or manually:

```bash
# Install dependencies
brew install automake autoconf libtool intltool gettext glib \
    gtkmm3 glibmm libsigc++ libxml++ libxslt libarchive pkg-config gnu-tar

# Set up environment
export PATH="/opt/homebrew/opt/libtool/libexec/gnubin:/opt/homebrew/opt/gettext/bin:/opt/homebrew/opt/gnu-tar/libexec/gnubin:$PATH"
export ACLOCAL_PATH="/opt/homebrew/share/aclocal:$ACLOCAL_PATH"
export PKG_CONFIG_PATH="/opt/homebrew/opt/libxslt/lib/pkgconfig:/opt/homebrew/opt/libarchive/lib/pkgconfig:$PKG_CONFIG_PATH"

# Build
./autogen.sh
CPPFLAGS="-I/opt/homebrew/opt/libarchive/include" \
LDFLAGS="-L/opt/homebrew/opt/libarchive/lib" \
./configure --disable-sound
make -j$(sysctl -n hw.ncpu)

# Install locally (no sudo required)
make DESTDIR=$(pwd)/local_install install
```

**Configuration:** Edit `~/.config/lordsawar/lordsawarrc` to set the data path:
```xml
<d_datapath>/path/to/lordsawar/local_install/usr/local/share/lordsawar</d_datapath>
```

---

## Game Mechanics

### Turn Structure

1. **Turn Initialization** - Reset movement points, collect taxes, pay upkeep
2. **Player Actions** - Move stacks, manage cities, engage in combat
3. **Production** - New units appear in cities
4. **Turn End** - Save actions, advance to next player

### Combat System

Combat is resolved unit-by-unit:
1. Each side sends out a champion
2. Champions fight until one is defeated
3. The winner stays until defeated
4. Battle ends when all units on one side are eliminated

Combat factors:
- Unit strength
- Terrain bonuses
- City defense bonuses
- Hero command bonuses
- Items and artifacts

### City Management

Cities provide:
- **Gold** - Income each turn
- **Production** - Create new army units
- **Defense** - Defensive bonus in combat

City operations:
- Change production
- Buy new production types
- Vector units to other locations
- Upgrade defenses

### Hero System

Heroes are special units that:
- Can search ruins
- Carry items in a backpack
- Accept quests from temples
- Provide command bonuses to stacks
- Level up with experience

### Quest Types

| Quest | Objective |
|-------|-----------|
| `KILLHERO` | Kill a specific enemy hero |
| `KILLARMIES` | Kill a number of enemy units |
| `CITYSACK` | Sack a specific city |
| `CITYRAZE` | Raze a specific city |
| `CITYOCCUPY` | Occupy a specific city |
| `KILLARMYTYPE` | Kill specific army types |
| `PILLAGEGOLD` | Pillage a gold amount |

---

## File Formats

### Save Game Files (`.sav`)

Save games are **uncompressed TAR archives** containing:
- Scenario XML file (main game state)
- `.lwc` - LordsAWar cityset
- `.lws` - LordsAWar shieldset
- `.lwa` - LordsAWar armyset
- `.lwt` - LordsAWar tileset

### Asset Package Files

| Extension | Contents |
|-----------|----------|
| `.lwc` | Cityset - buildings, city graphics |
| `.lws` | Shieldset - shields, progress bars |
| `.lwa` | Armyset - army unit graphics and stats |
| `.lwt` | Tileset - terrain graphics |

Each package is a TAR archive containing an XML configuration file and PNG images.

### XML Structure

The main scenario file follows this structure:

```xml
<lordsawar version="xxx">
    <counter>...</counter>
    <itemlist>...</itemlist>
    <playerlist>...</playerlist>
    <map>...</map>
    <citylist>...</citylist>
    <templelist>...</templelist>
    <ruinlist>...</ruinlist>
    <rewardlist>...</rewardlist>
    <signpostlist>...</signpostlist>
    <roadlist>...</roadlist>
    <portlist>...</portlist>
    <bridgelist>...</bridgelist>
    <stonelist>...</stonelist>
    <questlist>...</questlist>
    <vectoredunitlist>...</vectoredunitlist>
    <turnlist>...</turnlist>
    <herotemplates>...</herotemplates>
    <scenario>...</scenario>
</lordsawar>
```

See `doc/Savefile` for complete format documentation.

---

## Multiplayer System

### Architecture

```
                    +------------------+
                    |  Game List       |
                    |  Server (GLS)    |
                    +--------+---------+
                             |
              +--------------+--------------+
              |                             |
    +---------v---------+         +---------v---------+
    |  Game Host        |         |  Game Host        |
    |  Server (GHS)     |         |  Server (GHS)     |
    +---------+---------+         +---------+---------+
              |                             |
    +---------+----------+        +---------+----------+
    |         |          |        |         |          |
 Client1  Client2    Client3   Client1  Client2    Client3
```

### Game List Server (GLS)

- Maintains a list of available games
- Allows game advertisement and discovery
- Tracks recently played games

### Game Host Server (GHS)

- Hosts multiplayer game sessions
- Synchronizes game state between clients
- Handles player connections and disconnections

### Network Actions

Player actions are encoded as **Action** objects and transmitted over the network to maintain game state synchronization. Action types include:
- Stack movement, split, join
- Combat resolution
- City operations
- Diplomacy changes
- Turn management

---

## AI System

### AI Player Types

| Type | Class | Behavior |
|------|-------|----------|
| Neutral | `AI_Dummy` | Stays still, waits to be conquered |
| Easy | `AI_Fast` | Makes stacks of 8, attacks nearby cities |
| Hard | `AI_Hard` | Evaluates threats, calculates army strength |

### AI Class Hierarchy

```
Player (abstract)
    └── RealPlayer (human)
            ├── AI_Dummy (neutral)
            ├── AI_Fast (easy)
            └── AI_Hard (hard)
```

### Key AI Concepts

1. **Stack Management** - Grouping units for efficient movement and combat
2. **Vectoring** - Directing new units to strategic locations
3. **Threat Assessment** - Evaluating enemy strength and prioritizing targets
4. **Temple Visiting** - Blessing units for combat bonuses
5. **Item Collection** - Heroes picking up useful items

### AI Limitations

The current AI:
- Does not create focal points with vectoring
- Does not reorder battle order strategically
- Does not take calculated risks (only 50/50 or 99/1 battles)
- Uses full map knowledge (no fog of war limitations)

See `doc/ai-notes.txt` for detailed AI documentation.

---

## Asset System

### Tileset Structure

Tilesets define terrain graphics with:
- Tile images for each terrain type
- Movement costs per terrain
- Smallmap appearance (patterns and colors)
- Road, bridge, and fog graphics
- Selectors for unit highlighting

### Terrain Types

| Type | Description | Base Movement Cost |
|------|-------------|-------------------|
| `GRASS` | Open plains | 2 |
| `WATER` | Lakes, rivers, ocean | - |
| `FOREST` | Forested areas | 4 |
| `HILLS` | Hilly terrain | 4 |
| `MOUNTAIN` | Impassable except by flight | - |
| `SWAMP` | Marshy terrain | 4 |

### Army Unit Properties

| Property | Description |
|----------|-------------|
| `strength` | Combat power (minimum 1) |
| `max_moves` | Movement points per turn |
| `sight` | Vision radius |
| `production` | Turns to produce |
| `upkeep` | Gold cost per turn |
| `move_bonus` | Terrain movement bonuses |
| `army_bonus` | Combat ability bonuses |

### Combat Bonuses

Units can have special abilities:
- `ADD1STRINOPEN` - +1 strength in open terrain
- `ADD2STRINCITY` - +2 strength in cities
- `ADD1STACK` - +1 strength to all stack units
- `SUB1ENEMYSTACK` - -1 strength to enemy stack
- `SUBALLCITYBONUS` - Negate enemy city bonuses
- `FORTIFY` - +1 when fortified
- `CONFER_MOVE_BONUS` - Share movement bonus with stack

---

## Development Guide

### Code Organization

- **Game logic** is in `src/` (independent of GUI)
- **GUI code** is in `src/gui/` (GTK+ dependent)
- **Editors** are in `src/editor/`
- **Network code** is in `src/` (files with `network_` prefix)

### Signal/Slot Architecture

The game uses `libsigc++` for event-driven communication:

```cpp
// Emitting a signal
signal_city_captured.emit(city);

// Connecting to a signal
player->signal_city_captured.connect(
    sigc::mem_fun(*this, &Game::on_city_captured));
```

This allows:
- GUI to observe game state changes
- Headless operation for stress testing
- Clean separation between game logic and presentation

### Adding New Features

1. **New Army Type** - Add to armyset XML, create graphics
2. **New Quest Type** - Extend `Quest` class, add to `Quest::Type` enum
3. **New AI Behavior** - Inherit from `RealPlayer`, implement virtual methods
4. **New Terrain Type** - Add to `Tile::Type`, create tileset graphics

### Testing

```bash
# Run stress test (AI vs AI)
./lordsawar --stress-test

# This runs without GUI and tests AI logic
```

### Contributing

- License: GNU GPL v3 or later
- Coding style: Use existing code as reference
- Signal-based architecture: Keep GUI and game logic separate

---

## Appendix: Player Types

```cpp
enum PlayerType {
    HUMAN = 0,      // Human player
    AI_FAST = 1,    // Easy AI
    AI_DUMMY = 2,   // Neutral player
    AI_SMART = 4,   // Hard AI
    NETWORKED = 8   // Network player
};
```

## Appendix: Diplomatic States

```cpp
enum DiplomaticState {
    PEACE = 1,          // At peace
    WAR_IN_FIELD = 2,   // War in the field
    FULL_WAR = 3        // Full war
};
```

## Appendix: Useful Commands

```bash
# Build with debug symbols
./configure CXXFLAGS="-g -O0"

# Run with verbose output
./lordsawar -v

# Start game list server
./gls

# Start game host server
./ghs
```

---

*This documentation was generated from source code analysis of LordsAWar! version 0.3.3-pre3*
