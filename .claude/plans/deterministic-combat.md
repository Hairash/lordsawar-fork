# Implementation Plan: Deterministic Combat System

## Summary

Replace the random dice-based combat system with a deterministic damage calculation. Instead of rolling dice until someone is hit, each side takes turns dealing damage based on their hit probability multiplied by a scaling factor (5). This makes combat outcomes predictable and removes RNG while preserving the relative strength advantages of the original system.

**New formula:**
- `damage = (attacker_strength / dice_sides) * ((dice_sides - defender_strength) / dice_sides) * 5`
- Attacker always strikes first, then defender counter-attacks
- Continue alternating until one army dies (HP <= 0)

## Files to Modify

- `src/fight.h` - Change HP types from `guint32` to `double` in FightItem, add new damage calculation method
- `src/fight.cpp` - Rewrite `fightArmies()` and `doRound()` for deterministic combat
- `src/army.h` - Change `d_hp` from `guint32` to `double`, update getter/setter signatures
- `src/army.cpp` - Update `damage()`, `heal()`, `setHP()`, `getHP()` for floating-point HP
- `src/defs.h` - Add damage multiplier constant

## Implementation Steps

### Phase 1: Update HP Data Type

- [x] In `src/army.h`, change HP storage from integer to floating-point
  - Change `guint32 d_hp;` to `double d_hp;`
  - Change `guint32 d_max_hp;` to `double d_max_hp;`
  - Update `getHP()` return type from `guint32` to `double`
  - Update `setHP(guint32 hp)` parameter to `setHP(double hp)`
  - Update `getStat()` to return `double` for HP stat (or add separate method)

- [x] In `src/army.cpp`, update HP-related methods
  - Update `damage(guint32 damageDone)` to `damage(double damageDone)`
  - Update comparison `d_hp <= 0` to handle floating point (use small epsilon or `<= 0.0`)
  - Update `heal()` method to work with floating-point values
  - Ensure HP capping logic works with doubles

- [x] In `src/fight.h`, update FightItem struct
  - Change `int damage;` to `double damage;` in FightItem struct

### Phase 2: Implement Deterministic Combat Logic

- [x] In `src/defs.h`, add damage multiplier constant
  ```cpp
  #define DETERMINISTIC_DAMAGE_MULTIPLIER 5.0
  ```

- [x] In `src/fight.cpp`, create new deterministic damage calculation function
  ```cpp
  double Fight::calculateDeterministicDamage(Fighter* attacker, Fighter* defender) {
      double dice_sides = d_intense_combat ? BATTLE_DICE_SIDES_INTENSE : BATTLE_DICE_SIDES_NORMAL;
      double hit_chance = (attacker->terrain_strength / dice_sides) *
                          ((dice_sides - defender->terrain_strength) / dice_sides);
      return hit_chance * DETERMINISTIC_DAMAGE_MULTIPLIER;
  }
  ```

- [x] In `src/fight.h`, declare the new method
  ```cpp
  double calculateDeterministicDamage(Fighter* attacker, Fighter* defender);
  ```

- [x] In `src/fight.cpp`, rewrite `fightArmies()` method
  - Remove the `while (damage == 0)` random loop
  - Implement one-hit-per-round structure:
    1. Determine current attacker (from attacking side's front army)
    2. Calculate damage using `calculateDeterministicDamage()`
    3. Apply damage to defender
    4. Check if defender dies (HP <= 0)
    5. If defender dies, remove from fight and get next defender
    6. Record FightItem for this single hit
    7. Return after ONE hit (not both sides attacking)
  - Keep XP tracking logic (adjust for new damage values)

### Phase 3: Update Combat Round Logic

- [x] In `src/fight.cpp`, update `doRound()` method
  - Implement alternating hit structure:
    - Track whose turn it is (attacker side or defender side)
    - Call `fightArmies()` for a single hit from the current side
    - Swap turn to the other side
    - Continue until one side has no armies left
  - Keep the front-army selection logic
  - Update death check to use `<= 0.0` comparison
  - Ensure proper removal of dead armies (immediate replacement from stack)

- [x] In `src/fight.cpp`, update `determineResult()` method
  - Update HP checks to use floating-point comparison (already works with `> 0` in `battle()`)

- [x] In `src/fight.cpp`, update `initial_hps` map
  - Change from `std::map<guint32, guint32>` to `std::map<guint32, double>`
  - Update `fillInInitialHPs()` accordingly

- [x] In `src/fight.h`, update initial_hps declaration
  - Change `std::map<guint32, guint32> initial_hps;` to `std::map<guint32, double> initial_hps;`
  - Update `getInitialHPs()` return type

### Phase 4: Update Dependent Code

- [x] Search for all usages of `getHP()` and ensure they handle `double`
  - `src/gui/fight-window.cpp` - Display logic (updated map type)
  - `src/editor/battle-calculator-dialog.cpp` - Editor display (updated map type, display column uses int for truncation)
  - Any save/load serialization code

- [x] Update XML serialization for HP
  - In `src/army.cpp`, update `save()` and constructor from XML_Helper
  - Ensure HP is saved/loaded as floating point (automatic via XML_Helper double overloads)

- [x] Update healing logic in `src/stack.cpp`
  - Ensure `heal()` calls work with new signature (already compatible)

### Phase 5: Handle Edge Cases

- [x] Add minimum damage threshold
  - Ensure very weak units still deal some damage (e.g., minimum 0.1)
  - Prevent infinite combat loops (added MINIMUM_DAMAGE_PER_HIT constant)

- [x] Handle HP display in UI
  - Decide whether to show fractional HP or round for display (truncate to int for display)
  - Update any HP display code in GUI (implicit truncation on display columns)

- [x] Update FOR_KICKS battle type
  - Ensure HP restoration works with floating-point values (already works via double initial_hps map)

## Decision Points

### Decision 1: HP Data Type
- **Proposed**: Use `double` for HP storage
- **Alternative**: Use fixed-point integer (e.g., HP * 1000) to avoid floating-point issues
- **Trade-offs**: `double` is simpler to implement and understand; fixed-point avoids potential floating-point comparison issues but adds complexity. Going with `double` for clarity.

### Decision 2: Combat Turn Structure
- **Chosen**: One hit per round with strict alternation
- **Structure**: Attacker hits → check death → defender hits → check death → next attacker → ...
- **Key mechanic**: The hit queue is permanent. If a defender dies, the next defender in line takes the next hit (doesn't get skipped). The alternation continues: side A, side B, side A, side B...
- **Example**: Attacker A1 hits Defender D1 → D1 dies → D2 hits A1 → A1 hits D2 → ...

### Decision 3: Death Check Timing
- **Chosen**: Check immediately after each hit
- **Behavior**: If attacker kills defender with a hit, that defender does NOT get to respond - it dies immediately
- **Replacement mechanic**: The next defender in the stack takes over and gets the next hit (maintaining alternation)
- **Example flow**:
  1. Attacker A1 hits Defender D1 for 1.125 damage
  2. D1 HP: 2 - 1.125 = 0.875 (alive) → D1 responds
  3. D1 hits A1 for 0.375 damage
  4. A1 HP: 2 - 0.375 = 1.625 (alive) → A1 responds
  5. A1 hits D1 for 1.125 damage
  6. D1 HP: 0.875 - 1.125 = -0.25 (dead) → D1 replaced by D2
  7. D2 hits A1 (it's still defender's turn in the alternation)

### Decision 4: Damage Multiplier Value
- **Proposed**: Use 5.0 as specified by user
- **Alternative**: Make it configurable via game options
- **Trade-offs**: Hardcoding 5.0 matches user spec; configurability adds complexity. Start with hardcoded, can refactor later.

## Testing

### Unit Tests (in `src/fight-test.cpp`)

- [x] Test 1: Single hit damage calculation
  - Input: Attacker str 5, Defender str 2, dice_sides 20
  - Expected: damage = (5/20) * ((20-2)/20) * 5 = 1.125

- [x] Test 2: Combat ends when defender dies mid-round
  - Setup: A1 (str 5, HP 2) vs D1 (str 2, HP 1)
  - Expected: A1 hits D1 for 1.125 → D1 dies (HP -0.125) → combat ends if no more defenders

- [x] Test 3: Defender replacement on death
  - Setup: A1 (str 5, HP 5) vs D1 (str 2, HP 1), D2 (str 3, HP 2)
  - Expected: A1 kills D1 → D2 takes next hit → alternation continues

- [x] Test 4: Attacker replacement on death
  - Setup: A1 (str 2, HP 1), A2 (str 4, HP 3) vs D1 (str 5, HP 5)
  - Expected: When A1 dies, A2 takes over on attacker's turn

- [x] Test 5: Equal strength combat
  - Setup: Both str 5, HP 2
  - Expected: Equal damage dealt each round (0.9375 per hit)

- [x] Test 6: Minimum damage threshold
  - Setup: str 1 attacker vs str 9 defender
  - Expected: Damage is small but non-zero, combat eventually resolves

- [x] Test 7: Intense combat mode (24-sided dice)
  - Verify formula uses correct dice_sides value

- [x] Test 8: Multiple round combat (str 5 vs str 2, HP 2 each)
  - Verifies exact HP values after combat

- [x] Test 9: Deterministic results
  - Same inputs always produce identical outputs

### Manual Tests

- [ ] Manual test: Verify str 5 vs str 2 combat produces expected damage values
  - Attacker damage: (5/20) * (18/20) * 5 = 1.125
  - Defender damage: (2/20) * (15/20) * 5 = 0.375

- [ ] Manual test: Verify combat resolves correctly with multiple rounds
  - Round 1: Def HP = 2 - 1.125 = 0.875, Att HP = 2 - 0.375 = 1.625
  - Round 2: Def HP = 0.875 - 1.125 = -0.25 (dead), combat ends

- [ ] Manual test: Verify stack combat works (next unit enters after death)

- [ ] Manual test: Verify FOR_KICKS battles restore HP correctly

- [ ] Manual test: Test intense combat mode (24-sided dice)

- [ ] Manual test: Verify save/load preserves fractional HP values

- [ ] Test edge case: Very weak attacker (str 1) vs strong defender (str 9)

- [ ] Test edge case: Equal strength units

## Verification

- [x] Run build: `make` or equivalent
- [x] Run linter and fix any issues (if available) - No linter configured
- [x] Run tests: `make check` or equivalent (pre-existing POTFILES.in issue, no combat-related failures)
- [x] Verify build succeeds without warnings

## Documentation

- [x] Update `.claude/docs/BATTLE_MECHANICS.md` with new deterministic formula
- [x] Add code comments in `fightArmies()` explaining the new damage calculation
- [x] Document the damage multiplier constant in `defs.h`

---

## Changes Made (Iteration 1)

| Feedback | Resolution |
|----------|------------|
| "The idea is to make one hit per round... hits queue is permanent regardless the fact of army death" | Updated Decision 2 to clarify one-hit-per-round structure with permanent alternation. Updated `fightArmies()` implementation steps to do single hit per call. |
| "We should do one hit in a round. After each hit there should be a check if the unit dies or not" | Updated Decision 3 to immediate death check after each hit. Dead units don't counter-attack; next unit in stack takes over and gets the next hit in alternation. |
| "Add unit tests for different cases" | Added 7 unit tests covering: damage calculation, death mid-round, defender/attacker replacement, equal strength, minimum damage, and intense combat mode. |

**Summary**: Changed from "both sides attack per round" to "one hit per round with strict alternation". Death is now checked immediately after each hit. When a unit dies, it doesn't get to respond - the next unit in the stack takes its place and gets the next turn in the alternation sequence. Added comprehensive unit test cases.
