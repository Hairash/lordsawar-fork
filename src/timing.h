//  Copyright (C) 2007 Ole Laursen
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
#ifndef TIMING_H
#define TIMING_H

#include <sigc++/slot.h>
#include <sigc++/connection.h>
#include <sigc++/signal.h>

//! A simple timing framework.
/**
  * Main function is register_timer. The timer_registered signal hook is used
  * to do the actual work.
  */
class Timing
{
 public:
    static Timing &instance();
    ~Timing() {};

    enum { STOP = false, CONTINUE = true };
    typedef sigc::slot<bool> timer_slot;

    // register a callback, returns a handle that can be used to disconnect the
    // timer - alternatively, return Timing::STOP
    sigc::connection register_timer(timer_slot s, int msecs_interval);

    // the entity providing timing should hook into this to make things happen 
    sigc::signal<sigc::connection, timer_slot, int> timer_registered;
    
 private:
    Timing();
};

#endif
