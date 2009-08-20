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

#include <pwman.h>

/*xtern int curitem;*/

PwSearch *
search_new()
{
	PwSearch *new;

	new = malloc(sizeof(PwSearch));

	new->search_term = malloc(STRING_MEDIUM);
	new->search_term[0] = 0;

	return new;
}

/*
 * String checking only case insensitive using gnu glibc
 */
static char*
search_strcasestr(char *haystack, char *needle){
#ifdef HAVE_STRCASESTR
	return (char*)strcasestr(haystack, needle);
#else
	return (char*)strstr(haystack, needle);
#endif
}

int
search_apply(Pw *pw, PwSearch* srch)
{
	if( (srch == NULL) || (srch->search_term == NULL) ){
		/* no search object */
		return 1;
	}
	if( strlen(srch->search_term) == 0 ){
		/* no search */
		return 1;
	}

	// TODO
	return 1;
}

void
search_get()
{
	char c;
	if(options->search == NULL) {
		debug("No options->search");
	} else {
		if(options->search->search_term == NULL) {
			debug("No options->search->search_term");
		} else {
			debug("Len was %d", strlen(options->search->search_term));
		}
	}

	options->search->search_term =
	ui_statusline_ask_str("String to search for: ", options->search->search_term, STRING_MEDIUM);

	// TODO - searching

	current_pw_sublist->current_item = -1;
	uilist_refresh();
}


int
search_alert(PwSearch* srch)
{
	char alert[80];	

	if( (srch == NULL) || (srch->search_term == NULL) ){
		/* no search object */
		return 1;
	}
	if( strlen(srch->search_term) == 0 ){
		/* no search */
		return 1;
	}

	sprintf(alert, " (Searching for '%s')", srch->search_term);

	ui_statusline_clear();
	ui_statusline_msg(alert);
}


