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

#ifndef HELP_H 
#define HELP_H

static char *help[] = {
	"	?		help\n",
	"	q/Q		quit\n",
	"	^L		refresh windows\n",
	"\n",
	"	arrows / j,k	scroll list\n",
	"	enter/space/e	view/edit item\n",
	"	U		move up a level\n",
	"\n",
	"	a		add item\n",
	"	A		add sublist\n",
	"	m		move item/sublist\n",
	"	M		move item/sublist up one level\n",
	"	r		rename item/sublist\n",
	"	d/del		delete item/sublist\n",
	"\n",
	"	l		launch item\n",
	"	Format:		%u = user\n",
	"			%h = host\n",
	"			%p = password\n",
	"	e.g.	mysql -Dmydb -u%u -p%p -h%h\n",
	"\n",
	"	Editor Help\n",
	"	[#]		edit field denoted by #\n",
	"	i.e.	press 3 to edit field 3\n",
	"\n",
	"	Misc Help\n",
	"	o		edit options\n",
	"	^W		write database to file\n",
	"	^R		read database from file\n",
	"	I		import password from file\n",
	"	E		export password to file\n",
	"	^G		Generate password\n",
	"	^F		Forget passphrase\n",
	NULL
};

#endif


