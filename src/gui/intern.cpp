// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <vector>

#include "gui/frame.h"
#include "gui/manager.h"
#include "gui/theme.h"
#include "gui/widget.h"

static std::vector<JWidget>* widgets;

int _ji_widgets_init()
{
  widgets = new std::vector<JWidget>;
  return 0;
}

void _ji_widgets_exit()
{
  delete widgets;
}

JWidget _ji_get_widget_by_id(JID widget_id)
{
  ASSERT((widget_id >= 0) && (widget_id < widgets->size()));

  return (*widgets)[widget_id];
}

JWidget* _ji_get_widget_array(int* n)
{
  *n = widgets->size();
  return &widgets->front();
}

void _ji_add_widget(JWidget widget)
{
  JID widget_id;

  // first widget
  if (widgets->empty()) {
    widgets->resize(2);

    // id=0 no widget
    (*widgets)[0] = NULL;

    // id>0 all widgets
    (*widgets)[1] = widget;
    (*widgets)[1]->id = widget_id = 1;
  }
  else {
    // find a free slot
    for (widget_id=1; widget_id<widgets->size(); widget_id++) {
      // is it free?
      if ((*widgets)[widget_id] == NULL)
        break;
    }

    // we need space for other widget more?
    if (widget_id == widgets->size())
      widgets->resize(widgets->size()+1);

    (*widgets)[widget_id] = widget;
    (*widgets)[widget_id]->id = widget_id;
  }
}

void _ji_remove_widget(JWidget widget)
{
  ASSERT_VALID_WIDGET(widget);

  (*widgets)[widget->id] = NULL;
}

bool _ji_is_valid_widget(JWidget widget)
{
  return (widget &&
          widget->id >= 0 &&
          widget->id < widgets->size() &&
          (*widgets)[widget->id] &&
          (*widgets)[widget->id]->id == widget->id);
}

void _ji_set_font_of_all_widgets(FONT* f)
{
  size_t c;

  // first of all, we have to set the font to all the widgets
  for (c=0; c<widgets->size(); c++)
    if (_ji_is_valid_widget((*widgets)[c]))
      (*widgets)[c]->setFont(f);

}

void _ji_reinit_theme_in_all_widgets()
{
  size_t c;

  // Then we can reinitialize the theme of each widget
  for (c=0; c<widgets->size(); c++)
    if (_ji_is_valid_widget((*widgets)[c])) {
      (*widgets)[c]->setTheme(CurrentTheme::get());
      (*widgets)[c]->initTheme();
    }

  // Remap the windows
  for (c=0; c<widgets->size(); c++)
    if (_ji_is_valid_widget((*widgets)[c])) {
      if ((*widgets)[c]->type == JI_FRAME)
        static_cast<Frame*>((*widgets)[c])->remap_window();
    }

  // Redraw the whole screen
  gui::Manager::getDefault()->invalidate();
}
