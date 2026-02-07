# Implementation Plan: Add Army HP to Info Label

## Summary
Add the army's current HP to the context help tooltip that appears when right-clicking on an army. The tooltip currently shows Name, Strength, Moves, and Upkeep. We will add HP information in a similar format, displaying both current and maximum HP values.

## Files to Modify
- `src/gui/army-info-tip.cpp` - Add HP line to the info string generation

## Implementation Steps

### Phase 1: Add HP to Army Info Tooltip
- [ ] Modify `ArmyInfoTip::ArmyInfoTip(Gtk::Widget *target, const Army *army)` constructor in `src/gui/army-info-tip.cpp`
  - Add HP line after Strength line (or after Moves, depending on logical grouping)
  - Use format: `HP: current/max` similar to Moves display
  - Code to add:
    ```cpp
    s += "\n";
    s += String::ucompose(_("HP: %1/%2"),
                          army->getHP(), army->getMaxHP());
    ```
  - Note: `getHP()` returns `double`, may need formatting consideration

### Phase 2: Build and Test
- [ ] Build the project with `make -j4`
- [ ] Copy any modified glade files if needed (not needed for this change)

## Decision Points

### Decision 1: HP Display Format
- **Proposed**: Display as "HP: current/max" (e.g., "HP: 2/2" or "HP: 1.5/2")
- **Alternative**: Display only current HP, or as percentage
- **Trade-offs**: The current/max format is consistent with how Moves are displayed and gives full information to the player

### Decision 2: Placement in Info String
- **Proposed**: Place HP after Strength (since both are combat-related stats)
- **Alternative**: Place after Moves or at the end
- **Trade-offs**: Grouping combat stats (Strength, HP) together provides logical information flow

### Decision 3: Decimal Display for HP
- **Proposed**: Display HP as-is (may show decimals like 1.5)
- **Alternative**: Round to integer for cleaner display
- **Trade-offs**: Showing actual value is more accurate; the game may use fractional HP internally

## Testing

- [ ] Manual test: Start the game, right-click on an army in a stack, verify HP appears in tooltip
- [ ] Manual test: Engage in combat, after combat check that damaged armies show reduced HP
- [ ] Manual test: Verify HP display updates correctly after healing/rest

## Verification

- [ ] Run build: `make -j4`
- [ ] Verify build succeeds with no new warnings

## Documentation

- [ ] No CLAUDE.md updates needed (no new patterns introduced)
- [ ] No skill/command updates needed
- [ ] Code follows existing pattern - no additional comments needed
