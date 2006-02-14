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

#include <ui.h>
#include <pwman.h>

WINDOW *list = NULL;
int lines = 0;
//int curitem = 0;

int
uilist_init()
{
	char str[80];
	list = newwin(LIST_LINES, COLS, LIST_TOP, 0);
	
	scrollok(list, TRUE);
}
/*
int
resize_list()
{
	wresize(list, LIST_LINES, COLS);
}
*/

int
uilist_free()
{
	delwin(list);
	list = NULL;
}

int 
uilist_highlight_line(int line)
{
	wstandout(list);
/*	mvwchgat(list, i, 0, -1, A_STANDOUT, 0, NULL);*/
        scrollok(list, FALSE);
	        {
                int i;
                wmove(list, line, 0);
                for(i = 0; i < COLS; i++)
                        waddch(list, ' ');
	        /*wattrset(win, 0);*/
        }
        scrollok(list, TRUE);
}

PWList *
uilist_get_highlighted_sublist()
{
	PWList *iter;
	int i = -1;

	if(!current_pw_sublist){ return NULL; }
	if(current_pw_sublist->parent){ i++; }

	for(iter = current_pw_sublist->sublists; iter != NULL; iter = iter->next){
		i++;
		if(i == current_pw_sublist->current_item){
			break;
		}
	}
	return iter;
}

Pw *
uilist_get_highlighted_item()
{	
	Pw *iter;
	PWList *listiter;
	int i = -1;

	if(current_pw_sublist->parent){ i++; }
	
	for(listiter = current_pw_sublist->sublists; listiter != NULL; listiter = listiter->next){
		i++;
	}
		
	for(iter = current_pw_sublist->list; iter != NULL; iter = iter->next){
		if( filter_apply(iter, options->filter) ){
			i++;
		}
		if( i == current_pw_sublist->current_item ){
			debug("get_highlighted_item: found %d, break now", i);
			return iter;
		}
	}
/*	fprintf(stderr, "%d.", curitem);
	for(iter = pwlist; (iter != NULL) && i <= curitem; iter = iter->next){
		if( apply_filter(iter, options->filter) ){
			i++;	
		}	
	}*/
	debug("get_highlighted_item: nothing found, return NULL");
	return NULL;
}

LIST_ITEM_TYPE
uilist_get_highlighted_type()
{
	Pw *iter;
	PWList *listiter;
	int i = -1;

	if(current_pw_sublist->parent){
		if(current_pw_sublist->current_item == 0){
			return PW_UPLEVEL;
		}
		i++;
	}
	for(listiter = current_pw_sublist->sublists; listiter != NULL; listiter = listiter->next){
		i++;
		if(i == current_pw_sublist->current_item){
			return PW_SUBLIST;
		}
	}
		
	for(iter = current_pw_sublist->list; iter != NULL; iter = iter->next){
		if( filter_apply(iter, options->filter) ){
			i++;
		}
		if( i == current_pw_sublist->current_item ){
			return PW_ITEM;
		}
	}
	return PW_NULL;
}

int
uilist_refresh()
{
	Pw *iter;
	PWList *listiter;
	int i = 0;
	static int first_list_item = 0;
	int num_shown = 0;

	debug("refresh_list: refreshing list");
	if(list == NULL){
		uilist_init();
	}
	if(current_pw_sublist == NULL){
		return -1;
	}

	uilist_clear();;
	lines = 0;

	uilist_headerline();

	// Ensure we don't end up off the screen
	if(current_pw_sublist->current_item < 0){
		current_pw_sublist->current_item = 0;
	}
	if(current_pw_sublist->current_item < first_list_item){
		first_list_item = current_pw_sublist->current_item;		
	} else if((current_pw_sublist->current_item > LAST_LIST_ITEM)){
		first_list_item = current_pw_sublist->current_item - (LIST_LINES-1);
	}

	if(current_pw_sublist->parent){
		if((i >= first_list_item) && (i <= LAST_LIST_ITEM)){
			if(lines == current_pw_sublist->current_item){
				uilist_highlight_line(num_shown);
			} else {
				wattrset(list, A_BOLD);
			}
			mvwprintw(list, num_shown, NAMEPOS, "<Up One Level - \"%s\">", current_pw_sublist->parent->name);
			wattrset(list, A_NORMAL);
			wstandend(list);
			num_shown++;
		}
		i++; 
		lines++;
	}
	for(listiter = current_pw_sublist->sublists; listiter != NULL; listiter = listiter->next){
		if((i >= first_list_item) && (i <= LAST_LIST_ITEM)){
			if(lines == current_pw_sublist->current_item){
				uilist_highlight_line(num_shown);
			} else {
				wattrset(list, A_BOLD);
			}
			mvwprintw(list, num_shown, NAMEPOS, "%s ->", listiter->name);
			wattrset(list, A_NORMAL);
			wstandend(list);
			num_shown++;
		}
		i++; 
		lines++;
	}
	for(iter = current_pw_sublist->list; (iter != NULL); iter = iter->next){
		/*
		 * if line satifies filter criteria increment i and lines
		 */
		if( filter_apply(iter, options->filter) ){
			if((i >= first_list_item) && (i <= LAST_LIST_ITEM)){
				if(lines == current_pw_sublist->current_item){
					uilist_highlight_line(num_shown);
				}
				mvwaddnstr(list, num_shown, NAMEPOS, iter->name, NAMELEN);
				mvwaddnstr(list, num_shown, HOSTPOS, iter->host, HOSTLEN);
				mvwaddnstr(list, num_shown, USERPOS, iter->user, USERLEN);
				wstandend(list);
				num_shown++;
			}
			i++;	
			lines++;
		}
		
	}

	wrefresh(list);
	hide_cursor();

	debug("refresh_list: done refreshing list");
}

int
uilist_clear()
{
	int i;
	
	werase(list);
	for(i = 0; i < COLS; i++){
		mvaddch(2, i, ' ');
	}
}

int
uilist_headerline()
{
	int i;

	show_cursor();
	attrset(A_BOLD);

	mvaddnstr(2, NAMEPOS, "Name", NAMELEN);
	mvaddnstr(2, HOSTPOS, "Host", HOSTLEN);
	mvaddnstr(2, USERPOS, "Username", USERLEN);

	attrset(A_NORMAL);
	hide_cursor();
}

int 
uilist_page_up()
{
	current_pw_sublist->current_item -= (LIST_LINES - 1);

	if(current_pw_sublist->current_item < 1){
		current_pw_sublist->current_item = 0;
	}
	uilist_refresh();
}

int
uilist_page_down()
{
	current_pw_sublist->current_item += (LIST_LINES - 1);

	if(current_pw_sublist->current_item >= (lines - 1)){
		current_pw_sublist->current_item = lines -1;
	}
	uilist_refresh();
}

int
uilist_up()
{
	if(current_pw_sublist->current_item < 1){
		return;
	}

	current_pw_sublist->current_item--;
	
	uilist_refresh();
}

int
uilist_down()
{
	if(current_pw_sublist->current_item >= (lines-1)){
		return;
	}

	current_pw_sublist->current_item++;

	uilist_refresh();
}
