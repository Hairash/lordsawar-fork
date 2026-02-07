// Copyright (C) 2001, 2002, 2003 Michael Bartl
// Copyright (C) 2002, 2003, 2004, 2005, 2006 Ulf Lorenz
// Copyright (C) 2004, 2006 Andrea Paternesi
// Copyright (C) 2004 Bryan Duff
// Copyright (C) 2006, 2007, 2008, 2011, 2014, 2015, 2020 Ben Asselstine
// Copyright (C) 2008 Ole Laursen
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

#include "fight.h"
#include "defs.h"
#include "combat-damage.h"
#include <assert.h>
#include <math.h>       // for has_hit()
#include <iomanip>      // for std::fixed, std::setprecision
#include "army.h"
#include "hero.h"
#include "stacklist.h"
#include "player.h"
#include "playerlist.h"
#include "Item.h"
#include "GameMap.h"
#include "citylist.h"
#include "city.h"
#include "stack.h"
#include "Backpack.h"
#include "stacktile.h"
#include "rnd.h"

#define debug(x) {std::cerr<<__FILE__<<": "<<__LINE__<<": "<<x<<std::endl<<std::flush;}
//#define debug(x)

Fighter::Fighter(const Fighter &f)
 : army(f.army), pos(f.pos), terrain_strength(f.terrain_strength)
{
}

Fighter::Fighter(Army* a, Vector<int> p)
    :army(a), pos(p), terrain_strength (1)
{
}

//take a list of stacks and create an ordered list of armies
void Fight::orderArmies(const std::list<Stack*> &stacks, std::vector<Army*> &armies)
{
  if (stacks.empty())
    return;
  for (std::list<Stack*>::const_iterator it = stacks.begin();
       it != stacks.end(); ++it)
    for (Stack::iterator sit = (*it)->begin(); sit != (*it)->end(); ++sit)
      armies.push_back((*sit));

  //okay now sort the army list according to the player's fight order
  std::sort(armies.begin(), armies.end(), Stack::armyCompareFightOrder);

  return;
}

Fight::Fight(Stack* attacker, Stack* defender, FightType type)
    : d_turn(0), d_result(DRAW), d_type(type), d_intense_combat (false), d_attacker_turn(true)
{
  std::list<Stack *> attackers;
  std::list<Stack *> defenders;

  debug("Fight between " <<attacker->getId() <<" and " <<defender->getId());

  // Duel case: two stacks fight each other; Nothing further to be done
  // Important: We always assume that the attacking/defending stacks are
  // the first in the list!!!
  attackers.push_back(attacker);
  defenders.push_back(defender);

  // What we do here: In the setup, we need to find out all armies that
  // participate in the fight.  If a city is being attacked then the
  // defender gets any other stacks in the cities.
  //

  Maptile *mtile = GameMap::getInstance()->getTile(defender->getPos());
  City *city = GameMap::getCity(defender->getPos());
  //Vector<int> p = defender->getPos();

  if (city && city->isBurnt() == false &&
      city->getOwner() == defender->getOwner())
    {
      /* we check the owner here because:
       * StackInfoDialog does a fight for kicks with a neutral scout,
       * on the tile of the selected stack, which could be in a city.
       */
      std::vector<Stack*> stacks = city->getDefenders();
      for (std::vector<Stack*>::iterator it = stacks.begin();
           it != stacks.end(); ++it)
        {
          Stack *s = *it;
          if (s == defenders.front())
            continue;
          defenders.push_back(s);
        }
    }
  else if ((!city || city->isBurnt() == true) &&
           defender->getOwner() != Playerlist::getInstance()->getNeutral())
    {
      Vector<int> pos = defender->getPos();
      std::vector<Stack*> stacks =
        GameMap::getStacks(pos)->getEnemyStacks(attacker->getOwner());
      for (std::vector<Stack*>::iterator it = stacks.begin();
           it != stacks.end(); ++it)
        {
          Stack *s = *it;
          if (s == defenders.front())
            continue;
          defenders.push_back(s);
        }
    }

  setupFight (attackers, defenders, city != NULL, mtile->getType(), type);
}

Fight::Fight(const std::list<Stack*> &attackers,
             const std::list<Stack*> &defenders,
             const std::list<FightItem> &history)
 : d_attackers (attackers), d_defenders (defenders), d_actions (history),
    d_turn (0), d_result (DRAW), d_type (FOR_KEEPS), d_intense_combat (false), d_attacker_turn(true)
{

  fillInInitialHPs();
}

void Fight::setupFight(const std::list<Stack*> &attackers, const std::list<Stack*> &defenders, bool city, Tile::Type terrain, FightType type)
{
  d_type = type;
  d_attackers = attackers;
  d_defenders = defenders;
  std::vector<Army*> def;
  orderArmies (d_defenders, def);
  for (auto a : def)
    d_def_close.push_back(new Fighter(a, defenders.front()->getPos()));
  std::vector<Army*> att;
  orderArmies (d_attackers, att);
  for (auto a : att)
    d_att_close.push_back(new Fighter(a, attackers.front()->getPos()));
  fillInInitialHPs();
  for (auto f : d_att_close)
    d_initial_att_close.push_back(new Fighter(*f));
  for (auto f : d_def_close)
    d_initial_def_close.push_back(new Fighter(*f));
  Maptile *mtile = new Maptile(-1, -1, terrain);
  if (city)
    mtile->setBuilding(Maptile::CITY);
  calculateBonus(mtile);
  delete mtile;
}

Fight::Fight(const std::list<Stack*> &attackers, const std::list<Stack*> &defenders, bool city, Tile::Type terrain, FightType type)
  : d_attackers (attackers), d_defenders (defenders), d_turn (0),
    d_result (DRAW), d_type (FOR_KEEPS), d_intense_combat (false), d_attacker_turn(true)
{
  setupFight (attackers, defenders, city, terrain, type);
}

Fight::~Fight()
{
  d_attackers.clear();
  d_defenders.clear();

  // clear all fighter items in all lists
  while (!d_att_close.empty())
    {
      delete (*d_att_close.begin());
      d_att_close.erase(d_att_close.begin());
    }

  while (!d_def_close.empty())
    {
      delete (*d_def_close.begin());
      d_def_close.erase(d_def_close.begin());
    }

  while (!d_initial_att_close.empty())
    {
      delete (*d_initial_att_close.begin());
      d_initial_att_close.erase(d_initial_att_close.begin());
    }

  while (!d_initial_def_close.empty())
    {
      delete (*d_initial_def_close.begin());
      d_initial_def_close.erase(d_initial_def_close.begin());
    }
}

// =============================================================================
// Fight::battle() - Main Battle Calculation
// =============================================================================
// This is the core battle calculation method. IMPORTANT: All combat events are
// calculated here BEFORE any animation starts. The FightWindow class later
// replays these events for visual display.
//
// Key concepts:
// 1. PRE-CALCULATION: Every hit, damage amount, and death is computed upfront
// 2. TURN-BASED: Combat alternates between attacker and defender (d_attacker_turn)
// 3. EVENT RECORDING: Each hit creates a FightItem stored in d_actions list
//
// The d_actions list (accessible via getCourseOfEvents()) contains:
//   - FightItem.turn: Which round the hit occurred
//   - FightItem.id: Army ID that was damaged
//   - FightItem.damage: Damage amount dealt
//
// After battle():
//   - d_result is set to ATTACKER_WON, DEFENDER_WON, or DRAW
//   - d_actions contains complete fight history for animation replay
//   - Army HP values are updated (damage applied to actual Army objects)
//
// For FOR_KICKS fights (military advisor simulation), HP is restored after.
// =============================================================================
void Fight::battle(bool intense)
{
  d_intense_combat = intense;

  // Execute rounds until one side is eliminated
  // Each doRound() call processes one hit (attacker or defender attacks)
  for (d_turn = 0; doRound(); d_turn++);

  // Now we have to set the fight result.

  // First, look if the attacker died; the attacking stack is the first
  // one in the list
  bool survivor = false;
  Stack* s = d_attackers.front();
  for (Stack::const_iterator it = s->begin(); it != s->end(); ++it)
    if ((*it)->getHP() > 0)
      {
	survivor = true;
	break;
      }

  if (!survivor)
      d_result = DEFENDER_WON;
  else
    {
      // Now look if the defender died; also the first in the list
      survivor = false;
      s = d_defenders.front();
      for (Stack::const_iterator it = s->begin(); it != s->end(); ++it)
	if ((*it)->getHP() > 0)
	  {
	    survivor = true;
	    break;
	  }

      if (!survivor)
	d_result = ATTACKER_WON;
    }

  if (d_type == FOR_KICKS)
    {
      //revert the hitpoints to what they started out as.
      //if they were already hurt prior to the battle, they go back to
      //being already hurt.
      for (std::list<Stack*>::iterator it = d_attackers.begin();
            it != d_attackers.end(); ++it)
	for (Stack::iterator sit = (*it)->begin(); sit != (*it)->end(); ++sit)
          (*sit)->setHP(initial_hps[(*sit)->getId()]);

      for (std::list<Stack*>::iterator it = d_defenders.begin();
           it != d_defenders.end(); ++it)
	for (Stack::iterator sit = (*it)->begin(); sit != (*it)->end(); ++sit)
          (*sit)->setHP(initial_hps[(*sit)->getId()]);
    }
}

Army *Fight::findArmyById(const std::list<Stack *> &l, guint32 id)
{
  for (std::list<Stack *>::const_iterator i = l.begin(), end = l.end();
       i != end; ++i) {
    Army *a = (*i)->getArmyById(id);
    if (a)
      return a;
  }

  return 0;
}

Fight::Result Fight::battleFromHistory()
{
  for (std::list<FightItem>::iterator i = d_actions.begin(),
         end = d_actions.end(); i != end; ++i) {
    FightItem &f = *i;

    Army *a = findArmyById(d_attackers, f.id);
    if (!a)
      a = findArmyById(d_defenders, f.id);

    a->damage(f.damage);
  }
  //is there anybody alive in the attackers?
  for (std::list<Stack*>::iterator it = d_attackers.begin(); it != d_attackers.end(); ++it)
    {
      for (Stack::iterator i = (*it)->begin(); i != (*it)->end(); ++i)
	{
	  if ((*i)->getHP() > 0)
	    return Fight::ATTACKER_WON;
	}
    }

  return Fight::DEFENDER_WON;
}

// =============================================================================
// Fight::doRound() - Execute One Combat Round
// =============================================================================
// Executes a single round of combat. In alternating combat mode:
// - If d_attacker_turn is true: attacker's front army hits defender's front army
// - If d_attacker_turn is false: defender's front army hits attacker's front army
//
// Each round:
// 1. Get front armies from both sides (first in their respective lists)
// 2. The active side calls fightArmies() to deal damage
// 3. Check if the target army died (HP <= 0), remove if so
// 4. Swap d_attacker_turn for next round
//
// The fight continues until one side has no armies left.
//
// Returns:
//   true: Continue fighting (both sides have armies)
//   false: Fight ended (one side eliminated or max rounds reached)
// =============================================================================
bool Fight::doRound()
{
  // Check max rounds limit (0 means unlimited)
  if (MAX_ROUNDS && d_turn >= MAX_ROUNDS)
    return false;

  debug ("Fight round #" <<d_turn);
  debug("[COMBAT] === Turn " << d_turn << " ("
        << (d_attacker_turn ? "Attacker's" : "Defender's") << " turn) ===");

  // Get the front armies from each side (first in fight order)
  std::list<Fighter*>::iterator attacker_it = d_att_close.begin();
  std::list<Fighter*>::iterator defender_it = d_def_close.begin();

  // If either side is empty, fight is over
  if (attacker_it == d_att_close.end() || defender_it == d_def_close.end())
    return false;

  Fighter* attacker = *attacker_it;
  Fighter* defender = *defender_it;

  // Alternating hit structure: one side attacks per round
  if (d_attacker_turn)
    {
      // Attacker's turn: attacker hits defender
      fightArmies(attacker, defender);

      // Check if defender died from this hit
      if (defender->army->getHP() <= 0.0)
        {
          debug("[COMBAT] " << defender->army->getName() << " (" << defender->army->getId() << ") DIED!");
          remove(defender);  // Remove from d_def_close list
        }
    }
  else
    {
      // Defender's turn: defender hits attacker
      fightArmies(defender, attacker);

      // Check if attacker died
      if (attacker->army->getHP() <= 0.0)
        {
          debug("[COMBAT] " << attacker->army->getName() << " (" << attacker->army->getId() << ") DIED!");
          remove(attacker);
        }
    }

  // Swap turn for next round
  d_attacker_turn = !d_attacker_turn;

  // Continue if both sides still have armies
  if (d_def_close.empty() || d_att_close.empty())
    return false;

  return true;
}

void Fight::calculateBaseStrength(std::list<Fighter*> fighters)
{
  std::list<Fighter*>::iterator fit;
  for (fit = fighters.begin(); fit != fighters.end(); ++fit)
    {
      if ((*fit)->army->getStat(Army::SHIP))
	(*fit)->terrain_strength = (*fit)->army->getStat(Army::BOAT_STRENGTH);
      else
	(*fit)->terrain_strength = (*fit)->army->getStat(Army::STRENGTH);
    }
}

void Fight::calculateTerrainModifiers(std::list<Fighter*> fighters, Maptile *mtile, bool defender)
{
  guint32 army_bonus;
  std::list<Fighter*>::iterator fit;
  for (fit = fighters.begin(); fit != fighters.end(); ++fit)
    {
      if ((*fit)->army->getStat(Army::SHIP))
	continue;

      bool tower = false;
      if (defender)
        tower = (*fit)->army->getFortified();
      mtile = GameMap::getInstance()->getTile((*fit)->pos);
      army_bonus = (*fit)->army->getStat(Army::ARMY_BONUS);

      if (army_bonus & Army::ADD1STRINOPEN && mtile->isOpenTerrain() == Tile::GRASS && !tower)
	(*fit)->terrain_strength += 1;

      if (army_bonus & Army::ADD1STRINFOREST &&
	  mtile->getType() == Tile::FOREST && !mtile->isCityTerrain() && !tower)
	(*fit)->terrain_strength += 1;

      if (army_bonus & Army::ADD2STRINFOREST &&
	  mtile->getType() == Tile::FOREST && !mtile->isCityTerrain() && !tower)
	(*fit)->terrain_strength += 2;

      if (army_bonus & Army::ADD1STRINHILLS && mtile->isHillyTerrain() &&
          !tower)
	(*fit)->terrain_strength += 1;

      if (army_bonus & Army::ADD2STRINHILLS && mtile->isHillyTerrain() &&
          !tower)
	(*fit)->terrain_strength += 2;

      if (army_bonus & Army::ADD1STRINCITY && (mtile->isCityTerrain() || tower))
	(*fit)->terrain_strength += 1;

      if (army_bonus & Army::ADD2STRINCITY && (mtile->isCityTerrain() || tower))
	(*fit)->terrain_strength += 2;

      if (army_bonus & Army::ADD2STRINOPEN && mtile->isOpenTerrain() && !tower)
	(*fit)->terrain_strength += 2;

      if ((*fit)->terrain_strength > 9) //terrain strength can't ever exceed 9
	(*fit)->terrain_strength = 9;

    }
}

void Fight::calculateModifiedStrengths (std::list<Fighter*>friendly,
					std::list<Fighter*>enemy,
					bool friendlyIsDefending,
					Hero *strongestHero,
                                        Maptile *mtile)
{
  guint32 army_bonus;

  //find highest non-hero bonus
  guint32 highest_non_hero_bonus = 0;
  for (std::list<Fighter*>::iterator fit = friendly.begin();
       fit != friendly.end(); ++fit)
    {
      guint32 non_hero_bonus = 0;
      if ((*fit)->army->isHero())
	continue;
      if ((*fit)->army->getStat(Army::SHIP))
        continue;
      army_bonus = (*fit)->army->getStat(Army::ARMY_BONUS);

      if (army_bonus & Army::ADD1STACKINHILLS && mtile->isHillyTerrain())
	non_hero_bonus += 1;

      if (army_bonus & Army::ADD1STACK)
	non_hero_bonus += 1;

      if (army_bonus & Army::ADD2STACK)
	non_hero_bonus += 2;

      if (non_hero_bonus > highest_non_hero_bonus)
	highest_non_hero_bonus = non_hero_bonus;
    }

  // does the defender cancel our non hero bonus?
  for (std::list<Fighter*>::iterator fit = enemy.begin();
       fit != enemy.end(); ++fit)
    {
      army_bonus = (*fit)->army->getStat(Army::ARMY_BONUS);
      if (army_bonus & Army::SUBALLNONHEROBONUS)
	{
	  highest_non_hero_bonus = 0; //yes
	  break;
	}
    }

  //find hero bonus of strongest hero
  guint32 hero_bonus = 0;
  if (strongestHero)
    {
      // first get command items from ALL heroes in the stack
      for (std::list<Fighter*>::iterator fit = friendly.begin();
           fit != friendly.end(); ++fit)
	{
	  if ((*fit)->army->isHero())
	    {
	      Hero *h = dynamic_cast<Hero*>((*fit)->army);
	      hero_bonus = h->getBackpack()->countStackStrengthBonuses();
	    }
	}
    }

  //now add on the hero's natural command
  if (strongestHero)
    hero_bonus += strongestHero->calculateNaturalCommand();

  // does the defender cancel our hero bonus?
  for (std::list<Fighter*>::iterator fit = enemy.begin();
       fit != enemy.end(); ++fit)
    {
      army_bonus = (*fit)->army->getStat(Army::ARMY_BONUS);
      if (army_bonus & Army::SUBALLHEROBONUS)
	{
	  hero_bonus = 0; //yep
	  break;
	}
    }

  guint32 fortify_bonus = 0;
  guint32 city_bonus = 0;
  City *c = NULL;
  if (friendlyIsDefending)
    {
      bool city_is_burnt = false;
      guint32 city_defense_level = 0;
      // calculate the city bonus
      std::list<Fighter*>::iterator ffit = friendly.begin();
      if (mtile->getPos () != Vector<int>(-1,-1))
        {
          mtile = GameMap::getInstance()->getTile((*ffit)->pos);
          c = Citylist::getInstance()->getNearestCity((*ffit)->pos);
          city_is_burnt = c->isBurnt();
          city_defense_level = c->getDefenseLevel();
        }
      else
        {
          city_defense_level = 1;
        }

      if (mtile->getBuilding() == Maptile::CITY)
        {
          if (city_is_burnt)
            city_bonus = 0;
          else
            city_bonus = city_defense_level - 1;
        }
      else
        {
          if (mtile->getBuilding() == Maptile::TEMPLE)
            city_bonus = 2;
          else if (mtile->getBuilding() == Maptile::RUIN)
            city_bonus = 2;
          else if (mtile->isCityTerrain() == false)
            {
              for (std::list<Fighter*>::iterator fit = friendly.begin(); fit != friendly.end(); ++fit)
                {
                  army_bonus = (*fit)->army->getStat(Army::ARMY_BONUS);
                  if (army_bonus & Army::FORTIFY)
                    {
                      fortify_bonus = 1;
                      break;
                    }
                }
            }
        }

      // does the attacker cancel our city bonus?
      for (std::list<Fighter*>::iterator fit = enemy.begin();
           fit != enemy.end(); ++fit)
        {
          if ((*fit)->army->getStat(Army::SHIP))
            continue;
          army_bonus = (*fit)->army->getStat(Army::ARMY_BONUS);
          if (army_bonus & Army::SUBALLCITYBONUS)
            {
              city_bonus = 0; //yep
              fortify_bonus = 0;
              break;
            }
        }
    }

  guint32 total_bonus = highest_non_hero_bonus + hero_bonus + fortify_bonus +
    city_bonus;

  if (total_bonus > 5) //total bonus can't exceed 5
    total_bonus = 5;

  //add it to the terrain strength of each unit
  for (std::list<Fighter*>::iterator fit = friendly.begin();
       fit != friendly.end(); ++fit)
    {
      if ((*fit)->army->getStat(Army::SHIP))
        continue;
      (*fit)->terrain_strength += total_bonus;
    }
}

void Fight::calculateFinalStrengths (std::list<Fighter*> friendly, std::list<Fighter*> enemy)
{
  guint32 army_bonus;
  for (std::list<Fighter*>::iterator efit = enemy.begin();
       efit != enemy.end(); ++efit)
    {
      army_bonus = (*efit)->army->getStat(Army::ARMY_BONUS);
      if (army_bonus & Army::SUB1ENEMYSTACK ||
          army_bonus & Army::SUB2ENEMYSTACK)
	{
          int dec = 0;
          if (army_bonus & Army::SUB1ENEMYSTACK)
            dec += 1;
          if (army_bonus & Army::SUB2ENEMYSTACK)
            dec += 2;
	  for (std::list<Fighter*>::iterator ffit = friendly.begin();
               ffit != friendly.end(); ++ffit)
            {
              if ((*ffit)->army->getStat(Army::SHIP))
                continue;
              (*ffit)->terrain_strength -= dec;
              if ((*ffit)->terrain_strength <= 0)
                (*ffit)->terrain_strength = 1;
            }
	  break;
	}
    }
}

void Fight::calculateBonus(Maptile *mtile)
{
  // If there is a hero, add a +1 strength bonus

  // go get the base strengths of all attackers
  // this includes items with battle bonuses for the hero
  // naval units always have strength = 4
  calculateBaseStrength (d_att_close);
  calculateBaseStrength (d_def_close);

  // now determine the terrain strength by adding the terrain modifiers
  // to the base strength
  // naval units always have a strength of 4
  calculateTerrainModifiers (d_att_close, mtile, false);
  calculateTerrainModifiers (d_def_close, mtile, true);

  //calculate hero, non-hero, city, and fortify bonuses
  std::list<Stack*>::const_iterator iit = d_attackers.begin();
  Army *a = (*iit)->getStrongestHero();
  Hero *h = dynamic_cast<Hero*>(a);
  calculateModifiedStrengths (d_att_close, d_def_close, false, h, mtile);
  Hero *strongestHero = 0;
  guint32 highest_strength = 0;
  for (std::list<Stack*>::const_iterator it = d_defenders.begin(); it != d_defenders.end(); ++it)
    {
      a = (*it)->getStrongestHero();
      if (!a)
	continue;
      h = dynamic_cast<Hero*>(a);
      if (h->getStat(Army::STRENGTH) > highest_strength)
	{
	  highest_strength = h->getStat(Army::STRENGTH);
	  strongestHero = h;
	}
    }
  calculateModifiedStrengths (d_def_close, d_att_close, true, strongestHero,
                              mtile);

  calculateFinalStrengths (d_att_close, d_def_close);
  calculateFinalStrengths (d_def_close, d_att_close);

}

// =============================================================================
// Fight::calculateDeterministicDamage() - Calculate Damage for One Hit
// =============================================================================
// Calculates the damage dealt when attacker hits defender. This is a
// deterministic formula (no randomness) defined in combat-damage.h:
//
// Formula: (attacker_str / dice_sides) * ((dice_sides - defender_str) / dice_sides) * multiplier
//
// Where:
//   - attacker_str: Attacker's terrain_strength (base + terrain + bonuses)
//   - defender_str: Defender's terrain_strength
//   - dice_sides: 20 for normal combat, 24 for intense combat
//   - multiplier: Scaling factor (defined in combat-damage.h)
//
// The formula models:
//   - Higher attacker strength = more damage
//   - Higher defender strength = less damage
//   - Intense combat (24 sides) = generally lower damage per hit
//
// Typical damage values range from ~0.1 to ~0.5 HP per hit, depending on
// relative strengths. Since armies start with 2 HP, it takes several hits
// to kill even a weak unit.
// =============================================================================
double Fight::calculateDeterministicDamage(Fighter* attacker, Fighter* defender)
{
  // Use the shared damage calculation function from combat-damage.h
  return calculateCombatDamage(attacker->terrain_strength, defender->terrain_strength, d_intense_combat);
}

// =============================================================================
// Fight::fightArmies() - Execute Single Hit Between Two Armies
// =============================================================================
// This method executes one hit from attacker to defender:
//
// 1. Calculate damage using deterministic formula (no randomness)
// 2. Update medal/XP tracking statistics (for FOR_KEEPS fights)
// 3. Apply damage to the defender Army object (modifies Army::d_hp)
// 4. Create a FightItem record and add to d_actions for animation replay
//
// The FightItem created here is what FightWindow uses to animate the hit:
//   - item.turn: Current round number (for pacing animation)
//   - item.id: Defender's army ID (to find the right ArmyItem to animate)
//   - item.damage: Damage dealt (for HP label and damage display)
//
// Note: This method does NOT check for death - the caller (doRound) handles
// that by checking defender HP after this call returns.
// =============================================================================
void Fight::fightArmies(Fighter* attacker, Fighter* defender)
{
  if (!attacker || !defender)
    return;

  Army *a = attacker->army;
  Army *d = defender->army;

  debug("Army " << a->getId() << " attacks " << d->getId());

  // Calculate deterministic damage for this single hit
  double damage = calculateDeterministicDamage(attacker, defender);

  // Factor used for medal calculations (XP scaling)
  double xp_factor = a->getXpReward() / d->getXpReward();

  // Update medal tracking statistics (only for real fights, not simulations)
  if (d_type == FOR_KEEPS)
    {
      a->setNumberHasHit(a->getNumberHasHit() + (damage / xp_factor));
      d->setNumberHasBeenHit(d->getNumberHasBeenHit() + (damage / xp_factor));
    }

  // Apply damage to defender's HP (modifies the actual Army object)
  d->damage(damage);

  // Log combat details for debugging
  debug("[COMBAT] Round " << d_turn << ": "
        << a->getName() << " (" << a->getId() << ") hits "
        << d->getName() << " (" << d->getId() << ") for "
        << std::fixed << std::setprecision(2) << damage << " damage. "
        << "Defender HP: " << d->getHP() << "/" << d->getStat(Army::HP));

  // Record this hit for animation replay by FightWindow
  // The FightItem struct captures: when (turn), who (id), how much (damage)
  FightItem item;
  item.turn = d_turn;      // Round number - used to pace animation
  item.id = d->getId();    // Defender's ID - used to find ArmyItem to animate
  item.damage = damage;    // Damage dealt - used for HP label update
  d_actions.push_back(item);
}

void Fight::remove(Fighter* f)
{

  // is the fighter in the attacker lists?
  for (std::list<Fighter*>::iterator it = d_att_close.begin();
       it != d_att_close.end(); ++it)
    if ((*it) == f)
      {
	d_att_close.erase(it);
	delete f;
	return;
      }

  // or in the defender lists?
  for (std::list<Fighter*>::iterator it = d_def_close.begin();
       it != d_def_close.end(); ++it)
    if ((*it) == f)
      {
	d_def_close.erase(it);
	delete f;
	return;
      }

  // if the fighter wa sin no list, we are rather careful and don't do anything
  debug("Fight: fighter without list!")
}

guint32 Fight::getModifiedStrengthBonus(Army *a)
{
  for (std::list<Fighter*>::iterator it = d_att_close.begin();
       it != d_att_close.end(); ++it)
    if ((*it)->army == a)
      return (*it)->terrain_strength;
  for (std::list<Fighter*>::iterator it = d_def_close.begin();
       it != d_def_close.end(); ++it)
    if ((*it)->army == a)
      return (*it)->terrain_strength;
  return 0;
}

void Fight::fillInInitialHPs()
{
  for (std::list<Stack *>::iterator i = d_attackers.begin();
       i != d_attackers.end(); ++i)
    for (Stack::iterator j = (*i)->begin(); j != (*i)->end(); ++j)
      initial_hps[(*j)->getId()] = (*j)->getHP();

  for (std::list<Stack *>::iterator i = d_defenders.begin();
       i != d_defenders.end(); ++i)
    for (Stack::iterator j = (*i)->begin(); j != (*i)->end(); ++j)
      initial_hps[(*j)->getId()] = (*j)->getHP();
}

LocationBox Fight::calculateFightBox(Fight &fight)
{
  /*
   this is all about figuring out where the explosion
   is supposed to appear on the big map.
   the desired behaviour is:
   when we attack a city the explosion covers where we are
   attacking from, to where we are attacking to.
   it's tricky though because we step into the city, so we
   have to look at our track to see where we were.
   when attacking in the field, the explosion covers the
   enemy stack.
   maybe defenders can be empty?
   */
  Vector<int> dest = fight.getAttackers().front()->getPos();
  if (Citylist::getInstance()->getObjectAt(dest) == NULL)
    {
      if (!fight.getDefenders().empty())
        return LocationBox(fight.getDefenders().front()->getPos());
      else
        return LocationBox(dest);
    }
  Player *p = fight.getAttackers().front()->getOwner();
  Stack *s = fight.getAttackers().front();
  std::list<Vector<int> > tracks = p->getStackTrack(s);
  if (tracks.size() >= 2)
    {
      std::list<Vector<int> >::iterator it = tracks.end();
      --it; --it;
      return LocationBox (*it, dest);
    }
  else
    {
      //this shouldn't be the case
      return LocationBox(s->getPos(), dest);
    }
}

Glib::ustring Fight::getStrongestLivingHeroName(std::vector<Army *> s) const
{
  Glib::ustring name = "";
  guint32 highest_strength = 0;
  for (auto a : s)
    {
      if (a->isHero() && a->getHP () > 0)
        {
          if (a->getStat(Army::STRENGTH) > highest_strength)

            {
              highest_strength = a->getStat(Army::STRENGTH);
              name = a->getName ();
            }
        }
    }
  return name;
}
