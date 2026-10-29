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

#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QStringList>
#include <QWidget>

class QAction;
class QGridLayout;
class QLabel;
class QPushButton;

//! \brief The Start workspace: the ways to begin as large cards, a place to
//! drop files, the recently used files with a preview, and a few links.
//! (Not "StartPage": that name is taken by a Windows GDI function.)
class HomePage : public QWidget {
    Q_OBJECT
   public:
    explicit HomePage(QWidget *parent = nullptr);

    //! the cards run \a newHdr, \a open and \a batch
    void setCardActions(QAction *newHdr, QAction *open, QAction *batch);
    //! the links under the recent files
    void setLinkActions(const QList<QAction *> &links);
    void setRecentFiles(const QStringList &files);

   signals:
    void recentFileRequested(const QString &file);

   protected:
    void changeEvent(QEvent *event) override;

   private:
    struct Card {
        QPushButton *button;
        QLabel *icon;
        QLabel *title;
        QLabel *text;
        QLabel *shortcut;
        QAction *action;
        QString iconName;
    };
    Card makeCard();
    void retranslate();
    void refreshIcons();
    void rebuildRecent();

    QLabel *m_title;
    QLabel *m_subtitle;
    QLabel *m_drop;
    QLabel *m_recentLabel;
    QWidget *m_recentBox;
    QGridLayout *m_recentGrid;
    QWidget *m_links;
    Card m_cards[3];
    QStringList m_recent;
};

#endif
