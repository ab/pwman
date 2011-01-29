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

#ifndef PW_GNUPG_H
#define PW_GNUPG_H

#include <libxml/tree.h>
#include <libxml/parser.h>

void gnupg_forget_passphrase();

int gnupg_check_id(char*);
void gnupg_get_id(char*);
void gnupg_get_ids(char **, int);

char* gnupg_get_filename(char*, char);
const char* gnupg_get_passphrase();

int gnupg_read(char*, xmlDocPtr*);
int gnupg_write(xmlDocPtr, char*, char*);
int gnupg_write_many(xmlDocPtr, char**, int, char*);

#endif
