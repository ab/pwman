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
search_apply()
{
	PWList *stack[MAX_SEARCH_DEPTH];
	PWList *tmpList; 
	Pw *tmp; 
	int depth;

	PWSearchResult *cur;
	PWSearchResult *next;

	// Replaces current_pw_sublist with a custom list of things
	//  we found that looked suitable

	if( search_active(options->search) == 0 ) {
		return 1;
	}

	if(search_results != NULL) {
		_search_free();
	}

	// Setup for start
	depth = 0;
	tmpList = pwlist;

	// Find anything we like the look of
	while(depth >= 0) {
		// Any sublists?
		if(tmpList->sublists != NULL && depth < MAX_SEARCH_DEPTH) {
			// Prepare to descend
			stack[depth] = tmpList;
			depth++;
			tmpList = tmpList->sublists;
			// Test
			if(search_strcasestr(tmpList->name, options->search->search_term)) {
				next = malloc( sizeof(PWSearchResult) );
				next->sublist = tmpList;

				if(cur != NULL) {
					cur->next = next;
				} else {
					search_results = next;
				}
				cur = next;
			}
			// Descend
			continue;
		}

		// Any entries?
		if(tmpList->list) {
			tmp = tmpList->list;
			while(tmp != NULL) {
				// Test
				if(search_strcasestr(tmp->name, options->search->search_term)
				 || search_strcasestr(tmp->host, options->search->search_term)
				 || search_strcasestr(tmp->user, options->search->search_term)
				 || search_strcasestr(tmp->passwd, options->search->search_term)
				 || search_strcasestr(tmp->launch, options->search->search_term)
				) {
					next = malloc( sizeof(PWSearchResult) );
					next->entry = tmp;

					if(cur != NULL) {
						cur->next = next;
					} else {
						search_results = next;
					}
					cur = next;
				}

				// Next
				tmp = tmp->next;
			}
		}

		// Next sibling if there is one
		if(tmpList->next != NULL) {
			tmpList = tmpList->next;
			continue;
		}

		// Otherwise step up
		depth--;
		if(depth >= 0) {
			tmpList = stack[depth];
		}
	}
	
	// All done
	return 1;
}

int 
search_remove()
{
	// Put things back how they should have been
	current_pw_sublist = pwlist;
	current_pw_sublist->current_item = -1;

	// Free the memory held by the search results
	_search_free();

	// Back to the old screen
	uilist_refresh();
}

int 
_search_free()
{
	PWSearchResult *cur;
	PWSearchResult *next;

	// Free the memory held by the search results
	cur = search_results;
	while(cur != NULL) {
		next = cur->next;
		free(cur);
		cur = next;
	}
	search_results = NULL;
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

	search_apply();

	current_pw_sublist->current_item = -1;
	uilist_refresh();
}


int
search_alert(PwSearch* srch)
{
	char alert[80];	

	if( search_active(srch) == 0 ) {
		return 1;
	}

	sprintf(alert, " (Searching for '%s')", srch->search_term);

	ui_statusline_clear();
	ui_statusline_msg(alert);
}


int search_active(PwSearch* srch)
{
	if( (srch == NULL) || (srch->search_term == NULL) ){
		/* no search object */
		return 0;
	}
	if( strlen(srch->search_term) == 0 ){
		/* no search */
		return 0;
	}
	return 1;
}
