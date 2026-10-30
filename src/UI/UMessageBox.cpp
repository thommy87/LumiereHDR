/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2011 Davide Anastasia
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
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 *
 * This file provides an unified style for all the QMessageBox in Luminance HDR
 *
 * Original Work
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 *
 */

#include <QtGlobal>

#include "UI/UMessageBox.h"

#include "Common/GitSHA1.h"
#include "Common/archs.h"
#include "Common/config.h"
#include "UI/ui_about.h"

namespace {
const int UMESSAGEBOX_WIDTH = 450;  // pixel
}  // end anonymous namespace

void UMessageBox::init() {
    QGridLayout *layout = reinterpret_cast<QGridLayout *>(this->layout());
    layout->addItem(m_horizontalSpacer, layout->rowCount(), 0, 1,
                    layout->columnCount());

    this->setWindowTitle(LUMINANCEDISPLAYNAME " " LUMINANCEVERSION);
#ifdef Q_OS_MACOS
    this->setWindowModality(
        Qt::WindowModal);  // In OS X, the QMessageBox is modal to the window
#endif
}

UMessageBox::UMessageBox(QWidget *parent)
    : QMessageBox(parent),
      m_horizontalSpacer(new QSpacerItem(
          UMESSAGEBOX_WIDTH, 0, QSizePolicy::Minimum, QSizePolicy::Expanding)) {
    init();
}

UMessageBox::~UMessageBox() {}

UMessageBox::UMessageBox(const QString &title, const QString &text, Icon icon,
                         int button0, int button1, int button2, QWidget *parent,
                         Qt::WindowFlags f)
    : QMessageBox(title, text, icon, button0, button1, button2, parent, f),
      m_horizontalSpacer(new QSpacerItem(
          UMESSAGEBOX_WIDTH, 0, QSizePolicy::Minimum, QSizePolicy::Expanding)) {
    init();
}

// void UMessageBox::showEvent(QShowEvent *event)
//{
//    QMessageBox::showEvent(event);
//}

void UMessageBox::about(QWidget *parent) {
    QDialog *about = new QDialog(parent);
    about->setAttribute(Qt::WA_DeleteOnClose);
    Ui::AboutLuminance ui;
    ui.setupUi(about);
    ui.authorsBox->setOpenExternalLinks(true);
    ui.thanksToBox->setOpenExternalLinks(true);
    ui.GPLbox->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ui.label_version->setText(
        QStringLiteral("<b style=\"font-size:15pt\">Lumière HDR</b><br>%1 "
                       "[Build %2]")
            .arg(QStringLiteral(LUMINANCEVERSION),
                 QString(g_GIT_SHA1).left(6)));

    // the program, then thanks: to the Luminance HDR team, whose work this
    // continues, and to the projects whose libraries it is built on
    ui.label->setText(
        QStringLiteral("<p><b>Lumière HDR</b></p><p>%1</p>"
                       "<p><a href=\"https://github.com/thommy87/LumiereHDR\">"
                       "github.com/thommy87/LumiereHDR</a></p>")
            .arg(tr("Merge bracketed photos into HDR images and develop them "
                    "into finished pictures.")));
    const QStringList libraries = {
        QStringLiteral("Qt"),        QStringLiteral("LibRaw"),
        QStringLiteral("Lensfun"),   QStringLiteral("Exiv2"),
        QStringLiteral("OpenEXR"),   QStringLiteral("LibTIFF"),
        QStringLiteral("libjpeg"),   QStringLiteral("libpng"),
        QStringLiteral("libavif"),   QStringLiteral("libjxl"),
        QStringLiteral("libwebp"),   QStringLiteral("libultrahdr"),
        QStringLiteral("Little CMS"), QStringLiteral("FFTW"),
        QStringLiteral("GSL"),       QStringLiteral("Eigen"),
        QStringLiteral("Boost"),     QStringLiteral("CFITSIO"),
        QStringLiteral("Hugin (align_image_stack)"),
        QStringLiteral("pfstools / pfstmo")};
    ui.thanksToBox->setHtml(
        QStringLiteral("<h3>%1</h3><p>%2</p><h3>%3</h3><p>%4</p><p>%5</p>"
                       "<h3>%6</h3><p>%7</p>")
            .arg(tr("Thanks to the former Luminance HDR team"),
                 tr("Lumière HDR continues Luminance HDR. Many thanks to "
                    "everyone who developed, translated and packaged it over "
                    "the years."),
                 tr("Thanks to the teams of the libraries"),
                 tr("Lumière HDR is built on the work of these projects:"),
                 libraries.join(QStringLiteral(" · ")),
                 tr("Thanks to the community"),
                 tr("Everyone who reports bugs, tests and shares ideas.")));
    // the people are thanked as a team now; their list goes
    ui.tabWidget->removeTab(ui.tabWidget->indexOf(ui.tab_2));
    ui.tabWidget->setCurrentIndex(0);

    bool license_file_not_found = true;
    QString docDir = QCoreApplication::applicationDirPath();
    docDir.append("/../Resources");
    QStringList paths =
        QStringList(BASEDIR "/share/doc/luminance-hdr")
        << BASEDIR "/share/luminance-hdr/doc" << docDir
        << QStringLiteral("/Applications/luminance.app/Contents/Resources")
        << QStringLiteral("./") << QCoreApplication::applicationDirPath();
    foreach (const QString &path, paths) {
        QString fname(path + QStringLiteral("/LICENSE"));
        if (QFile::exists(fname)) {
            QFile file(fname);
            // try opening it
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) break;
            QTextStream ts(&file);
            ui.GPLbox->setAcceptRichText(false);
            // the GPL asks to keep the original authors' notices; they stay
            // in the source code, and this line points to them
            ui.GPLbox->setPlainText(
                tr("Lumière HDR is based on Luminance HDR and is free "
                   "software under the GNU General Public License, version 2 "
                   "or later. The copyright notices of the original authors "
                   "are kept in the source code.") +
                QStringLiteral("\n\n") + ts.readAll());
            license_file_not_found = false;
            break;
        }
    }
    if (license_file_not_found) {
        ui.GPLbox->setOpenExternalLinks(true);
        ui.GPLbox->setTextInteractionFlags(Qt::TextBrowserInteraction);
        ui.GPLbox->setHtml(
            tr("%1 License document not found, you can find it online: "
               "%2here%3",
               "%2 and %3 are html tags")
                .arg(QStringLiteral("<html>"),
                     QStringLiteral("<a \
                                    href=\"http://www.gnu.org/licenses/ \
                                    old-licenses/gpl-2.0.txt\">"),
                     QStringLiteral("</a></html>")));
    }
    bool changelog_file_not_found = true;
    foreach (const QString &path, paths) {
        QString fname(path + QStringLiteral("/Changelog"));
        if (QFile::exists(fname)) {
            QFile file(fname);
            // try opening it
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) break;
            QTextStream ts(&file);
            ui.ChangelogBox->setAcceptRichText(false);
            ui.ChangelogBox->setPlainText(ts.readAll());
            changelog_file_not_found = false;
            break;
        }
    }
    if (changelog_file_not_found) {
        ui.ChangelogBox->setOpenExternalLinks(true);
        ui.ChangelogBox->setTextInteractionFlags(Qt::TextBrowserInteraction);
        ui.ChangelogBox->setHtml(
            tr("%1 Changelog document not found, you can find it online: "
               "%2here%3",
               "%2 and %3 are html tags")
                .arg(QStringLiteral("<html>"),
                     QStringLiteral("<a \
                                    href=\"http://qtpfsgui.sourceforge.net/ \
                                    updater/Changelog\">"),
                     QStringLiteral("</a></html>")));
    }
    about->show();
}

int UMessageBox::warning(const QString &title, const QString &description,
                         QWidget *parent) {
    UMessageBox WarningMsgBox(parent);
    WarningMsgBox.setText(title);
    WarningMsgBox.setInformativeText(description);
    WarningMsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    WarningMsgBox.setDefaultButton(QMessageBox::No);
    WarningMsgBox.setIcon(QMessageBox::Warning);

    return WarningMsgBox.exec();
}

int UMessageBox::question(const QString &title, const QString &description,
                          QWidget *parent) {
    UMessageBox questionMsgBox(parent);
    questionMsgBox.setText(title);
    questionMsgBox.setInformativeText(description);
    questionMsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    questionMsgBox.setDefaultButton(QMessageBox::No);
    questionMsgBox.setIcon(QMessageBox::Question);

    return questionMsgBox.exec();
}

int UMessageBox::saveDialog(const QString &title, const QString &description,
                            QWidget *parent) {
    UMessageBox WarningMsgBox(parent);
    WarningMsgBox.setText(title);
    WarningMsgBox.setInformativeText(description);
    WarningMsgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard |
                                     QMessageBox::Cancel);
    WarningMsgBox.setDefaultButton(QMessageBox::Cancel);
    WarningMsgBox.setIcon(QMessageBox::Warning);

    return WarningMsgBox.exec();
}

void UMessageBox::donationSplashMB(QWidget *parent) {
    UMessageBox donationMB(parent);
    donationMB.setText(tr("Donation"));
    donationMB.setInformativeText(tr("Would you like to donate?"));
    donationMB.setIcon(QMessageBox::Question);

    QPushButton *yes =
        donationMB.addButton(tr("Yes, I'd love to!"), QMessageBox::ActionRole);
    QPushButton *dontBother =
        donationMB.addButton(tr("Stop Bothering Me"), QMessageBox::ActionRole);
    QPushButton *later =
        donationMB.addButton(tr("Remind me later"), QMessageBox::ActionRole);

    donationMB.exec();

    if (donationMB.clickedButton() == yes) {
        qDebug() << "Open Donation Web page";
    } else if (donationMB.clickedButton() == dontBother) {
        qDebug() << "Stop bother me";

    } else if (donationMB.clickedButton() == later) {
        qDebug() << "Remind me later";
    }
}
