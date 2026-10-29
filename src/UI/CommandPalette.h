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

#ifndef COMMANDPALETTE_H
#define COMMANDPALETTE_H

#include <QDialog>
#include <QList>
#include <QPointer>

class QAction;
class QLineEdit;
class QListWidget;
class QMenu;

//! \brief Ctrl+K: finds every command of the menus by its name (and the
//! name of its menu) and runs the chosen one.
class CommandPalette : public QDialog {
    Q_OBJECT
   public:
    explicit CommandPalette(const QList<QMenu *> &menus,
                            QWidget *parent = nullptr);

   protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

   private:
    struct Entry {
        QPointer<QAction> action;
        QString text;
        QString path;
    };
    void collect(QMenu *menu, const QString &path);
    void filter(const QString &query);
    void runCurrent();

    QLineEdit *m_query;
    QListWidget *m_list;
    QList<Entry> m_entries;
};

#endif
