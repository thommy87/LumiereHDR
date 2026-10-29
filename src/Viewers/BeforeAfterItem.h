/*
 * This file is a part of LumiereHDR package.
 * ----------------------------------------------------------------------
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * ----------------------------------------------------------------------
 */

#ifndef BEFOREAFTERITEM_H
#define BEFOREAFTERITEM_H

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QPixmap>

//! \brief Lies over the picture of a viewer: left of a line the picture
//! before (the unedited HDR), right of it the picture shown. The line can
//! be dragged; elsewhere the mouse still pans and selects.
class BeforeAfterItem : public QGraphicsItem {
    Q_DECLARE_TR_FUNCTIONS(BeforeAfterItem)
   public:
    BeforeAfterItem(const QPixmap &before, QGraphicsItem *parent);

    void setBefore(const QPixmap &before);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

   protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    QPainterPath shape() const override;

   private:
    QRectF pictureRect() const;
    qreal lineX() const;
    bool nearLine(const QPointF &pos, const QTransform &view) const;

    QPixmap m_before;
    qreal m_split = 0.5;  // share of the width that shows "before"
};

#endif
