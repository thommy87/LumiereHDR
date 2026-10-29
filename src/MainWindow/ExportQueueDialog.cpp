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

#include "MainWindow/ExportQueueDialog.h"

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

#include "Common/LuminanceOptions.h"

namespace {
enum Column { Picture, Look, Format, Status };
const int IdRole = Qt::UserRole;
const int StateRole = Qt::UserRole + 1;
const int FileRole = Qt::UserRole + 2;
}  // namespace

ExportQueueDialog::ExportQueueDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("Export queue"));
    resize(720, 360);

    QLabel *intro = new QLabel(
        tr("Pictures sent to \"Fast export\" are made one after the other in "
           "the background and saved to the export folder."),
        this);
    intro->setWordWrap(true);
    intro->setObjectName(QStringLiteral("QueueIntro"));

    m_list = new QTreeWidget(this);
    m_list->setColumnCount(4);
    m_list->setHeaderLabels(
        {tr("Picture"), tr("Look"), tr("Format"), tr("Status")});
    m_list->setRootIsDecorated(false);
    m_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_list->setAlternatingRowColors(true);
    m_list->header()->setStretchLastSection(true);
    m_list->header()->setSectionResizeMode(Picture, QHeaderView::Stretch);

    m_cancel = new QPushButton(tr("&Cancel"), this);
    m_cancel->setToolTip(tr("Stops the selected pictures that are waiting "
                            "or being made"));
    m_retry = new QPushButton(tr("&Try again"), this);
    m_retry->setToolTip(tr("Puts the selected failed or cancelled pictures "
                           "back in line"));
    m_open = new QPushButton(tr("&Open folder"), this);
    m_open->setToolTip(tr("Opens the folder of the selected picture, or the "
                          "export folder"));
    m_clear = new QPushButton(tr("&Remove finished"), this);
    m_clear->setToolTip(tr("Removes done and cancelled pictures from the "
                           "list; the files stay"));

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addWidget(m_cancel);
    buttons->addWidget(m_retry);
    buttons->addStretch();
    buttons->addWidget(m_open);
    buttons->addWidget(m_clear);

    QDialogButtonBox *close = new QDialogButtonBox(QDialogButtonBox::Close, this);
    close->setObjectName(QStringLiteral("QueueClose"));
    connect(close, &QDialogButtonBox::rejected, this, &QDialog::hide);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(intro);
    layout->addWidget(m_list);
    layout->addLayout(buttons);
    layout->addWidget(close);

    connect(m_list, &QTreeWidget::itemSelectionChanged, this,
            &ExportQueueDialog::updateButtons);
    connect(m_list, &QTreeWidget::itemDoubleClicked, this,
            [](QTreeWidgetItem *item) {
                const QString file = item->data(Picture, FileRole).toString();
                if (item->data(Picture, StateRole).toInt() == Done &&
                    !file.isEmpty())
                    QDesktopServices::openUrl(QUrl::fromLocalFile(file));
            });
    connect(m_cancel, &QPushButton::clicked, this, [this]() {
        for (QTreeWidgetItem *item : m_list->selectedItems()) {
            const int state = item->data(Picture, StateRole).toInt();
            if (state == Waiting || state == Running)
                emit cancelRequested(item->data(Picture, IdRole).toInt());
        }
    });
    connect(m_retry, &QPushButton::clicked, this, [this]() {
        for (QTreeWidgetItem *item : m_list->selectedItems()) {
            const int state = item->data(Picture, StateRole).toInt();
            if (state == Failed || state == Canceled) {
                setState(item, Waiting);
                emit retryRequested(item->data(Picture, IdRole).toInt());
            }
        }
        updateButtons();
    });
    connect(m_open, &QPushButton::clicked, this,
            &ExportQueueDialog::openSelected);
    connect(m_clear, &QPushButton::clicked, this, [this]() {
        for (int i = m_list->topLevelItemCount() - 1; i >= 0; --i) {
            const int state =
                m_list->topLevelItem(i)->data(Picture, StateRole).toInt();
            if (state == Done || state == Canceled)
                delete m_list->takeTopLevelItem(i);
        }
        updateButtons();
    });
    updateButtons();
}

void ExportQueueDialog::addJob(int id, const QString &picture,
                               const QString &look, const QString &format) {
    QTreeWidgetItem *item = new QTreeWidgetItem(m_list);
    item->setText(Picture, picture);
    item->setText(Look, look);
    item->setText(Format, format);
    item->setData(Picture, IdRole, id);
    setState(item, Waiting);
    updateButtons();
}

int ExportQueueDialog::openJobs() const {
    int open = 0;
    for (int i = 0; i < m_list->topLevelItemCount(); ++i) {
        const int state =
            m_list->topLevelItem(i)->data(Picture, StateRole).toInt();
        if (state == Waiting || state == Running) ++open;
    }
    return open;
}

void ExportQueueDialog::jobStarted(int id) {
    if (QTreeWidgetItem *it = item(id)) setState(it, Running);
    updateButtons();
}

void ExportQueueDialog::jobFinished(int id, const QString &file, int result) {
    QTreeWidgetItem *it = item(id);
    if (!it) return;
    it->setData(Picture, FileRole, file);
    if (!file.isEmpty()) it->setToolTip(Picture, file);
    setState(it, result == 0 ? Done : result == 2 ? Canceled : Failed);
    updateButtons();
}

QTreeWidgetItem *ExportQueueDialog::item(int id) const {
    for (int i = 0; i < m_list->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = m_list->topLevelItem(i);
        if (it->data(Picture, IdRole).toInt() == id) return it;
    }
    return nullptr;
}

void ExportQueueDialog::setState(QTreeWidgetItem *item, State state) {
    item->setData(Picture, StateRole, int(state));
    switch (state) {
        case Waiting:
            item->setText(Status, tr("Waiting"));
            break;
        case Running:
            item->setText(Status, tr("Being made..."));
            break;
        case Done:
            item->setText(Status,
                          tr("Done: %1").arg(QFileInfo(item->data(Picture,
                                                                  FileRole)
                                                           .toString())
                                                 .fileName()));
            break;
        case Failed:
            item->setText(Status, tr("Failed"));
            break;
        case Canceled:
            item->setText(Status, tr("Cancelled"));
            break;
    }
}

void ExportQueueDialog::updateButtons() {
    bool cancel = false, retry = false;
    for (QTreeWidgetItem *it : m_list->selectedItems()) {
        const int state = it->data(Picture, StateRole).toInt();
        cancel |= state == Waiting || state == Running;
        retry |= state == Failed || state == Canceled;
    }
    m_cancel->setEnabled(cancel);
    m_retry->setEnabled(retry);
    bool finished = false;
    for (int i = 0; i < m_list->topLevelItemCount(); ++i) {
        const int state =
            m_list->topLevelItem(i)->data(Picture, StateRole).toInt();
        finished |= state == Done || state == Canceled;
    }
    m_clear->setEnabled(finished);
}

void ExportQueueDialog::openSelected() {
    QString folder = LuminanceOptions().getExportDir();
    const QList<QTreeWidgetItem *> selected = m_list->selectedItems();
    if (!selected.isEmpty()) {
        const QString file = selected.first()->data(Picture, FileRole).toString();
        if (!file.isEmpty()) folder = QFileInfo(file).absolutePath();
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}
