/*
 *  PWMan - password management application
 *
 *  Copyright (C) 2002  Ivan Kelly <ivan@ivankelly.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PW_MINDER_H
#define PW_MINDER_H

#include <curses.h>
#include <stdio.h>
#include <string.h>
#include <config.h>
#include <time.h>
#include <stdarg.h>

#define CONF_FILE 	".pwmanrc" 
/*
#define NAME_LEN	40
#define HOST_LEN	40
#define USER_LEN	40
#define PASS_LEN	40
#define LAUNCH_LEN	40

#define SHORT_STR	20
#define	MED_STR		30
#define LONG_STR	40
#define V_LONG_STR	256
*/

/* string lengths */
#define STRING_SHORT	64
#define STRING_MEDIUM	128
#define STRING_LONG	256

#define MAIN_HELPLINE 	"q:quit  ?:help  a:add  e:edit  d:delete"
#define READONLY_MSG	"RO"

#define MAX_SEARCH_DEPTH 25

#define DEFAULT_UMASK 066


#define FF_VERSION 3 

struct _PW {
	int id;
	char *name;
	char *host;
	char *user;
	char *passwd;
	char *launch;
	struct _PW *next;
};
typedef struct _PW Pw;

struct _PWList {
	char *name;
	Pw *list;
	struct _PWList *parent;
	struct _PWList *sublists;
	struct _PWList *next;

	/* ui stuff, shouldn't be here but this is a quick hack */
	int current_item;
};
typedef struct _PWList PWList;

struct _PWSearchResult {
	/* Will have one of these two, never both! */
	PWList *sublist;
	Pw *entry;

	/* The next one along, as with other structs */
	struct _PWSearchResult* next;
};
typedef struct _PWSearchResult PWSearchResult;


typedef struct {
	int field;
	char *filter;
} PwFilter;

typedef struct {
	char *search_term;
} PwSearch;

typedef struct {
	char *gpg_id;
	char *gpg_path;
	char *password_file;
	int passphrase_timeout;
	PwFilter *filter;
	PwSearch *search;
	int readonly;
} Options;

Options *options;
int write_options;
PWList *pwlist;
PWList *current_pw_sublist;
PWSearchResult *search_results;
time_t time_base;

char *trim_ws(char*);
void debug(char*, ...);
int ui_init();
int ui_run();
int ui_end();

PwFilter * filter_new();
PwSearch * search_new();
Options * options_new();

Pw* pwlist_new_pw();
PWList *pwlist_new(char*);
#endif
