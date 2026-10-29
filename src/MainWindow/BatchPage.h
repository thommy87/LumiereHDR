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

#ifndef BATCHPAGE_H
#define BATCHPAGE_H

#include <QPointer>
#include <QSqlDatabase>
#include <QWidget>

class BatchHDRDialog;
class BatchTMDialog;
class QButtonGroup;
class QStackedWidget;

//! \brief Batch processing as a page of the window: HDRs from brackets
//! and looks on HDRs, behind a two-part switch. When the HDRs are made they
//! can go straight on to the looks.
class BatchPage : public QWidget {
    Q_OBJECT
   public:
    enum Kind { Hdr = 0, Looks = 1 };

    BatchPage(const QSqlDatabase &db, QWidget *parent = nullptr);

    void showKind(Kind kind);

   Q_SIGNALS:
    //! the user closed it, or the work is done
    void closed();

   private:
    void makeHdrDialog();
    void makeLooksDialog();
    template <class Dialog>
    void embed(Dialog *dialog, int index);

    QSqlDatabase m_db;
    QButtonGroup *m_switch;
    QStackedWidget *m_pages;
    QPointer<BatchHDRDialog> m_hdr;
    QPointer<BatchTMDialog> m_looks;
};

#endif  // BATCHPAGE_H
