# Implementation Plan: Debug and Fix Battle Animation Display Issues

## Summary

The battle animation display has several synchronization issues between the fight logic (fight.cpp) and the animation playback (fight-window.cpp). The main problems are:

1. **HP type mismatch**: `ArmyItem.hp` is declared as `int` but damage values are `double`, causing precision loss
2. **Dead army animations continue**: Explosions appear at empty spaces because the animation continues processing hits on dead armies
3. **No damage numbers shown**: User cannot see how much damage each hit deals
4. **Logs appear all at once**: Debug logs are written during fight calculation, not during animation playback
5. **Missing code comments**: Battle code needs documentation for maintainability

## Files to Modify

- `src/gui/fight-window.h` - Change `ArmyItem.hp` from `int` to `double`, add damage label pointer
- `src/gui/fight-window.cpp` - Fix HP handling, add damage display, add animation-time logging, add comments
- `dat/glade/fighter.ui` - Add damage label widget to show damage per hit
- `src/fight.cpp` - Add comprehensive comments explaining the battle logic

## Implementation Steps

### Phase 1: Fix HP Type Mismatch (Root Cause of 0/2 HP Issues)

- [x] In `fight-window.h` line 57: Change `int hp;` to `double hp;` in `ArmyItem` struct
  - The `hp` field stores fractional damage values from the fight calculation
  - Currently truncating to int causes premature 0 HP display
  - This fixes the "0/2 HP but still fighting" visual bug

### Phase 2: Add Damage Display Per Hit

- [x] In `dat/glade/fighter.ui`: Add a new `GtkLabel` for damage display after hp_label
  ```xml
  <child>
    <object class="GtkLabel" id="damage_label">
      <property name="visible">True</property>
      <property name="can-focus">False</property>
    </object>
    <packing>
      <property name="expand">False</property>
      <property name="fill">False</property>
      <property name="position">2</property>
    </packing>
  </child>
  ```
  - Update water_image position to 3

- [x] In `fight-window.h`: Add `Gtk::Label *damage_label;` to `ArmyItem` struct (line ~61)

- [x] In `fight-window.cpp` `add_army()`: Get and store the damage_label widget
  ```cpp
  xml->get_widget("damage_label", damage_label);
  item.damage_label = damage_label;
  ```

- [x] In `fight-window.cpp` `do_round()`: Display damage value when processing each hit
  ```cpp
  // After: i->hp -= f.damage;
  // Show damage dealt
  Glib::ustring damage_text = String::ucompose("-%1",
      (f.damage >= 0.01) ? String::ucompose("%1", std::fixed, std::setprecision(2), f.damage) : "0");
  i->damage_label->set_text(damage_text);
  ```

### Phase 3: Fix Animation for Dead Armies (Stop Hitting Empty Spaces)

- [x] In `fight-window.cpp` `do_round()` around line 269-294: Add check to skip processing hits on already-dead armies
  ```cpp
  for (army_items_type::iterator i = army_items.begin(),
       end = army_items.end(); i != end; ++i)
    if (i->army->getId() == f.id)
      {
        // Skip if this army is already dead (hp <= 0)
        if (i->hp <= 0.0)
          break;

        i->hp -= f.damage;
        // ... rest of logic
      }
  ```
  - Note: This is a safeguard. The root cause may be that multiple FightItems target the same dead army.

- [x] Add debugging output during animation to trace what's happening:
  ```cpp
  // At start of processing each FightItem
  std::cerr << "[ANIMATION] Processing FightItem: army_id=" << f.id
            << " damage=" << f.damage << " turn=" << f.turn << std::endl;

  // When finding the army
  std::cerr << "[ANIMATION] Found army " << i->army->getName()
            << " current_hp=" << i->hp << std::endl;
  ```

### Phase 4: Clear Damage Label When Army Dies or Round Ends

- [x] In `do_round()` explosion handling (line ~280-291): Clear the damage label when army dies
  ```cpp
  if (fraction == 0.0)
    {
      // ... explosion code ...
      i->damage_label->set_text("DEAD");  // Or empty string ""
    }
  ```

- [x] In `do_round()` when clearing explosions (line ~246-259): Also clear the damage label
  ```cpp
  if (!i->exploding)
    continue;
  // ... existing explosion clear code ...
  i->damage_label->set_text("");  // Clear damage text
  ```

### Phase 5: Add Comprehensive Comments to fight-window.cpp

- [x] Add file header comment explaining the purpose of fight-window.cpp

- [x] Add comment block before `FightWindow::FightWindow()` explaining:
  - How fight data flows from Fight class to FightWindow
  - The role of `actions` list (pre-calculated fight events to replay)
  - The role of `army_items` (visual state for each army during animation)

- [x] Add comment block before `do_round()` explaining:
  - The three phases: clear explosions, process actions, show result
  - How timing works (returns CONTINUE to reschedule, STOP to end)
  - The relationship between `round` and `FightItem.turn`

- [x] Add inline comments throughout `do_round()`:
  - Phase 1: Clearing explosions from previous frame
  - Phase 2: Processing FightItems and updating visuals
  - Phase 3: End-of-battle result display

### Phase 6: Add Comprehensive Comments to fight.cpp

- [x] Add comment block before `Fight::battle()` explaining:
  - Pre-calculation: all fight events are computed before animation
  - Turn-based alternating combat (d_attacker_turn)
  - How FightItem list is built for later replay

- [x] Add comment block before `doRound()` explaining:
  - Single round execution: one side attacks
  - Death detection and removal
  - Turn swap after each round

- [x] Add comment block before `fightArmies()` explaining:
  - Single hit calculation
  - Damage application to Army object
  - FightItem creation for replay

- [x] Add comment block before `calculateDeterministicDamage()` explaining:
  - The damage formula
  - How attacker/defender strength affects damage
  - The role of dice_sides and multiplier

### Phase 7: Improve HP Display Formatting

- [x] Change HP label to show decimal precision for fractional HP:
  ```cpp
  // In do_round() line 276-278
  Glib::ustring hp_text;
  if (i->hp > 0 && i->hp < 1.0) {
    // Show one decimal for fractional HP below 1
    hp_text = String::ucompose("%1/%2",
        String::ucompose("%.1f", i->hp), i->army->getStat(Army::HP));
  } else {
    hp_text = String::ucompose("%1/%2",
        (int)std::ceil(i->hp), i->army->getStat(Army::HP));
  }
  i->hp_label->set_text(hp_text);
  ```

## Decision Points

### Decision 1: HP Display Format
- **Proposed**: Show decimal (e.g., "0.5/2") when HP is fractional and below 1
- **Alternative**: Always show integer with ceiling (current approach)
- **Trade-offs**: Decimal shows more accurate state but may be visually cluttered; ceiling can be misleading (shows 1/2 when actually 0.1/2)

### Decision 2: Damage Label Position
- **Proposed**: Add below HP label, above water image
- **Alternative**: Overlay on army image, or show as tooltip
- **Trade-offs**: Separate label is clearer but takes more vertical space

### Decision 3: Dead Army Handling
- **Proposed**: Skip processing FightItems for already-dead armies as safeguard
- **Alternative**: Trust that Fight class only generates valid FightItems
- **Trade-offs**: Safeguard prevents visual bugs even if Fight logic has issues, but may hide underlying problems

## Testing

- [ ] Manual test: Start a battle and observe HP labels update correctly
- [ ] Manual test: Verify damage numbers appear for each hit
- [ ] Manual test: Confirm explosions only appear once per army death
- [ ] Manual test: Check that no hits animate on empty spaces after death
- [ ] Manual test: Test with quick mode (! key) to verify timing still works
- [ ] Manual test: Test battles with multiple armies on each side

## Verification

- [x] Run build: `make` in the build directory
- [x] Verify no compiler warnings related to changed files
- [ ] Test the game launches without crashes (requires manual verification)

## Documentation

- [x] Code comments have been added throughout fight.cpp and fight-window.cpp
- [x] No CLAUDE.md updates needed (internal bug fix)
- [x] No skill/command changes needed
