# Implementation Plan: Battle Display Improvements

## Summary
Fix the HP display rounding issue (showing "2/2" instead of actual fractional HP like "1.3/2"), make damage values temporary (visible for a few seconds after hit then disappear), and add a visual hit effect (image blinking) when armies take damage.

## Files to Modify
- `src/gui/fight-window.h` - Add timer/timeout tracking for damage label hiding and hit effect state
- `src/gui/fight-window.cpp` - Fix HP display logic, implement damage fade-out, add hit blink effect

## Implementation Steps

### Phase 1: Fix HP Display Rounding

- [x] In `do_round()`, change the HP display logic at lines 392-405
  - Current logic uses `std::ceil(i->hp)` which rounds 1.3 up to 2
  - Change to use `std::floor(i->hp)` for integer display (1.3 becomes 1)
  - Or better: always show one decimal place for consistency (e.g., "1.3/2")
  - Decision: Show one decimal place for all HP values for clarity

```cpp
// Replace lines 392-405 with:
std::ostringstream hp_oss;
hp_oss << std::fixed << std::setprecision(1) << i->hp;
Glib::ustring hp_text = String::ucompose("%1/%2", hp_oss.str(), i->army->getStat(Army::HP));
i->hp_label->set_text(hp_text);
```

- [x] Also update the initial HP display in `add_army()` at line 221 to use same format
  - Currently shows integer: `String::ucompose("%1/%2", (int)initial_hp, army->getStat(Army::HP))`
  - Change to show decimal for consistency

### Phase 2: Temporary Damage Display

- [x] Add to `ArmyItem` struct in `fight-window.h`:
  - `int damage_show_frames` - Counter for how many frames to show damage (decrements each frame)

- [x] In `do_round()`, when showing damage:
  - Set `damage_show_frames` to a value (e.g., 3-4 frames at normal speed)
  - On each frame, decrement counter and clear damage label when it reaches 0
  - **Remove the "DEAD" text override** - always show the actual damage value (line 427 in current code: `i->damage_label->set_text("DEAD")` should be removed)

- [x] At the start of `do_round()`, add logic to decrement `damage_show_frames` for all armies and clear labels when expired

### Phase 3: Hit Effect (Image Blink)

- [x] Add to `ArmyItem` struct in `fight-window.h`:
  - `int blink_frames` - Counter for blink effect duration
  - `Glib::RefPtr<Gdk::Pixbuf> original_pixbuf` - Store original image for restoration

- [x] In `add_army()`:
  - Store the original army pixbuf in `item.original_pixbuf`

- [x] In `do_round()`, when army takes damage (but doesn't die):
  - Set `blink_frames` to a value (e.g., 2-3 frames)
  - Darken or tint the army image red/black to show hit

- [x] At the start of `do_round()`:
  - Process blink effects: restore original image when `blink_frames` reaches 0
  - Use modulation or overlay to create "hit" visual (darken image)

## Decision Points

### Decision 1: HP Display Format
- **Proposed**: Always show one decimal place (e.g., "1.3/2", "2.0/2")
- **Alternative**: Show integer with floor() instead of ceil() (e.g., "1/2")
- **Trade-offs**: Decimal is more accurate and consistent; integer is cleaner but loses precision info. Decimal chosen for better visual feedback on damage.

### Decision 2: Damage Display Duration
- **Proposed**: Show for 3 frames (approx 1-1.5 seconds at normal speed)
- **Alternative**: Use actual timer-based timeout
- **Trade-offs**: Frame-based is simpler and integrates with existing animation loop. Timer-based would require additional complexity. Frame-based chosen for simplicity.

### Decision 3: Hit Effect Style
- **Proposed**: Darken/dim the army image temporarily (multiply RGB values by 0.5)
- **Alternative 1**: Red tint overlay
- **Alternative 2**: Add a small marker/icon over the army
- **Trade-offs**: Darkening is simple, doesn't require additional graphics, and is clearly visible. Red tint might clash with team colors. Marker adds visual clutter. Darkening chosen for simplicity and clarity.

## Testing

- [ ] Manual test: Start a battle, observe HP values update correctly with decimal precision (USER TO VERIFY)
- [ ] Manual test: Verify damage values appear on hit and disappear after ~1-2 seconds (USER TO VERIFY)
- [ ] Manual test: Verify hit effect (darkening) is visible when army takes damage (USER TO VERIFY)
- [ ] Manual test: Verify hit effect doesn't interfere with death explosion (USER TO VERIFY)
- [ ] Manual test: Test quick mode (press any key) to ensure effects still work at higher speed (USER TO VERIFY)

## Verification

- [x] Run build: `make -j4`
- [x] Verify no compiler warnings in modified files
- [x] Copy updated fighter.ui if modified: `cp dat/glade/fighter.ui local_install/usr/local/share/lordsawar/glade/` (N/A - fighter.ui not modified)

## Documentation

- [x] Add comments explaining the blink and damage timing logic
- [x] Update CLAUDE.md if new patterns introduced (likely not needed for this change)

---

## Changes Made (Iteration 1)

| Feedback | Resolution |
|----------|------------|
| "Don't display 'DEAD' in the damage field when the army is dead, it's obvious with the animation, but the damage is always important value even if army was killed with 1 shot" | Updated Phase 2 to explicitly remove the "DEAD" text override at line 427. The damage value will now always be shown, even on killing blows. |

**Summary**: Removed the "DEAD" text replacement behavior. The damage label will now consistently show the actual damage dealt for all hits, including the killing blow. The explosion animation already clearly indicates death.

---

## Changes Made (Iteration 2)

| Feedback | Resolution |
|----------|------------|
| "Decrease time of army blink and damage display - make it 3 times faster" | Changed from hardcoded values to constants: `BLINK_EFFECT_FRAMES = 1`, `DAMAGE_DISPLAY_FRAMES = 1` (was 2 and 3 respectively) |
| "Add some constant of battle speed to be able to tune it" | Added timing constants at top of fight-window.cpp: `BLINK_EFFECT_FRAMES`, `DAMAGE_DISPLAY_FRAMES`, `POST_HIT_PAUSE_FRAMES` |
| "Add pauses between hits" | Restructured do_round() to process ONE hit per cycle. Added `last_hit_army_id` and `post_hit_pause_frames` tracking. Sequence is now: hit -> effects clear -> pause -> next hit |

**Summary**: Battle animation now processes one hit at a time with a proper pause between each hit. Timing constants added for easy tuning. Blink and damage display reduced to 1 frame each (3x faster).
