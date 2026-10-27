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

#ifndef EXPORTQUEUEDIALOG_H
#define EXPORTQUEUEDIALOG_H

#include <QDialog>

class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

//! The export queue: every picture sent to "Fast export", whether it is
//! waiting, being made, done or failed. Jobs can be cancelled, failed ones
//! tried again, and finished pictures opened.
class ExportQueueDialog : public QDialog {
    Q_OBJECT

   public:
    enum State { Waiting, Running, Done, Failed, Canceled };

    explicit ExportQueueDialog(QWidget *parent = nullptr);

    //! a new job, waiting in line
    void addJob(int id, const QString &picture, const QString &look,
                const QString &format);
    //! jobs not finished yet
    int openJobs() const;

   public Q_SLOTS:
    void jobStarted(int id);
    //! \a result: 0 written, 1 failed, 2 cancelled
    void jobFinished(int id, const QString &file, int result);

   Q_SIGNALS:
    void cancelRequested(int id);
    void retryRequested(int id);

   private:
    QTreeWidgetItem *item(int id) const;
    void setState(QTreeWidgetItem *item, State state);
    void updateButtons();
    void openSelected();

    QTreeWidget *m_list;
    QPushButton *m_cancel;
    QPushButton *m_retry;
    QPushButton *m_open;
    QPushButton *m_clear;
};

#endif  // EXPORTQUEUEDIALOG_H
