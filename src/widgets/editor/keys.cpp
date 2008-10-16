/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <allegro/keyboard.h>

#include "jinete/jsystem.h"
#include "jinete/jview.h"
#include "jinete/jwidget.h"

#include "core/app.h"
#include "core/color.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/tools.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "widgets/colbar.h"
#include "widgets/editor.h"

int editor_keys_toset_zoom(JWidget widget, int scancode)
{
  Editor *editor = editor_data (widget);

  if ((editor->sprite) &&
      (jwidget_has_mouse (widget)) &&
      !(key_shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG))) {
    JWidget view = jwidget_get_view(widget);
    JRect vp = jview_get_viewport_position(view);
    int x, y, zoom;

    x = 0;
    y = 0;
    zoom = -1;

    switch (scancode) { /* TODO make these keys configurable */
      case KEY_1: zoom = 0; break;
      case KEY_2: zoom = 1; break;
      case KEY_3: zoom = 2; break;
      case KEY_4: zoom = 3; break;
      case KEY_5: zoom = 4; break;
      case KEY_6: zoom = 5; break;
    }

    /* zoom */
    if (zoom >= 0) {
      hide_drawing_cursor(widget);
      screen_to_editor(widget, jmouse_x(0), jmouse_y(0), &x, &y);

      x = editor->offset_x - jrect_w(vp)/2 + ((1<<zoom)>>1) + (x << zoom);
      y = editor->offset_y - jrect_h(vp)/2 + ((1<<zoom)>>1) + (y << zoom);

      if ((editor->zoom != zoom) ||
	  (editor->cursor_editor_x != (vp->x1+vp->x2)/2) ||
	  (editor->cursor_editor_y != (vp->y1+vp->y2)/2)) {
	int use_refresh_region = (editor->zoom == zoom) ? TRUE: FALSE;

	editor->zoom = zoom;

	editor_update(widget);
	editor_set_scroll(widget, x, y, use_refresh_region);

	jmouse_set_position((vp->x1+vp->x2)/2, (vp->y1+vp->y2)/2);
	jrect_free(vp);

	show_drawing_cursor(widget);
	return TRUE;
      }
    }

    jrect_free(vp);
  }

  return FALSE;
}

int editor_keys_toset_brushsize(JWidget widget, int scancode)
{
  Editor *editor = editor_data(widget);

  if ((editor->sprite) &&
      (jwidget_has_mouse (widget)) &&
      !(key_shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG))) {
    /* TODO configurable keys */
    /* set the thickness */
    if (scancode == KEY_MINUS_PAD) {
      if (get_brush_size () > 1)
	set_brush_size (get_brush_size ()-1);
      return TRUE;
    }
    else if (scancode == KEY_PLUS_PAD) {
      if (get_brush_size () < 32)
	set_brush_size (get_brush_size ()+1);
      return TRUE;
    }
  }

  return FALSE;
}