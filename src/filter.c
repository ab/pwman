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

extern int curitem;

PwFilter *
new_filter()
{
	PwFilter *new;

	new = malloc(sizeof(PwFilter));

	new->field = -1;
	new->filter = malloc(SHORT_STR);
	new->filter[0] = 0;

	return new;
}

/*
 * String checking only case insensitive using gnu glibc
 */
static char*
pwstrcasestr(char *haystack, char *needle){
#ifdef HAVE_STRCASESTR
	return strcasestr(haystack, needle);
#else
	return strstr(haystack, needle);
#endif
}

int
apply_filter(Pw *pw, PwFilter* fil)
{
	if( strlen(fil->filter) == 0 ){
		/* no filter */
		return 1;
	}
	switch(fil->field){
		case 0:
			if( pwstrcasestr(pw->name, fil->filter) ){
				return 1;
			}					
			break;
		case 1:
			if( pwstrcasestr(pw->host, fil->filter) ){
				return 1;
			}
			break;
		case 2:
			if( pwstrcasestr(pw->user, fil->filter) ){
				return 1;
			}
			break;
		case 3:
			if( pwstrcasestr(pw->launch, fil->filter) ){
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
get_filter()
{
	char c;

	statusline_ask_char("Filter which field? (n)ame (h)ost (u)ser (l)aunch n(o)ne", &c, "nhulo\n");
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

			refresh_list();
			return;
			break;
	}
	statusline_ask_str("String to search for: ", options->filter->filter, SHORT_STR);

	curitem = -1;
	refresh_list();
}
















