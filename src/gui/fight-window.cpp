//  Copyright (C) 2007, 2008 Ole Laursen
//  Copyright (C) 2007, 2008, 2009, 2010, 2012, 2014, 2015, 2020 Ben Asselstine
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

// =============================================================================
// FightWindow - Battle Animation Display
// =============================================================================
//
// This file handles the graphical display and animation of battles. The key
// architectural concept is the separation between fight CALCULATION (in
// fight.cpp) and fight ANIMATION (in this file):
//
// 1. Fight::battle() pre-calculates ALL combat events before animation starts
// 2. The events are stored in a list of FightItem structs (turn, army_id, damage)
// 3. FightWindow receives this list and replays the events with visual effects
//
// Data Flow:
//   Fight::battle() -> d_actions list -> FightWindow::actions -> do_round()
//
// Key Components:
//   - ArmyItem: Visual state for each army (local hp tracking, image refs)
//   - actions: List of FightItem events to replay
//   - do_round(): Timer callback that processes events frame-by-frame
//
// The animation uses a timer-based approach where do_round() is called
// repeatedly. It returns Timing::CONTINUE to reschedule or Timing::STOP to end.
// =============================================================================

#include <config.h>

#include <numeric>
#include <vector>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <gtkmm.h>

#include "fight-window.h"
#include "builder-cache.h"

#include "timing.h"
#include "File.h"
#include "player.h"
#include "army.h"
#include "ImageCache.h"
#include "Configuration.h"
#include "snd.h"
#include "GameMap.h"
#include "tileset.h"
#include "tilesetlist.h"
#include "Tile.h"
#include "SmallTile.h"
#include "font-size.h"

#define method(x) sigc::mem_fun(*this, &FightWindow::x)

bool FightWindow::s_quick_all = false;

// =============================================================================
// Battle Animation Timing Constants
// =============================================================================
// These control the pacing of the battle animation. All values are in frames.
// One frame duration = normal_round_speed (typically 300-500ms).
// Adjust these to tune battle feel.
static const int BLINK_EFFECT_FRAMES = 1;     // Frames to show darkened hit effect
static const int DAMAGE_DISPLAY_FRAMES = 1;   // Frames to show damage number
static const int POST_HIT_PAUSE_FRAMES = 1;   // Frames to pause after effects clear

// -----------------------------------------------------------------------------
// FightWindow Constructor
// -----------------------------------------------------------------------------
// Initializes the fight animation window by:
// 1. Loading the UI from fight-window.ui
// 2. Extracting attacker and defender armies from the Fight object
// 3. Creating ArmyItem visual representations for each army
// 4. Getting the pre-calculated fight events (actions) from Fight::getCourseOfEvents()
// 5. Determining the result message to display at the end
//
// The Fight object has already calculated all combat events before this
// constructor is called. We just store them in 'actions' for playback.
//
// Key members initialized:
//   - army_items: Visual state for each army (local hp copy, image widgets)
//   - actions: List of FightItem events to replay during animation
//   - d_decision: Result message shown at end of battle
// -----------------------------------------------------------------------------
FightWindow::FightWindow(Gtk::Window &parent, Fight &fight)
{
  Glib::RefPtr<Gtk::Builder> xml = BuilderCache::get("fight-window.ui");

  xml->get_widget("window", window);
  window->set_transient_for(parent);
  guint32 height = GameMap::getTileset()->getTileSize () * 3;
  guint32 width = height * (16.0/9.0);
  window->set_size_request (width, height);

  window->signal_key_release_event().connect_notify
    (method(on_key_release_event));

  xml->get_widget("decision_label", decision_label);

  Gtk::Box *attacker_close_vbox;
  Gtk::Box *defender_close_vbox;
  xml->get_widget("attacker_close_vbox", attacker_close_vbox);
  xml->get_widget("defender_close_vbox", defender_close_vbox);

  // extract attackers and defenders
  armies_type attackers, defenders;

  Fight::orderArmies (fight.getAttackers(), attackers);
  Fight::orderArmies (fight.getDefenders(), defenders);

  // add the armies
  std::vector<Gtk::Box *> close_hboxes;
  int close;
  std::map<guint32, double> initial_hps = fight.getInitialHPs();

  // ... attackers
  close = 0;
  for (armies_type::iterator i = attackers.begin(); i != attackers.end(); ++i)
    add_army(*i, initial_hps[(*i)->getId()], close_hboxes, attacker_close_vbox, 
             close++);

  close_hboxes.clear();

  // ... defenders
  close = 0;
  for (armies_type::iterator i = defenders.begin(); i != defenders.end(); ++i)
    add_army(*i, initial_hps[(*i)->getId()], close_hboxes, defender_close_vbox, 
             close++);

  // fill in shield pictures
  ImageCache *gc = ImageCache::getInstance();

  Gtk::Image *defender_shield_image;
  Player *p = defenders.front()->getOwner();
  xml->get_widget("defender_shield_image", defender_shield_image);
  defender_shield_image->property_pixbuf()=
    gc->getShieldPic(2, p, false,
                     FontSize::getInstance ()->get_height ())->to_pixbuf();

  Gtk::Image *attacker_shield_image;
  p = attackers.front()->getOwner();
  xml->get_widget("attacker_shield_image", attacker_shield_image);
  attacker_shield_image->property_pixbuf()=
    gc->getShieldPic(2, p, false,
                     FontSize::getInstance ()->get_height ())->to_pixbuf();

  actions = fight.getCourseOfEvents();
  d_quick = false;

  if (fight.getResult () == Fight::ATTACKER_WON)
    {
      Glib::ustring hero = fight.getStrongestLivingHeroName (attackers);
      if (hero.empty () == true)
        d_decision = _("Your armies have won the city!");
      else
        d_decision = String::ucompose (_("%1 has won the battle!"), hero);
    }
  else
    d_decision = _("You have lost!");

  fast_round_speed = Configuration::s_displayFightRoundDelayFast; //ms
  normal_round_speed = Configuration::s_displayFightRoundDelaySlow; //ms
  Snd::getInstance()->disableBackground();
  Snd::getInstance()->play("battle", -1, true);

}

FightWindow::~FightWindow()
{
  Snd::getInstance()->halt(true);
  Snd::getInstance()->enableBackground();
  delete window;
}

void FightWindow::run(bool *quick)
{
  round = 0;
  action_iterator = actions.begin();
  post_hit_pause_frames = 0;
  last_hit_army_id = 0;

  if (s_quick_all)
    Timing::instance().register_timer (method(do_round), fast_round_speed / 3);
  else
    Timing::instance().register_timer(method(do_round), 
                                      *quick == true ?
                                      fast_round_speed : normal_round_speed);

  window->show_all();
  main_loop = Glib::MainLoop::create();
  main_loop->run();
  if (quick && *quick == false)
    *quick = d_quick;
  if (s_quick_all)
    d_quick = true;
}

void FightWindow::add_army(Army *army, double initial_hp,
                           std::vector<Gtk::Box *> &hboxes, Gtk::Box *vbox,
                           int current_no)
{
  Gtk::Box *army_box;
  Gtk::Image *army_image;
  Gtk::Image *water_image;
  Gtk::Label *hp_label;
  Gtk::Label *damage_label;

  Glib::RefPtr<Gtk::Builder> xml = BuilderCache::get("fighter.ui");

  xml->get_widget("army_box", army_box);
  xml->get_widget("army_image", army_image);
  xml->get_widget("water_image", water_image);
  xml->get_widget("hp_label", hp_label);
  xml->get_widget("damage_label", damage_label);

  // Set initial HP text showing current/max HP (one decimal place for consistency)
  std::ostringstream init_hp_oss;
  init_hp_oss << std::fixed << std::setprecision(1) << initial_hp;
  hp_label->set_text(String::ucompose("%1/%2", init_hp_oss.str(), army->getStat(Army::HP)));

  // image
  guint32 fs = FontSize::getInstance()->get_height ();
  PixMask *armypic = ImageCache::getInstance()->getDialogArmyPic(army, fs);
  armypic_width = armypic->get_width ();
  armypic_height = armypic->get_height ();
  army_image->property_pixbuf() = armypic->to_pixbuf();
  int height = 3;
  SmallTile *water =
    Tilesetlist::getInstance()->getSmallTile(GameMap::getTileset()->getBaseName(), Tile::WATER);
  if (army->getStat(Army::SHIP, false) && water)
    {
      Cairo::RefPtr<Cairo::Surface> surf = Cairo::ImageSurface::create (Cairo::FORMAT_ARGB32, armypic->get_width(), height);
      Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(surf);
      Gdk::RGBA w = water->getColor();
      cr->set_source_rgb(w.get_red(), w.get_green(), w.get_blue());
      cr->rectangle(0, 0, armypic->get_width(), height);
      cr->paint();
      Glib::RefPtr<Gdk::Pixbuf> p =
        Gdk::Pixbuf::create(surf, 0, 0, armypic->get_width(), height);
      water_image->property_pixbuf() = p;
    }
  else
    {
      Glib::RefPtr<Gdk::Pixbuf> empty =
        Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, height, height);
      empty->fill(0x00000000);
      water_image->property_pixbuf() = empty;
    }

  // then add it to the right hbox
  int current_row = (current_no / max_cols);
  if (current_row >= int(hboxes.size()))
    {
      // add an hbox
      Gtk::Box *hbox = manage(new Gtk::Box (Gtk::ORIENTATION_HORIZONTAL));
      hbox->set_spacing(6);
      hboxes.push_back(hbox);

      Gtk::Box *a = manage(new Gtk::Box());
      a->add(*hbox);
      vbox->pack_start(*a, Gtk::PACK_SHRINK);
    }

  Gtk::Box *hbox = hboxes[current_row];
  army_box->get_parent()->remove(*army_box);
  Gtk::Box *box = new Gtk::Box(Gtk::ORIENTATION_VERTICAL );
  box->pack_start(*army_box, Gtk::PACK_SHRINK);
  army_box->reparent(*box);
  hbox->pack_start(*Gtk::manage(box), Gtk::PACK_SHRINK);

  // finally add an entry for later use
  ArmyItem item;
  item.army = army;
  item.hp = initial_hp;
  item.water_image = water_image;
  item.image = army_image;
  item.hp_label = hp_label;
  item.damage_label = damage_label;
  item.exploding = false;
  item.damage_show_frames = 0;  // No damage shown initially
  item.blink_frames = 0;  // No blink effect initially
  item.original_pixbuf = army_image->property_pixbuf().get_value();  // Store original for blink restoration
  army_items.push_back(item);
}

// -----------------------------------------------------------------------------
// do_round() - Animation Timer Callback
// -----------------------------------------------------------------------------
// This is the core animation loop, called repeatedly by a timer. It processes
// the pre-calculated fight events (FightItem) and updates the visual display.
//
// The animation has three phases:
//
// PHASE 1: Clear Explosions (lines below)
//   - If any army has exploding=true from previous frame, clear the explosion
//   - Return CONTINUE to show the cleared state for one frame
//
// PHASE 2: Process Actions (main loop)
//   - Iterate through FightItem events from the actions list
//   - For each event: find the target army, apply damage to LOCAL hp,
//     update HP label, show damage dealt, trigger explosion if hp reaches 0
//   - The 'round' variable tracks which battle turn we're displaying
//   - When we reach an event from the next turn (f.turn > round), we pause
//     by returning CONTINUE so the player can see the current round's results
//
// PHASE 3: Show Result (after all actions processed)
//   - Display the decision label (win/lose message)
//   - Pause briefly, then hide window and quit main loop
//   - Return STOP to end the timer
//
// Key variables:
//   - action_iterator: Current position in the events list
//   - round: Current battle turn being displayed (0-indexed)
//   - FightItem.turn: Turn number when this event occurred
//   - ArmyItem.hp: LOCAL hp tracking for animation (separate from Army::d_hp)
//
// Returns:
//   - Timing::CONTINUE: Reschedule timer for next frame
//   - Timing::STOP: End animation, close window
// -----------------------------------------------------------------------------
bool FightWindow::do_round()
{
  // Prepare explosion graphic (scaled to army image size)
  ImageCache *gc = ImageCache::getInstance();
  PixMask *p = gc->getExplosionPic ()->copy ();
  PixMask::scale (p, armypic_width, armypic_height);
  Glib::RefPtr<Gdk::Pixbuf> expl = p->to_pixbuf();
  delete p;

  // =========================================================================
  // PHASE 1: Clear explosions from previous frame
  // =========================================================================
  // If any army died in the previous frame, their explosion graphic is still
  // showing. We clear it here and return CONTINUE to show the empty space
  // for one frame before processing more events.
  for (army_items_type::iterator i = army_items.begin(), end = army_items.end();
       i != end; ++i)
    {
      if (!i->exploding)
        continue;

      // Replace explosion with transparent empty image
      Glib::RefPtr<Gdk::Pixbuf> empty_pic
        = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, armypic_width, armypic_height);
      empty_pic->fill(0x00000000);
      i->image->property_pixbuf() = empty_pic;
      i->exploding = false;
      // Note: damage label cleared by frame counter below, not here
      return Timing::CONTINUE;  // Show cleared state for one frame
    }

  // =========================================================================
  // PHASE 1.5: Process timed effects (damage display and hit blink)
  // =========================================================================
  // Decrement frame counters and clear/restore visuals when they expire
  for (army_items_type::iterator i = army_items.begin(), end = army_items.end();
       i != end; ++i)
    {
      // Handle temporary damage display
      if (i->damage_show_frames > 0)
        {
          i->damage_show_frames--;
          if (i->damage_show_frames == 0)
            i->damage_label->set_text("");  // Clear damage text
        }

      // Handle hit blink effect - restore original image when blink expires
      if (i->blink_frames > 0)
        {
          i->blink_frames--;
          if (i->blink_frames == 0 && i->hp > 0)
            i->image->property_pixbuf() = i->original_pixbuf;  // Restore original
        }
    }

  // =========================================================================
  // PHASE 1.6: Check if last hit's effects are done, then start pause
  // =========================================================================
  // We wait for the hit army's blink and damage display to clear before pausing
  if (last_hit_army_id != 0)
    {
      for (army_items_type::iterator i = army_items.begin(), end = army_items.end();
           i != end; ++i)
        {
          if (i->army->getId() == last_hit_army_id)
            {
              // Check if effects are still active
              if (i->exploding || i->blink_frames > 0 || i->damage_show_frames > 0)
                return Timing::CONTINUE;  // Wait for effects to clear

              // Effects done - start the post-hit pause
              post_hit_pause_frames = POST_HIT_PAUSE_FRAMES;
              last_hit_army_id = 0;
              break;
            }
        }
    }

  // =========================================================================
  // PHASE 1.7: Handle post-hit pause
  // =========================================================================
  if (post_hit_pause_frames > 0)
    {
      post_hit_pause_frames--;
      return Timing::CONTINUE;  // Still pausing between hits
    }

  // =========================================================================
  // PHASE 2: Process ONE fight event (FightItem) from the pre-calculated list
  // =========================================================================
  // Each FightItem represents one hit: which army was damaged and by how much.
  // We process ONE event per frame, then wait for effects and pause.
  if (action_iterator != actions.end())
    {
      FightItem &f = *action_iterator;

      // Debug: log each FightItem as it's processed during animation
      std::cerr << "[ANIMATION] Processing FightItem: army_id=" << f.id
                << " damage=" << f.damage << " turn=" << f.turn << std::endl;

      // Track turn boundaries
      if (f.turn > round)
        ++round;

      ++action_iterator;

      for (army_items_type::iterator i = army_items.begin(),
           end = army_items.end(); i != end; ++i)
        if (i->army->getId() == f.id)
          {
            // Debug: log army state when found
            std::cerr << "[ANIMATION] Found army " << i->army->getName()
                      << " (id=" << i->army->getId() << ")"
                      << " current_hp=" << i->hp << std::endl;

            // Skip if this army is already dead (hp <= 0) - prevents
            // explosions appearing on empty spaces for stale FightItems
            if (i->hp <= 0.0)
              {
                std::cerr << "[ANIMATION] Skipping - army already dead" << std::endl;
                break;
              }

            i->hp -= f.damage;
            if (i->hp < 0)
              i->hp = 0;

            std::cerr << "[ANIMATION] After damage: hp=" << i->hp << std::endl;

            // Update HP label to show current HP after taking damage
            // Always show one decimal place for consistent display (e.g., "1.3/2")
            std::ostringstream hp_oss;
            hp_oss << std::fixed << std::setprecision(1) << i->hp;
            Glib::ustring hp_text = String::ucompose("%1/%2", hp_oss.str(), i->army->getStat(Army::HP));
            i->hp_label->set_text(hp_text);

            // Show damage dealt for this hit (with 2 decimal precision)
            // Damage display is temporary - will be cleared after damage_show_frames decrements to 0
            std::ostringstream damage_oss;
            damage_oss << std::fixed << std::setprecision(2) << f.damage;
            Glib::ustring damage_text = String::ucompose("-%1", damage_oss.str());
            i->damage_label->set_text(damage_text);
            i->damage_show_frames = DAMAGE_DISPLAY_FRAMES;

            double fraction = double(i->hp) / i->army->getStat(Army::HP);
            if (fraction == 0.0)
              {
                std::cerr << "[ANIMATION] Army died - showing explosion" << std::endl;
                Glib::RefPtr<Gdk::Pixbuf> w = i->water_image->property_pixbuf ();
                w->fill(0x00000000);
                i->water_image->property_pixbuf () = w;
                Glib::RefPtr<Gdk::Pixbuf> a = i->image->property_pixbuf();
                //draw the explosion on the army
                expl->copy_area (0, 0, armypic_width, armypic_height, a, 0 , 0);
                i->image->property_pixbuf () = a;
                i->exploding = true;
                // Clear HP label for dead army
                i->hp_label->set_text("");
              }
            else
              {
                // Army took damage but didn't die - show hit blink effect
                i->blink_frames = BLINK_EFFECT_FRAMES;
                // Darken the army image to show it was hit
                Glib::RefPtr<Gdk::Pixbuf> darkened = i->original_pixbuf->copy();
                // Multiply each pixel's RGB by 0.5 to darken
                int width = darkened->get_width();
                int height = darkened->get_height();
                int rowstride = darkened->get_rowstride();
                int n_channels = darkened->get_n_channels();
                guchar *pixels = darkened->get_pixels();
                for (int y = 0; y < height; y++)
                  {
                    guchar *row = pixels + y * rowstride;
                    for (int x = 0; x < width; x++)
                      {
                        guchar *pixel = row + x * n_channels;
                        pixel[0] = pixel[0] / 2;  // Red
                        pixel[1] = pixel[1] / 2;  // Green
                        pixel[2] = pixel[2] / 2;  // Blue
                        // Alpha (pixel[3]) unchanged
                      }
                  }
                i->image->property_pixbuf() = darkened;
              }

            // Track this army so we wait for its effects before next hit
            last_hit_army_id = i->army->getId();
            break;
          }

      return Timing::CONTINUE;  // Wait for effects then process next hit
    }

  // =========================================================================
  // PHASE 3: All events processed - show battle result
  // =========================================================================
  // Brief pause before showing result, then longer pause to read it
  guint32 first_pause = 100000;   // 100ms before result
  guint32 second_pause = 2000000; // 2 seconds to read result

  // Process pending GTK events to ensure display is updated
  while (g_main_context_iteration(NULL, FALSE));
  if (d_quick || s_quick_all)
    Glib::usleep (first_pause / 100);  // Faster in quick mode
  else
    Glib::usleep (first_pause);

  // Show win/lose message
  decision_label->set_text (d_decision);

  // Process pending GTK events and pause for player to read
  while (g_main_context_iteration(NULL, FALSE));
  if (d_quick || s_quick_all)
    Glib::usleep (second_pause / 100);  // Faster in quick mode
  else
    Glib::usleep (second_pause);

  // Close the fight window and return control to caller
  window->hide();
  main_loop->quit();

  return Timing::STOP;  // End animation timer
}

void FightWindow::on_key_release_event(GdkEventKey *e)
{
  if (e->keyval == GDK_KEY_exclam)
    {
      if (s_quick_all)
        {
          Timing::instance().register_timer (method(do_round),
                                             normal_round_speed);
          d_quick = false;
          s_quick_all = false;
        }
      else
        {
          d_quick = true;
          s_quick_all = true;
          Timing::instance().register_timer (method(do_round), 
                                             fast_round_speed / 3);
        }
    }
  else
    {
      Timing::instance().register_timer (method(do_round), fast_round_speed);
      d_quick = true;
    }
}
