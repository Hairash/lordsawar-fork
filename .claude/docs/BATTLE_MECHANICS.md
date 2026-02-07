# LordsAWar Battle Mechanics

This document describes how combat is calculated in LordsAWar.

## Recent Changes: Deterministic Combat System

The combat system was updated from a random dice-based system to a fully deterministic damage calculation.

### What Changed

| Aspect | Before | After |
|--------|--------|-------|
| Damage calculation | Random dice rolls until hit | Deterministic formula |
| Damage per hit | Always 1 HP | Fractional (e.g., 1.125 HP) |
| HP data type | `guint32` (integer) | `double` (floating-point) |
| Combat flow | Both sides roll each round | Strict alternation (A→D→A→D...) |
| Predictability | Random outcomes | Same matchup = same result |

### Why the Change

- **Predictable outcomes**: Players can calculate exact combat results
- **Preserves balance**: The formula uses the same strength relationships as the original probability system
- **Simpler logic**: No need for random number generation or re-rolling on ties

### Files Modified

| File | Changes |
|------|---------|
| `src/combat-damage.h` | **NEW**: Shared damage calculation function and constants |
| `src/army.h` | HP types changed to `double` |
| `src/army.cpp` | `damage()`, `heal()` updated for floating-point |
| `src/fight.h` | Added `calculateDeterministicDamage()`, `d_attacker_turn` |
| `src/fight.cpp` | Uses shared damage formula, alternating hit logic |
| `src/fight-test.cpp` | **NEW**: Unit tests using shared damage function |
| `src/defs.h` | Constants moved to `combat-damage.h` |
| `src/gui/fight-window.cpp` | Updated for `double` HP |
| `src/editor/battle-calculator-dialog.cpp` | Updated for `double` HP |

---

## Key Source Files

| File | Purpose |
|------|---------|
| `src/combat-damage.h` | **Shared damage formula** (used by game and tests) |
| `src/fight.cpp` | Core battle calculation logic |
| `src/fight.h` | Fight class definition, enums |
| `src/fight-test.cpp` | Unit tests for combat system |
| `src/army.cpp` | Army stats, damage, healing |
| `src/army.h` | Army attributes and bonus flags |
| `src/armybase.h` | Army bonus enum definitions |
| `src/hero.cpp` | Hero command bonus calculation |
| `src/Backpack.cpp` | Item bonus counting |
| `src/city.cpp` | City defense level |
| `src/defs.h` | Game constants |
| `src/stack.cpp` | Stack reset and healing |

## Combat Algorithm Overview

Combat is fully deterministic with alternating hits:

1. Attacker's front army hits defender's front army
2. Check if defender dies (HP <= 0); if so, remove and replace with next defender
3. Defender's (new) front army hits attacker's front army
4. Check if attacker dies; if so, remove and replace with next attacker
5. Repeat alternating hits until one side is eliminated

## Deterministic Damage System

### Constants (`src/defs.h`)

- **Normal Combat dice sides:** 20 (`BATTLE_DICE_SIDES_NORMAL = 20`)
- **Intense Combat dice sides:** 24 (`BATTLE_DICE_SIDES_INTENSE = 24`)
- **Damage Multiplier:** 5.0 (`DETERMINISTIC_DAMAGE_MULTIPLIER = 5.0`)
- **Minimum Damage:** 0.1 (`MINIMUM_DAMAGE_PER_HIT = 0.1`)

### Damage Formula (`src/combat-damage.h`)

```cpp
inline double calculateCombatDamage(int attacker_strength, int defender_strength, bool intense_combat) {
    double dice_sides = intense_combat ? 24 : 20;
    double hit_chance = (attacker_strength / dice_sides) *
                        ((dice_sides - defender_strength) / dice_sides);
    double damage = hit_chance * DETERMINISTIC_DAMAGE_MULTIPLIER;
    return std::max(damage, MINIMUM_DAMAGE_PER_HIT);  // Ensure minimum 0.1 damage
}
```

This function is defined in `combat-damage.h` and used by both the Fight class and unit tests.

**Formula:** `damage = (attacker_str / dice_sides) * ((dice_sides - defender_str) / dice_sides) * 5`

### Example Calculation

Attacker strength 5 vs Defender strength 2 (normal combat, d20):
- Attacker damage: `(5/20) * ((20-2)/20) * 5 = 0.25 * 0.9 * 5 = 1.125`
- Defender damage: `(2/20) * ((20-5)/20) * 5 = 0.10 * 0.75 * 5 = 0.375`

**Key insight:** Higher strength means dealing MORE damage and receiving LESS damage. The formula preserves the relative advantages from the original probability-based system.

## Strength Calculation

Final combat strength is built in layers. Each layer modifies `terrain_strength`.

### Layer 1: Base Strength (`src/fight.cpp:319-329`)

```cpp
if (army->getStat(Army::SHIP))
    terrain_strength = Army::BOAT_STRENGTH;  // Always 4 for ships
else
    terrain_strength = army->getStat(Army::STRENGTH);  // 1-9
```

### Layer 2: Terrain Modifiers (`src/fight.cpp:331-378`)

Individual army bonuses based on current terrain:

| Bonus Flag | Terrain | Effect |
|------------|---------|--------|
| `ADD1STRINOPEN` | Grass | +1 |
| `ADD2STRINOPEN` | Grass | +2 |
| `ADD1STRINFOREST` | Forest | +1 |
| `ADD2STRINFOREST` | Forest | +2 |
| `ADD1STRINHILLS` | Hills/Mountains | +1 |
| `ADD2STRINHILLS` | Hills/Mountains | +2 |
| `ADD1STRINCITY` | City/Tower | +1 |
| `ADD2STRINCITY` | City/Tower | +2 |

**Note:** Terrain bonuses do NOT apply to:
- Naval units (ships always use boat strength)
- Fortified armies in towers (tower bonus overrides terrain)

**Cap:** `terrain_strength` cannot exceed 9.

### Layer 3: Stack Bonuses (`src/fight.cpp:380-535`)

These bonuses apply to ALL armies in the stack.

#### Non-Hero Stack Bonuses (highest single value taken)

| Bonus Flag | Effect |
|------------|--------|
| `ADD1STACK` | +1 to all stack members |
| `ADD2STACK` | +2 to all stack members |
| `ADD1STACKINHILLS` | +1 if in hills terrain |

#### Hero Command Bonus (`src/hero.cpp:119-130`)

Based on hero's strength stat:

```cpp
if (hero_strength == 9) return 3;
else if (hero_strength > 6) return 2;
else if (hero_strength > 3) return 1;
else return 0;
```

| Hero Strength | Command Bonus |
|---------------|---------------|
| 9 | +3 |
| 7-8 | +2 |
| 4-6 | +1 |
| 1-3 | +0 |

#### Item Bonuses (`src/Backpack.cpp:106-119`)

Items in hero's backpack stack additively:

| Item Bonus | Effect |
|------------|--------|
| `ADD1STACK` | +1 |
| `ADD2STACK` | +2 |
| `ADD3STACK` | +3 |

#### City Defense Bonus (`src/city.cpp`)

When defending in a city:

| City Condition | Defense Level | Bonus |
|----------------|---------------|-------|
| Burnt city | 0 | 0 |
| Neutral city (≤2 production slots) | 1 | 0 |
| Player city (≤2 production slots) | 2 | 1 |
| Neutral city (>2 production slots) | 2 | 1 |
| Player city (>2 production slots) | 3 | 2 |

Formula: `city_bonus = city_defense_level - 1`

#### Fortification Bonus

Armies in towers get +1 bonus.

#### Total Bonus Calculation (`src/fight.cpp:521-525`)

```cpp
total_bonus = non_hero_bonus + hero_bonus + fortify_bonus + city_bonus;
if (total_bonus > 5)
    total_bonus = 5;  // HARD CAP
```

This total bonus is added to `terrain_strength` for ALL units in the stack.

### Layer 4: Enemy Debuffs (`src/fight.cpp:537-564`)

Some units can reduce enemy strength:

| Bonus Flag | Effect |
|------------|--------|
| `SUB1ENEMYSTACK` | -1 to all enemies |
| `SUB2ENEMYSTACK` | -2 to all enemies |
| `SUBALLCITYBONUS` | Negates defender's city bonus |
| `SUBALLNONHEROBONUS` | Negates non-hero stack bonuses |
| `SUBALLHEROBONUS` | Negates hero command bonus |

**Minimum strength:** Cannot go below 1.

## Complete Strength Example

**Attacker:** Strength 5 hero with +2 item, attacking from grass
- Base: 5
- Terrain (grass, no bonus): 0
- Hero command (5 > 3): +1
- Item: +2
- Total bonus: 3 (under 5 cap)
- **Final: 5 + 3 = 8**

**Defender:** Strength 6 unit in fortified hills with `ADD1STRINHILLS`
- Base: 6
- Terrain (hills + bonus): +1 → 7
- Fortify bonus: +1
- Total bonus: 1
- **Final: 7 + 1 = 8**

**Combat odds with d20:**
- Attacker needs roll < 8 to hit: 8/20 = 40%
- Defender needs roll < 8 to hit: 8/20 = 40%

## Battle Flow

### Single Combat Round (`src/fight.cpp:doRound`)

Each round consists of ONE hit from ONE side (alternating):

```cpp
bool Fight::doRound() {
    Fighter* attacker = d_att_close.front();
    Fighter* defender = d_def_close.front();

    if (d_attacker_turn) {
        // Attacker hits defender
        fightArmies(attacker, defender);
        if (defender->army->getHP() <= 0.0)
            remove(defender);  // Next defender takes over
    } else {
        // Defender hits attacker
        fightArmies(defender, attacker);
        if (attacker->army->getHP() <= 0.0)
            remove(attacker);  // Next attacker takes over
    }

    d_attacker_turn = !d_attacker_turn;  // Swap turn

    return !d_def_close.empty() && !d_att_close.empty();
}
```

**Key behavior:** When a unit dies, it does NOT get to counter-attack. The next unit in the stack immediately takes its place and will get the next turn in the alternation sequence.

### Full Battle (`src/fight.cpp:201-254`)

```cpp
void Fight::battle(bool intense) {
    d_intense_combat = intense;

    while (doRound()) {
        // Continue until one side eliminated
    }

    d_result = determineResult();

    if (d_type == FOR_KICKS) {
        // Restore HP for simulations
        restoreInitialHP();
    }
}
```

### Battle Results

```cpp
enum Result {
    DRAW = 0,           // Not possible (MAX_ROUNDS = 0)
    ATTACKER_WON = 1,
    DEFENDER_WON = 2
};
```

## HP and Damage System

### HP Data Type

HP is stored as `double` to support fractional damage from the deterministic combat system.

### Damage (`src/army.cpp`)

Damage is applied directly and can be fractional:

```cpp
bool Army::damage(double damageDone) {
    d_hp -= damageDone;
    if (d_hp <= 0.0) {
        d_hp = 0;
        return true;   // Dead
    }
    return false;
}
```

### HP Persistence Within Battle

HP damage is **permanent within a battle**. If Army A (4 HP) takes 3 hits fighting Army B, Army A continues with 1 HP against the next defender.

### HP Persistence Between Turns

Armies heal at the start of each turn via `stacksReset()` → `heal()`.

### Healing Formula (`src/army.cpp`)

```cpp
void Army::heal(double hp) {
    if (hp == 0) {
        // End-of-turn healing
        hp = d_max_hp / 10;  // 10% of max HP
        if (hp <= 5)
            hp = 1;          // Minimum 1 HP
        else
            hp += 5;         // Bonus for high-HP units
    }

    d_hp += hp;
    if (d_hp > d_max_hp)
        d_hp = d_max_hp;     // Cap at max
}
```

**Healing rate:** ~10% of max HP per turn, minimum 1 HP.

### HP Display

**HP is NOT displayed in the game UI.** The army tooltip shows:
- Name
- Strength
- Moves (current/max)
- Upkeep

The only place HP is visible is in the **editor's battle calculator**.

## Battle Types

### FOR_KEEPS (Default)

Used for all real gameplay battles:
- Damage is permanent
- Dead armies are removed from the game
- XP is awarded for hits

### FOR_KICKS

Used internally for simulations:
- HP is restored after battle
- No XP awarded
- Used by:
  - AI odds calculation (runs 100 battles to estimate win %)
  - Stack info dialog (to calculate modified strength display)

```cpp
// AI calculating odds (src/player.cpp:1554-1560)
for (int i = 0; i < 100; i++) {
    Fight fight(stack, target, Fight::FOR_KICKS);
    fight.battle(intense_combat);
    if (fight.getResult() == Fight::ATTACKER_WON)
        percent += 1.0;
}
```

There is **no UI setting** to choose between battle types.

## Multi-Stack Battles

When defending a city, ALL stacks in the city participate (`src/fight.cpp:69-126`):

```cpp
// Collect all defenders in city
for (each stack on city tile) {
    defenders.push_back(stack);
}
```

Combat continues until ALL armies on one side are dead.

## Army Ordering

Armies fight in order determined by `orderArmies()`. The fight order can be customized in the game UI (fight order dialog).

## Experience and Medals

### XP Recording (`src/fight.cpp:fightArmies`)

Only in `FOR_KEEPS` battles, XP is now based on damage dealt:

```cpp
if (d_type == FOR_KEEPS) {
    attacker->setNumberHasHit(attacker->getNumberHasHit() + (damage / xp_factor));
    defender->setNumberHasBeenHit(defender->getNumberHasBeenHit() + (damage / xp_factor));
}
```

### Medal System

Three medal types available, awarded based on battle performance. Stored as `d_medal_bonus[3]` boolean array.

## Strategic Implications

1. **Strength matters exponentially** - Higher strength means both MORE damage dealt AND LESS damage received
2. **Stack bonuses are powerful** - A hero with items can add +5 to entire stack
3. **City defense is significant** - +2 bonus from developed cities
4. **HP doesn't heal instantly** - Damaged armies remain vulnerable
5. **Front-line positioning matters** - Strong units should absorb hits first
6. **Predictable outcomes** - Combat is deterministic; the same matchup always produces the same result
7. **Attacker advantage** - Attacker always strikes first, which matters when units can kill in one hit
8. **No HP visibility** - You can't see which enemies are wounded (displayed as truncated integers)

---

## Debug Logging

Combat debug logging can be enabled to trace battle calculations in the console.

### Enable Debug Output

In `src/fight.cpp`, the debug macro controls logging:

```cpp
// ENABLED (uncomment to see combat logs):
#define debug(x) {std::cerr<<__FILE__<<": "<<__LINE__<<": "<<x<<std::endl<<std::flush;}

// DISABLED (comment out to silence logs):
//#define debug(x)
```

### Log Format

When enabled, combat logs show:

```
fight.cpp: 302: [COMBAT] === Turn 0 (Attacker's turn) ===
fight.cpp: 671: [COMBAT] Round 0: Warrior (1234) hits Archer (5678) for 1.13 damage. Defender HP: 0.88/2
fight.cpp: 302: [COMBAT] === Turn 1 (Defender's turn) ===
fight.cpp: 671: [COMBAT] Round 1: Archer (5678) hits Warrior (1234) for 0.38 damage. Defender HP: 1.62/2
...
fight.cpp: 325: [COMBAT] Archer (5678) DIED!
```

### Log Contents

- **Turn number and side**: Shows whose turn it is (Attacker/Defender)
- **Unit names and IDs**: Identifies which armies are fighting
- **Damage dealt**: Exact damage from deterministic formula
- **Defender HP**: Current HP / Max HP after damage
- **Death notifications**: When a unit's HP reaches 0

---

## HP Display in Fight Window

The fight window now shows HP values on each army icon during battle:

- Format: `current/max` (e.g., "2/2" → "1/2" → "0/2")
- HP updates in real-time as damage is applied
- Uses `std::ceil()` to round up for display (shows "1/2" instead of "0.88/2")

This provides visual feedback for the deterministic combat system where armies may take multiple hits before dying.

---

## Unit Tests

See [QUICKSTART.md](QUICKSTART.md#running-tests) for instructions on running the combat system unit tests.
