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

#include "CommandPalette.h"

#include <QAction>
#include <QApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

#include "LumiereIcons.h"

namespace {
enum Roles { PathRole = Qt::UserRole + 1, ShortcutRole, IndexRole };

QString plain(QString text) {
    text.remove(QLatin1Char('&'));
    if (text.endsWith(QLatin1String("..."))) text.chop(3);
    return text.trimmed();
}

//! name on the left; its menu and the shortcut quieter on the right
class EntryDelegate : public QStyledItemDelegate {
   public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override {
        return QSize(QStyledItemDelegate::sizeHint(option, index).width(), 40);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        const QPalette &pal = opt.palette;
        const bool selected = opt.state & QStyle::State_Selected;
        const bool enabled = index.flags() & Qt::ItemIsEnabled;

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        if (selected) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(pal.color(QPalette::Highlight));
            painter->drawRoundedRect(opt.rect.adjusted(6, 1, -6, -1), 8, 8);
        }
        const QRect r = opt.rect.adjusted(18, 0, -18, 0);
        const QColor main = selected ? pal.color(QPalette::HighlightedText)
                            : enabled
                                ? pal.color(QPalette::Text)
                                : pal.color(QPalette::Disabled, QPalette::Text);
        QColor quiet = selected ? pal.color(QPalette::HighlightedText)
                                : pal.color(QPalette::PlaceholderText);

        const QString shortcut = index.data(ShortcutRole).toString();
        const QString path = index.data(PathRole).toString();
        QFontMetrics fm(opt.font);
        const int rightWidth =
            fm.horizontalAdvance(path) +
            (shortcut.isEmpty() ? 0 : fm.horizontalAdvance(shortcut) + 24);

        painter->setPen(quiet);
        QRect right(r.right() - rightWidth, r.top(), rightWidth, r.height());
        if (!shortcut.isEmpty()) {
            painter->drawText(right, Qt::AlignVCenter | Qt::AlignRight,
                              shortcut);
            right.setRight(right.right() - fm.horizontalAdvance(shortcut) -
                           24);
        }
        painter->drawText(right, Qt::AlignVCenter | Qt::AlignRight, path);

        painter->setPen(main);
        const QRect left(r.left(), r.top(), r.width() - rightWidth - 16,
                         r.height());
        painter->drawText(left, Qt::AlignVCenter | Qt::AlignLeft,
                          fm.elidedText(opt.text, Qt::ElideRight,
                                        left.width()));
        painter->restore();
    }
};
}  // namespace

CommandPalette::CommandPalette(const QList<QMenu *> &menus, QWidget *parent)
    : QDialog(parent, Qt::Popup | Qt::FramelessWindowHint) {
    setObjectName(QStringLiteral("CommandPalette"));
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_StyledBackground);
    resize(660, 480);

    m_query = new QLineEdit(this);
    m_query->setObjectName(QStringLiteral("CommandQuery"));
    m_query->setPlaceholderText(tr("Search a command or setting"));
    m_query->setClearButtonEnabled(true);
    m_query->addAction(LumiereIcons::icon(QStringLiteral("search")),
                       QLineEdit::LeadingPosition);
    m_query->installEventFilter(this);

    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("CommandList"));
    m_list->setItemDelegate(new EntryDelegate(m_list));
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setUniformItemSizes(true);

    QLabel *hint = new QLabel(
        tr("Up/Down to choose · Enter to run · Esc to close"), this);
    hint->setObjectName(QStringLiteral("CommandHint"));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_query);
    layout->addWidget(m_list, 1);
    layout->addWidget(hint);

    for (QMenu *menu : menus) collect(menu, plain(menu->title()));

    connect(m_query, &QLineEdit::textChanged, this, &CommandPalette::filter);
    connect(m_list, &QListWidget::itemActivated, this,
            [this](QListWidgetItem *) { runCurrent(); });
    filter(QString());

    if (parent) {
        const QRect p = parent->window()->geometry();
        move(p.center().x() - width() / 2, p.top() + 100);
    }
    m_query->setFocus();
}

void CommandPalette::collect(QMenu *menu, const QString &path) {
    for (QAction *action : menu->actions()) {
        if (action->isSeparator()) continue;
        if (QMenu *sub = action->menu()) {
            collect(sub, path + QStringLiteral(" › ") + plain(sub->title()));
            continue;
        }
        const QString text = plain(action->text());
        // hidden entries (unused recent-file slots) are left out
        if (text.isEmpty() || !action->isVisible()) continue;
        m_entries.append({action, text, path});
    }
}

void CommandPalette::filter(const QString &query) {
    m_list->clear();
    const QStringList words =
        query.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (int i = 0; i < m_entries.size(); ++i) {
        const Entry &e = m_entries.at(i);
        if (!e.action) continue;
        const QString haystack = e.text + QLatin1Char(' ') + e.path;
        bool match = true;
        for (const QString &w : words)
            if (!haystack.contains(w, Qt::CaseInsensitive)) {
                match = false;
                break;
            }
        if (!match) continue;
        QListWidgetItem *item = new QListWidgetItem(e.text, m_list);
        item->setData(PathRole, e.path);
        item->setData(ShortcutRole, e.action->shortcut().toString(
                                        QKeySequence::NativeText));
        item->setData(IndexRole, i);
        if (!e.action->isEnabled()) item->setFlags(Qt::NoItemFlags);
    }
    // first item that can run
    for (int r = 0; r < m_list->count(); ++r)
        if (m_list->item(r)->flags() & Qt::ItemIsEnabled) {
            m_list->setCurrentRow(r);
            break;
        }
}

void CommandPalette::runCurrent() {
    QListWidgetItem *item = m_list->currentItem();
    if (!item || !(item->flags() & Qt::ItemIsEnabled)) return;
    QPointer<QAction> action =
        m_entries.at(item->data(IndexRole).toInt()).action;
    close();
    // run after the popup is gone, a dialog may open from it
    if (action)
        QMetaObject::invokeMethod(action, "trigger", Qt::QueuedConnection);
}

bool CommandPalette::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_query && event->type() == QEvent::KeyPress) {
        QKeyEvent *key = static_cast<QKeyEvent *>(event);
        int step = 0;
        if (key->key() == Qt::Key_Down) step = 1;
        if (key->key() == Qt::Key_Up) step = -1;
        if (step != 0) {
            int row = m_list->currentRow();
            for (int r = row + step; r >= 0 && r < m_list->count(); r += step)
                if (m_list->item(r)->flags() & Qt::ItemIsEnabled) {
                    m_list->setCurrentRow(r);
                    break;
                }
            return true;
        }
        if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
            runCurrent();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}
