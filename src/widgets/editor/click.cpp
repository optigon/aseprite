/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "gui/manager.h"
#include "gui/rect.h"
#include "gui/system.h"
#include "gui/view.h"
#include "gui/widget.h"
#include "ini_file.h"
#include "modules/gui.h"
#include "widgets/editor/editor.h"

/**********************************************************************/
/* editor-click interface */
/**********************************************************************/

static int click_mode;
static int click_first;

static int click_start_x;
static int click_start_y;
static int click_start_b;

static int click_last_x;
static int click_last_y;
static int click_last_b;

static int click_prev_last_b;

void Editor::editor_click_start(int mode, int *x, int *y, int *b)
{
  click_mode = mode;
  click_first = true;

  click_start_x = click_last_x = jmouse_x(0);
  click_start_y = click_last_y = jmouse_y(0);
  click_start_b = click_last_b = jmouse_b(0) ? jmouse_b(0): 1;

  click_prev_last_b = click_last_b;

  screenToEditor(click_start_x, click_start_y, x, y);
  *b = click_start_b;

  captureMouse();
}

void Editor::editor_click_continue(int mode, int *x, int *y)
{
  click_mode = mode;
  click_first = true;

  click_start_x = click_last_x = jmouse_x(0);
  click_start_y = click_last_y = jmouse_y(0);
  click_start_b = click_last_b = click_prev_last_b;

  screenToEditor(click_start_x, click_start_y, x, y);
}

void Editor::editor_click_done()
{
  releaseMouse();
  clear_keybuf();
}

// Returns false when the user stop the click-loop: releases the
// button or press the second click (depend of the mode)
int Editor::editor_click(int *x, int *y, int *update,
                         void (*scroll_callback) (int before_change))
{
  int prev_x, prev_y;

  poll_keyboard();

  if (click_first) {
    click_first = false;

    if (click_mode == MODE_CLICKANDCLICK) {
      do {
        jmouse_poll();
        gui_feedback();
      } while (jmouse_b(0));

      jmouse_set_position(click_start_x, click_start_y);
      clear_keybuf();
    }
  }

  *update = jmouse_poll();

  screenToEditor(click_last_x, click_last_y, &prev_x, &prev_y);

  click_prev_last_b = click_last_b;

  click_last_x = jmouse_x(0);
  click_last_y = jmouse_y(0);
  click_last_b = jmouse_b(0);

  screenToEditor(click_last_x, click_last_y, x, y);

  /* the mouse was moved */
  if (*update) {
    View* view = View::getView(this);
    gfx::Rect vp = view->getViewportBounds();

    /* update scroll */
    if (jmouse_control_infinite_scroll(vp)) {
      if (scroll_callback)
        (*scroll_callback)(true);

      /* smooth scroll movement */
      if (get_config_bool("Options", "MoveSmooth", true)) {
        jmouse_set_position(MID(vp.x+1, click_last_x, vp.x+vp.w-2),
                            MID(vp.y+1, click_last_y, vp.y+vp.h-2));
      }
      /* this is better for high resolutions: scroll movement by big steps */
      else {
        jmouse_set_position((click_last_x != jmouse_x(0)) ?
                            (click_last_x + (vp.x+vp.w/2))/2: jmouse_x(0),

                            (click_last_y != jmouse_y(0)) ?
                            (click_last_y + (vp.y+vp.h/2))/2: jmouse_y(0));
      }

      gfx::Point scroll = view->getViewScroll();
      setEditorScroll(scroll.x+click_last_x-jmouse_x(0),
                      scroll.y+click_last_y-jmouse_y(0), true);

      click_last_x = jmouse_x(0);
      click_last_y = jmouse_y(0);

      if (scroll_callback)
        (*scroll_callback)(false);
    }

    // If the cursor hasn't subpixel movement
    if (!editor_cursor_is_subpixel()) {
      // Check if the mouse change to other pixel of the sprite
      *update = ((prev_x != *x) || (prev_y != *y));
    }
    else {
      // Check if the mouse change to other pixel of the screen
      *update = ((prev_x != click_last_x) || (prev_y != click_last_y));
    }
  }

  /* click-and-click mode */
  if (click_mode == MODE_CLICKANDCLICK) {
    if (click_last_b) {
      click_prev_last_b = click_last_b;

      do {
        jmouse_poll();
        gui_feedback();
      } while (jmouse_b(0));

      jmouse_set_position(click_last_x, click_last_y);
      clear_keybuf();

      return false;
    }
    else {
      return true;
    }
  }
  /* click-and-release mode */
  else {
    return (click_last_b) ? true: false;
  }
}

int Editor::editor_click_cancel()
{
  return (click_start_b != click_prev_last_b);
}
