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

#ifndef TOPBAR_H
#define TOPBAR_H

#include <QWidget>

class QButtonGroup;
class QLabel;
class QMenu;
class QToolButton;

//! \brief The bar across the top of the main window: the four workspaces
//! in the order of the work (Start, Merge HDR, Develop, Export) in the
//! middle, search, batch, settings and the full menu on the right.
class TopBar : public QWidget {
    Q_OBJECT
   public:
    enum Mode { Start = 0, Merge = 1, Develop = 2, Export = 3 };

    explicit TopBar(QWidget *parent = nullptr);

    void setMode(Mode mode);
    Mode mode() const;
    void setModeEnabled(Mode mode, bool enabled);
    //! a page that is not one of the steps: batch or the preferences
    enum Utility { NoUtility, Batch, Settings };
    void showUtility(Utility utility);
    //! the menus of the menu bar, offered under the menu button
    void setMenus(const QList<QMenu *> &menus);

   signals:
    void modeRequested(TopBar::Mode mode);
    void searchRequested();
    void batchRequested();
    void settingsRequested();

   protected:
    void changeEvent(QEvent *event) override;

   private:
    void retranslate();
    void refreshIcons();

    QLabel *m_logo;
    QLabel *m_name;
    QButtonGroup *m_modes;
    QToolButton *m_modeButtons[4];
    QToolButton *m_search;
    QToolButton *m_batch;
    QToolButton *m_settings;
    QToolButton *m_menu;
    QMenu *m_menuPopup;
};

#endif
