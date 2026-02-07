# LordsAWar Fork - Project Guide

## Build & Run

```bash
# Build
make -j4

# Run (from project root)
./src/lordsawar
```

## File Paths

### Configuration
- Config file: `~/.config/lordsawar-fork/lordsawarrc`

### Data Storage
- User data: `~/.local/share/lordsawar-fork/`
- Game data path (set in config): Check `<d_datapath>` in lordsawarrc
- Default local install: `local_install/usr/local/share/lordsawar/`

### Source Data Files
- UI files (Glade): `dat/glade/*.ui`
- After modifying UI files, copy to the data path location:
  ```bash
  cp dat/glade/<file>.ui local_install/usr/local/share/lordsawar/glade/
  ```

## Key Source Files

### Battle System
- `src/fight.cpp` - Battle calculation logic (pre-calculates all combat events)
- `src/fight.h` - Fight class, FightItem struct, Fighter class
- `src/gui/fight-window.cpp` - Battle animation display
- `src/gui/fight-window.h` - FightWindow class, ArmyItem struct
- `src/combat-damage.h` - Deterministic damage formula
- `dat/glade/fighter.ui` - Individual army display widget in battle
- `dat/glade/fight-window.ui` - Battle window layout

### Architecture Notes
- Battle calculation happens BEFORE animation (Fight::battle() pre-computes all events)
- FightItem list stores combat events for replay by FightWindow
- ArmyItem.hp is local to animation (separate from Army::d_hp)
