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

#include "CompareDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

SplitCompareView::SplitCompareView(QWidget *parent)
    : QWidget(parent), m_split(0.5) {
    setMinimumSize(320, 240);
    setCursor(Qt::SplitHCursor);
    setMouseTracking(false);
}

void SplitCompareView::setImages(const QImage &left, const QImage &right) {
    m_left = left;
    m_right = right;
    update();
}

void SplitCompareView::setLabels(const QString &left, const QString &right) {
    m_leftLabel = left;
    m_rightLabel = right;
    update();
}

QRect SplitCompareView::fitRect(const QImage &image) const {
    if (image.isNull()) return QRect();
    QSize size = image.size();
    size.scale(this->size(), Qt::KeepAspectRatio);
    QRect r(QPoint(0, 0), size);
    r.moveCenter(rect().center());
    return r;
}

QRect SplitCompareView::imageRect() const {
    // each image keeps its own aspect ratio; the split spans both
    return fitRect(m_left).united(fitRect(m_right));
}

void SplitCompareView::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), palette().window());
    const QRect r = imageRect();
    if (r.isEmpty()) return;
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    const int x = r.left() + qRound(m_split * r.width());
    p.save();
    p.setClipRect(QRect(r.left(), r.top(), x - r.left(), r.height()));
    if (!m_left.isNull()) p.drawImage(fitRect(m_left), m_left);
    p.restore();
    p.save();
    p.setClipRect(QRect(x, r.top(), r.right() - x + 1, r.height()));
    if (!m_right.isNull()) p.drawImage(fitRect(m_right), m_right);
    p.restore();

    // name of each side in its top corner
    auto tag = [&](const QString &text, bool right) {
        if (text.isEmpty()) return;
        const QFontMetrics fm(font());
        QRect box(0, 0, fm.horizontalAdvance(text) + 12, fm.height() + 6);
        box.moveTop(r.top() + 8);
        if (right)
            box.moveRight(r.right() - 8);
        else
            box.moveLeft(r.left() + 8);
        p.fillRect(box, QColor(0, 0, 0, 150));
        p.setPen(Qt::white);
        p.drawText(box, Qt::AlignCenter, text);
    };
    tag(m_leftLabel, false);
    tag(m_rightLabel, true);

    // the split line with a handle in the middle
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(Qt::white, 2));
    p.drawLine(x, r.top(), x, r.bottom());
    p.setBrush(palette().highlight());
    p.drawEllipse(QPoint(x, r.center().y()), 9, 9);
}

void SplitCompareView::moveSplit(int x) {
    const QRect r = imageRect();
    if (r.width() <= 0) return;
    m_split = qBound(0.0, double(x - r.left()) / r.width(), 1.0);
    update();
}

void SplitCompareView::mousePressEvent(QMouseEvent *event) {
    moveSplit(event->pos().x());
}

void SplitCompareView::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) moveSplit(event->pos().x());
}

CompareDialog::CompareDialog(const QList<QPair<QString, QImage>> &images,
                             int left, int right, const QString &note,
                             QWidget *parent)
    : QDialog(parent), m_images(images) {
    setWindowTitle(tr("Compare images"));
    setAttribute(Qt::WA_DeleteOnClose);

    m_leftBox = new QComboBox(this);
    m_rightBox = new QComboBox(this);
    // long names would make the window as wide as the longest one
    for (const auto &image : m_images) {
        QString shown = image.first;
        if (shown.size() > 20) shown = shown.left(19) + QChar(0x2026);
        for (QComboBox *box : {m_leftBox, m_rightBox}) {
            box->addItem(shown);
            box->setItemData(box->count() - 1, image.first, Qt::ToolTipRole);
        }
    }
    for (QComboBox *box : {m_leftBox, m_rightBox}) {
        box->setSizeAdjustPolicy(
            QComboBox::AdjustToMinimumContentsLengthWithIcon);
        box->setMinimumContentsLength(20);
    }
    m_leftBox->setCurrentIndex(left);
    m_rightBox->setCurrentIndex(right);

    QPushButton *swap = new QPushButton(tr("Swap"), this);
    connect(swap, &QPushButton::clicked, this, [this]() {
        const int l = m_leftBox->currentIndex();
        m_leftBox->setCurrentIndex(m_rightBox->currentIndex());
        m_rightBox->setCurrentIndex(l);
    });

    QHBoxLayout *choose = new QHBoxLayout;
    choose->addWidget(new QLabel(tr("Left:"), this));
    choose->addWidget(m_leftBox, 1);
    choose->addWidget(swap);
    choose->addWidget(new QLabel(tr("Right:"), this));
    choose->addWidget(m_rightBox, 1);

    m_view = new SplitCompareView(this);
    QString hintText = tr("Drag the line to move between the two images.");
    if (!note.isEmpty()) hintText += QLatin1Char(' ') + note;
    QLabel *hint = new QLabel(hintText, this);
    hint->setWordWrap(true);
    hint->setEnabled(false);

    QDialogButtonBox *buttons =
        new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(choose);
    layout->addWidget(m_view, 1);
    layout->addWidget(hint);
    layout->addWidget(buttons);

    connect(m_leftBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CompareDialog::updateView);
    connect(m_rightBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CompareDialog::updateView);
    updateView();
    resize(1000, 700);
}

void CompareDialog::updateView() {
    const int l = m_leftBox->currentIndex();
    const int r = m_rightBox->currentIndex();
    if (l < 0 || r < 0) return;
    m_view->setImages(m_images[l].second, m_images[r].second);
    m_view->setLabels(m_images[l].first, m_images[r].first);
}
