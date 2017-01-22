/* gb-beautifier-private.h
 *
 * Copyright (C) 2016 sebastien lafargue <slafargue@gnome.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GB_BEAUTIFIER_PRIVATE_H
#define GB_BEAUTIFIER_PRIVATE_H

#include <glib-object.h>

#include "ide.h"

G_BEGIN_DECLS

struct _GbBeautifierWorkbenchAddin
{
  GObject                parent_instance;

  IdeWorkbench          *workbench;
  IdeEditorPerspective  *editor;
  GArray                *entries;
};

G_END_DECLS

#endif /* GB_BEAUTIFIER_PRIVATE_H */
