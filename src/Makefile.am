INCLUDES = -I$(top_srcdir) -I$(includedir) \
	  -DG_LOG_DOMAIN=\"pwman\" 
		
bin_PROGRAMS = pwman convert_pwdb

pwman_SOURCES = options.c pwlist.c ui.c uilist.c pwman.c actions.c launch.c ui.h \
		pwman.h help.h filter.c gnupg.c import_export.c pwgen.c misc.c 

convert_pwdb_SOURCES = convert_pwdb.c
