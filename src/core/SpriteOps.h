#pragma once
#include "Sprite.h"
#include <QRect>
#include <QTransform>

namespace spx {

// All pure: take by value, return the new sprite. Out-of-range indices return the sprite unchanged.
Sprite addLayer(Sprite, int index);
Sprite removeLayer(Sprite, int index);
Sprite moveLayer(Sprite, int from, int to);
Sprite setLayerProps(Sprite, int index, QString name, bool visible, int opacity);
Sprite addFrame(Sprite, int index);
Sprite duplicateFrame(Sprite, int index);
Sprite removeFrame(Sprite, int index);
Sprite moveFrame(Sprite, int from, int to);
Sprite setFrameDuration(Sprite, int index, int ms);
Sprite addTag(Sprite, Tag);
Sprite removeTag(Sprite, int index);
Sprite setTag(Sprite, int index, Tag);
Sprite resizeCanvas(Sprite, QSize, Qt::Alignment anchor);
Sprite cropCanvas(Sprite, QRect);
Sprite flipSprite(Sprite, Qt::Orientation);
Sprite rotateSprite(Sprite, int degrees);
Sprite transformCel(Sprite, int layer, int frame, const QTransform&);

} // namespace spx
