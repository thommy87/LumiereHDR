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

#ifndef FILMSTRIP_H
#define FILMSTRIP_H

#include <QWidget>

class QListWidget;
class QPushButton;
class QTabWidget;
class QTimer;

//! \brief The open images as a row of small pictures under the image: the
//! HDR first, then the results. Takes the place of the tabs, whose widget
//! still holds the viewers.
class FilmStrip : public QWidget {
    Q_OBJECT
   public:
    FilmStrip(QTabWidget *tabs, QWidget *parent = nullptr);

    //! a button at the right end, e.g. to compare
    void addButton(QAction *action);

   public Q_SLOTS:
    //! picks up new, closed, renamed and changed images
    void refresh();

   signals:
    void closeRequested(int index);

   protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

   private:
    QTabWidget *m_tabs;
    QListWidget *m_list;
    QTimer *m_timer;
    QStringList m_signature;
    QWidget *m_buttons;
};

#endif
