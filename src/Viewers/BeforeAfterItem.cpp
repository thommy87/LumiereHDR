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

#include "BeforeAfterItem.h"

#include <QCursor>
#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPainter>

namespace {
// how close to the line, in screen pixels, the mouse can grab it
const qreal kGrab = 14.0;
}

BeforeAfterItem::BeforeAfterItem(const QPixmap &before, QGraphicsItem *parent)
    : QGraphicsItem(parent), m_before(before) {
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

void BeforeAfterItem::setBefore(const QPixmap &before) {
    m_before = before;
    update();
}

QRectF BeforeAfterItem::pictureRect() const {
    return parentItem() ? parentItem()->boundingRect() : QRectF();
}

QRectF BeforeAfterItem::boundingRect() const { return pictureRect(); }

qreal BeforeAfterItem::lineX() const {
    const QRectF r = pictureRect();
    return r.left() + m_split * r.width();
}

QPainterPath BeforeAfterItem::shape() const {
    // only the line takes the mouse; the rest goes on to the picture
    QPainterPath path;
    const QRectF r = pictureRect();
    qreal grab = kGrab;
    if (scene() && !scene()->views().isEmpty()) {
        const qreal scale =
            scene()->views().first()->transform().m11() *
            sceneTransform().m11();
        if (scale > 0) grab = kGrab / scale;
    }
    path.addRect(QRectF(lineX() - grab, r.top(), 2 * grab, r.height()));
    return path;
}

bool BeforeAfterItem::nearLine(const QPointF &pos, const QTransform &) const {
    return shape().contains(pos);
}

void BeforeAfterItem::paint(QPainter *painter,
                            const QStyleOptionGraphicsItem *, QWidget *) {
    const QRectF r = pictureRect();
    if (r.isEmpty()) return;
    const qreal x = lineX();
    if (!m_before.isNull()) {
        const QRectF target(r.left(), r.top(), x - r.left(), r.height());
        const QRectF source(0, 0, m_split * m_before.width(),
                            m_before.height());
        painter->drawPixmap(target, m_before, source);
    }

    // line, handle and the two words drawn at screen size
    const QTransform world = painter->worldTransform();
    const QPointF top = world.map(QPointF(x, r.top()));
    const QPointF bottom = world.map(QPointF(x, r.bottom()));
    const QPointF topLeft = world.map(r.topLeft());
    const QPointF topRight = world.map(r.topRight());
    painter->save();
    painter->resetTransform();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(QColor(236, 238, 241), 2));
    painter->drawLine(top, bottom);
    const QPointF centre(top.x(), (top.y() + bottom.y()) / 2);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(236, 238, 241));
    painter->drawEllipse(centre, 16, 16);
    painter->setPen(QPen(QColor(15, 16, 19), 1.6, Qt::SolidLine, Qt::RoundCap,
                         Qt::RoundJoin));
    // two small arrows: the line moves sideways
    painter->drawPolyline(QPolygonF({centre + QPointF(-3, -5),
                                     centre + QPointF(-8, 0),
                                     centre + QPointF(-3, 5)}));
    painter->drawPolyline(QPolygonF({centre + QPointF(3, -5),
                                     centre + QPointF(8, 0),
                                     centre + QPointF(3, 5)}));

    QFont font = painter->font();
    font.setPixelSize(12);
    painter->setFont(font);
    const QFontMetricsF fm(font);
    auto tag = [&](const QString &text, const QPointF &at, bool right) {
        const QSizeF size(fm.horizontalAdvance(text) + 16, fm.height() + 8);
        QRectF box(right ? at.x() - size.width() : at.x(), at.y(),
                   size.width(), size.height());
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(15, 16, 19, 204));
        painter->drawRoundedRect(box, 6, 6);
        painter->setPen(QColor(236, 238, 241));
        painter->drawText(box, Qt::AlignCenter, text);
    };
    tag(tr("Before"), topLeft + QPointF(12, 12), false);
    tag(tr("After"), topRight + QPointF(-12, 12), true);
    painter->restore();
}

void BeforeAfterItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    event->accept();
}

void BeforeAfterItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    const QRectF r = pictureRect();
    if (r.width() <= 0) return;
    prepareGeometryChange();
    m_split = qBound(0.0, (event->pos().x() - r.left()) / r.width(), 1.0);
    update();
}

void BeforeAfterItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    setCursor(nearLine(event->pos(), QTransform()) ? Qt::SizeHorCursor
                                                   : Qt::ArrowCursor);
}
