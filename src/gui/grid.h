// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_GRID_H_INCLUDED
#define GUI_GRID_H_INCLUDED

#include "base/compiler_specific.h"
#include "gui/widget.h"

#include <vector>

class Grid : public Widget
{
public:
  Grid(int columns, bool same_width_columns);
  ~Grid();

  void addChildInCell(Widget* child, int hspan, int vspan, int align);

protected:
  // Events
  bool onProcessMessage(Message* msg) OVERRIDE;
  void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
  void onPaint(PaintEvent& ev) OVERRIDE;

private:
  struct Cell {
    Cell* parent;
    Widget* child;
    int hspan;
    int vspan;
    int align;
    int w, h;

    Cell();
  };

  struct Strip {
    int size;
    int expand_count;
  };

  void sumStripSize(const std::vector<Strip>& strip, int& size);
  void setGridPosition(JRect rect);
  void calculateCellSize(int start, int span, const std::vector<Strip>& strip, int& size);
  void calculateSize();
  void calculateStripSize(std::vector<Strip>& colstrip,
                          std::vector<Strip>& rowstrip, int align);
  void expandStrip(std::vector<Strip>& colstrip,
                   std::vector<Strip>& rowstrip,
                   void (Grid::*incCol)(int, int));
  void distributeSize(JRect rect);
  void distributeStripSize(std::vector<Strip>& colstrip,
                           int rect_size, int border_size, bool same_width);
  bool putWidgetInCell(Widget* child, int hspan, int vspan, int align);
  void expandRows(int rows);
  void incColSize(int col, int size);
  void incRowSize(int row, int size);

  bool m_same_width_columns;
  std::vector<Strip> m_colstrip;
  std::vector<Strip> m_rowstrip;
  std::vector<std::vector<Cell*> > m_cells;
};

#endif
