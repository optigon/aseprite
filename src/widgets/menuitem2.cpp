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

#include "widgets/menuitem2.h"

#include "commands/command.h"
#include "commands/params.h"
#include "gui/hook.h"
#include "gui/menu.h"
#include "gui/message.h"
#include "gui/widget.h"
#include "modules/gui.h"
#include "ui_context.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

MenuItem2::MenuItem2(const char* text, Command* command, Params* params)
 : MenuItem(text)
 , m_command(command)
 , m_params(params ? params->clone(): NULL)
{
}

MenuItem2::~MenuItem2()
{
  delete m_params;
}

bool MenuItem2::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_OPEN: {
      UIContext* context = UIContext::instance();

      if (m_command) {
        if (m_params)
          m_command->loadParams(m_params);

        setEnabled(m_command->isEnabled(context));
        setSelected(m_command->isChecked(context));
      }
      break;
    }

    case JM_CLOSE:
      // disable the menu (the keyboard shortcuts are processed by "manager_msg_proc")
      setEnabled(false);
      break;

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_MENUITEM_SELECT) {
        UIContext* context = UIContext::instance();

        if (m_command) {
          if (m_params)
            m_command->loadParams(m_params);

          if (m_command->isEnabled(context)) {
            context->executeCommand(m_command);
            return true;
          }
        }
      }
      break;

    default:
      if (msg->type == jm_open_menuitem()) {
        // Update the context flags after opening the menuitem's
        // submenu to update the "enabled" flag of each command
        // correctly.
        Context* context = UIContext::instance();
        context->updateFlags();
      }
      break;
  }

  return MenuItem::onProcessMessage(msg);
}
