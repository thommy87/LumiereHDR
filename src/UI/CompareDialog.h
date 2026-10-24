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

#ifndef COMPAREDIALOG_H
#define COMPAREDIALOG_H

#include <QDialog>
#include <QImage>
#include <QList>
#include <QPair>
#include <QWidget>

class QComboBox;

//! \brief Shows two images on top of each other, split by a line the user
//! drags: the left part shows the first image, the right part the second.
class SplitCompareView : public QWidget {
    Q_OBJECT
   public:
    explicit SplitCompareView(QWidget *parent = nullptr);
    void setImages(const QImage &left, const QImage &right);
    void setLabels(const QString &left, const QString &right);

   protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

   private:
    QRect fitRect(const QImage &image) const;
    QRect imageRect() const;
    void moveSplit(int x);

    QImage m_left;
    QImage m_right;
    QString m_leftLabel;
    QString m_rightLabel;
    double m_split;  // 0..1 across the image
};

//! \brief Before/after comparison of two open images.
class CompareDialog : public QDialog {
    Q_OBJECT
   public:
    //! \a images: tab title and image of every open tab, in tab order
    CompareDialog(const QList<QPair<QString, QImage>> &images, int left,
                  int right, const QString &note = QString(),
                  QWidget *parent = nullptr);

   private:
    void updateView();

    QList<QPair<QString, QImage>> m_images;
    QComboBox *m_leftBox;
    QComboBox *m_rightBox;
    SplitCompareView *m_view;
};

#endif
