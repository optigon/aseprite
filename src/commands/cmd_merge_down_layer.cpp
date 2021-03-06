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

#include "app.h"
#include "commands/command.h"
#include "document_wrappers.h"
#include "gui/gui.h"
#include "modules/gui.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "undo/undo_history.h"
#include "undoers/add_cel.h"
#include "undoers/add_image.h"
#include "undoers/close_group.h"
#include "undoers/open_group.h"
#include "undoers/remove_layer.h"
#include "undoers/replace_image.h"
#include "undoers/set_cel_position.h"
#include "undoers/set_current_layer.h"

//////////////////////////////////////////////////////////////////////
// merge_down_layer

class MergeDownLayerCommand : public Command
{
public:
  MergeDownLayerCommand();
  Command* clone() { return new MergeDownLayerCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

MergeDownLayerCommand::MergeDownLayerCommand()
  : Command("MergeDownLayer",
            "Merge Down Layer",
            CmdRecordableFlag)
{
}

bool MergeDownLayerCommand::onEnabled(Context* context)
{
  ActiveDocumentWriter document(context);
  Sprite* sprite(document ? document->getSprite(): 0);
  if (!sprite)
    return false;

  Layer* src_layer = sprite->getCurrentLayer();
  if (!src_layer || !src_layer->is_image())
    return false;

  Layer* dst_layer = sprite->getCurrentLayer()->get_prev();
  if (!dst_layer || !dst_layer->is_image())
    return false;

  return true;
}

void MergeDownLayerCommand::onExecute(Context* context)
{
  ActiveDocumentWriter document(context);
  Sprite* sprite(document->getSprite());
  undo::UndoHistory* undo(document->getUndoHistory());
  Layer *src_layer, *dst_layer;
  Cel *src_cel, *dst_cel;
  Image *src_image, *dst_image;
  int frpos, index;

  src_layer = sprite->getCurrentLayer();
  dst_layer = sprite->getCurrentLayer()->get_prev();

  if (undo->isEnabled()) {
    undo->setLabel("Merge Down Layer");
    undo->setModification(undo::ModifyDocument);
    undo->pushUndoer(new undoers::OpenGroup());
  }

  for (frpos=0; frpos<sprite->getTotalFrames(); ++frpos) {
    // Get frames
    src_cel = static_cast<LayerImage*>(src_layer)->getCel(frpos);
    dst_cel = static_cast<LayerImage*>(dst_layer)->getCel(frpos);

    // Get images
    if (src_cel != NULL)
      src_image = sprite->getStock()->getImage(src_cel->getImage());
    else
      src_image = NULL;

    if (dst_cel != NULL)
      dst_image = sprite->getStock()->getImage(dst_cel->getImage());
    else
      dst_image = NULL;

    // With source image?
    if (src_image != NULL) {
      // No destination image
      if (dst_image == NULL) {  // Only a transparent layer can have a null cel
        // Copy this cel to the destination layer...

        // Creating a copy of the image
        dst_image = Image::createCopy(src_image);

        // Adding it in the stock of images
        index = sprite->getStock()->addImage(dst_image);
        if (undo->isEnabled())
          undo->pushUndoer(new undoers::AddImage(
              undo->getObjects(), sprite->getStock(), index));

        // Creating a copy of the cell
        dst_cel = new Cel(frpos, index);
        dst_cel->setPosition(src_cel->getX(), src_cel->getY());
        dst_cel->setOpacity(src_cel->getOpacity());

        if (undo->isEnabled())
          undo->pushUndoer(new undoers::AddCel(undo->getObjects(), dst_layer, dst_cel));

        static_cast<LayerImage*>(dst_layer)->addCel(dst_cel);
      }
      /* with destination */
      else {
        int x1, y1, x2, y2, bgcolor;
        Image *new_image;

        /* merge down in the background layer */
        if (dst_layer->is_background()) {
          x1 = 0;
          y1 = 0;
          x2 = sprite->getWidth();
          y2 = sprite->getHeight();
          bgcolor = app_get_color_to_clear_layer(dst_layer);
        }
        /* merge down in a transparent layer */
        else {
          x1 = MIN(src_cel->getX(), dst_cel->getX());
          y1 = MIN(src_cel->getY(), dst_cel->getY());
          x2 = MAX(src_cel->getX()+src_image->w-1, dst_cel->getX()+dst_image->w-1);
          y2 = MAX(src_cel->getY()+src_image->h-1, dst_cel->getY()+dst_image->h-1);
          bgcolor = 0;
        }

        new_image = image_crop(dst_image,
                               x1-dst_cel->getX(),
                               y1-dst_cel->getY(),
                               x2-x1+1, y2-y1+1, bgcolor);

        /* merge src_image in new_image */
        image_merge(new_image, src_image,
                    src_cel->getX()-x1,
                    src_cel->getY()-y1,
                    src_cel->getOpacity(),
                    static_cast<LayerImage*>(src_layer)->getBlendMode());

        if (undo->isEnabled())
          undo->pushUndoer(new undoers::SetCelPosition(undo->getObjects(), dst_cel));

        dst_cel->setPosition(x1, y1);

        if (undo->isEnabled())
          undo->pushUndoer(new undoers::ReplaceImage(undo->getObjects(),
              sprite->getStock(), dst_cel->getImage()));

        sprite->getStock()->replaceImage(dst_cel->getImage(), new_image);

        image_free(dst_image);
      }
    }
  }

  if (undo->isEnabled()) {
    undo->pushUndoer(new undoers::SetCurrentLayer(undo->getObjects(), sprite));
    undo->pushUndoer(new undoers::RemoveLayer(undo->getObjects(), src_layer));
    undo->pushUndoer(new undoers::CloseGroup());
  }

  sprite->setCurrentLayer(dst_layer);
  src_layer->get_parent()->remove_layer(src_layer);

  delete src_layer;

  update_screen_for_document(document);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createMergeDownLayerCommand()
{
  return new MergeDownLayerCommand;
}
