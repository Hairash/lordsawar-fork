// Copyright (C) 2011, 2014, 2015 Ben Asselstine
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

#include <config.h>

#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include "Configuration.h"
#include "File.h"
#include "gamelist.h"
#include "profilelist.h"
#include "file-compat.h"

#include "gamehost-server.h"
#include "vector.h"
#include "ucompose.hpp"

int max_vector_width;
    
int main(int argc, char* argv[])
{
  bool foreground = false;
  std::list<Glib::ustring> members;
  Glib::ustring hostname = "";
  srand(time(NULL));         // set the random seed

  initialize_configuration();
  Profilelist::support_backward_compatibility();
  Gamelist::support_backward_compatibility();
  FileCompat::getInstance()->initialize();
  Vector<int>::setMaximumWidth(1000);

  setlocale(LC_ALL, Configuration::s_lang.c_str());

  int port = 0;
  Gtk::Main *gtk_main = new Gtk::Main(argc, argv);
  if (argc > 1)
    {
      for (int i = 2; i <= argc; i++)
	{
          Glib::ustring parameter(argv[i-1]); 
	  if (parameter == "--port" || parameter == "-p")
	    {
	      i++;
	      //convert the next argument
	      char* error = 0;
	      long userport = strtol(argv[i-1], &error, 10);
	      if (error && (*error != '\0'))
		{
                  std::cerr <<_("non-numerical value for --port") <<std::endl;
		  exit(-1);
		}
              if (userport > 65535 || userport < 1000)
                {
                  std::cerr <<_("invalid value for --port") <<std::endl;
		  exit(-1);
                }
              port = userport;
	    }
	  else if (parameter == "--host" || parameter == "-h")
            {
              hostname = parameter;
            }
	  else if (parameter == "--foreground" || parameter == "-f")
            {
              foreground = true;
            }
	  else if (parameter == "--members" || parameter == "-m")
            {
              members = GamehostServer::load_members_from_file(parameter);
            }
	  else if (parameter == "--help" || parameter == "-?")
	    {
              std::cout << Glib::get_prgname() << " [OPTION]..." << std::endl << std::endl;
              std::cout << "LordsAWar! Game-host Server " << _("version") << " " << VERSION << std::endl << std::endl;
              std::cout << _("Options:") << std::endl << std::endl; 
              std::cout << "  -?, --help                 " << _("Display this help and exit") <<std::endl;
              std::cout << "  -f, --foreground           " << _("Do not detach from the controlling terminal") << std::endl;
              std::cout << "  -h, --host <string>        " << _("Advertise our hostname as this to game clients") << std::endl;
              std::cout << "  -p, --port <number>        " << _("Start the server on the given port") << std::endl;
              std::cout << "  -m, --members <file>       " << _("Allow the profile ids in this file to host games") << std::endl;
              std::cout << std::endl;
              std::cout << _("Report bugs to") << " <" << PACKAGE_BUGREPORT ">." << std::endl;
	      exit(0);
	    }
	}
    }
  Glib::ustring lordsawar = Glib::find_program_in_path(PACKAGE);
  if (lordsawar == "")
    {
      std::cerr << 
        String::ucompose(_("Error: could not find %1 program in path."), 
                         PACKAGE) << std::endl;
      return EXIT_FAILURE;
    }

  if (foreground == false)
    {
#ifndef __MINGW32__
      if (daemon (0, 0) == -1)
        std::cerr << _("Could not detach from controlling terminal.") << std::endl;
#endif
    }

  GamehostServer *gamehostserver = GamehostServer::getInstance();
  if (port == 0)
    port = LORDSAWAR_GAMEHOST_PORT;
  if (hostname == "")
    hostname = Configuration::s_gamehost_server_hostname;
  gamehostserver->setHostname(hostname);
  gamehostserver->setMembers(members);
  gamehostserver->start(port);
  gtk_main->run();
  delete gtk_main;

  return EXIT_SUCCESS;
}
