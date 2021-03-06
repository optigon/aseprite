// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_SEPARATOR_H_INCLUDED
#define GUI_SEPARATOR_H_INCLUDED

#include "base/compiler_specific.h"
#include "gui/widget.h"

class Separator : public Widget
{
public:
  Separator(const char* text, int align);

protected:
  bool onProcessMessage(Message* msg) OVERRIDE;
  void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
};

#endif
