// Copyright (C) 2024 LordsAWar Contributors
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Library General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
//  02110-1301, USA.

#pragma once
#ifndef COMBAT_DAMAGE_H
#define COMBAT_DAMAGE_H

#include <algorithm>

// Combat system constants
const unsigned int BATTLE_DICE_SIDES_NORMAL = 20;
const unsigned int BATTLE_DICE_SIDES_INTENSE = 24;
const double DETERMINISTIC_DAMAGE_MULTIPLIER = 5.0;
const double MINIMUM_DAMAGE_PER_HIT = 0.1;

/**
 * Calculate deterministic damage for one combat hit.
 *
 * Formula: (attacker_strength / dice_sides) * ((dice_sides - defender_strength) / dice_sides) * multiplier
 *
 * This function is used by both the Fight class and unit tests to ensure
 * consistent damage calculation.
 *
 * @param attacker_strength  The attacking army's terrain-modified strength (1-14)
 * @param defender_strength  The defending army's terrain-modified strength (1-14)
 * @param intense_combat     If true, use 24-sided dice; otherwise use 20-sided dice
 * @return The damage to deal to the defender (minimum MINIMUM_DAMAGE_PER_HIT)
 */
inline double calculateCombatDamage(int attacker_strength, int defender_strength, bool intense_combat)
{
    double dice_sides = intense_combat ? BATTLE_DICE_SIDES_INTENSE : BATTLE_DICE_SIDES_NORMAL;
    double hit_chance = (attacker_strength / dice_sides) *
                        ((dice_sides - defender_strength) / dice_sides);
    double damage = hit_chance * DETERMINISTIC_DAMAGE_MULTIPLIER;
    return std::max(damage, MINIMUM_DAMAGE_PER_HIT);
}

#endif // COMBAT_DAMAGE_H
