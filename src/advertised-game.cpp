//  Copyright (C) 2011, 2014, 2015 Ben Asselstine
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

//#include <iostream>
#include <fstream>
#include <sstream>
#include "advertised-game.h"
#include "xmlhelper.h"
#include "profile.h"
#include "network-connection.h"
#include "connection-manager.h"


Glib::ustring AdvertisedGame::d_tag_name = "advertisedgame";

//#define debug(x) {cerr<<__FILE__<<": "<<__LINE__<<": "<<x<<endl<<flush;}
#define debug(x)

AdvertisedGame::AdvertisedGame(GameScenario *scen, Profile *p)
	:RecentlyPlayedNetworkedGame(scen, p)
{
  d_creation_date.assign_current_time();
  d_last_pinged_date.assign_current_time();
  d_profile = new Profile(*p);
}
	
	
AdvertisedGame::AdvertisedGame(const RecentlyPlayedNetworkedGame &orig, Profile *p)
        :RecentlyPlayedNetworkedGame(orig)
{
  d_creation_date.assign_current_time();
  d_last_pinged_date.assign_current_time();
  d_profile = new Profile(*p);
}

AdvertisedGame::AdvertisedGame(const AdvertisedGame &orig)
        :RecentlyPlayedNetworkedGame(orig), 
        d_creation_date(orig.d_creation_date),
        d_last_pinged_date(orig.d_last_pinged_date)
{
  d_profile = new Profile(*orig.d_profile);
}

AdvertisedGame::AdvertisedGame(XML_Helper *helper)
	:RecentlyPlayedNetworkedGame(helper)
{
  Glib::ustring s;
  helper->getData(s, "created_on");
  d_creation_date.assign_from_iso8601(s);
  helper->getData(s, "last_pinged_on");
  d_last_pinged_date.assign_from_iso8601(s);
  helper->registerTag(Profile::d_tag, 
		      sigc::mem_fun(*this, &AdvertisedGame::loadProfile));
}

AdvertisedGame::~AdvertisedGame()
{
  delete d_profile;
}

bool AdvertisedGame::doSave(XML_Helper *helper) const
{
  bool retval = true;
  Glib::ustring s = d_creation_date.as_iso8601();
  retval &= helper->saveData("created_on", s);
  s = d_last_pinged_date.as_iso8601();
  retval &= helper->saveData("last_pinged_on", s);
  retval &= helper->saveData("host", getHost());
  retval &= helper->saveData("port", getPort());
  retval &= d_profile->save(helper);
  return retval;
}

bool AdvertisedGame::saveEntry(XML_Helper* helper) const
{
  bool retval = true;
  retval &= helper->openTag(d_tag_name);
  retval &= dynamic_cast<const RecentlyPlayedGame*>(this)->saveContents(helper);
  retval &= helper->closeTag();
  return retval;
}

bool AdvertisedGame::loadProfile(Glib::ustring tag, XML_Helper *helper)
{
  if (tag == Profile::d_tag)
    {
      d_profile = new Profile(helper);
      return true;
    }
  return false;
}
        
void AdvertisedGame::ping()
{
  NetworkConnection *conn = ConnectionManager::create_connection();

  conn->connected.connect
    (sigc::bind(sigc::mem_fun(*this, &AdvertisedGame::on_connected_to_game), 
                conn));
  conn->connection_failed.connect
    (sigc::bind(sigc::mem_fun(*this, 
                              &AdvertisedGame::on_could_not_connect_to_game), 
                conn));
  conn->connectToHost(getHost(), getPort());
}

void AdvertisedGame::on_connected_to_game(NetworkConnection *conn)
{
  conn->tear_down_connection();
  d_last_pinged_date.assign_current_time();
  pinged.emit(true);
}

void AdvertisedGame::on_could_not_connect_to_game(NetworkConnection *conn)
{
  conn->tear_down_connection();
  pinged.emit(false);
}
