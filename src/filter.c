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

PwFilter *
filter_new()
{
	PwFilter *new;

	new = malloc(sizeof(PwFilter));
	if(new == NULL) {
		pw_abort("Failed to allocate memory to hold filtering details!");
	}

	new->field = -1;
	new->filter = malloc(STRING_MEDIUM);
	if(new == NULL) {
		pw_abort("Failed to allocate memory to hold filtering term!");
	}
	new->filter[0] = 0;

	return new;
}

/*
 * String checking only case insensitive using gnu glibc
 */
static char*
filter_strcasestr(char *haystack, char *needle){
#ifdef HAVE_STRCASESTR
	return (char*)strcasestr(haystack, needle);
#else
	return (char*)strstr(haystack, needle);
#endif
}

int
filter_apply(Pw *pw, PwFilter* fil)
{
	if( (fil == NULL) || (fil->filter == NULL) ){
		/* no filter object */
		return 1;
	}
	if( strlen(fil->filter) == 0 ){
		/* no filter */
		return 1;
	}
	switch(fil->field){
		case 0:
			if( filter_strcasestr(pw->name, fil->filter) ){
				return 1;
			}					
			break;
		case 1:
			if( filter_strcasestr(pw->host, fil->filter) ){
				return 1;
			}
			break;
		case 2:
			if( filter_strcasestr(pw->user, fil->filter) ){
				return 1;
			}
			break;
		case 3:
			if( filter_strcasestr(pw->launch, fil->filter) ){
				return 1;
			}
			break;
		default:
/*			fprintf(stderr, "Invalid filter field %d\n", fil->field);*/
			break;
	}
	return 0;
}

void
filter_get()
{
	char c;

	ui_statusline_ask_char("Filter which field? (n)ame (h)ost (u)ser (l)aunch n(o)ne", &c, "nhulo\n");
	switch(c){
		case 'n':
			options->filter->field = 0;
			break;
		case 'h':
			options->filter->field = 1;
			break;
		case 'u':
			options->filter->field = 2;
			break;
		case 'l':
			options->filter->field = 3;
			break;
		case 'o':
		default:
			options->filter->field = -1;
			options->filter->filter[0] = 0;

			uilist_refresh();
			return;
			break;
	}
	ui_statusline_ask_str("String to search for: ", options->filter->filter, STRING_MEDIUM);

	current_pw_sublist->current_item = -1;
	uilist_refresh();
}


int
filter_alert(PwFilter* fil)
{
	char alert[80];	

	if( (fil == NULL) || (fil->filter == NULL) ){
		/* no filter object */
		return 1;
	}
	if( strlen(fil->filter) == 0 ){
		/* no filter */
		return 1;
	}
	switch(fil->field){
		case 0:
			sprintf(alert, " (Filtering on name with '%s')", fil->filter);
			break;
		case 1:
			sprintf(alert, " (Filtering on host with '%s')", fil->filter);
			break;
		case 2:
			sprintf(alert, " (Filtering on user with '%s')", fil->filter);
		case 3:
			sprintf(alert, " (Filtering on launch with '%s')", fil->filter);
		default:
/*			fprintf(stderr, "Invalid filter field %d\n", fil->field);*/
			break;
	}

	ui_statusline_clear();
	ui_statusline_msg(alert);
}


