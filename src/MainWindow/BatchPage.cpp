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

#include "BatchPage.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include <BatchHDR/BatchHDRDialog.h>
#include <BatchTM/BatchTMDialog.h>
#include <UI/LumiereIcons.h>

BatchPage::BatchPage(const QSqlDatabase &db, QWidget *parent)
    : QWidget(parent), m_db(db) {
    setObjectName(QStringLiteral("BatchPage"));
    setAttribute(Qt::WA_StyledBackground);

    QLabel *title = new QLabel(tr("Batch processing"));
    title->setObjectName(QStringLiteral("PageTitle"));
    QLabel *subtitle =
        new QLabel(tr("Many pictures at once, while you do something else."));
    subtitle->setObjectName(QStringLiteral("PageSubtitle"));
    QVBoxLayout *titles = new QVBoxLayout;
    titles->setSpacing(4);
    titles->addWidget(title);
    titles->addWidget(subtitle);

    // HDRs from brackets | Looks on HDRs
    QWidget *segment = new QWidget;
    segment->setObjectName(QStringLiteral("Segment"));
    segment->setFixedWidth(380);
    QHBoxLayout *segmentLayout = new QHBoxLayout(segment);
    segmentLayout->setContentsMargins(3, 3, 3, 3);
    segmentLayout->setSpacing(2);
    m_switch = new QButtonGroup(this);
    const QStringList names = {tr("HDRs from brackets"), tr("Looks on HDRs")};
    for (int i = 0; i < names.size(); ++i) {
        QToolButton *b = new QToolButton;
        b->setObjectName(QStringLiteral("SegmentButton"));
        b->setText(names.at(i));
        b->setCheckable(true);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_switch->addButton(b, i);
        segmentLayout->addWidget(b);
    }

    QHBoxLayout *header = new QHBoxLayout;
    header->addLayout(titles, 1);
    header->addWidget(segment, 0, Qt::AlignVCenter);

    m_pages = new QStackedWidget;
    m_pages->setObjectName(QStringLiteral("BatchPages"));
    m_pages->addWidget(new QWidget);
    m_pages->addWidget(new QWidget);
    makeHdrDialog();
    makeLooksDialog();
    connect(m_switch, &QButtonGroup::idClicked, m_pages,
            &QStackedWidget::setCurrentIndex);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 24, 32, 24);
    layout->setSpacing(18);
    layout->addLayout(header);
    layout->addWidget(m_pages, 1);

    showKind(Hdr);
}

void BatchPage::showKind(Kind kind) {
    m_switch->button(kind)->setChecked(true);
    m_pages->setCurrentIndex(kind);
}

template <class Dialog>
void BatchPage::embed(Dialog *dialog, int index) {
    dialog->setWindowFlags(Qt::Widget);
    dialog->setObjectName(QStringLiteral("BatchWorkspace"));
    QWidget *old = m_pages->widget(index);
    m_pages->insertWidget(index, dialog);
    m_pages->removeWidget(old);
    old->deleteLater();
    // line icons in place of the theme's, which may be missing
    static const QList<QPair<const char *, const char *>> icons = {
        {"add_HDRs_Button", "plus"},          {"add_TMopts_Button", "plus"},
        {"add_dir_HDRs_Button", "folder"},    {"add_dir_TMopts_Button", "folder"},
        {"remove_HDRs_Button", "minus"},      {"remove_TMOpts_Button", "minus"},
        {"remove_all_HDRs_Button", "trash"},  {"remove_all_TMOpts_Button", "trash"},
        {"from_Database_Button", "star"},     {"formatSettingsButton", "settings"},
        {"clearTextToolButton", "trash"}};
    for (const auto &entry : icons)
        if (QToolButton *b = dialog->template findChild<QToolButton *>(
                QLatin1String(entry.first))) {
            b->setIcon(LumiereIcons::icon(QLatin1String(entry.second)));
            b->setIconSize(QSize(18, 18));
            b->setProperty("iconButton", true);
        }
    // the button that starts the work stands out
    for (const char *name : {"startButton", "BatchGoButton"})
        if (QPushButton *start = dialog->template findChild<QPushButton *>(
                QLatin1String(name))) {
            start->setProperty("primary", true);
            start->style()->unpolish(start);
            start->style()->polish(start);
        }
    dialog->show();
    if (m_switch->checkedId() >= 0)
        m_pages->setCurrentIndex(m_switch->checkedId());
}

void BatchPage::makeHdrDialog() {
    m_hdr = new BatchHDRDialog(nullptr, m_db);
    embed(m_hdr.data(), Hdr);
    connect(m_hdr.data(), &QDialog::finished, this, [this]() {
        const QStringList created = m_hdr->writtenFiles();
        // a finished run cannot start again: a fresh one takes its place
        makeHdrDialog();
        // go straight on from brackets to finished pictures
        if (!created.isEmpty() && m_looks &&
            QMessageBox::question(
                this, tr("Batch HDR"),
                tr("%n HDR image(s) created. Give them a look now?", "",
                   created.size())) == QMessageBox::Yes) {
            m_looks->addHdrFiles(created);
            showKind(Looks);
            return;
        }
        emit closed();
    });
}

void BatchPage::makeLooksDialog() {
    m_looks = new BatchTMDialog(nullptr, m_db);
    embed(m_looks.data(), Looks);
    connect(m_looks.data(), &QDialog::finished, this, [this]() {
        makeLooksDialog();
        emit closed();
    });
}
