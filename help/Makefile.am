SUBDIRS=figures
EXTRA_DIST = $(noinst_DATA) $(man_MANS)
#helpdir = ${datadir}/gnome/help/lordsawar/C/

noinst_DATA = legal.xml lordsawar.xml manual.map

man_MANS = lordsawar.6 \
	   lordsawar-game-list-server.6 lordsawar-game-list-client.6 \
	   lordsawar-game-host-server.6 lordsawar-game-host-client.6 \
	   lordsawar-import.6

## for html publishing install docbook-utils:
## cd into this directory
## $ docbook2html --nochunks -e no-valid lordsawar.xml
## cp -r figures lordsawar.html lordsawar-web/lordsawar/manual/0.1.1/
## do the necessary cvs adds, and then commit

