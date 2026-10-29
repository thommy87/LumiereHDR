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

#ifndef WELCOMEPANEL_H
#define WELCOMEPANEL_H

#include <QWidget>

class QAction;
class QLabel;
class QPushButton;

//! \brief Shown in the image area while no image is open: the two ways to
//! start, as large buttons, and a hint that photos can be dropped here.
class WelcomePanel : public QWidget {
    Q_OBJECT
   public:
    explicit WelcomePanel(QWidget *parent = nullptr);

    //! The buttons run \a newHdr and \a open.
    void setActions(QAction *newHdr, QAction *open);

   protected:
    void changeEvent(QEvent *event) override;

   private:
    void retranslate();

    QLabel *m_title;
    QLabel *m_hint;
    QPushButton *m_newButton;
    QPushButton *m_openButton;
};

#endif
