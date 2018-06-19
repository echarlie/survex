/* pos.cc
 * Export from Aven as Survex .pos.
 */
/* Copyright (C) 2001,2002,2011,2013,2014,2015 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "pos.h"

#include "export.h" // For LABELS, etc

#include <algorithm>
#include <stdio.h>
#include <string.h>

#include "message.h"
#include "namecompare.h"
#include "osalloc.h"
#include "useful.h"

using namespace std;

POS::~POS()
{
    vector<pos_label*>::const_iterator i;
    for (i = todo.begin(); i != todo.end(); ++i) {
	delete [] *i;
    }
    todo.clear();
}

const int *
POS::passes() const
{
    static const int default_passes[] = { LABELS|ENTS|FIXES|EXPORTS, 0 };
    return default_passes;
}

void POS::header(const char *, const char *, time_t,
		 double, double, double, double, double, double)
{
    /* TRANSLATORS: Heading line for .pos file.  Please try to ensure the “,”s
     * (or at least the columns) are in the same place */
    fputsnl(msg(/*( Easting, Northing, Altitude )*/195), fh);
}

void
POS::label(const img_point *p, const char *s, bool /*fSurface*/, int /*type*/)
{
    size_t len = strlen(s);
    pos_label * l = (pos_label*)new char[offsetof(pos_label, name) + len + 1];
    l->x = p->x;
    l->y = p->y;
    l->z = p->z;
    memcpy(l->name, s, len + 1);
    todo.push_back(l);
}

class pos_label_ptr_cmp {
    char separator;

  public:
    explicit pos_label_ptr_cmp(char separator_) : separator(separator_) { }

    bool operator()(const POS::pos_label* a, const POS::pos_label* b) {
	return name_cmp(a->name, b->name, separator) < 0;
    }
};

void
POS::footer()
{
    sort(todo.begin(), todo.end(), pos_label_ptr_cmp(separator));
    vector<pos_label*>::const_iterator i;
    for (i = todo.begin(); i != todo.end(); ++i) {
	fprintf(fh, "(%8.2f, %8.2f, %8.2f ) %s\n",
		(*i)->x, (*i)->y, (*i)->z, (*i)->name);
    }
}
