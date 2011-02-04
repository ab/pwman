INCLUDES = -I$(top_srcdir) -I$(includedir) \
	  -DG_LOG_DOMAIN=\"pwman\" 
		
bin_PROGRAMS = pwman convert_pwdb pwdb2csv

pwman_SOURCES = actions.c filter.c gnupg.c gnupg.h help.h \
                launch.c misc.c options.c pwgen.c pwlist.c \
                pwman.c pwman.h search.c ui.c ui.h uilist.c

convert_pwdb_SOURCES = convert_pwdb.c

pwdb2csv_SOURCES = pwdb2csv.c import_export.c
