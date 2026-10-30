/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
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
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Improvements, bugfixing
 * @author Franco Comida <fcomida@users.sourceforge.net>
 * Implement class deriving from QSettings (override Singleton pattern with
 * QSettings functionalities)
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#include <QtGlobal>
#include <QApplication>
#include <QDate>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QLocale>
#include <QMessageBox>
#include <QString>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>

#include "Common/LuminanceOptions.h"
#include "Common/config.h"
#include "Common/init_fftw.h"

#if defined(Q_OS_WIN)
const QString LuminanceOptions::LUMINANCE_HDR_HOME_FOLDER = "LuminanceHDR";
#elif defined(Q_OS_MACOS)
const QString LuminanceOptions::LUMINANCE_HDR_HOME_FOLDER =
    ".config/.LuminanceHDR";
#else
const QString LuminanceOptions::LUMINANCE_HDR_HOME_FOLDER =
    QStringLiteral(".config/luminance-hdr");
#endif

const static QString MAC_THEME = QStringLiteral("Macintosh");

bool LuminanceOptions::isCurrentPortableMode = false;

void LuminanceOptions::checkHomeFolder() {
    if (isCurrentPortableMode) {
        return;
    }

    QDir dir(QDir::homePath());
    if (!dir.exists(LUMINANCE_HDR_HOME_FOLDER)) {
        dir.mkdir(LUMINANCE_HDR_HOME_FOLDER);
    }
}

LuminanceOptions::LuminanceOptions() : QObject() { initSettings(); }

LuminanceOptions::~LuminanceOptions() { delete m_settingHolder; }

void LuminanceOptions::conditionallyDoUpgrade() {
    LuminanceOptions options;
    int currentVersion = options.value(KEY_OPTIONS_VERSION, 0).toInt();

    // check if update needed
    if (currentVersion < LUMINANCEVERSION_NUM) {
        if (currentVersion < 2030099) {
            options.setRawWhiteBalanceMethod(1);
        }
        if (currentVersion < 2040099) {
            options.setAlignImageStackOptions(
                sanitizeAISparams(options.getAlignImageStackOptions()));
        }

        options.setValue(KEY_WARNING_WOW64,
                         0);  // remind the user again with a new version
        options.setValue(KEY_OPTIONS_VERSION, LUMINANCEVERSION_NUM);
    }
}

bool LuminanceOptions::doShowWindowsOnWindows64Message() {
    int currentTimes = value(KEY_WARNING_WOW64, 0).toInt() + 1;
    bool result = currentTimes <= 3;
    if (result) setValue(KEY_WARNING_WOW64, currentTimes);

    return result;
}

void LuminanceOptions::setPortableMode(bool isPortable) {
    if (LuminanceOptions::isCurrentPortableMode != isPortable) {
        QSettings *oldSettings = m_settingHolder;
        LuminanceOptions::isCurrentPortableMode = isPortable;
        initSettings();
        foreach (const QString &key, oldSettings->allKeys()) {
            m_settingHolder->setValue(key, oldSettings->value(key));
        }
        delete oldSettings;

#ifdef Q_OS_MACOS
        QString settingsDirName =
            QGuiApplication::applicationDirPath() + QStringLiteral("/../../../.LuminanceHDR");
        QFile settingsDir(settingsDirName);
        if (!settingsDir.exists()) {
            QDir settingsPath(QGuiApplication::applicationDirPath() + QStringLiteral("/../../../"));
            settingsPath.mkpath(settingsDirName);
        }
#endif
        QString filePath =
#ifdef Q_OS_MACOS
            QGuiApplication::applicationDirPath() + QStringLiteral("/../../../.LuminanceHDR/PortableMode.txt");
#else
            QGuiApplication::applicationDirPath() + QStringLiteral("/PortableMode.txt");
#endif
        QFile file(filePath);
        if (isPortable && !file.exists()) {
            if (file.open(QIODevice::WriteOnly)) file.close();
        } else if (!isPortable && file.exists()) {
            file.remove();
        }
    }
}

bool LuminanceOptions::checkForUpdate() {
    if (!isUpdateCheckEnabled()) return false;
    QDate date = value(KEY_UPDATE_CHECKED_ON, QDate(1, 1, 1)).toDate();
    return date < QDate::currentDate();
}

// off by default: the app contacts the network only when the user opts in
bool LuminanceOptions::isUpdateCheckEnabled() {
    return value(KEY_UPDATE_CHECK_ENABLED, false).toBool();
}

void LuminanceOptions::setUpdateCheckEnabled(bool enabled) {
    setValue(KEY_UPDATE_CHECK_ENABLED, enabled);
}

void LuminanceOptions::setUpdateChecked() {
    setValue(KEY_UPDATE_CHECKED_ON, QDate::currentDate());
}

void LuminanceOptions::initSettings() {
    if (LuminanceOptions::isCurrentPortableMode) {
        QString iniFile =
#ifdef Q_OS_MACOS
            QGuiApplication::applicationDirPath() + QStringLiteral("/../../../.LuminanceHDR/settings.ini");
        iniFile = QDir::cleanPath(iniFile);
#else
            QGuiApplication::applicationDirPath() + QStringLiteral("/settings.ini");
#endif
        m_settingHolder =
            new QSettings( iniFile, QSettings::IniFormat);
    }
    else {
        m_settingHolder = new QSettings();
    }
    // the tone mapping core has no Qt; tell it where FFTW keeps its plans
    set_fftw_wisdom_file(getFftwWisdomFileName().toStdString());
}

void LuminanceOptions::setValue(const QString &key, const QVariant &value) {
    m_settingHolder->setValue(key, value);
}

QVariant LuminanceOptions::value(const QString &key,
                                 const QVariant &defaultValue) const {
    return m_settingHolder->value(key, defaultValue);
}

QString LuminanceOptions::getDatabaseFileName() {
    QString filename;
    if (LuminanceOptions::isCurrentPortableMode) {
        filename = QGuiApplication::applicationDirPath();
#ifdef Q_OS_MACOS
        filename += QLatin1String("/../../../.LuminanceHDR");
        filename = QDir::cleanPath(filename);
#endif
    } else {
        filename = QDir(QDir::homePath()).absolutePath() + "/" +
                   LUMINANCE_HDR_HOME_FOLDER;
    }
    filename += QLatin1String("/saved_parameters.db");

    return filename;
}

QString LuminanceOptions::getFftwWisdomFileName() {
    QString filename;
    if (LuminanceOptions::isCurrentPortableMode) {
        filename = QGuiApplication::applicationDirPath();
    } else {
        filename = QDir(QDir::homePath()).absolutePath() + "/" +
                   LUMINANCE_HDR_HOME_FOLDER;
    }
    filename += QLatin1String("/lhdrwisdom.fftw");

    return filename;
}

QString LuminanceOptions::getGuiTheme() {
#ifdef Q_OS_MACOS
    return m_settingHolder->value(KEY_GUI_THEME, MAC_THEME).toString();
#else
    return m_settingHolder->value(KEY_GUI_THEME, "Fusion").toString();
#endif
}

void LuminanceOptions::setGuiTheme(const QString &s) {
    if (s == MAC_THEME) {
        setGuiDarkMode(false);
    }

    m_settingHolder->setValue(KEY_GUI_THEME, s);
}

bool LuminanceOptions::isGuiDarkMode() const {
    return m_settingHolder->value(KEY_GUI_DARKMODE, false).toBool();
}

void LuminanceOptions::setGuiDarkMode(bool b) {
    m_settingHolder->setValue(KEY_GUI_DARKMODE, b);
}

QString LuminanceOptions::getGuiColorScheme() const {
    const QString scheme = m_settingHolder->value(KEY_GUI_COLORSCHEME).toString();
    if (scheme == QLatin1String("light") || scheme == QLatin1String("dark") ||
        scheme == QLatin1String("system"))
        return scheme;
    // settings from before LumiereHDR 2.8 only had a dark mode switch
    return isGuiDarkMode() ? QStringLiteral("dark") : QStringLiteral("system");
}

void LuminanceOptions::setGuiColorScheme(const QString &scheme) {
    m_settingHolder->setValue(KEY_GUI_COLORSCHEME, scheme);
    setGuiDarkMode(scheme == QLatin1String("dark"));
}

bool LuminanceOptions::isSystemDark() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#elif defined(Q_OS_WIN)
    QSettings personalize(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\"
                       "CurrentVersion\\Themes\\Personalize"),
        QSettings::NativeFormat);
    return personalize.value(QStringLiteral("AppsUseLightTheme"), 1).toInt() == 0;
#else
    return false;
#endif
}

namespace {
// LumiereHDR accent, the light blue of the logo's star, a shade darker so
// white text on it (selections, the current step) reads at 4.5:1
const QColor kAccent(40, 116, 184);

QPalette darkPalette() {
    QPalette p;
    // panels a step above the near-black image ground; disabled text still
    // reads at 4.5:1 on panels and buttons
    const QColor window(21, 23, 27), base(28, 31, 36), button(34, 37, 43),
        text(236, 238, 241), disabled(138, 144, 154);
    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, base);
    p.setColor(QPalette::AlternateBase, window);
    p.setColor(QPalette::ToolTipBase, button);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::PlaceholderText, disabled);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, button);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::BrightText, QColor(255, 107, 107));
    p.setColor(QPalette::Light, QColor(52, 57, 68));
    p.setColor(QPalette::Midlight, QColor(43, 47, 54));
    p.setColor(QPalette::Mid, QColor(35, 38, 44));
    p.setColor(QPalette::Dark, QColor(15, 16, 19));
    p.setColor(QPalette::Shadow, QColor(0, 0, 0));
    p.setColor(QPalette::Highlight, kAccent);
    p.setColor(QPalette::HighlightedText, Qt::white);
    // the accent itself is too dark for text on the dark panels
    p.setColor(QPalette::Link, QColor(124, 184, 242));
    p.setColor(QPalette::LinkVisited, QColor(169, 209, 247));
    for (QPalette::ColorRole role :
         {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        p.setColor(QPalette::Disabled, role, disabled);
    }
    p.setColor(QPalette::Disabled, QPalette::Highlight, QColor(52, 57, 68));
    return p;
}

QPalette lightPalette(const QStyle *style) {
    // our own light colours instead of the style's standard palette: the
    // native Windows styles hand out a warm grey that clashed with the white
    // panels of the new pages
    QPalette p = style ? style->standardPalette() : QPalette();
    const QColor window(255, 255, 255), base(255, 255, 255),
        alternate(246, 247, 249), button(246, 247, 249), text(22, 24, 28),
        disabled(125, 125, 125);
    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, base);
    p.setColor(QPalette::AlternateBase, alternate);
    p.setColor(QPalette::ToolTipBase, base);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::PlaceholderText, QColor(110, 116, 126));
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, button);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::Light, QColor(255, 255, 255));
    p.setColor(QPalette::Midlight, QColor(238, 240, 243));
    p.setColor(QPalette::Mid, QColor(213, 217, 223));
    // around the image: a calm light grey, like the page ground
    p.setColor(QPalette::Dark, QColor(228, 231, 235));
    p.setColor(QPalette::Shadow, QColor(160, 166, 175));
    p.setColor(QPalette::Highlight, kAccent);
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Link, kAccent.darker(120));
    // the standard light grey for disabled text is barely visible; this one
    // reads, yet stays clearly lighter than the black of enabled text
    for (QPalette::ColorRole role :
         {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        p.setColor(QPalette::Disabled, role, disabled);
    }
    return p;
}

//! the look of the new main window parts (top bar, start page, command
//! search, image toolbar); everything else keeps the style's own drawing
QString lumiereStyleSheet(bool dark) {
    struct Tokens {
        const char *ground, *bar, *panel, *input, *chip, *line, *line2,
            *text, *text2, *text3, *accentText, *hover, *selected;
    };
    static const Tokens kDark = {"#0F1013", "#131418", "#15171B", "#1C1F24",
                                 "#22252B", "#2B2F36", "#23262C", "#ECEEF1",
                                 "#A7ADB7", "#8A909A", "#7CB8F2",
                                 "rgba(255,255,255,0.06)", "#1E2836"};
    static const Tokens kLight = {"#F1F2F4", "#FFFFFF", "#FFFFFF", "#F6F7F9",
                                  "#E9ECF0", "#D5D9DF", "#E4E7EB", "#16181C",
                                  "#474D57", "#5E6470", "#1F64A3",
                                  "rgba(0,0,0,0.05)", "#E3EEF9"};
    const Tokens &t = dark ? kDark : kLight;
    QString qss = QStringLiteral(R"(
#TopBar { background: @bar; border-bottom: 1px solid @line2; }
#TopBar QLabel#TopBarName { color: @text; font-weight: 600; font-size: 14px; }
#TopBar QToolButton { border: none; border-radius: 8px; color: @text2;
  padding: 0 8px; min-height: 36px; }
#TopBar QToolButton:hover { background: @hover; color: @text; }
#TopBar QToolButton::menu-indicator { image: none; width: 0; }
#TopBar QToolButton#ModeButton { padding: 0 16px; font-weight: 500; }
#TopBar QToolButton#ModeButton:checked { background: @chip; color: @text; }
#TopBar QToolButton#ModeButton:disabled { color: @text3; }
#TopBar QToolButton#SearchButton { background: @input; border: 1px solid @line;
  color: @text3; padding: 0 12px; min-width: 150px; text-align: left; }
#TopBar QToolButton#SearchButton:hover { border-color: @text3; }

#StartPage, #StartScroll, #StartCanvas, #StartColumn { background: @ground; }
#StartTitle { color: @text; font-size: 28px; font-weight: 600; }
#StartSubtitle { color: @text2; font-size: 15px; }
QPushButton#StartCard { background: @panel; border: 1px solid @line;
  border-radius: 14px; text-align: left; }
QPushButton#StartCard:hover { border-color: @text3; }
QPushButton#StartCard:focus { border: 2px solid @accentText; }
QPushButton#StartCard[primary="true"] { border-color: @accent; }
#StartCardIcon { background: @chip; border-radius: 12px; }
QPushButton#StartCard[primary="true"] #StartCardIcon { background: @accent; }
#StartCardTitle { color: @text; font-size: 16px; font-weight: 600; }
#StartCardText { color: @text2; font-size: 13px; }
#Keycap { color: @text2; border: 1px solid @line; border-radius: 4px;
  padding: 1px 6px; font-size: 11px; }
#StartDrop { color: @text3; border: 1px dashed @line; border-radius: 14px;
  font-size: 13px; }
#SectionLabel { color: @text3; font-size: 11px; font-weight: 600;
  letter-spacing: 1px; }
QToolButton#RecentTile { border: none; border-radius: 10px; color: @text2;
  padding: 6px; }
QToolButton#RecentTile:hover { background: @hover; color: @text; }
QToolButton#LinkButton { border: none; color: @accentText; padding: 4px 0; }
QToolButton#LinkButton:hover { text-decoration: underline; }

#ImageToolBar { background: @bar; border: none;
  border-bottom: 1px solid @line2; spacing: 2px; padding: 4px 8px; }
#ImageToolBar QToolButton { border: none; border-radius: 8px; padding: 6px; }
#ImageToolBar QToolButton:hover { background: @hover; }
#ImageToolBar QToolButton:checked { background: @chip; }
#ImageToolBar::separator { background: @line; width: 1px; margin: 8px 6px; }

#CommandPalette { background: @panel; border: 1px solid @line;
  border-radius: 16px; }
QLineEdit#CommandQuery { background: transparent; border: none;
  border-bottom: 1px solid @line2; color: @text; font-size: 17px;
  padding: 0 12px; min-height: 58px; }
QListWidget#CommandList { background: transparent; border: none;
  outline: none; }
#CommandHint { color: @text3; font-size: 12px; padding: 8px 18px;
  border-top: 1px solid @line2; }

#LooksPanel, #Inspector, #OwnLooks, #InspectorScroll { background: @panel; }
#OwnLooks { border-top: 1px solid @line2; }
#LooksPanel QScrollArea, #PresetGrid, #HistoryView { background: transparent; }
#HistoryView { padding: 6px 8px; outline: none; }
#HistoryView::item { padding: 6px 8px; border-radius: 6px; color: @text2; }
#HistoryView::item:hover { background: @hover; color: @text; }
#HistoryView::item:selected { background: @selected; color: @text;
  font-weight: 600; }
#Segment { background: @input; border-radius: 8px; }
QToolButton#SegmentButton { border: none; border-radius: 6px; padding: 5px 8px;
  color: @text2; font-weight: 500; }
QToolButton#SegmentButton:checked { background: @chip; color: @text; }
QToolButton#LookButton { border: none; border-radius: 10px; padding: 4px;
  color: @text2; }
QToolButton#LookButton:hover { background: @hover; color: @text; }
QToolButton#LookButton:checked { background: @selected; color: @text;
  font-weight: 600; }
QPushButton#QuietButton { border: none; background: transparent;
  color: @text; padding: 4px 0; text-align: left; }
QPushButton#QuietButton:hover { color: @accentText; }
#HintLabel { color: @text3; font-size: 12px; }
#Inspector QToolBox::tab { background: transparent; border: none;
  border-top: 1px solid @line2; color: @text; font-weight: 600; }
#Inspector QToolBox::tab:selected { color: @text; }
#Inspector QToolBox QScrollArea, #Inspector QToolBox QScrollArea > QWidget,
#Inspector QToolBox QScrollArea > QWidget > QWidget { background: transparent; }
#Inspector QGroupBox { border: none; border-top: 1px solid @line2;
  margin-top: 22px; padding-top: 6px; font-weight: 600; }
#Inspector QGroupBox::title { subcontrol-origin: margin; left: 6px;
  padding: 0; color: @text; font-size: 14px; }
#Inspector QComboBox, #Inspector QDoubleSpinBox, #Inspector QSpinBox,
#Inspector QLineEdit { background: @input; border: 1px solid @line;
  border-radius: 6px; padding: 3px 6px; color: @text; min-height: 22px; }
#Inspector QComboBox:focus, #Inspector QDoubleSpinBox:focus,
#Inspector QSpinBox:focus, #Inspector QLineEdit:focus {
  border-color: @accentText; }
#Inspector QPushButton, #Inspector QToolButton#queueButton {
  background: @input; border: 1px solid @line; border-radius: 8px;
  padding: 6px 12px; color: @text; }
#Inspector QPushButton:hover { border-color: @text3; }
#MergeWorkspace QGroupBox,
#SettingsContent QGroupBox,
#BatchWorkspace QGroupBox { border: none; border-top: 1px solid @line2;
  margin-top: 22px; padding-top: 6px; font-weight: 600; }#MergeWorkspace QGroupBox::title,
#SettingsContent QGroupBox::title,
#BatchWorkspace QGroupBox::title { subcontrol-origin: margin; left: 6px;
  padding: 0; color: @text; font-size: 14px; }#MergeWorkspace QComboBox,
#SettingsContent QComboBox,
#BatchWorkspace QComboBox,
#MergeWorkspace QDoubleSpinBox,
#SettingsContent QDoubleSpinBox,
#BatchWorkspace QDoubleSpinBox,
#MergeWorkspace QSpinBox,
#SettingsContent QSpinBox,
#BatchWorkspace QSpinBox,
#MergeWorkspace QLineEdit,
#SettingsContent QLineEdit,
#BatchWorkspace QLineEdit { background: @input; border: 1px solid @line;
  border-radius: 6px; padding: 3px 6px; color: @text; min-height: 22px; }#MergeWorkspace QComboBox:focus,
#SettingsContent QComboBox:focus,
#BatchWorkspace QComboBox:focus,
#MergeWorkspace QDoubleSpinBox:focus,
#SettingsContent QDoubleSpinBox:focus,
#BatchWorkspace QDoubleSpinBox:focus,
#MergeWorkspace QSpinBox:focus,
#SettingsContent QSpinBox:focus,
#BatchWorkspace QSpinBox:focus,
#MergeWorkspace QLineEdit:focus,
#SettingsContent QLineEdit:focus,
#BatchWorkspace QLineEdit:focus {
  border-color: @accentText; }#MergeWorkspace QPushButton,
#SettingsContent QPushButton,
#BatchWorkspace QPushButton,
#MergeWorkspace QToolButton#queueButton,
#SettingsContent QToolButton#queueButton,
#BatchWorkspace QToolButton#queueButton {
  background: @input; border: 1px solid @line; border-radius: 8px;
  padding: 6px 12px; color: @text; }#MergeWorkspace QPushButton:hover,
#SettingsContent QPushButton:hover,
#BatchWorkspace QPushButton:hover { border-color: @text3; }
#Inspector QToolButton#defaultButton, #Inspector QToolButton#undoButton,
#Inspector QToolButton#redoButton { background: transparent; border: none;
  border-radius: 6px; padding: 4px 8px; color: @accentText; }
#Inspector QToolButton#defaultButton:hover, #Inspector QToolButton#undoButton:hover,
#Inspector QToolButton#redoButton:hover { background: @hover; }
#Inspector QToolButton#defaultButton:disabled, #Inspector QToolButton#undoButton:disabled,
#Inspector QToolButton#redoButton:disabled { color: @text3; }
#Inspector QPushButton:disabled { color: @text3; }#MergeWorkspace,
#SettingsContent,
#BatchWorkspace { background: @ground; }#MergeWorkspace QTableWidget,
#SettingsContent QTableWidget,
#BatchWorkspace QTableWidget,
#MergeWorkspace QTextEdit,
#SettingsContent QTextEdit,
#BatchWorkspace QTextEdit { background: @panel;
  border: 1px solid @line2; border-radius: 10px; }#MergeWorkspace QHeaderView::section,
#SettingsContent QHeaderView::section,
#BatchWorkspace QHeaderView::section { background: @panel; color: @text3;
  border: none; border-bottom: 1px solid @line2; padding: 6px; }#MergeWorkspace QToolButton#loadImagesButton,
#SettingsContent QToolButton#loadImagesButton,
#BatchWorkspace QToolButton#loadImagesButton,
#MergeWorkspace QToolButton#removeImageButton,
#SettingsContent QToolButton#removeImageButton,
#BatchWorkspace QToolButton#removeImageButton,
#MergeWorkspace QToolButton#clearListButton,
#SettingsContent QToolButton#clearListButton,
#BatchWorkspace QToolButton#clearListButton { background: @input;
  border: 1px solid @line; border-radius: 8px; padding: 6px; }QPushButton[primary="true"],
#MergeWorkspace QPushButton[primary="true"],
#SettingsContent QPushButton[primary="true"],
#BatchWorkspace QPushButton[primary="true"] {
  background: @accent; border-color: @accent; color: white; font-weight: 600; }QPushButton[primary="true"]:disabled,
#MergeWorkspace QPushButton[primary="true"]:disabled,
#SettingsContent QPushButton[primary="true"]:disabled,
#BatchWorkspace QPushButton[primary="true"]:disabled { background: @chip; border-color: @line;
  color: @text3; }
QToolButton[reset="true"] { border: none; border-radius: 6px; padding: 3px; }
QToolButton[reset="true"]:hover { background: @hover; }
#ExportStage { background: @ground; }
#ExportQueueBox { background: @bar; border-top: 1px solid @line2; }
#ExportQueueBox QTreeWidget, #ExportQueueBox QHeaderView {
  background: transparent; border: none; }
#ExportQueueBox QHeaderView::section { background: transparent; color: @text3;
  border: none; border-bottom: 1px solid @line2; padding: 4px 6px; }
#ExportSide, #ExportScroll, #ExportPanel { background: @panel; }
#ExportSide { border-left: 1px solid @line2; }
#ExportActions { background: @panel; border-top: 1px solid @line2; }
#ExportPanel QComboBox, #ExportPanel QLineEdit { background: @input;
  border: 1px solid @line; border-radius: 8px; padding: 6px 8px; color: @text; }
#ExportPage QPushButton { background: @input; border: 1px solid @line;
  border-radius: 8px; padding: 6px 14px; color: @text; }
#ExportPage QPushButton:hover { border-color: @text3; }
#ExportPage QPushButton[primary="true"] { background: @accent;
  border-color: @accent; color: white; font-weight: 600; }
#ExportPage QPushButton#QuietButton { background: transparent; border: none;
  color: @accentText; padding: 4px 0; }
#ExportPage QPushButton#FormatTile { background: @input; border: 1px solid @line;
  border-radius: 10px; padding: 0; text-align: left; }
#ExportPage QPushButton#FormatTile:hover { border-color: @text3; }
#ExportPage QPushButton#FormatTile:checked { background: @selected;
  border-color: @accentText; }
#FormatName { color: @text; font-size: 14px; font-weight: 600; }
#FormatHint { color: @text2; font-size: 12px; }
#PageTitle { color: @text; font-size: 22px; font-weight: 600; }
#PageSubtitle { color: @text2; font-size: 13px; }
#BatchPage, #SettingsWorkspace { background: @ground; }
#SettingsNav { background: @bar; border-right: 1px solid @line2; }
QToolButton#SettingsNavButton { border: none; border-radius: 8px;
  padding: 0 12px; color: @text2; font-weight: 500; text-align: left; }
QToolButton#SettingsNavButton:hover { background: @hover; color: @text; }
QToolButton#SettingsNavButton:checked { background: @chip; color: @text; }
#SettingsContent QStackedWidget > QWidget { background: transparent; }
#BatchWorkspace QProgressBar { background: @input; border: none;
  border-radius: 3px; max-height: 6px; text-align: center; color: transparent; }
#BatchWorkspace QProgressBar::chunk { background: @accentText; border-radius: 3px; }
#TopBar QToolButton:checked { background: @chip; color: @text; }
#BatchWorkspace QToolButton { background: @input;
  border: 1px solid @line; border-radius: 8px; padding: 5px; }
#BatchWorkspace QToolButton:hover { border-color: @text3; }
#FilmStrip { background: @bar; border-top: 1px solid @line2; }
QListWidget#FilmList { background: transparent; border: none; color: @text2;
  outline: none; }
QListWidget#FilmList::item { border-radius: 8px; padding: 4px; }
QListWidget#FilmList::item:hover { background: @hover; }
QListWidget#FilmList::item:selected { background: @selected; color: @text; }
QToolButton#FilmButton { background: @input; border: 1px solid @line;
  border-radius: 8px; padding: 7px 14px; color: @text; }
QToolButton#FilmButton:hover { border-color: @text3; }
QSplitter#DevelopSplitter::handle { background: @line2; }

QSlider::groove:horizontal { height: 4px; background: @line;
  border-radius: 2px; }
QSlider::sub-page:horizontal { background: @accentText; border-radius: 2px; }
QSlider::handle:horizontal { width: 14px; height: 14px; margin: -5px 0;
  border-radius: 7px; background: @text; }
QSlider::sub-page:horizontal:disabled { background: @text3; }
QSlider::handle:horizontal:disabled { background: @text3; }
QScrollBar:vertical { width: 10px; background: transparent; margin: 0; }
QScrollBar:horizontal { height: 10px; background: transparent; margin: 0; }
QScrollBar::handle { background: @line; border-radius: 4px; margin: 2px; }
QScrollBar::handle:vertical { min-height: 32px; }
QScrollBar::handle:horizontal { min-width: 32px; }
QScrollBar::handle:hover { background: @text3; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
QMenu { background: @panel; border: 1px solid @line; padding: 4px; }
QMenu::item { padding: 6px 28px 6px 12px; border-radius: 6px; color: @text; }
QMenu::item:selected { background: @selected; }
QMenu::item:disabled { color: @text3; }
QMenu::separator { height: 1px; background: @line2; margin: 4px 8px; }

QToolTip { background: @chip; color: @text; border: 1px solid @line;
  border-radius: 6px; padding: 4px 8px; }
)");
    const QList<QPair<QString, QString>> values = {
        {QStringLiteral("@ground"), QLatin1String(t.ground)},
        {QStringLiteral("@bar"), QLatin1String(t.bar)},
        {QStringLiteral("@panel"), QLatin1String(t.panel)},
        {QStringLiteral("@input"), QLatin1String(t.input)},
        {QStringLiteral("@chip"), QLatin1String(t.chip)},
        {QStringLiteral("@line2"), QLatin1String(t.line2)},
        {QStringLiteral("@line"), QLatin1String(t.line)},
        {QStringLiteral("@text3"), QLatin1String(t.text3)},
        {QStringLiteral("@text2"), QLatin1String(t.text2)},
        {QStringLiteral("@text"), QLatin1String(t.text)},
        {QStringLiteral("@accentText"), QLatin1String(t.accentText)},
        {QStringLiteral("@accent"), kAccent.name()},
        {QStringLiteral("@hover"), QLatin1String(t.hover)},
        {QStringLiteral("@selected"), QLatin1String(t.selected)}};
    // longer names first, so @line2 is not read as @line plus "2"
    for (const auto &v : values) qss.replace(v.first, v.second);
    return qss;
}
}  // namespace

void LuminanceOptions::applyTheme(bool /*init*/) {
    QString theme = getGuiTheme();
    const QString scheme = getGuiColorScheme();
    const bool dark = scheme == QLatin1String("dark") ||
                      (scheme == QLatin1String("system") && isSystemDark());

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    // tell styles that follow the OS colour scheme which one we want
    QGuiApplication::styleHints()->setColorScheme(
        scheme == QLatin1String("dark")    ? Qt::ColorScheme::Dark
        : scheme == QLatin1String("light") ? Qt::ColorScheme::Light
                                           : Qt::ColorScheme::Unknown);
#endif

    // the native Windows styles (windows11, windowsvista) paint toolbars and
    // buttons themselves and ignore most of a custom palette, which left dark
    // grey text on dark controls; dark mode therefore uses Fusion
    if (dark && theme.startsWith(QLatin1String("windows"), Qt::CaseInsensitive))
        theme = QStringLiteral("Fusion");

    QApplication::setStyle(QStyleFactory::create(theme));

    // the bundled icon theme (Windows, macOS) has a light and a dark variant;
    // on Linux the desktop's icon theme is used and left alone
    if (QIcon::themeName().startsWith(QLatin1String("luminance-hdr")))
        QIcon::setThemeName(dark ? QStringLiteral("luminance-hdr-dark")
                                 : QStringLiteral("luminance-hdr"));

    // the native macOS style draws its own colours
    if (theme.compare(QLatin1String("Macintosh")) != 0)
        QApplication::setPalette(dark ? darkPalette()
                                      : lightPalette(QApplication::style()));
    qApp->setStyleSheet(lumiereStyleSheet(dark));
}

// write system default language the first time around (discard "_country")
QString LuminanceOptions::getGuiLang() {
    return m_settingHolder
        ->value(KEY_GUI_LANG, QLocale::system().name().left(2))
        .toString();
}

void LuminanceOptions::setGuiLang(const QString &s) {
    m_settingHolder->setValue(KEY_GUI_LANG, s);
}

bool LuminanceOptions::isRawFourColorRGB() const {
    return m_settingHolder->value(KEY_FOUR_COLOR_RGB, false).toBool();
}

void LuminanceOptions::setRawFourColorRGB(bool b) {
    m_settingHolder->setValue(KEY_FOUR_COLOR_RGB, b);
}

bool LuminanceOptions::isRawDoNotUseFujiRotate() const {
    return m_settingHolder->value(KEY_DO_NOT_USE_FUJI_ROTATE, false).toBool();
}

void LuminanceOptions::setRawDoNotUseFujiRotate(bool b) {
    m_settingHolder->setValue(KEY_DO_NOT_USE_FUJI_ROTATE, b);
}

double LuminanceOptions::getRawAber0() const {
    return m_settingHolder->value(KEY_ABER_0, 1.0).toDouble();
}

void LuminanceOptions::setRawAber0(double v) {
    m_settingHolder->setValue(KEY_ABER_0, v);
}

double LuminanceOptions::getRawAber1() const {
    return m_settingHolder->value(KEY_ABER_1, 1.0).toDouble();
}

void LuminanceOptions::setRawAber1(double v) {
    m_settingHolder->setValue(KEY_ABER_1, v);
}

double LuminanceOptions::getRawAber2() const {
    return m_settingHolder->value(KEY_ABER_2, 1.0).toDouble();
}

void LuminanceOptions::setRawAber2(double v) {
    m_settingHolder->setValue(KEY_ABER_2, v);
}

double LuminanceOptions::getRawAber3() const {
    return m_settingHolder->value(KEY_ABER_3, 1.0).toDouble();
}

void LuminanceOptions::setRawAber3(double v) {
    m_settingHolder->setValue(KEY_ABER_3, v);
}

double LuminanceOptions::getRawGamm0() const {
    return m_settingHolder->value(KEY_GAMM_0, 1.0 / 2.4).toDouble();
}

void LuminanceOptions::setRawGamm0(double v) {
    m_settingHolder->setValue(KEY_GAMM_0, v);
}

double LuminanceOptions::getRawGamm1() const {
    return m_settingHolder->value(KEY_GAMM_1, 12.92).toDouble();
}

void LuminanceOptions::setRawGamm1(double v) {
    m_settingHolder->setValue(KEY_GAMM_1, v);
}

int LuminanceOptions::getRawTemperatureKelvin() const {
    return m_settingHolder->value(KEY_TK, 6500).toInt();
}

void LuminanceOptions::setRawTemperatureKelvin(int v) {
    m_settingHolder->setValue(KEY_TK, v);
}

float LuminanceOptions::getRawGreen() const {
    return m_settingHolder->value(KEY_GREEN, 1.0f).toFloat();
}

void LuminanceOptions::setRawGreen(float v) {
    m_settingHolder->setValue(KEY_GREEN, v);
}

float LuminanceOptions::getRawUserMul0() const {
    return m_settingHolder->value(KEY_USER_MUL_0, 1.0f).toFloat();
}

void LuminanceOptions::setRawUserMul0(float v) {
    m_settingHolder->setValue(KEY_USER_MUL_0, v);
}

float LuminanceOptions::getRawUserMul1() const {
    return m_settingHolder->value(KEY_USER_MUL_1, 1.0f).toFloat();
}

void LuminanceOptions::setRawUserMul1(float v) {
    m_settingHolder->setValue(KEY_USER_MUL_1, v);
}

float LuminanceOptions::getRawUserMul2() const {
    return m_settingHolder->value(KEY_USER_MUL_2, 1.0f).toFloat();
}

void LuminanceOptions::setRawUserMul2(float v) {
    m_settingHolder->setValue(KEY_USER_MUL_2, v);
}

float LuminanceOptions::getRawUserMul3() const {
    return m_settingHolder->value(KEY_USER_MUL_3, 1.0f).toFloat();
}

void LuminanceOptions::setRawUserMul3(float v) {
    m_settingHolder->setValue(KEY_USER_MUL_3, v);
}

bool LuminanceOptions::isRawAutoBrightness() const {
    return m_settingHolder->value(KEY_AUTO_BRIGHT, false).toBool();
}

void LuminanceOptions::setRawAutoBrightness(bool b) {
    m_settingHolder->setValue(KEY_AUTO_BRIGHT, b);
}

float LuminanceOptions::getRawBrightness() const {
    return m_settingHolder->value(KEY_BRIGHTNESS, 1.0f).toFloat();
}

void LuminanceOptions::setRawBrightness(float f) {
    m_settingHolder->setValue(KEY_BRIGHTNESS, f);
}

float LuminanceOptions::getRawNoiseReductionThreshold() const {
    return m_settingHolder->value(KEY_THRESHOLD, 100.0f).toFloat();
}

void LuminanceOptions::setRawNoiseReductionThreshold(float v) {
    m_settingHolder->setValue(KEY_THRESHOLD, v);
}

bool LuminanceOptions::isWarnNewDomosaicingAlgos() const {
    return m_settingHolder->value(KEY_WARN_DEMOSAICING, true).toBool();
}

void LuminanceOptions::setWarnNewDomosaicingAlgos(bool warn) {
    m_settingHolder->setValue(KEY_WARN_DEMOSAICING, warn);
}

int LuminanceOptions::getRawHalfSize() const {
    return m_settingHolder->value(KEY_HALF_SIZE, 0).toInt();
}

void LuminanceOptions::setRawHalfSize(int v) {
    m_settingHolder->setValue(KEY_HALF_SIZE, v);
}

int LuminanceOptions::getRawWhiteBalanceMethod() const {
    return m_settingHolder->value(KEY_WB_METHOD, 1).toInt();
}

void LuminanceOptions::setRawWhiteBalanceMethod(int v) {
    m_settingHolder->setValue(KEY_WB_METHOD, v);
}

int LuminanceOptions::getRawOutputColor() const {
    return m_settingHolder->value(KEY_OUTPUT_COLOR, 1).toInt();
}

// double check those!
QString LuminanceOptions::getRawOutputProfile() const {
    // QFile::encodeName(this->value(KEY_OUTPUT_PROFILE).toString()).constData();
    return QFile::encodeName(
        m_settingHolder->value(KEY_OUTPUT_PROFILE).toString());
}

void LuminanceOptions::setRawOutputProfile(const QString &v) {
    m_settingHolder->setValue(KEY_OUTPUT_PROFILE, v);
}

QString LuminanceOptions::getRawCameraProfile() const {
    // QFile::encodeName(this->value(KEY_CAMERA_PROFILE).toString()).constData();
    return QFile::encodeName(
        m_settingHolder->value(KEY_CAMERA_PROFILE).toString());
}

void LuminanceOptions::setRawCameraProfile(const QString &v) {
    m_settingHolder->setValue(KEY_CAMERA_PROFILE, v);
}

int LuminanceOptions::getRawUserFlip() const {
    return m_settingHolder->value(KEY_USER_FLIP, 0).toInt();
}

#define USER_QUALITY 10  // default to AMaZE interpolation

int LuminanceOptions::getRawUserQuality() const {
    return m_settingHolder->value(KEY_USER_QUAL, USER_QUALITY).toInt();
}

void LuminanceOptions::setRawUserQuality(int v) {
    m_settingHolder->setValue(KEY_USER_QUAL, v);
}

int LuminanceOptions::getRawUserSaturation() const {
    return m_settingHolder->value(KEY_USER_SAT, 20000).toInt();
}

void LuminanceOptions::setRawUserSaturation(int v) {
    m_settingHolder->setValue(KEY_USER_SAT, v);
}

int LuminanceOptions::getRawMedPasses() const {
    return m_settingHolder->value(KEY_MED_PASSES, 0).toInt();
}

void LuminanceOptions::setRawMedPasses(int v) {
    m_settingHolder->setValue(KEY_MED_PASSES, v);
}

int LuminanceOptions::getRawHighlightsMode() const {
    return m_settingHolder->value(KEY_HIGHLIGHTS, 0).toInt();
}

void LuminanceOptions::setRawHighlightsMode(int v) {
    m_settingHolder->setValue(KEY_HIGHLIGHTS, v);
}

int LuminanceOptions::getRawLevel() const {
    return m_settingHolder->value(KEY_LEVEL, 0).toInt();
}

void LuminanceOptions::setRawLevel(int v) {
    m_settingHolder->setValue(KEY_LEVEL, v);
}

float LuminanceOptions::getRawAutoBrightnessThreshold() const {
    return m_settingHolder->value(KEY_AUTO_BRIGHT_THR, 0.001f).toFloat();
}

void LuminanceOptions::setRawAutoBrightnessThreshold(float v) {
    m_settingHolder->setValue(KEY_AUTO_BRIGHT_THR, v);
}

float LuminanceOptions::getRawMaximumThreshold() const {
    return m_settingHolder->value(KEY_ADJUST_MAXIMUM_THR, 0.0f).toFloat();
}

void LuminanceOptions::setRawMaximumThreshold(float v) {
    m_settingHolder->setValue(KEY_ADJUST_MAXIMUM_THR, v);
}

bool LuminanceOptions::isRawUseBlack() const {
    return m_settingHolder->value(KEY_USE_BLACK, false).toBool();
}

void LuminanceOptions::setRawUseBlack(bool b) {
    m_settingHolder->setValue(KEY_USE_BLACK, b);
}

int LuminanceOptions::getRawUserBlack() const {
    return m_settingHolder->value(KEY_USER_BLACK, 0).toInt();
}

void LuminanceOptions::setRawUserBlack(int v) {
    m_settingHolder->setValue(KEY_USER_BLACK, v);
}

bool LuminanceOptions::isRawUseSaturation() const {
    return m_settingHolder->value(KEY_USE_SAT, false).toBool();
}

void LuminanceOptions::setRawUseSaturation(bool b) {
    m_settingHolder->setValue(KEY_USE_SAT, b);
}

bool LuminanceOptions::isRawUseNoiseReduction() const {
    return m_settingHolder->value(KEY_USE_NOISE, true).toBool();
}

void LuminanceOptions::setRawUseNoiseReduction(bool b) {
    m_settingHolder->setValue(KEY_USE_NOISE, b);
}

bool LuminanceOptions::isRawLensCorrection() const {
    return m_settingHolder->value(QStringLiteral("RawLensCorrection"), true)
        .toBool();
}

void LuminanceOptions::setRawLensCorrection(bool b) {
    m_settingHolder->setValue(QStringLiteral("RawLensCorrection"), b);
}

bool LuminanceOptions::isRawUseChromaAber() const {
    return m_settingHolder->value(KEY_USE_CHROMA, false).toBool();
}

void LuminanceOptions::setRawUseChromaAber(bool b) {
    m_settingHolder->setValue(KEY_USE_CHROMA, b);
}

QString LuminanceOptions::getBatchHdrPathInput(const QString &defaultPath) {
    return m_settingHolder->value(KEY_BATCH_HDR_PATH_INPUT, defaultPath)
        .toString();
}

void LuminanceOptions::setBatchHdrPathInput(const QString &qstr) {
    m_settingHolder->setValue(KEY_BATCH_HDR_PATH_INPUT, qstr);
}

QString LuminanceOptions::getBatchHdrPathOutput(const QString &defaultPath) {
    return m_settingHolder->value(KEY_BATCH_HDR_PATH_OUTPUT, defaultPath)
        .toString();
}

void LuminanceOptions::setBatchHdrPathOutput(const QString &qstr) {
    m_settingHolder->setValue(KEY_BATCH_HDR_PATH_OUTPUT, qstr);
}

QString LuminanceOptions::getBatchTmPathHdrInput() {
    return m_settingHolder->value(KEY_BATCH_TM_PATH_INPUT, QDir::currentPath())
        .toString();
}

void LuminanceOptions::setBatchTmPathHdrInput(const QString &s) {
    m_settingHolder->setValue(KEY_BATCH_TM_PATH_INPUT, s);
}

QString LuminanceOptions::getBatchTmPathTmoSettings() {
    return m_settingHolder
        ->value(KEY_BATCH_TM_PATH_TMO_SETTINGS, QDir::currentPath())
        .toString();
}

void LuminanceOptions::setBatchTmPathTmoSettings(const QString &s) {
    m_settingHolder->setValue(KEY_BATCH_TM_PATH_TMO_SETTINGS, s);
}

QString LuminanceOptions::getBatchTmPathLdrOutput() {
    return m_settingHolder->value(KEY_BATCH_TM_PATH_OUTPUT, QDir::currentPath())
        .toString();
}

void LuminanceOptions::setBatchTmPathLdrOutput(const QString &s) {
    m_settingHolder->setValue(KEY_BATCH_TM_PATH_OUTPUT, s);
}

int LuminanceOptions::getBatchTmNumThreads() {
    return m_settingHolder->value(KEY_BATCH_TM_NUM_THREADS, 1).toInt();
}

void LuminanceOptions::setBatchTmNumThreads(int v) {
    m_settingHolder->setValue(KEY_BATCH_TM_NUM_THREADS, v);
}

namespace {
#ifdef QT_DEBUG
struct PrintTempDir {
    explicit PrintTempDir(const QString &str) : str_(str) {}

    ~PrintTempDir() { qDebug() << "Temporary directory: " << str_; }

   private:
    const QString &str_;
};
#endif  // QT_DEBUG
}

QString LuminanceOptions::getTempDir() {
    QString os_temp_dir_name = QDir::temp().absolutePath();
    QString temp_dir_name =
        m_settingHolder
            ->value(KEY_TEMP_RESULT_PATH, QDir::temp().absolutePath())
            .toString();
#ifdef QT_DEBUG
    PrintTempDir print_temp_dir(temp_dir_name);
#endif
    if (temp_dir_name == os_temp_dir_name) {
        // temporary directory is equal to the OS's
        return temp_dir_name;
    }

    QDir temp_dir(temp_dir_name);
    if (!temp_dir.exists()) {
        // directory doesn't exist!
        qDebug() << "Candidate temporary directory does not exist";
        // reset to OS temporary directory;
        temp_dir_name = os_temp_dir_name;
        remove(KEY_TEMP_RESULT_PATH);
        return temp_dir_name;
    }

    // directory exists...
    // let's check whether I can create a file or not!
    QFile file(temp_dir.filePath(QStringLiteral("test_write.txt")));
    if (!file.open(QIODevice::ReadWrite)) {
        // directory is not writtable
        qDebug() << "Candidate temporary directory is not writtable";
        // reset to OS temporary directory;
        temp_dir_name = os_temp_dir_name;
        remove(KEY_TEMP_RESULT_PATH);
        // return temp_dir_name;
    }

    return temp_dir_name;
}

void LuminanceOptions::setTempDir(const QString &path) {
    m_settingHolder->setValue(KEY_TEMP_RESULT_PATH, path);
}

QString LuminanceOptions::getDefaultPathHdrIn() {
    return m_settingHolder->value(KEY_RECENT_PATH_LOAD_HDR, QDir::currentPath())
        .toString();
}

void LuminanceOptions::setDefaultPathHdrIn(const QString &path) {
    m_settingHolder->setValue(KEY_RECENT_PATH_LOAD_HDR, path);
}

QString LuminanceOptions::getDefaultPathHdrOut() {
    return m_settingHolder->value(KEY_RECENT_PATH_SAVE_HDR, QDir::currentPath())
        .toString();
}

void LuminanceOptions::setDefaultPathHdrOut(const QString &path) {
    m_settingHolder->setValue(KEY_RECENT_PATH_SAVE_HDR, path);
}

QString LuminanceOptions::getDefaultPathLdrIn() {
    return m_settingHolder->value(KEY_RECENT_PATH_LOAD_LDR, QDir::currentPath())
        .toString();
}

void LuminanceOptions::setDefaultPathLdrIn(const QString &path) {
    m_settingHolder->setValue(KEY_RECENT_PATH_LOAD_LDR, path);
}

QString LuminanceOptions::getDefaultPathLdrOut() {
    return m_settingHolder
        ->value(KEY_RECENT_PATH_SAVE_LDR, getDefaultPathHdrOut())
        .toString();
}

void LuminanceOptions::setDefaultPathLdrOut(const QString &path) {
    m_settingHolder->setValue(KEY_RECENT_PATH_SAVE_LDR, path);
}

bool LuminanceOptions::isShowMissingEVsWarning() {
    return m_settingHolder->value(KEY_WIZARD_SHOW_MISSING_EVS_WARNING, true)
        .toBool();
}

void LuminanceOptions::setShowMissingEVsWarning(const bool b) {
    m_settingHolder->setValue(KEY_WIZARD_SHOW_MISSING_EVS_WARNING, b);
}

QString LuminanceOptions::getDefaultPathTmoSettings() {
    return m_settingHolder
        ->value(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS, QDir::currentPath())
        .toString();
}

void LuminanceOptions::setDefaultPathTmoSettings(const QString &path) {
    m_settingHolder->setValue(KEY_RECENT_PATH_LOAD_SAVE_TMO_SETTINGS, path);
}

QString LuminanceOptions::getDefaultResponseCurveFilename() {
    return m_settingHolder
        ->value(KEY_RECENT_RESPONSE_CURVE_FILENAME, "")
        .toString();
}

void LuminanceOptions::setDefaultResponseCurveFilename(const QString &filename) {
    m_settingHolder->setValue(KEY_RECENT_RESPONSE_CURVE_FILENAME, filename);
}

/*
settings->beginGroup(GROUP_EXTERNALTOOLS);
//bug 2001032, remove spurious default QString "-a aligned_" value set by ver
1.9.2
if (!settings->contains(KEY_EXTERNAL_AIS_OPTIONS) ||
settings->value(KEY_EXTERNAL_AIS_OPTIONS).toString()=="-v -a aligned_")
    settings->m_settingHolder->setValue(KEY_EXTERNAL_AIS_OPTIONS, QStringList()
<< "-v" << "-a" << "aligned_");
align_image_stack_options=settings->value(KEY_EXTERNAL_AIS_OPTIONS).toStringList();
settings->endGroup();
*/
QStringList LuminanceOptions::getAlignImageStackOptions() {
    // return m_settingHolder->value(KEY_EXTERNAL_AIS_OPTIONS,
    //                              QStringList() << "-v" <<
    //                              "aligned_").toStringList();
    return m_settingHolder
        ->value(KEY_EXTERNAL_AIS_OPTIONS, QStringList() << QStringLiteral("-v"))
        .toStringList();
}

QStringList LuminanceOptions::sanitizeAISparams(QStringList temp_ais_options,
                                                bool verbose) {
    bool align_opt_was_ok = true;

    // check that we don't have '-a "aligned_"'
    int idx_a = temp_ais_options.indexOf(QStringLiteral("-a"));

    if (idx_a != -1) {
        if (idx_a != temp_ais_options.size() - 1 &&
            !temp_ais_options.at(idx_a + 1).startsWith(QLatin1String("-"))) {
            temp_ais_options.removeAt(idx_a + 1);
        }
        temp_ais_options.removeAt(idx_a);

        align_opt_was_ok = false;
    }

    // check if we have '-v'
    if (temp_ais_options.indexOf(QStringLiteral("-v")) < 0) {
        temp_ais_options.insert(0, QStringLiteral("-v"));
        align_opt_was_ok = false;
    }

    if (verbose && !align_opt_was_ok) {
        QMessageBox::information(
            0, QObject::tr("Option -v -a..."),
            QObject::tr(
                "LumièreHDR requires align_image_stack to be executed "
                "with the \"-v\" and without the \"-a\" options. Command "
                "line options have been corrected."));
    }
    return temp_ais_options;
}

void LuminanceOptions::setAlignImageStackOptions(const QStringList &qstrlist,
                                                 bool verbose) {
    m_settingHolder->setValue(KEY_EXTERNAL_AIS_OPTIONS,
                              sanitizeAISparams(qstrlist, verbose));
}

bool LuminanceOptions::isShowFattalWarning() {
    return m_settingHolder->value(KEY_TMOWARNING_FATTALSMALL, true).toBool();
}

void LuminanceOptions::setShowFattalWarning(const bool b) {
    m_settingHolder->setValue(KEY_TMOWARNING_FATTALSMALL, b);
}

int LuminanceOptions::getMainWindowToolBarMode() {
    return m_settingHolder->value(KEY_TOOLBAR_MODE, Qt::ToolButtonIconOnly)
        .toInt();
}

void LuminanceOptions::setMainWindowToolBarMode(int mode) {
    m_settingHolder->setValue(KEY_TOOLBAR_MODE, mode);
}

bool LuminanceOptions::isPreviewPanelActive() {
    return m_settingHolder->value(KEY_TMOWINDOW_SHOWPREVIEWPANEL, true)
        .toBool();
}

void LuminanceOptions::setPreviewPanelActive(bool status) {
    m_settingHolder->setValue(KEY_TMOWINDOW_SHOWPREVIEWPANEL, status);
}

bool LuminanceOptions::isRealtimePreviewsActive() {
    return m_settingHolder->value(KEY_TMOWINDOW_REALTIMEPREVIEWS_ACTIVE, false)
        .toBool();
}

void LuminanceOptions::setRealtimePreviewsActive(bool status) {
    m_settingHolder->setValue(KEY_TMOWINDOW_REALTIMEPREVIEWS_ACTIVE, status);
}

int LuminanceOptions::getPreviewWidth() {
    return m_settingHolder->value(KEY_TMOWINDOW_PREVIEWS_WIDTH, 400).toInt();
}

void LuminanceOptions::setPreviewWidth(int v) {
    m_settingHolder->setValue(KEY_TMOWINDOW_PREVIEWS_WIDTH, v);
}

QString LuminanceOptions::getCameraProfileFileName() {
    return m_settingHolder->value(KEY_COLOR_CAMERA_PROFILE_FILENAME, "")
        .toString();
}

void LuminanceOptions::setCameraProfileFileName(const QString &fname) {
    m_settingHolder->setValue(KEY_COLOR_CAMERA_PROFILE_FILENAME, fname);
}

QString LuminanceOptions::getMonitorProfileFileName() {
    return m_settingHolder->value(KEY_COLOR_MONITOR_PROFILE_FILENAME)
        .toString();
}

void LuminanceOptions::setMonitorProfileFileName(const QString &fname) {
    m_settingHolder->setValue(KEY_COLOR_MONITOR_PROFILE_FILENAME, fname);
}

QString LuminanceOptions::getPrinterProfileFileName() {
    return m_settingHolder->value(KEY_COLOR_PRINTER_PROFILE_FILENAME)
        .toString();
}

void LuminanceOptions::setPrinterProfileFileName(const QString &fname) {
    m_settingHolder->setValue(KEY_COLOR_PRINTER_PROFILE_FILENAME, fname);
}

int LuminanceOptions::getPreviewPanelMode()  // 0 means on the right, 1 on the
                                             // bottom
{
    return m_settingHolder->value(KEY_PREVIEW_PANEL_MODE).toInt();
}

void LuminanceOptions::setPreviewPanelMode(int mode) {
    m_settingHolder->setValue(KEY_PREVIEW_PANEL_MODE, mode);
}

void LuminanceOptions::setExportDir(const QString &dir) {
    m_settingHolder->setValue(KEY_EXPORT_FILE_PATH, dir);
}

QString LuminanceOptions::getExportDir() {
    QString path = m_settingHolder->value(KEY_EXPORT_FILE_PATH).toString();
    if (path.isEmpty()) {
        path = QDir::homePath();
    } else {
        QDir dir(path);
        if (!dir.exists()) {
            path = QDir::homePath();
        }
    }
    return path;
}
