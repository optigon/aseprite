// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "base/compiler_specific.h"
#include "gfx/size.h"
#include "gui/gui.h"
#include "gui/preferred_size_event.h"

#include <allegro.h>

using namespace gfx;

class ComboBoxButton : public Button
{
public:
  ComboBoxButton() : Button("") { }

  void onPaint(PaintEvent& ev) OVERRIDE
  {
    getTheme()->paintComboBoxButton(ev);
  }
};

struct ComboBox::Item
{
  std::string text;
  void* data;

  Item() : data(NULL) { }
};

#define IS_VALID_ITEM(combobox, index)                          \
  (index >= 0 && index < combobox->getItemCount())

static bool combobox_entry_msg_proc(JWidget widget, Message* msg);
static bool combobox_listbox_msg_proc(JWidget widget, Message* msg);

ComboBox::ComboBox()
  : Widget(JI_COMBOBOX)
{
  m_entry = new Entry(256, "");
  m_button = new ComboBoxButton();
  m_window = NULL;
  m_selected = 0;
  m_editable = false;
  m_clickopen = true;
  m_casesensitive = true;

  m_entry->user_data[0] = this;
  m_button->user_data[0] = this;

  // TODO this separation should be from the Theme*
  this->child_spacing = 0;

  this->setFocusStop(true);
  jwidget_add_hook(m_entry, JI_WIDGET, combobox_entry_msg_proc, NULL);

  m_entry->setExpansive(true);

  // When the "m_button" is clicked ("Click" signal) call onButtonClick() method
  m_button->Click.connect(&ComboBox::onButtonClick, this);

  addChild(m_entry);
  addChild(m_button);

  setEditable(m_editable);

  initTheme();
}

ComboBox::~ComboBox()
{
  removeAllItems();
}

void ComboBox::setEditable(bool state)
{
  m_editable = state;

  if (state) {
    m_entry->setReadOnly(false);
    m_entry->showCaret();
  }
  else {
    m_entry->setReadOnly(true);
    m_entry->hideCaret();
  }
}

void ComboBox::setClickOpen(bool state)
{
  m_clickopen = state;
}

void ComboBox::setCaseSensitive(bool state)
{
  m_casesensitive = state;
}

bool ComboBox::isEditable()
{
  return m_editable;
}

bool ComboBox::isClickOpen()
{
  return m_clickopen;
}

bool ComboBox::isCaseSensitive()
{
  return m_casesensitive;
}

int ComboBox::addItem(const std::string& text)
{
  bool sel_first = m_items.empty();
  Item* item = new Item();
  item->text = text;

  m_items.push_back(item);

  if (sel_first)
    setSelectedItem(0);

  return m_items.size()-1;
}

void ComboBox::insertItem(int itemIndex, const std::string& text)
{
  bool sel_first = m_items.empty();
  Item* item = new Item();
  item->text = text;

  m_items.insert(m_items.begin() + itemIndex, item);

  if (sel_first)
    setSelectedItem(0);
}

void ComboBox::removeItem(int itemIndex)
{
  ASSERT(itemIndex >= 0 && (size_t)itemIndex < m_items.size());

  Item* item = m_items[itemIndex];

  m_items.erase(m_items.begin() + itemIndex);
  delete item;
}

void ComboBox::removeAllItems()
{
  std::vector<Item*>::iterator it, end = m_items.end();
  for (it = m_items.begin(); it != end; ++it)
    delete *it;

  m_items.clear();
}

int ComboBox::getItemCount()
{
  return m_items.size();
}

std::string ComboBox::getItemText(int itemIndex)
{
  if (itemIndex >= 0 && (size_t)itemIndex < m_items.size()) {
    Item* item = m_items[itemIndex];
    return item->text;
  }
  else
    return "";
}

void ComboBox::setItemText(int itemIndex, const std::string& text)
{
  ASSERT(itemIndex >= 0 && (size_t)itemIndex < m_items.size());

  Item* item = m_items[itemIndex];
  item->text = text;
}

int ComboBox::findItemIndex(const std::string& text)
{
  int itemIndex = 0;

  std::vector<Item*>::iterator it, end = m_items.end();
  for (it = m_items.begin(); it != end; ++it) {
    Item* item = *it;

    if ((m_casesensitive && ustrcmp(item->text.c_str(), text.c_str()) == 0) ||
        (!m_casesensitive && ustricmp(item->text.c_str(), text.c_str()) == 0)) {
      return itemIndex;
    }

    itemIndex++;
  }

  return -1;
}

int ComboBox::getSelectedItem()
{
  return !m_items.empty() ? m_selected: -1;
}

void ComboBox::setSelectedItem(int itemIndex)
{
  if (itemIndex >= 0 && (size_t)itemIndex < m_items.size()) {
    m_selected = itemIndex;

    std::vector<Item*>::iterator it = m_items.begin() + itemIndex;
    Item* item = *it;
    m_entry->setText(item->text.c_str());
  }
}

void* ComboBox::getItemData(int itemIndex)
{
  if (itemIndex >= 0 && (size_t)itemIndex < m_items.size()) {
    Item* item = m_items[itemIndex];
    return item->data;
  }
  else
    return NULL;
}

void ComboBox::setItemData(int itemIndex, void* data)
{
  ASSERT(itemIndex >= 0 && (size_t)itemIndex < m_items.size());

  Item* item = m_items[itemIndex];
  item->data = data;
}

Entry* ComboBox::getEntryWidget()
{
  return m_entry;
}

Button* ComboBox::getButtonWidget()
{
  return m_button;
}

bool ComboBox::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_CLOSE:
      closeListBox();
      break;

    case JM_SETPOS: {
      JRect cbox = jrect_new_copy(&msg->setpos.rect);
      jrect_copy(this->rc, cbox);

      // Button
      Size buttonSize = m_button->getPreferredSize();
      cbox->x1 = msg->setpos.rect.x2 - buttonSize.w;
      jwidget_set_rect(m_button, cbox);

      // Entry
      cbox->x2 = cbox->x1;
      cbox->x1 = msg->setpos.rect.x1;
      jwidget_set_rect(m_entry, cbox);

      jrect_free(cbox);
      return true;
    }

    case JM_WINMOVE:
      if (m_window) {
        JRect rc = getListBoxPos();
        m_window->move_window(rc);
        jrect_free(rc);
      }
      break;

    case JM_BUTTONPRESSED:
      if (m_window != NULL) {
        if (!View::getView(m_listbox)->hasMouse()) {
          closeListBox();
          return true;
        }
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

void ComboBox::onPreferredSize(PreferredSizeEvent& ev)
{
  Size reqSize(0, 0);
  Size entrySize = m_entry->getPreferredSize();

  // Get the text-length of every item and put in 'w' the maximum value
  std::vector<Item*>::iterator it, end = m_items.end();
  for (it = m_items.begin(); it != end; ++it) {
    int item_w =
      2*jguiscale()+
      text_length(this->getFont(), (*it)->text.c_str())+
      10*jguiscale();

    reqSize.w = MAX(reqSize.w, item_w);
  }

  reqSize.w += entrySize.w;
  reqSize.h += entrySize.h;

  Size buttonSize = m_button->getPreferredSize();
  reqSize.w += buttonSize.w;
  reqSize.h = MAX(reqSize.h, buttonSize.h);
  ev.setPreferredSize(reqSize);
}

static bool combobox_entry_msg_proc(JWidget widget, Message* msg)
{
  ComboBox* combobox = reinterpret_cast<ComboBox*>(widget->user_data[0]);

  switch (msg->type) {

    case JM_KEYPRESSED:
      if (widget->hasFocus()) {
        if (!combobox->isEditable()) {
          if (msg->key.scancode == KEY_SPACE ||
              msg->key.scancode == KEY_ENTER ||
              msg->key.scancode == KEY_ENTER_PAD) {
            combobox->switchListBox();
            return true;
          }
        }
        else {
          if (msg->key.scancode == KEY_ENTER ||
              msg->key.scancode == KEY_ENTER_PAD) {
            combobox->switchListBox();
            return true;
          }
        }
      }
      break;

    case JM_BUTTONPRESSED:
      if (combobox->isClickOpen()) {
        combobox->switchListBox();
      }

      if (combobox->isEditable()) {
        widget->getManager()->setFocus(widget);
      }
      else
        return true;
      break;

    case JM_DRAW:
      widget->getTheme()->draw_combobox_entry(static_cast<Entry*>(widget),
                                              &msg->draw.rect);
      return true;

  }

  return false;
}

static bool combobox_listbox_msg_proc(JWidget widget, Message* msg)
{
  ComboBox* combobox = reinterpret_cast<ComboBox*>(widget->user_data[0]);

  switch (msg->type) {

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_LISTBOX_CHANGE) {
        int index = static_cast<ListBox*>(widget)->getSelectedIndex();

        if (IS_VALID_ITEM(combobox, index))
          combobox->setSelectedItem(index);
      }
      break;

    case JM_BUTTONRELEASED:
      {
        int index = combobox->getSelectedItem();

        combobox->closeListBox();

        if (IS_VALID_ITEM(combobox, index)) {
          combobox->Change();
          jwidget_emit_signal(combobox, JI_SIGNAL_COMBOBOX_SELECT);
        }
      }
      return true;

/*     case JM_IDLE: { */
/*       /\* if the user clicks outside the listbox *\/ */
/*       if (!jmouse_b(1) && jmouse_b(0) && !jwidget_has_mouse(widget)) { */
/*      ComboBox *combobox = jwidget_get_data(combo_widget, JI_COMBOBOX); */

/*      if (combobox->entry && !jwidget_has_mouse(combobox->entry) && */
/*          combobox->button && !jwidget_has_mouse(combobox->button) && */
/*          combobox->window && !jwidget_has_mouse(combobox->window)) { */
/*        combobox_close_window(combo_widget); */
/*        return true; */
/*      } */
/*       } */
/*       break; */
/*     } */

    case JM_KEYPRESSED:
      if (widget->hasFocus()) {
        if (msg->key.scancode == KEY_SPACE ||
            msg->key.scancode == KEY_ENTER ||
            msg->key.scancode == KEY_ENTER_PAD) {
          combobox->closeListBox();
          return true;
        }
      }
      break;
  }

  return false;
}

// When the mouse is clicked we switch the visibility-status of the list-box
void ComboBox::onButtonClick(Event& ev)
{
  switchListBox();
}

void ComboBox::openListBox()
{
  if (!m_window) {
    m_window = new Frame(false, NULL);
    View* view = new View();
    m_listbox = new ListBox();

    m_listbox->user_data[0] = this;
    jwidget_add_hook(m_listbox, JI_WIDGET,
                     combobox_listbox_msg_proc, NULL);

    std::vector<Item*>::iterator it, end = m_items.end();
    for (it = m_items.begin(); it != end; ++it) {
      Item* item = *it;
      m_listbox->addChild(new ListBox::Item(item->text.c_str()));
    }

    m_window->set_ontop(true);
    jwidget_noborders(m_window);

    Widget* viewport = view->getViewport();
    int size = getItemCount();
    jwidget_set_min_size
      (viewport,
       m_button->rc->x2 - m_entry->rc->x1 - view->border_width.l - view->border_width.r,
       +viewport->border_width.t
       +(2*jguiscale()+jwidget_get_text_height(m_listbox))*MID(1, size, 16)+
       +viewport->border_width.b);

    m_window->addChild(view);
    view->attachToView(m_listbox);

    jwidget_signal_off(m_listbox);
    m_listbox->selectIndex(m_selected);
    jwidget_signal_on(m_listbox);

    m_window->remap_window();

    JRect rc = getListBoxPos();
    m_window->position_window(rc->x1, rc->y1);
    jrect_free(rc);

    getManager()->addMessageFilter(JM_BUTTONPRESSED, this);

    m_window->open_window_bg();
    getManager()->setFocus(m_listbox);
  }
}

void ComboBox::closeListBox()
{
  if (m_window) {
    m_window->closeWindow(this);
    delete m_window;            // window, frame
    m_window = NULL;

    getManager()->removeMessageFilter(JM_BUTTONPRESSED, this);
    getManager()->setFocus(m_entry);
  }
}

void ComboBox::switchListBox()
{
  if (!m_window)
    openListBox();
  else
    closeListBox();
}

JRect ComboBox::getListBoxPos()
{
  JRect rc = jrect_new(m_entry->rc->x1,
                       m_entry->rc->y2,
                       m_button->rc->x2,
                       m_entry->rc->y2+jrect_h(m_window->rc));
  if (rc->y2 > JI_SCREEN_H)
    jrect_displace(rc, 0, -(jrect_h(rc)+jrect_h(m_entry->rc)));
  return rc;
}
