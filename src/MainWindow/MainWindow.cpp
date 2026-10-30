/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006-2007 Giuseppe Rota
 * Copyright (C) 2011-2013 Davide Anastasia
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
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * Original Work
 *
 * @author Franco Comida <fcomida@users.sourceforge.net>
 * Improvements, bugfixing
 *
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 * Implementation of the SDI functionalities
 * New Central Widget based on QTabWidget
 * Division of the Central Widget using QSplitter
 *
 */

#ifdef QT_DEBUG
#include <QDebug>
#endif
#include <QActionGroup>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QUrl>

#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QMimeData>
#include <QSignalMapper>
#include <QStackedWidget>
#include <QStyle>
#include <QString>
#include <QTextStream>
#include <QDockWidget>
#include <QImageReader>
#include <QRegularExpression>
#include <QTabBar>
#include <QToolBar>
#include <QTimer>
#include <QUndoGroup>
#include <QUndoView>
#include <QVBoxLayout>
#include <QWhatsThis>
#include <QtConcurrentRun>

#include <functional>
#include <vector>
#include <cmath>
#include <algorithm>
#include <memory>

//#include <functional>
#include <boost/bind/bind.hpp>

#include "MainWindow/ui_MainWindow.h"

#include <MainWindow/DnDOption.h>
#include <MainWindow/DonationDialog.h>
#include <MainWindow/MainWindow.h>
#include <MainWindow/UpdateChecker.h>

#include <Fileformat/pfsoutldrimage.h>
#include <Libpfs/frame.h>
#include <Libpfs/manip/copy.h>
#include <Libpfs/manip/cut.h>
#include <Libpfs/manip/gamma_levels.h>
#include <Libpfs/manip/rotate.h>
#include <Libpfs/params.h>

#include <Common/CommonFunctions.h>
#include <Common/archs.h>
#include <Common/config.h>
#include <Common/global.h>

#include <OsIntegration/osintegration.h>

#include <BatchHDR/BatchHDRDialog.h>
#include <BatchTM/BatchTMDialog.h>

#include <TransplantExif/TransplantExifDialog.h>
#include <Viewers/HdrViewer.h>
#include <Libpfs/io/hdrencoding.h>
#ifdef Q_OS_WIN
#include <Viewers/HdrDisplayWindow.h>
#endif
#include "MainWindow/ExportQueueDialog.h"
#include <Libpfs/utils/clamp.h>
#include <Libpfs/utils/chain.h>
#include <Libpfs/colorspace/rgbremapper.h>
#include <Libpfs/colorspace/normalizer.h>
#include <Viewers/LdrViewer.h>
#include <Viewers/LuminanceRangeWidget.h>

#include <UI/ExportToHtmlDialog.h>
#include <UI/GammaAndLevels.h>
#include <UI/ImageQualityDialog.h>
#include <UI/CompareDialog.h>
#include <UI/HistogramPanel.h>
#include "MainWindow/FilmStrip.h"
#include "MainWindow/ExportPage.h"
#include "MainWindow/BatchPage.h"
#include <UI/CommandPalette.h>
#include <UI/LumiereIcons.h>
#include <UI/HomePage.h>
#include <UI/TopBar.h>
#include <UI/SupportedCamerasDialog.h>
#include <UI/TiffModeDialog.h>
#include <UI/UMessageBox.h>

#ifdef HAVE_CFITSIO
#include <UI/FitsImporter.h>
#endif

#include <Core/IOWorker.h>
#include <Core/TMWorker.h>
#include <HdrWizard/AutoAntighosting.h>
#include <HdrWizard/HdrWizard.h>
#include <HdrWizard/WhiteBalance.h>
#include <HelpBrowser/helpbrowser.h>
#include <LibpfsAdditions/formathelper.h>
#include <Preferences/PreferencesDialog.h>
#include <PreviewPanel/PresetStrip.h>
#include <PreviewPanel/PreviewPanel.h>
#include <Projection/ProjectionsDialog.h>
#include <Resize/ResizeDialog.h>
#include <TonemappingPanel/TMOProgressIndicator.h>
#include <TonemappingPanel/TonemappingPanel.h>

namespace {
QString getLdrFileNameFromSaveDialog(const QString &suggestedFileName,
                                     QWidget *parent = 0) {
    QString filetypes = QObject::tr("All LDR formats");
    filetypes += QLatin1String(
        " (*.jpg *.jpeg *.png *.ppm *.pbm *.bmp *.JPG *.JPEG *.PNG *.PPM *.PBM "
        "*.BMP"
#ifdef HAVE_LIBAVIF
        " *.avif *.AVIF"
#endif
#ifdef HAVE_LIBJXL
        " *.jxl *.JXL"
#endif
#ifdef HAVE_LIBWEBP
        " *.webp *.WEBP"
#endif
        ");;");
    filetypes += QLatin1String("JPEG (*.jpg *.jpeg *.JPG *.JPEG);;");
#ifdef HAVE_LIBAVIF
    filetypes += QLatin1String("AVIF (*.avif *.AVIF);;");
#endif
#ifdef HAVE_LIBJXL
    filetypes += QLatin1String("JPEG XL (*.jxl *.JXL);;");
#endif
#ifdef HAVE_LIBWEBP
    filetypes += QLatin1String("WebP (*.webp *.WEBP);;");
#endif
    filetypes += QLatin1String("PNG (*.png *.PNG);;");
    filetypes += QLatin1String("PPM PBM (*.ppm *.pbm *.PPM *.PBM);;");
    filetypes += QLatin1String("BMP (*.bmp *.BMP);;");
    filetypes += QLatin1String("TIFF (*.tif *.tiff *.TIF *.TIFF)");

    QFileInfo qfi(suggestedFileName);
    QString selectedFilter;
    QString outputFilename = QFileDialog::getSaveFileName(
        parent, QObject::tr("Save the LDR image as..."),
        LuminanceOptions().getDefaultPathLdrOut() + QDir::separator() +
            qfi.completeBaseName() + "." + qfi.suffix(),
        filetypes, &selectedFilter);

    if (!outputFilename.isEmpty()) {
        QFileInfo qfi(outputFilename);

        LuminanceOptions().setDefaultPathLdrOut(qfi.path());
    }
    return outputFilename;
}

QString getHdrFileNameFromSaveDialog(const QString &suggestedFileName,
                                     QWidget *parent = 0) {
#ifdef QT_DEBUG
    qDebug() << "MainWindow::getHdrFileNameFromSaveDialog(" << suggestedFileName
             << ")";
#endif

    QStringList validHdrExtensions;
    validHdrExtensions << "exr"
                       << "avif"
                       << "jxl"
                       << "hdr"
                       << "tif"
                       << "tiff"
                       << "pfs";

    QString filetypes = QObject::tr("All HDR formats");
    filetypes += QLatin1String(
        " (*.exr *.EXR *.tiff *.TIFF *.hdr *.HDR *.pic *.PIC *.pfs *.PFS);;");
    filetypes += QLatin1String("OpenEXR (*.exr *.EXR);;");
    filetypes += QLatin1String("HDR TIFF (*.tiff *.tif *.TIFF *.TIT);;");
    filetypes += QLatin1String("Radiance RGBE (*.hdr *.pic *.HDR *.PIC);;");
    filetypes += QLatin1String("PFS Stream (*.pfs *.PFS);;");
#ifdef HAVE_LIBAVIF
    filetypes += QLatin1String("AVIF (*.avif *.AVIF);;");
#endif
#ifdef HAVE_LIBJXL
    filetypes += QLatin1String("JPEG XL (*.jxl *.JXL);;");
#endif

    QFileInfo qfi(suggestedFileName);

    QString suffix = qfi.suffix();

    if (suffix.isEmpty()) {
            suffix = "exr";
    }

    if (!validHdrExtensions.contains(suffix, Qt::CaseInsensitive)) {
        suffix = "exr";
    }

    QString outputFilename = QFileDialog::getSaveFileName(
        parent, QObject::tr("Save the HDR image as..."),
        LuminanceOptions().getDefaultPathLdrIn() + QDir::separator() +
            qfi.completeBaseName() + "." + suffix,
        filetypes);

    if (!outputFilename.isEmpty()) {
        QFileInfo qfi(outputFilename);

        LuminanceOptions().setDefaultPathHdrOut(qfi.path());
    }
    return outputFilename;
}

void getCropCoords(GenericViewer *gv, int &x_ul, int &y_ul, int &x_br,
                   int &y_br) {
    assert(gv != nullptr);

    QRect cropRect = gv->getSelectionRect().normalized();
    cropRect.getCoords(&x_ul, &y_ul, &x_br, &y_br);
}

GenericViewer::ViewerMode getCurrentViewerMode(
    const QTabWidget &curr_tab_widget) {
    if (curr_tab_widget.count() <= 0) {
        return GenericViewer::FIT_WINDOW;
    } else {
        GenericViewer *g_v = qobject_cast<GenericViewer *>(curr_tab_widget.currentWidget());
        return g_v->getViewerMode();
    }
}
}

// static members!
int MainWindow::sm_NumMainWindows = 0;
int MainWindow::sm_counter = 0;
QMap<int, MainWindow *> MainWindow::sm_mainWindowMap =
    QMap<int, MainWindow *>();
QScopedPointer<UpdateChecker> MainWindow::sm_updateChecker;
HelpBrowser *MainWindow::sm_helpBrowser = nullptr;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_Ui(new Ui::MainWindow),
      m_isFullscreenViewer(false),
      m_exportQueueSize(0),
      m_interpolationMethod(BilinearInterp),
      m_firstWindow(0),
      m_winId(0) {
    init();
}

MainWindow::MainWindow(pfs::Frame *curr_frame, const QString &new_file,
                       const QStringList &inputFileNames, bool needSaving,
                       QWidget *parent)
    : QMainWindow(parent),
      m_Ui(new Ui::MainWindow),
      m_isFullscreenViewer(false),
      m_exportQueueSize(0),
      m_interpolationMethod(BilinearInterp),
      m_firstWindow(0),
      m_winId(0) {
    init();

    emit load_success(curr_frame, new_file, inputFileNames, needSaving);
}

MainWindow::~MainWindow() {
}

void MainWindow::init() {
    sm_NumMainWindows++;
    sm_counter++;
    m_winId = sm_counter;
    sm_mainWindowMap.insert(m_winId, this);
    m_firstWindow = 0;

    num_ldr_generated = 0;
    curr_num_ldr_open = 0;
    splash = 0;
    m_processingAWB = false;

    if (sm_NumMainWindows == 1) {
        // Register symbols on the first activation!
        qRegisterMetaType<QImage>("QImage");
        qRegisterMetaType<pfs::Frame *>("pfs::Frame*");
        qRegisterMetaType<TonemappingOptions>("TonemappingOptions");
        qRegisterMetaType<TonemappingOptions *>("TonemappingOptions*");
        qRegisterMetaType<HdrViewer *>("HdrViewer*");
        qRegisterMetaType<LdrViewer *>("LdrViewer*");
        qRegisterMetaType<GenericViewer *>("GenericViewer*");
        qRegisterMetaType<QVector<float>>("QVector<float>");
        qRegisterMetaType<pfs::Params>("pfs::Params");
        qRegisterMetaType<pfs::Params *>("pfs::Params*");
        qRegisterMetaType<ExportJobPtr>("ExportJobPtr");
        qRegisterMetaType<InterpolationMethod>("InterpolationMethod");
    }

    createUI();
    loadOptions();
    createMenus();
    createToolBar();
    createCentralWidget();
    createShell();
    createStatusBar();
    setupIO();
    setupTM();
    setupQueue();
    createConnections();

    OsIntegration::getInstance().init(this);

    if (sm_NumMainWindows == 1) {
        if (OsIntegration::getInstance().isRunningOnSameCpuPlatform()) {
            sm_updateChecker.reset(new UpdateChecker(this));
            connect(sm_updateChecker.data(), &UpdateChecker::updateAvailable,
                    this, &MainWindow::onUpdateAvailable);

            m_firstWindow = 2;
        } else
            m_firstWindow = 1;
    }
}

void MainWindow::createUI() {
    m_Ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    restoreState(LuminanceOptions()
                     .value(QStringLiteral("MainWindowState"))
                     .toByteArray());
    restoreGeometry(LuminanceOptions()
                        .value(QStringLiteral("MainWindowGeometry"))
                        .toByteArray());

    if (isFullScreen()) m_Ui->actionShow_Full_Screen->setChecked(true);

    setAcceptDrops(true);
    setWindowModified(false);
    setWindowTitle(QString(LUMINANCEDISPLAYNAME " " LUMINANCEVERSION
                           " [*]"));  // + "  " + g_GIT_SHA1);
}

void MainWindow::createCentralWidget() {
    // Central Widget Area
    m_centralwidget_splitter = new QSplitter;  //(this);
    m_bottom_splitter = new QSplitter;         //(this);
    m_bottom_splitter->setOrientation(Qt::Vertical);

    // the Start page and the image workspace take turns in the window
    m_startPage = new HomePage;
    m_workspace = new QStackedWidget;
    m_workspace->addWidget(m_startPage);
    m_workspace->addWidget(m_centralwidget_splitter);
    m_exportPage = new ExportPage;
    m_workspace->addWidget(m_exportPage);
    connect(m_exportPage, &ExportPage::exportRequested, this,
            &MainWindow::exportFromPage);
    connect(m_exportPage, &ExportPage::queueRequested, this,
            &MainWindow::queueFromPage);
    setCentralWidget(m_workspace);

    m_PreviewPanel = new PreviewPanel();

    // create tonemapping panel
    m_tonemapPanel = new TonemappingPanel(
        sm_NumMainWindows, m_PreviewPanel);  //(m_centralwidget_splitter);
    connect(m_tonemapPanel, &TonemappingPanel::startTonemapping, this,
            [this](TonemappingOptions *) { m_resultSaved = false; });

    connect(m_Ui->actionRealtimePreviews, &QAction::toggled, m_tonemapPanel,
            &TonemappingPanel::setRealtimePreviews);
    connect(m_Ui->actionRealtimePreviews, &QAction::toggled, this,
            &MainWindow::setRealtimePreviewsActive);

    m_tabwidget = new QTabWidget;  //(m_centralwidget_splitter);

    m_tabwidget->setDocumentMode(true);
    m_tabwidget->setTabsClosable(true);
    // equal tabs: long file names end in "..." (full name in the tooltip)
    m_tabwidget->setElideMode(Qt::ElideRight);
    m_tabwidget->setUsesScrollButtons(true);
    m_tabwidget->tabBar()->setExpanding(false);
    applyTabStyle();

    // Develop: the looks on the left, the image in the middle, the
    // histogram and the settings of the look on the right
    m_centralwidget_splitter->setObjectName(QStringLiteral("DevelopSplitter"));
    m_centralwidget_splitter->setHandleWidth(1);
    m_centralwidget_splitter->setChildrenCollapsible(false);
    m_centralwidget_splitter->addWidget(createLooksPanel());
    m_centralwidget_splitter->addWidget(m_bottom_splitter);
    QWidget *inspector = new QWidget;
    inspector->setObjectName(QStringLiteral("Inspector"));
    inspector->setAttribute(Qt::WA_StyledBackground);
    QVBoxLayout *inspectorLayout = new QVBoxLayout(inspector);
    inspectorLayout->setContentsMargins(0, 0, 0, 0);
    inspectorLayout->setSpacing(0);
    m_histogramPanel->setContentsMargins(12, 10, 12, 4);
    inspectorLayout->addWidget(m_histogramPanel);
    inspectorLayout->addWidget(m_tonemapPanel, 1);
    // scrolls on small screens instead of making the window taller
    QScrollArea *inspectorScroll = new QScrollArea;
    inspectorScroll->setObjectName(QStringLiteral("InspectorScroll"));
    inspectorScroll->setWidget(inspector);
    inspectorScroll->setWidgetResizable(true);
    inspectorScroll->setFrameShape(QFrame::NoFrame);
    inspectorScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    inspectorScroll->setMinimumWidth(inspector->minimumSizeHint().width() + 12);
    m_centralwidget_splitter->addWidget(inspectorScroll);

    // the image tools sit right above the image they work on
    removeToolBar(m_Ui->toolBar);
    m_Ui->toolBar->setObjectName(QStringLiteral("ImageToolBar"));
    m_Ui->toolBar->setMovable(false);
    m_Ui->toolBar->setIconSize(QSize(20, 20));
    QWidget *imageColumn = new QWidget;
    QVBoxLayout *imageLayout = new QVBoxLayout(imageColumn);
    imageLayout->setContentsMargins(0, 0, 0, 0);
    imageLayout->setSpacing(0);
    imageLayout->addWidget(m_Ui->toolBar);
    imageLayout->addWidget(m_tabwidget, 1);
    // the open images as a film strip under the picture instead of tabs
    m_tabwidget->tabBar()->hide();
    m_filmStrip = new FilmStrip(m_tabwidget);
    connect(m_filmStrip, &FilmStrip::closeRequested, this,
            &MainWindow::removeTab);
    imageLayout->addWidget(m_filmStrip);
    m_Ui->toolBar->show();
    m_bottom_splitter->addWidget(imageColumn);

    m_centralwidget_splitter->setStretchFactor(0, 0);
    m_centralwidget_splitter->setStretchFactor(1, 1);
    m_centralwidget_splitter->setStretchFactor(2, 0);

    // create preview panel
    m_PreviewscrollArea = new QScrollArea;
    m_PreviewscrollArea->setWidgetResizable(true);
    m_PreviewscrollArea->setWidget(m_PreviewPanel);

    // the operator previews are replaced by the looks; the panel stays for
    // the tone mapping panel's own use
    m_PreviewscrollArea->setParent(m_bottom_splitter);
    m_PreviewscrollArea->hide();

    connect(m_presetStrip, &PresetStrip::presetClicked, m_tonemapPanel,
            &TonemappingPanel::applyPresetFile);
    connect(m_tonemapPanel, &TonemappingPanel::currentPresetChanged,
            m_presetStrip, &PresetStrip::setCurrent);
    connect(m_tonemapPanel, &TonemappingPanel::presetsChanged, this, [this]() {
        m_presetStrip->setPresets(m_tonemapPanel->presets());
        if (tm_status.curr_tm_frame)
            m_presetStrip->updatePreviews(tm_status.curr_tm_frame->getFrame());
    });
    connect(this, &MainWindow::updatedHDR, m_presetStrip,
            &PresetStrip::updatePreviews);

    connect(m_tabwidget, &QTabWidget::tabCloseRequested, this,
            &MainWindow::removeTab);
    connect(m_tabwidget, &QTabWidget::currentChanged, this,
            &MainWindow::updateActions);
    connect(m_tabwidget, &QTabWidget::currentChanged, this,
            &MainWindow::updateSoftProofing);
    connect(m_tonemapPanel, &TonemappingPanel::startTonemapping, this,
            &MainWindow::tonemapImage);
    connect(m_tonemapPanel, &TonemappingPanel::startExport, this,
            &MainWindow::exportImage);
    connect(m_tonemapPanel, &TonemappingPanel::uneditedRequested, this,
            &MainWindow::showUnedited);
    connect(this, &MainWindow::updatedHDR, m_tonemapPanel,
            &TonemappingPanel::updatedHDR);
    connect(this, &QObject::destroyed, m_PreviewPanel, &QObject::deleteLater);

    const QByteArray splitterState =
        LuminanceOptions()
            .value(QStringLiteral("DevelopSplitterState"))
            .toByteArray();
    if (splitterState.isEmpty())
        // first start: looks and settings at a comfortable width, the rest
        // for the image
        m_centralwidget_splitter->setSizes({244, 1000, 400});
    else
        m_centralwidget_splitter->restoreState(splitterState);
    m_centralwidget_splitter->restoreGeometry(
        LuminanceOptions()
            .value(QStringLiteral("MainWindowSplitterGeometry"))
            .toByteArray());
    m_bottom_splitter->restoreState(
        LuminanceOptions()
            .value(QStringLiteral("MainWindowBottomSplitterState"))
            .toByteArray());
    m_bottom_splitter->restoreGeometry(
        LuminanceOptions()
            .value(QStringLiteral("MainWindowBottomSplitterGeometry"))
            .toByteArray());

    // the darkest colour of the scheme around the image
    m_tabwidget->setBackgroundRole(QPalette::Dark);
    m_tabwidget->setAutoFillBackground(true);

    // replace with ->tabBar() one day, or subclass
    QTabBar *tabBar = m_tabwidget->findChild<QTabBar *>(
        QStringLiteral("qt_tabwidget_tabbar"));
    tabBar->setAutoFillBackground(true);
    tabBar->setBackgroundRole(QPalette::Window);

    m_PreviewscrollArea->hide();
}

void MainWindow::createShell() {
    m_topBar = new TopBar;
    QList<QMenu *> menus;
    for (QAction *a : menuBar()->actions())
        if (a->menu()) menus << a->menu();
    m_topBar->setMenus(menus);

    // the bar takes the place of the menu bar; its menus stay reachable
    // under the menu button, and their shortcuts keep working through
    // the window itself
#ifndef Q_OS_MACOS
    menuBar()->hide();
    std::function<void(QMenu *)> addShortcuts = [&](QMenu *menu) {
        for (QAction *a : menu->actions()) {
            if (a->menu())
                addShortcuts(a->menu());
            else if (!a->shortcuts().isEmpty())
                addAction(a);
        }
    };
    for (QMenu *m : menus) addShortcuts(m);
#endif
    QToolBar *bar = new QToolBar(this);
    bar->setObjectName(QStringLiteral("TopBarToolBar"));
    bar->setMovable(false);
    bar->setFloatable(false);
    bar->setContextMenuPolicy(Qt::PreventContextMenu);
    bar->toggleViewAction()->setVisible(false);
    bar->setStyleSheet(
        QStringLiteral("QToolBar { border: none; padding: 0; margin: 0; }"));
    bar->addWidget(m_topBar);
    addToolBar(Qt::TopToolBarArea, bar);

    connect(m_topBar, &TopBar::modeRequested, this, [this](TopBar::Mode mode) {
        switch (mode) {
            case TopBar::Start:
                showWorkspace(false);
                break;
            case TopBar::Develop:
                showWorkspace(true);
                break;
            case TopBar::Merge:
                createNewHdr(QStringList());
                break;
            case TopBar::Export:
                showExportWorkspace();
                break;
        }
    });
    connect(m_topBar, &TopBar::searchRequested, this,
            &MainWindow::openCommandPalette);
    connect(m_topBar, &TopBar::batchRequested, this,
            [this]() { showBatchPage(-1); });
    connect(m_topBar, &TopBar::settingsRequested, m_Ui->OptionsAction,
            &QAction::trigger);

    QAction *search = new QAction(tr("Search commands..."), this);
    search->setShortcut(QKeySequence(QStringLiteral("Ctrl+K")));
    connect(search, &QAction::triggered, this,
            &MainWindow::openCommandPalette);
    m_Ui->menuHelp->insertAction(m_Ui->menuHelp->actions().value(0), search);
    addAction(search);

    m_startPage->setCardActions(m_Ui->fileNewAction, m_Ui->fileOpenAction,
                                m_Ui->actionBatch_HDR);
    m_startPage->setLinkActions({m_Ui->actionFits_Importer,
                                 m_Ui->actionSupported_Cameras,
                                 m_Ui->actionOnline_Documentation});
    connect(m_startPage, &HomePage::recentFileRequested, this,
            [this](const QString &file) {
                QMetaObject::invokeMethod(m_IOWorker, "read_hdr_frame",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, file));
            });
    m_startPage->setRecentFiles(
        LuminanceOptions().value(KEY_RECENT_FILES).toStringList());

    // the image toolbar: view, edits and checks, in the order of the work
    QToolBar *tools = m_Ui->toolBar;
    QAction *first = tools->actions().value(0);
    const QList<QAction *> edits = {m_Ui->menuEdit->actions().value(0),
                                    m_Ui->menuEdit->actions().value(1)};
    tools->insertActions(first, edits);
    tools->insertSeparator(first);
    tools->addSeparator();
    tools->addAction(m_Ui->rotateccw);
    tools->addAction(m_Ui->rotatecw);
    tools->addAction(m_Ui->cropToSelectionAction);
    tools->addAction(m_Ui->action_Projective_Transformation);
    tools->addAction(m_Ui->actionResizeHDR);
    tools->addSeparator();
    // before/after in the picture itself: the HDR as it came, left of a
    // line that can be dragged
    m_beforeAfterAct = new QAction(tr("&Before / after"), this);
    m_beforeAfterAct->setCheckable(true);
    m_beforeAfterAct->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+B")));
    m_beforeAfterAct->setToolTip(
        tr("Before / after: the unedited HDR left of a line you can drag"));
    connect(m_beforeAfterAct, &QAction::toggled, this,
            &MainWindow::updateBeforeAfter);
    m_Ui->menuView->addAction(m_beforeAfterAct);
    addAction(m_beforeAfterAct);
    tools->addAction(m_beforeAfterAct);
    tools->addAction(m_compareAct);
#ifdef Q_OS_WIN
    m_hdrPreviewAct = new QAction(tr("HDR preview (HDR screens)..."), this);
    m_hdrPreviewAct->setToolTip(
        tr("HDR preview: shows the picture with real HDR brightness on an HDR "
           "screen, such as an HDR TV or monitor with \"Use HDR\" switched "
           "on in Windows"));
    connect(m_hdrPreviewAct, &QAction::triggered, this,
            &MainWindow::showHdrPreview);
    m_Ui->menuView->addAction(m_hdrPreviewAct);
    tools->addAction(m_hdrPreviewAct);
#endif
    m_lineIcons = {{edits.value(0), QStringLiteral("undo")},
                   {edits.value(1), QStringLiteral("redo")},
                   {m_Ui->fitToWindowAct, QStringLiteral("fit")},
                   {m_Ui->actionFill_to_Window, QStringLiteral("fill")},
                   {m_Ui->normalSizeAct, QStringLiteral("actual")},
                   {m_Ui->actionFix_Histogram, QStringLiteral("levels")},
                   {m_Ui->actionWhite_Balance, QStringLiteral("eyedropper")},
                   {m_Ui->actionSoft_Proofing, QStringLiteral("proof")},
                   {m_Ui->actionGamut_Check, QStringLiteral("gamut")},
                   {m_Ui->rotateccw, QStringLiteral("rotate-left")},
                   {m_Ui->rotatecw, QStringLiteral("rotate-right")},
                   {m_Ui->cropToSelectionAction, QStringLiteral("crop")},
                   {m_Ui->action_Projective_Transformation,
                    QStringLiteral("perspective")},
                   {m_Ui->actionResizeHDR, QStringLiteral("resize")},
                   {m_compareAct, QStringLiteral("compare")},
                   {m_beforeAfterAct, QStringLiteral("split")},
                   {m_hdrPreviewAct, QStringLiteral("hdr")},
                   {m_Ui->fileNewAction, QStringLiteral("layers")},
                   {m_Ui->fileOpenAction, QStringLiteral("folder")},
                   {m_Ui->fileSaveAsAction, QStringLiteral("download")},
                   {m_Ui->actionBatch_HDR, QStringLiteral("batch")},
                   {m_Ui->OptionsAction, QStringLiteral("settings")}};
    // the small "back to default" buttons beside the sliders
    for (QToolButton *b : m_tonemapPanel->findChildren<QToolButton *>()) {
        if (b->objectName().endsWith(QLatin1String("default"))) {
            b->setProperty("reset", true);
            b->setAutoRaise(true);
            b->setIconSize(QSize(16, 16));
            m_lineIconButtons << qMakePair(static_cast<QAbstractButton *>(b),
                                           QStringLiteral("reset"));
        }
    }
    if (QToolButton *add = m_tonemapPanel->findChild<QToolButton *>(
            QStringLiteral("addCustomSizeButton"))) {
        add->setAutoRaise(true);
        m_lineIconButtons << qMakePair(static_cast<QAbstractButton *>(add),
                                       QStringLiteral("plus"));
    }
    refreshLineIcons();
    showWorkspace(false);
}

void MainWindow::showHdrPreview() {
#ifdef Q_OS_WIN
    GenericViewer *shown =
        qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());
    if (!shown) return;
    if (shown->isHDR()) {
        // the HDR with real HDR brightness, on an HDR screen
        (new HdrDisplayWindow(shown->getFrameShared(), this))->show();
        return;
    }
    // the edit as an HDR screen shows its Ultra HDR file
    HdrViewer *hdr = qobject_cast<HdrViewer *>(tm_status.curr_tm_frame);
    if (!hdr || !shown->getFrame()) return;
    const pfs::Frame &result = *shown->getFrame();
    const std::vector<float> nits =
        pfs::io::hdr::resultToNits2020(result, *hdr->getFrame());
    auto picture = std::make_shared<pfs::Frame>(result.getWidth(),
                                                result.getHeight());
    pfs::Channel *r, *g, *b;
    picture->createXYZChannels(r, g, b);
    for (size_t i = 0; i < r->size(); ++i) {
        float c[3] = {nits[i * 3], nits[i * 3 + 1], nits[i * 3 + 2]};
        pfs::io::hdr::rec2020To709(c[0], c[1], c[2]);
        (*r)(i) = std::max(c[0], 0.f);
        (*g)(i) = std::max(c[1], 0.f);
        (*b)(i) = std::max(c[2], 0.f);
    }
    (new HdrDisplayWindow(picture, this, true))->show();
#endif
}

void MainWindow::updateBeforeAfter() {
    GenericViewer *shown =
        qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());
    GenericViewer *hdr = tm_status.curr_tm_frame;
    const bool possible = shown && !shown->isHDR() && hdr &&
                          m_tabwidget->indexOf(hdr) >= 0;
    m_beforeAfterAct->setEnabled(possible);
    for (int i = 0; i < m_tabwidget->count(); ++i) {
        GenericViewer *v = qobject_cast<GenericViewer *>(m_tabwidget->widget(i));
        if (!v) continue;
        if (v == shown && possible && m_beforeAfterAct->isChecked())
            v->setBeforePicture(hdr->picture());
        else if (v->showsBeforePicture())
            v->setBeforePicture(QPixmap());
    }
}

void MainWindow::refreshLineIcons() {
    for (const auto &entry : std::as_const(m_lineIcons))
        if (entry.first) entry.first->setIcon(LumiereIcons::icon(entry.second));
    for (const auto &entry : std::as_const(m_lineIconButtons))
        entry.first->setIcon(LumiereIcons::icon(entry.second));
}

void MainWindow::showWorkspace(bool images) {
    if (images && m_tabwidget->count() == 0) images = false;
    m_workspace->setCurrentWidget(images
                                      ? static_cast<QWidget *>(
                                            m_centralwidget_splitter)
                                      : m_startPage);
    m_topBar->setMode(images ? TopBar::Develop : TopBar::Start);
}

void MainWindow::openCommandPalette() {
    QList<QMenu *> menus;
    for (QAction *a : menuBar()->actions())
        if (a->menu()) menus << a->menu();
    CommandPalette *palette = new CommandPalette(menus, this);
    palette->show();
}

QWidget *MainWindow::createLooksPanel() {
    m_looksPanel = new QWidget;
    m_looksPanel->setObjectName(QStringLiteral("LooksPanel"));
    m_looksPanel->setAttribute(Qt::WA_StyledBackground);
    m_looksPanel->setMinimumWidth(220);

    // Looks | History, as a segmented switch
    QWidget *segment = new QWidget;
    segment->setObjectName(QStringLiteral("Segment"));
    QHBoxLayout *segmentLayout = new QHBoxLayout(segment);
    segmentLayout->setContentsMargins(3, 3, 3, 3);
    segmentLayout->setSpacing(2);
    m_looksSwitch = new QButtonGroup(this);
    const QStringList names = {tr("Looks"), tr("History")};
    for (int i = 0; i < names.size(); ++i) {
        QToolButton *b = new QToolButton;
        b->setObjectName(QStringLiteral("SegmentButton"));
        b->setText(names[i]);
        b->setCheckable(true);
        b->setChecked(i == 0);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_looksSwitch->addButton(b, i);
        segmentLayout->addWidget(b);
    }

    m_presetStrip = new PresetStrip;
    m_presetStrip->setPresets(m_tonemapPanel->presets());
    m_presetStrip->setColumns(2, QSize(100, 66));

    // own looks: save the current one, or work with settings files
    QWidget *own = new QWidget;
    own->setObjectName(QStringLiteral("OwnLooks"));
    QVBoxLayout *ownLayout = new QVBoxLayout(own);
    ownLayout->setContentsMargins(14, 12, 14, 14);
    ownLayout->setSpacing(8);
    QLabel *ownLabel = new QLabel(tr("MY LOOKS"));
    ownLabel->setObjectName(QStringLiteral("SectionLabel"));
    QPushButton *save = new QPushButton(LumiereIcons::icon(QStringLiteral("save")),
                                        tr("Save current look"));
    save->setObjectName(QStringLiteral("QuietButton"));
    connect(save, &QPushButton::clicked, m_tonemapPanel,
            &TonemappingPanel::saveCurrentLook);
    m_lineIconButtons << qMakePair(static_cast<QAbstractButton *>(save),
                                   QStringLiteral("save"));
    QLabel *files = new QLabel(
        tr("As a file: <a href=\"load\">load</a> · <a href=\"save\">save</a> "
           "· <a href=\"folder\">open folder</a>"));
    files->setObjectName(QStringLiteral("HintLabel"));
    files->setWordWrap(true);
    connect(files, &QLabel::linkActivated, this, [this](const QString &link) {
        if (link == QLatin1String("load"))
            m_tonemapPanel->loadLookFile();
        else if (link == QLatin1String("save"))
            m_tonemapPanel->saveLookFile();
        else {
            const QString folder = TonemappingPanel::ownPresetFolder();
            QDir().mkpath(folder);
            QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
        }
    });
    ownLayout->addWidget(ownLabel);
    ownLayout->addWidget(save, 0, Qt::AlignLeft);
    ownLayout->addWidget(files);

    QWidget *looks = new QWidget;
    QVBoxLayout *looksLayout = new QVBoxLayout(looks);
    looksLayout->setContentsMargins(0, 0, 0, 0);
    looksLayout->setSpacing(0);
    looksLayout->addWidget(m_presetStrip, 1);
    looksLayout->addWidget(own);

    m_looksPages = new QStackedWidget;
    m_looksPages->addWidget(looks);
    m_looksPages->addWidget(m_historyView);
    connect(m_looksSwitch, &QButtonGroup::idClicked, m_looksPages,
            &QStackedWidget::setCurrentIndex);

    QVBoxLayout *layout = new QVBoxLayout(m_looksPanel);
    layout->setContentsMargins(0, 12, 0, 0);
    layout->setSpacing(4);
    QHBoxLayout *top = new QHBoxLayout;
    top->setContentsMargins(12, 0, 12, 0);
    top->addWidget(segment);
    layout->addLayout(top);
    layout->addWidget(m_looksPages, 1);
    return m_looksPanel;
}

void MainWindow::createToolBar() {
    // main toolbars setup
    QActionGroup *toolBarOptsGroup = new QActionGroup(this);
    toolBarOptsGroup->addAction(m_Ui->actionText_Under_Icons);
    toolBarOptsGroup->addAction(m_Ui->actionIcons_Only);
    toolBarOptsGroup->addAction(m_Ui->actionText_Alongside_Icons);
    toolBarOptsGroup->addAction(m_Ui->actionText_Only);
    m_Ui->menuToolbars->addAction(m_Ui->toolBar->toggleViewAction());

    // undo/redo of HDR edits for whichever image is shown
    m_undoGroup = new QUndoGroup(this);
    QAction *undo = m_undoGroup->createUndoAction(this, tr("&Undo"));
    undo->setShortcut(QKeySequence::Undo);
    undo->setIcon(QIcon::fromTheme(QStringLiteral("edit-undo")));
    QAction *redo = m_undoGroup->createRedoAction(this, tr("&Redo"));
    redo->setShortcuts(QKeySequence::Redo);
    redo->setIcon(QIcon::fromTheme(QStringLiteral("edit-redo")));
    QAction *first = m_Ui->menuEdit->actions().value(0);
    m_Ui->menuEdit->insertAction(first, undo);
    m_Ui->menuEdit->insertAction(first, redo);
    m_Ui->menuEdit->insertSeparator(first);

    // the history and the histogram live in the Develop workspace
    m_historyView = new QUndoView(m_undoGroup);
    m_historyView->setObjectName(QStringLiteral("HistoryView"));
    m_historyView->setEmptyLabel(tr("Original image"));
    m_historyView->setFrameShape(QFrame::NoFrame);
    QAction *showHistory = new QAction(tr("&History"), this);
    connect(showHistory, &QAction::triggered, this, [this]() {
        m_looksPages->setCurrentIndex(1);
        m_looksSwitch->button(1)->setChecked(true);
    });
    m_Ui->menuView->addAction(showHistory);

    m_histogramPanel = new HistogramPanel;
    // coalesce bursts of changes (dragging a slider) into one update
    m_histogramTimer = new QTimer(this);
    m_histogramTimer->setSingleShot(true);
    m_histogramTimer->setInterval(150);
    connect(m_histogramTimer, &QTimer::timeout, this,
            &MainWindow::updateHistogram);
    QAction *showHistogram = new QAction(tr("Histo&gram"), this);
    showHistogram->setCheckable(true);
    showHistogram->setChecked(
        LuminanceOptions().value(QStringLiteral("ShowHistogram"), true).toBool());
    m_histogramPanel->setVisible(showHistogram->isChecked());
    connect(showHistogram, &QAction::toggled, this, [this](bool shown) {
        LuminanceOptions().setValue(QStringLiteral("ShowHistogram"), shown);
        m_histogramPanel->setVisible(shown);
        // only spend time on it while it is shown
        if (shown) updateHistogram();
    });
    m_Ui->menuView->addAction(showHistogram);

    // before/after: any two open images, split by a draggable line
    QAction *compare = m_compareAct =
        new QAction(tr("&Compare images..."), this);
    compare->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+C")));
    compare->setStatusTip(
        tr("Compare two open images, for example the HDR and a result"));
    connect(compare, &QAction::triggered, this, [this]() {
        QList<QPair<QString, QImage>> images;
        int hdrIndex = -1, firstResult = -1;
        for (int i = 0; i < m_tabwidget->count(); ++i) {
            GenericViewer *v =
                qobject_cast<GenericViewer *>(m_tabwidget->widget(i));
            QString name = m_tabwidget->tabText(i);
            if (name.startsWith(QLatin1String("(*) "))) name.remove(0, 4);
            QString label;
            if (v && v->isHDR()) {
                label = tr("HDR image: %1").arg(name);
                if (v == tm_status.curr_tm_frame) hdrIndex = images.size();
            } else {
                TonemappingOptions *opts =
                    v ? v->getTonemappingOptions() : nullptr;
                const QString op =
                    opts ? opts->getCaption(false, QStringLiteral("|"))
                               .section(QLatin1Char('|'), 0, 0)
                               .remove(QLatin1Char(':'))
                         : QString();
                label = op.isEmpty() ? tr("Result: %1").arg(name)
                                     : tr("Result: %1 (%2)").arg(name, op);
                if (firstResult < 0 || i == m_tabwidget->currentIndex())
                    firstResult = images.size();
            }
            images.append(qMakePair(label, v ? v->getQImage() : QImage()));
        }

        // the photos the HDR was merged from, as the real "before"
        const int firstOriginal = images.size();
        int skipped = 0;
        for (const QString &file : m_inputFilesName) {
            QImageReader reader(file);
            reader.setAutoTransform(true);
            const QSize size = reader.size();
            if (size.isValid() && qMax(size.width(), size.height()) > 2400)
                reader.setScaledSize(
                    size.scaled(2400, 2400, Qt::KeepAspectRatio));
            const QImage image = reader.read();
            if (image.isNull()) {
                // Qt reads JPEG/PNG/TIFF; raw files need a converter first
                static const QStringList raw = {
                    "cr2", "cr3", "crw", "nef", "nrw", "arw", "srf", "sr2",
                    "dng", "raf", "orf", "rw2", "pef", "srw", "x3f", "3fr"};
                if (raw.contains(QFileInfo(file).suffix().toLower()))
                    ++skipped;
                continue;
            }
            images.append(qMakePair(
                tr("Original: %1").arg(QFileInfo(file).fileName()), image));
        }
        const int originals = images.size() - firstOriginal;

        if (images.size() < 2) {
            QMessageBox::information(
                this, tr("Compare images"),
                tr("Open or create at least two images to compare them, "
                   "for example an HDR image and a tone mapped result."));
            return;
        }
        // before: the middle exposure (or the HDR); after: the result shown
        int left = originals > 0 ? firstOriginal + originals / 2 : hdrIndex;
        int right =
            firstResult >= 0 ? firstResult : m_tabwidget->currentIndex();
        if (left < 0 || left == right) left = right == 0 ? 1 : 0;
        const QString note =
            skipped > 0 ? tr("%n raw original(s) cannot be shown here yet.",
                             "", skipped)
                        : QString();
        CompareDialog *dialog =
            new CompareDialog(images, left, right, note, this);
        dialog->show();
    });
    m_Ui->menuView->addAction(compare);

    connect(m_Ui->actionText_Under_Icons, &QAction::triggered, this,
            &MainWindow::Text_Under_Icons);
    connect(m_Ui->actionIcons_Only, &QAction::triggered, this,
            &MainWindow::Icons_Only);
    connect(m_Ui->actionText_Alongside_Icons, &QAction::triggered, this,
            &MainWindow::Text_Alongside_Icons);
    connect(m_Ui->actionText_Only, &QAction::triggered, this,
            &MainWindow::Text_Only);

    // Preview Panel
    QActionGroup *previewPanelOptsGroup = new QActionGroup(this);
    previewPanelOptsGroup->addAction(m_Ui->actionShow_on_the_right);
    previewPanelOptsGroup->addAction(m_Ui->actionShow_on_the_bottom);
    connect(m_Ui->actionShow_on_the_right, &QAction::triggered, this,
            &MainWindow::showPreviewsOnTheRight);
    connect(m_Ui->actionShow_on_the_bottom, &QAction::triggered, this,
            &MainWindow::showPreviewsOnTheBottom);
}

void MainWindow::createMenus() {
    // icons (missing icons in some icon-theme)
    if (!QIcon::hasThemeIcon(QStringLiteral("document-save-all")))
        m_Ui->fileSaveAllAction->setIcon(
            QIcon(":/program-icons/document-save-all"));
    if (!QIcon::hasThemeIcon(QStringLiteral("zoom")))
        m_Ui->fitToWindowAct->setIcon(QIcon(":/program-icons/zoom"));
    if (!QIcon::hasThemeIcon(QStringLiteral("whitebalance")))
        m_Ui->actionWhite_Balance->setIcon(
            QIcon(":/program-icons/whitebalance"));
    if (!QIcon::hasThemeIcon(QStringLiteral("transform-crop")))
        m_Ui->cropToSelectionAction->setIcon(
            QIcon(":/program-icons/transform-crop"));
    if (!QIcon::hasThemeIcon(QStringLiteral("edit-selection-none")))
        m_Ui->removeSelectionAction->setIcon(
            QIcon(":/program-icons/edit-select-none"));
    if (!QIcon::hasThemeIcon(QStringLiteral("globe")))
        m_Ui->actionExportToHTML->setIcon(QIcon(":/program-icons/globe"));
    if (!QIcon::hasThemeIcon(QStringLiteral("selection-move-to-layer-below")))
        m_Ui->Transplant_Exif_Data_action->setIcon(
            QIcon(":/program-icons/selection-move-to-layer-below"));
    if (!QIcon::hasThemeIcon(QStringLiteral("tab-close")))
        m_Ui->actionRemove_Tab->setIcon(QIcon(":/program-icons/tab-close"));
    if (!QIcon::hasThemeIcon(QStringLiteral("help-donate")))
        m_Ui->actionDonate->setIcon(QIcon(":/program-icons/help-donate"));
    if (!QIcon::hasThemeIcon(QStringLiteral("document-save-as-template")))
        m_Ui->actionSave_Hdr_Preview->setIcon(
            QIcon(":/program-icons/document-save-as-template"));
    if (!QIcon::hasThemeIcon(QStringLiteral("help-whatsthis")))
        m_Ui->actionWhat_s_This->setIcon(
            QIcon(":/program-icons/help-whatsthis"));

    // About(s)
    connect(m_Ui->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(m_Ui->actionWhat_s_This, &QAction::triggered, this,
            &MainWindow::enterWhatsThis);

    // I/O
    connect(m_Ui->fileExitAction, &QAction::triggered, this, &QWidget::close);

    // Crop & Rotation
    connect(m_Ui->cropToSelectionAction, &QAction::triggered, this,
            &MainWindow::cropToSelection);
    m_Ui->cropToSelectionAction->setEnabled(false);

    connect(m_Ui->removeSelectionAction, &QAction::triggered, this,
            &MainWindow::disableCrop);

    connect(m_Ui->menuWindows, &QMenu::aboutToShow, this,
            &MainWindow::updateWindowMenu);
    connect(m_Ui->actionMinimize, &QAction::triggered, this,
            &QWidget::showMinimized);
    connect(m_Ui->actionMaximize, &QAction::triggered, this,
            &QWidget::showMaximized);
    connect(m_Ui->actionShowPreviewPanel, &QAction::toggled, this,
            &MainWindow::showPreviewPanel);
    connect(m_Ui->actionShowPreviewPanel, &QAction::toggled, this,
            &MainWindow::setPreviewPanelActive);

    // recent files
    initRecentFileActions();
    updateRecentFileActions();

#ifndef HAVE_CFITSIO
    m_Ui->actionFits_Importer->setVisible(false);
#endif
}

void MainWindow::createStatusBar() {
    statusBar()->showMessage(
        tr("Ready. Now open an existing HDR image or create a new one!"),
        10000);
}

void MainWindow::createConnections() {
    windowMapper = new QSignalMapper(this);
    // Qt 6 dropped QSignalMapper's QWidget overloads: map QObject pointers
    connect(windowMapper, &QSignalMapper::mappedObject, this,
            [this](QObject *o) { setActiveMainWindow(qobject_cast<QWidget *>(o)); });
    connect(&m_futureWatcher, &QFutureWatcherBase::finished, this,
            &MainWindow::whiteBalanceDone);
}

void MainWindow::loadOptions() {
    // load from settings the path where hdrs have been previously opened/loaded

    // load from settings the main toolbar visualization mode
    switch (LuminanceOptions().getMainWindowToolBarMode()) {
        case Qt::ToolButtonIconOnly:
            Icons_Only();
            m_Ui->actionIcons_Only->setChecked(true);
            break;
        case Qt::ToolButtonTextOnly:
            Text_Only();
            m_Ui->actionText_Only->setChecked(true);
            break;
        case Qt::ToolButtonTextBesideIcon:
            Text_Alongside_Icons();
            m_Ui->actionText_Alongside_Icons->setChecked(true);
            break;
        case Qt::ToolButtonTextUnderIcon:
            Text_Under_Icons();
            m_Ui->actionText_Under_Icons->setChecked(true);
            break;
    }
    m_Ui->actionShowPreviewPanel->setChecked(
        LuminanceOptions().isPreviewPanelActive());
    m_Ui->actionRealtimePreviews->setChecked(
        LuminanceOptions().isRealtimePreviewsActive());
    m_Ui->actionRealtimePreviews->setEnabled(
        LuminanceOptions().isPreviewPanelActive());
    // the preset strip sits under the image and needs no live operator
    // previews, so these options are gone
    m_Ui->actionRealtimePreviews->setVisible(false);
    m_Ui->menuPreview_Panel_2->menuAction()->setVisible(false);
    m_Ui->actionShowPreviewPanel->setText(tr("&Preset strip"));

    bool isPreviewPanelRight = LuminanceOptions().getPreviewPanelMode() == 0;
    m_Ui->actionShow_on_the_bottom->setChecked(!isPreviewPanelRight);
    m_Ui->actionShow_on_the_right->setChecked(isPreviewPanelRight);
}

void MainWindow::on_actionDonate_triggered() {
    DonationDialog::openDonationPage();
}

void MainWindow::on_fileNewAction_triggered() {
    createNewHdr(
        QStringList());  // redirect on createNewHdr-method to avoid moc warning
}

void MainWindow::createNewHdr(const QStringList &files) {
    // Merge HDR is a workspace of the window, not a dialog on top: it stays
    // while the other workspaces are looked at, until it is done
    if (m_mergeWizard) {
        showMergeWorkspace();
        return;
    }
    m_mergeWizard =
        new HdrWizard(this, files, m_inputFilesName, m_inputExpoTimes,
                      QSqlDatabase::database(m_tonemapPanel->getDatabaseConnection()));
    m_mergeWizard->setWindowFlags(Qt::Widget);
    m_mergeWizard->setObjectName(QStringLiteral("MergeWorkspace"));
    const QList<QPair<QString, QString>> wizardIcons = {
        {QStringLiteral("loadImagesButton"), QStringLiteral("plus")},
        {QStringLiteral("removeImageButton"), QStringLiteral("minus")},
        {QStringLiteral("clearListButton"), QStringLiteral("trash")}};
    for (const auto &entry : wizardIcons)
        if (QToolButton *b =
                m_mergeWizard->findChild<QToolButton *>(entry.first))
            b->setIcon(LumiereIcons::icon(entry.second));
    if (QPushButton *next = m_mergeWizard->findChild<QPushButton *>(
            QStringLiteral("NextFinishButton"))) {
        next->setProperty("primary", true);
        next->style()->unpolish(next);
        next->style()->polish(next);
    }
    m_workspace->addWidget(m_mergeWizard);
    connect(m_mergeWizard, &QDialog::finished, this, [this](int result) {
        HdrWizard *wizard = m_mergeWizard;
        m_mergeWizard = nullptr;
        m_topBar->setModeEnabled(TopBar::Merge, true);
        if (result == QDialog::Accepted) {
            const bool inThisWindow = !tm_status.is_hdr_ready;
            emit load_success(wizard->getPfsFrameHDR(),
                              wizard->getCaptionTEXT(),
                              wizard->getInputFilesNames(), true);
            // keep what was set in the preview, instead of starting over dark
            if (inThisWindow) {
                wizard->applyPreviewLook(
                    qobject_cast<HdrViewer *>(m_tabwidget->currentWidget()));
                // and show a finished picture right away, as other HDR
                // programs do: the first preset (Natural) on the new HDR
                for (const auto &preset : m_tonemapPanel->presets()) {
                    if (preset.second == TonemappingPanel::uneditedPreset())
                        continue;
                    m_tonemapPanel->applyPresetFile(preset.second, preset.first);
                    break;
                }
            }
        }
        m_workspace->removeWidget(wizard);
        wizard->deleteLater();
        showWorkspace(m_tabwidget->count() > 0);
    });
    showMergeWorkspace();
}

void MainWindow::showMergeWorkspace() {
    m_workspace->setCurrentWidget(m_mergeWizard);
    m_mergeWizard->show();
    m_topBar->setMode(TopBar::Merge);
}

void MainWindow::on_fileOpenAction_triggered() {
    QString filetypes = tr("All HDR formats ");
    QStringList hdrExtensionsList = getAllHdrFileExtensions();
    filetypes += QLatin1String("(");
    foreach (const QString &s, hdrExtensionsList) {
        filetypes += "*" + s + " ";
    }
    filetypes += QLatin1String(");;");
    filetypes += QLatin1String("OpenEXR (*.exr *.EXR);;");
    filetypes += QLatin1String("Radiance RGBE (*.hdr *.pic *.HDR *.PIC);;");
    filetypes += tr("HDR AVIF, HDR JPEG XL (*.avif *.jxl *.AVIF *.JXL);;");
    filetypes += QLatin1String("TIFF images (*.TIFF *.TIF *.tiff *.tif);;");
#if HAVE_CFITSIO
    filetypes += QLatin1String("FITS (*.fit *.FIT *.fits *.FITS);;");
#endif
    filetypes += QLatin1String(
        "RAW images (*.crw *.cr2 *.cr3 *.nef *.dng *.mrw *.orf *.kdc *.dcr *.arw "
        "*.raf "
        "*.ptx *.pef *.x3f *.raw "
        "*.rw2 *.sr2 *.3fr *.mef *.mos *.erf *.nrw *.mef *.mos *.erf *.nrw "
        "*.srw");
    filetypes += QLatin1String(
        "*.CRW *.CR2 *.CR3 *.NEF *.DNG *.MRW *.ORF *.KDC "
        "*.DCR *.ARW *.RAF *.PTX *.PEF "
        "*.X3F *.RAW *.RW2 *.SR2 "
        "*.3FR *.MEF *.MOS *.ERF *.NRW *.SRW);;");
    filetypes += QLatin1String("PFS stream (*.pfs *.PFS)");

    QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Load one or more HDR images..."),
        LuminanceOptions().getDefaultPathHdrIn(), filetypes);

    if (files.isEmpty()) return;

    // Update working folder
    // All the files are in the same folder, so I pick the first as reference to
    // update the settings
    QFileInfo qfi(files.first());

    LuminanceOptions().setDefaultPathHdrIn(qfi.absolutePath());

    foreach (const QString &filename, files) {
        // emit open_hdr_frame(filename);
        QMetaObject::invokeMethod(m_IOWorker, "read_hdr_frame",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, filename));
    }
}

void MainWindow::on_fileSaveAllAction_triggered() {
    if (m_tabwidget->count() <= 0) return;

    QString dir = QFileDialog::getExistingDirectory(
        this, tr("Save files in"), LuminanceOptions().getDefaultPathLdrOut());

    if (!dir.isEmpty()) {
        LuminanceOptions().setDefaultPathLdrOut(dir);

        for (int i = 0; i < m_tabwidget->count(); i++) {
            QWidget *wgt = m_tabwidget->widget(i);
            GenericViewer *g_v = qobject_cast<GenericViewer *>(wgt);

            if (!g_v->isHDR()) {
                LdrViewer *l_v = qobject_cast<LdrViewer *>(g_v);

                QString ldr_name = QFileInfo(getCurrentHDRName()).baseName();
                QString outfname = LuminanceOptions().getDefaultPathLdrOut() +
                                   "/" + ldr_name + "_" +
                                   l_v->getFileNamePostFix() + ".jpg";

                QString inputfname;
                if (!m_inputFilesName.isEmpty()) {
                    inputfname = m_inputFilesName.first();
                } else {
                    inputfname = "FromHdrFile";
                }

                QMetaObject::invokeMethod(
                    m_IOWorker, "write_ldr_frame", Qt::QueuedConnection,
                    Q_ARG(GenericViewer *, l_v), Q_ARG(QString, outfname),
                    Q_ARG(QString, inputfname),
                    Q_ARG(QVector<float>, QVector<float>()),
                    Q_ARG(TonemappingOptions *, l_v->getTonemappingOptions()),
                    Q_ARG(pfs::Params, pfs::Params("quality", 100u)));
            }
        }
    }
}

void MainWindow::on_fileSaveAsAction_triggered() {
    if (m_tabwidget->count() <= 0) return;

    GenericViewer *g_v = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());
    if (g_v->isHDR()) {
        // In this case I'm saving an HDR
        QString separator = "";
        if (!m_HdrCaption.isEmpty()) {
            separator = "-";
        }
        QString fname = getHdrFileNameFromSaveDialog(
            g_v->getFileName() + separator + m_HdrCaption , this);

        if (fname.isEmpty()) return;

        QFileInfo qfi(fname);
        QString format = qfi.suffix();

        pfs::Params p;
        // AVIF and JPEG XL can stay HDR (PQ, Rec. 2020): a tick box, on
        // by default, says so; off, they are saved as the HDR tab shows them
        const QString lower = format.toLower();
        if (lower == QLatin1String("avif") || lower == QLatin1String("jxl")) {
            ImageQualityDialog quality(nullptr, lower, 90, this);
            quality.setWindowTitle(tr("Save as %1").arg(format.toUpper()));
            quality.offerHdr(QStringLiteral("hdr-") + lower, tr("HDR"),
                             tr("Saves the full brightness range: highlights "
                                "glow on HDR screens (TV, monitor, Android, "
                                "Chrome). Without it the picture is saved as "
                                "the HDR tab shows it."),
                             true);
            if (quality.exec() == QDialog::Rejected) return;
            p.set("quality", (size_t)quality.getQuality());
            if (quality.hdrChosen())
                p.set("format", std::string(lower == QLatin1String("avif")
                                                ? "avif_hdr"
                                                : "jxl_hdr"));
        }
        if (format == QLatin1String("tif") || format == QLatin1String("tiff")) {
            TiffModeDialog t(true, -1, this);
            if (t.exec() == QDialog::Rejected) return;

            p.set("tiff_mode", t.getTiffWriterMode());
        }

        // CALL m_IOWorker->write_hdr_frame(qobject_cast<HdrViewer*>(g_v),
        // fname);
        QMetaObject::invokeMethod(
            m_IOWorker, "write_hdr_frame", Qt::QueuedConnection,
            Q_ARG(GenericViewer *, qobject_cast<HdrViewer *>(g_v)),
            Q_ARG(QString, fname), Q_ARG(pfs::Params, p));
    } else {
        // In this case I'm saving an LDR

        LdrViewer *l_v = qobject_cast<LdrViewer *>(g_v);

        if (l_v == nullptr) return;

        QString proposedFileName = proposedResultName(l_v) + ".jpg";

        // QString outputFilename = getLdrFileNameFromSaveDialog(ldr_name + "_"
        // +
        // l_v->getFileNamePostFix() + ".jpg", this);
        QString outputFilename =
            getLdrFileNameFromSaveDialog(proposedFileName, this);

        if (outputFilename.isEmpty()) return;

        QString format = QFileInfo(outputFilename).suffix().toLower();

        if (format.isEmpty()) {  // default as JPG
            format = QStringLiteral("jpg");
            outputFilename += QLatin1String(".jpg");
        }

        pfs::Params p;
        if (format == QLatin1String("png") || format == QLatin1String("jpg") ||
            format == QLatin1String("jpeg") || format == QLatin1String("avif") ||
            format == QLatin1String("jxl") || format == QLatin1String("webp")) {
            ImageQualityDialog savedFileQuality(l_v->getFrame(), format, -1,
                                                this);
            savedFileQuality.setWindowTitle(QObject::tr("Save as...") +
                                            format.toUpper());
            // the HDR behind the result lets the highlights glow on HDR
            // screens; offered as a tick box for the formats that can
            const bool isJpeg = format == QLatin1String("jpg") ||
                                format == QLatin1String("jpeg");
            const bool hasHdr = tm_status.curr_tm_frame != nullptr;
            const char *hdrFormat = nullptr;
#ifdef HAVE_LIBUHDR
            if (hasHdr && isJpeg) {
                savedFileQuality.offerHdr(
                    QStringLiteral("ultrahdr"), tr("Ultra HDR"),
                    tr("Highlights glow on HDR screens (iPhone, Android, "
                       "HDR monitors); elsewhere it is a normal JPEG."),
                    false);
                hdrFormat = "ultrahdr";
            }
#endif
            if (hasHdr && (format == QLatin1String("avif") ||
                           format == QLatin1String("jxl"))) {
                savedFileQuality.offerHdr(
                    QStringLiteral("result-hdr-") + format, tr("HDR"),
                    tr("Your edit with glowing highlights on HDR screens "
                       "(TV, monitor, Android, Chrome); on normal screens it "
                       "depends on the viewer."),
                    false);
                hdrFormat = format == QLatin1String("avif") ? "avif_hdr"
                                                            : "jxl_hdr";
            }
            if (savedFileQuality.exec() == QDialog::Rejected) return;

            p.set("quality", (size_t)savedFileQuality.getQuality());
            if (hdrFormat && savedFileQuality.hdrChosen()) {
                p.set("format", std::string(hdrFormat));
                p.set("hdr_frame", tm_status.curr_tm_frame->getFrameShared());
            }
        }

        if (format == QLatin1String("tif") || format == QLatin1String("tiff")) {
            TiffModeDialog t(false, -1, this);
            if (t.exec() == QDialog::Rejected) return;

            p.set("tiff_mode", t.getTiffWriterMode());
        }

        QString inputfname;
        if (!m_inputFilesName.isEmpty()) {
            inputfname = m_inputFilesName.first();
        } else {
            inputfname = "FromHdrFile";
        }

        QMetaObject::invokeMethod(
            m_IOWorker, "write_ldr_frame", Qt::QueuedConnection,
            Q_ARG(GenericViewer *, l_v), Q_ARG(QString, outputFilename),
            Q_ARG(QString, inputfname), Q_ARG(QVector<float>, m_inputExpoTimes),
            Q_ARG(TonemappingOptions *, l_v->getTonemappingOptions()),
            Q_ARG(pfs::Params, p));
    }
}

QString MainWindow::proposedResultName(LdrViewer *l_v) {
    // source photos (first-last), the operator as shown in the panel and the
    // preset, e.g. "IMG_1-IMG_3_Natural contrast (Mantiuk 2006)_Vivid",
    // instead of every parameter value
    QStringList parts;
    parts << QFileInfo(getCurrentHDRName()).completeBaseName();
    if (TonemappingOptions *opts = l_v->getTonemappingOptions()) {
        if (!opts->operatorLabel.isEmpty()) parts << opts->operatorLabel;
        if (!opts->presetName.isEmpty()) parts << opts->presetName;
    }
    if (parts.size() == 1) parts << l_v->getFileNamePostFix();
    QString proposedName = parts.join(QLatin1Char('_'));
    proposedName.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")),
                         QStringLiteral("-"));
    return proposedName;
}

void MainWindow::showExportWorkspace() {
    if (m_tabwidget->count() == 0) return;
    // the picture to save: the one looked at, else the newest result
    LdrViewer *result = qobject_cast<LdrViewer *>(m_tabwidget->currentWidget());
    for (int i = m_tabwidget->count() - 1; !result && i >= 0; --i)
        result = qobject_cast<LdrViewer *>(m_tabwidget->widget(i));
    m_exportResult = result;
    GenericViewer *hdr = tm_status.curr_tm_frame;
    if (!hdr || !hdr->isHDR()) {
        hdr = nullptr;
        for (int i = 0; !hdr && i < m_tabwidget->count(); ++i) {
            GenericViewer *v =
                qobject_cast<GenericViewer *>(m_tabwidget->widget(i));
            if (v && v->isHDR()) hdr = v;
        }
    }
    const QSize preview(1600, 1200);
    QPixmap resultPixmap, hdrPixmap;
    QString resultCaption, resultName, hdrCaption, hdrName;
    if (result) {
        resultPixmap = result->thumbnail(preview);
        QString look;
        if (TonemappingOptions *opts = result->getTonemappingOptions())
            look = opts->presetName.isEmpty() ? opts->operatorLabel
                                              : opts->presetName;
        resultCaption = tr("%1 · %2 × %3 px")
                            .arg(look.isEmpty() ? tr("Result") : look)
                            .arg(result->getWidth())
                            .arg(result->getHeight());
        resultName = proposedResultName(result);
    }
    if (hdr) {
        hdrPixmap = hdr->thumbnail(preview);
        hdrCaption = tr("HDR · %1 × %2 px")
                         .arg(hdr->getWidth())
                         .arg(hdr->getHeight());
        hdrName = QFileInfo(hdr->getFileName()).completeBaseName();
        if (!m_HdrCaption.isEmpty())
            hdrName += QStringLiteral("-") + m_HdrCaption;
        hdrName.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")),
                        QStringLiteral("-"));
    }
    m_exportPage->setSources(resultPixmap, resultCaption, resultName,
                             hdrPixmap, hdrCaption, hdrName);
    m_workspace->setCurrentWidget(m_exportPage);
    m_topBar->setMode(TopBar::Export);
}

void MainWindow::exportFromPage() {
    if (m_exportPage->exportsWebPage()) {
        m_Ui->actionExportToHTML->trigger();
        return;
    }
    const QString path = m_exportPage->filePath();
    if (QFileInfo::exists(path) &&
        QMessageBox::question(
            this, tr("Export"),
            tr("%1 is already there. Replace it?")
                .arg(QFileInfo(path).fileName())) != QMessageBox::Yes)
        return;
    QDir().mkpath(m_exportPage->folder());
    pfs::Params params = m_exportPage->params();
    if (m_exportPage->exportsHdr()) {
        GenericViewer *hdr = tm_status.curr_tm_frame;
        for (int i = 0; (!hdr || !hdr->isHDR()) && i < m_tabwidget->count(); ++i)
            hdr = qobject_cast<GenericViewer *>(m_tabwidget->widget(i));
        if (!hdr || !hdr->isHDR()) return;
        QMetaObject::invokeMethod(m_IOWorker, "write_hdr_frame",
                                  Qt::QueuedConnection,
                                  Q_ARG(GenericViewer *, hdr),
                                  Q_ARG(QString, path),
                                  Q_ARG(pfs::Params, params));
        return;
    }
    LdrViewer *l_v = m_exportResult;
    if (!l_v) return;
    std::string format;
    params.get("format", format);
    if ((format == "ultrahdr" || format == "avif_hdr" || format == "jxl_hdr") &&
        tm_status.curr_tm_frame)
        params.set("hdr_frame", tm_status.curr_tm_frame->getFrameShared());
    const QString inputfname = m_inputFilesName.isEmpty()
                                   ? QStringLiteral("FromHdrFile")
                                   : m_inputFilesName.first();
    QMetaObject::invokeMethod(
        m_IOWorker, "write_ldr_frame", Qt::QueuedConnection,
        Q_ARG(GenericViewer *, l_v), Q_ARG(QString, path),
        Q_ARG(QString, inputfname), Q_ARG(QVector<float>, m_inputExpoTimes),
        Q_ARG(TonemappingOptions *, l_v->getTonemappingOptions()),
        Q_ARG(pfs::Params, params));
}

void MainWindow::queueFromPage() {
    LdrViewer *l_v = m_exportResult;
    TonemappingOptions *opts = l_v ? l_v->getTonemappingOptions() : nullptr;
    if (!opts) return;
    addExportJob(*opts, m_exportPage->params(), m_exportPage->folder(),
                 m_exportPage->fileName());
}

void MainWindow::save_hdr_success(GenericViewer *saved_hdr,
                                  const QString &fname) {
    QFileInfo qfi(fname);

    setCurrentFile(qfi.absoluteFilePath());
    setWindowModified(false);

    // update name on the tab label
    m_tabwidget->setTabText(m_tabwidget->indexOf(saved_hdr), qfi.fileName());
}

void MainWindow::save_hdr_failed(const QString &fname) {
    // TODO give some kind of feedback to the user!
    // TODO pass the name of the file, so the user know which file didn't save
    // correctly
    // DONE!!! Once again, use unified style?
    QMessageBox::warning(0, QLatin1String(""),
                         tr("Failed to save %1").arg(fname), QMessageBox::Ok,
                         QMessageBox::NoButton);
}

void MainWindow::save_ldr_success(GenericViewer *saved_ldr,
                                  const QString &fname) {
    m_resultSaved = true;
    if (!saved_ldr->isHDR())
        m_tabwidget->setTabText(m_tabwidget->indexOf(saved_ldr),
                                QFileInfo(fname).fileName());
}

void MainWindow::save_ldr_failed(const QString &fname) {
    // TODO give some kind of feedback to the user!
    // TODO pass the name of the file, so the user know which file didn't save
    // correctly
    // DONE!!! Once again, use unified style?
    QMessageBox::warning(0, QLatin1String(""),
                         tr("Failed to save %1").arg(fname), QMessageBox::Ok,
                         QMessageBox::NoButton);
}

void MainWindow::on_actionSave_Hdr_Preview_triggered() {
    if (m_tabwidget->count() <= 0) return;

    GenericViewer *g_v = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());

    if (!g_v->isHDR()) return;
    try {
        QString ldr_name = QFileInfo(getCurrentHDRName()).baseName();

        QString outfname = getLdrFileNameFromSaveDialog(
            ldr_name + "_" + g_v->getFileNamePostFix() + ".jpg", this);

        if (outfname.isEmpty()) return;

        QString inputfname;
        if (!m_inputFilesName.isEmpty()) {
            inputfname = m_inputFilesName.first();
        } else {
            inputfname = "FromHdrFile";
        }

        QMetaObject::invokeMethod(
            m_IOWorker, "write_ldr_frame", Qt::QueuedConnection,
            Q_ARG(GenericViewer *, g_v), Q_ARG(QString, outfname),
            Q_ARG(QString, inputfname), Q_ARG(QVector<float>, QVector<float>()),
            Q_ARG(TonemappingOptions *, nullptr),
            Q_ARG(pfs::Params, pfs::Params("quality", 100u)));
    } catch (...) {
        return;
    }
}

void MainWindow::updateActions(int w) {
#ifdef QT_DEBUG
    qDebug() << "MainWindow::updateActions(" << w << ")";
#endif
    bool hasImage = w >= 0;
    GenericViewer *g_v = hasImage ? qobject_cast<GenericViewer *>(m_tabwidget->widget(w)) : 0;
    bool isHdr = g_v ? g_v->isHDR() : false;
    bool isLdr = g_v ? !g_v->isHDR() : false;
    LuminanceOptions luminance_opts;
    bool hasPrinterProfile =
        !luminance_opts.getPrinterProfileFileName().isEmpty();

    updateMagnificationButtons(g_v);  // g_v ? g_v : 0
    // a newly opened image shows the workspace; the last one closed brings
    // the Start page back
    const int tabs = m_tabwidget->count();
    QWidget *page = m_workspace->currentWidget();
    const bool onImage =
        page == m_centralwidget_splitter || page == m_exportPage;
    if ((tabs == 0 && onImage) || tabs > m_lastTabCount)
        showWorkspace(tabs > 0);
    m_lastTabCount = tabs;
    m_topBar->setModeEnabled(TopBar::Develop, hasImage);
    m_topBar->setModeEnabled(TopBar::Export, hasImage);

    m_Ui->fileSaveAsAction->setEnabled(hasImage);
    m_Ui->actionRemove_Tab->setEnabled(hasImage);
    m_Ui->actionSave_Hdr_Preview->setEnabled(hasImage && isHdr);
    m_Ui->fileSaveAllAction->setEnabled(hasImage && curr_num_ldr_open >= 2);

    // Histogram
    m_Ui->menuHDR_Histogram->setEnabled(isHdr);
    m_Ui->Low_dynamic_range->setEnabled(isHdr);
    m_Ui->Fit_to_dynamic_range->setEnabled(isHdr);
    m_Ui->Shrink_dynamic_range->setEnabled(isHdr);
    m_Ui->Extend_dynamic_range->setEnabled(isHdr);
    m_Ui->Decrease_exposure->setEnabled(isHdr);
    m_Ui->Increase_exposure->setEnabled(isHdr);

    m_Ui->actionResizeHDR->setEnabled(isHdr);
    m_Ui->action_Projective_Transformation->setEnabled(isHdr);
    m_Ui->actionExportToHTML->setEnabled(isHdr);
    m_Ui->rotateccw->setEnabled(isHdr);
    m_Ui->rotatecw->setEnabled(isHdr);

    m_Ui->actionFix_Histogram->setEnabled(isLdr);
    m_Ui->actionWhite_Balance->setEnabled(hasImage && !m_processingAWB);
    m_Ui->actionSoft_Proofing->setEnabled(isLdr && hasPrinterProfile);
    m_Ui->actionGamut_Check->setEnabled(isLdr && hasPrinterProfile);

    bool hasCropping = isHdr && tm_status.curr_tm_frame &&
                       tm_status.curr_tm_frame->hasSelection();
    m_Ui->cropToSelectionAction->setEnabled(hasCropping);
    m_Ui->removeSelectionAction->setEnabled(hasCropping);

    m_Ui->actionLock->setEnabled(hasImage && curr_num_ldr_open >= 1);
    if (m_hdrPreviewAct)
        m_hdrPreviewAct->setEnabled(isHdr ||
                                    (isLdr && tm_status.curr_tm_frame));
    m_Ui->actionShow_Image_Full_Screen->setEnabled(hasImage);

    // the full name of every image, as the tab text may be cut short
    for (int i = 0; i < m_tabwidget->count(); ++i)
        m_tabwidget->setTabToolTip(i, m_tabwidget->tabText(i));

    // undo/redo follows the image that is shown
    if (g_v && !m_undoGroup->stacks().contains(g_v->undoStack()))
        m_undoGroup->addStack(g_v->undoStack());
    m_undoGroup->setActiveStack(g_v ? g_v->undoStack() : nullptr);
    if (m_beforeAfterAct) updateBeforeAfter();

    updateHistogram();
}

namespace {
//! replaces the frame of a viewer; undo puts the previous one back
class FrameEditCommand : public QUndoCommand {
   public:
    FrameEditCommand(GenericViewer *viewer, pfs::Frame *edited,
                     const QString &text, std::function<void()> changed)
        : QUndoCommand(text),
          m_viewer(viewer),
          m_before(viewer->getFrameShared()),
          m_after(edited),
          m_changed(std::move(changed)) {}

    void redo() override {
        m_viewer->setFrameShared(m_after);
        m_changed();
    }
    void undo() override {
        m_viewer->setFrameShared(m_before);
        m_changed();
    }

   private:
    GenericViewer *m_viewer;  // owns the stack holding this command
    std::shared_ptr<pfs::Frame> m_before;
    std::shared_ptr<pfs::Frame> m_after;
    std::function<void()> m_changed;
};
}  // namespace

void MainWindow::markModified(GenericViewer *viewer) {
    if (viewer->needsSaving()) return;
    viewer->setNeedsSaving(true);

    int index = m_tabwidget->indexOf(viewer);
    QString text = m_tabwidget->tabText(index);
    m_tabwidget->setTabText(index, text.prepend("(*) "));

    setWindowModified(true);
}

void MainWindow::watchViewer(GenericViewer *viewer) {
    connect(viewer, &GenericViewer::imageChanged, this, [this, viewer]() {
        if (viewer == m_tabwidget->currentWidget()) m_histogramTimer->start();
    });
}

namespace {
//! stops between the darkest and brightest 0.1 % of an HDR frame
double dynamicRangeStops(const pfs::Frame *frame) {
    const pfs::Channel *r, *g, *b;
    frame->getXYZChannels(r, g, b);
    if (!r || !g || !b) return 0.0;
    std::vector<float> lum;
    const size_t n = frame->getWidth() * frame->getHeight();
    const size_t step = qMax<size_t>(1, n / 200000);  // a sample is enough
    lum.reserve(n / step + 1);
    for (size_t i = 0; i < n; i += step) {
        const float y =
            0.2126f * (*r)(i) + 0.7152f * (*g)(i) + 0.0722f * (*b)(i);
        if (y > 0.f) lum.push_back(y);
    }
    if (lum.size() < 2) return 0.0;
    std::sort(lum.begin(), lum.end());
    const float lo = lum[lum.size() / 1000];
    const float hi = lum[lum.size() - 1 - lum.size() / 1000];
    return lo > 0.f ? std::log2(double(hi) / lo) : 0.0;
}

QString exposureText(float seconds) {
    if (seconds <= 0.f) return QString();
    if (seconds >= 1.f) return QStringLiteral("%1 s").arg(seconds, 0, 'g', 3);
    return QStringLiteral("1/%1 s").arg(qRound(1.f / seconds));
}
}  // namespace

void MainWindow::updateHistogram() {
    if (!m_histogramPanel->isVisible()) return;
    GenericViewer *v =
        qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());
    m_histogramPanel->setImage(v ? v->getQImage() : QImage());

    QStringList info;
    if (v && v->getFrame()) {
        const pfs::Frame *f = v->getFrame();
        info << tr("Size: %1 x %2 pixels").arg(f->getWidth()).arg(f->getHeight());
        if (v->isHDR()) {
            // the range only changes with the frame, not with the display
            if (f != m_rangeFrame) {
                m_rangeFrame = f;
                m_rangeStops = dynamicRangeStops(f);
            }
            if (m_rangeStops > 0.0)
                info << tr("Dynamic range: %1 stops")
                            .arg(QLocale().toString(m_rangeStops, 'f', 1));
            if (m_inputFilesName.size() > 1) {
                QStringList times;
                for (float t : m_inputExpoTimes) {
                    const QString text = exposureText(t);
                    if (!text.isEmpty()) times << text;
                }
                info << (times.isEmpty()
                             ? tr("Merged from %1 photos")
                                   .arg(m_inputFilesName.size())
                             : tr("Merged from %1 photos: %2")
                                   .arg(m_inputFilesName.size())
                                   .arg(times.join(QStringLiteral(", "))));
            }
        } else if (TonemappingOptions *opts = v->getTonemappingOptions()) {
            info << tr("Tone mapping: %1")
                        .arg(opts->getCaption(false, QStringLiteral(", ")));
        }
    }
    m_histogramPanel->setInfo(info);
}

void MainWindow::applyHdrEdit(GenericViewer *viewer, pfs::Frame *edited,
                              const QString &text) {
    // push() runs redo(), which shows the edited frame
    viewer->undoStack()->push(
        new FrameEditCommand(viewer, edited, text, [this, viewer]() {
            markModified(viewer);
            emit updatedHDR(viewer->getFrame());
            updateHistogram();
        }));
}

void MainWindow::on_rotateccw_triggered() { dispatchrotate(false); }

void MainWindow::on_rotatecw_triggered() { dispatchrotate(true); }

void MainWindow::dispatchrotate(bool clockwise) {
    if (m_tabwidget->count() <= 0) return;

    GenericViewer *curr_g_v = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());

    m_Ui->rotateccw->setEnabled(false);
    m_Ui->rotatecw->setEnabled(false);

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    pfs::Frame *rotated = pfs::rotate(curr_g_v->getFrame(), clockwise);

    applyHdrEdit(curr_g_v, rotated,
                 clockwise ? tr("Rotate clockwise")
                           : tr("Rotate counter-clockwise"));
    QApplication::restoreOverrideCursor();

    m_Ui->rotateccw->setEnabled(true);
    m_Ui->rotatecw->setEnabled(true);
}

void MainWindow::on_actionResizeHDR_triggered() {
    if (m_tabwidget->count() <= 0) return;

    GenericViewer *curr_g_v = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());

    ResizeDialog *resizedialog = new ResizeDialog(this, curr_g_v->getFrame());
    if (resizedialog->exec() == QDialog::Accepted) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        applyHdrEdit(curr_g_v, resizedialog->getResizedFrame(),
                     tr("Resize"));
        QApplication::restoreOverrideCursor();
    }
    delete resizedialog;
}

void MainWindow::on_actionExportToHTML_triggered() {
    if (m_tabwidget->count() <= 0) return;

    GenericViewer *curr_g_v = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());
    ExportToHtmlDialog *exportDialog =
        new ExportToHtmlDialog(this, curr_g_v->getFrame());

    exportDialog->exec();
    delete exportDialog;
}

void MainWindow::on_actionSupported_Cameras_triggered() {
    SupportedCamerasDialog *supportedDialog = new SupportedCamerasDialog(this);

    supportedDialog->exec();
    delete supportedDialog;
}

void MainWindow::on_action_Projective_Transformation_triggered() {
    if (m_tabwidget->count() <= 0) return;

    GenericViewer *curr_g_v = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());

    ProjectionsDialog *projTranfsDialog =
        new ProjectionsDialog(this, curr_g_v->getFrame());
    if (projTranfsDialog->exec() == QDialog::Accepted) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        applyHdrEdit(curr_g_v, projTranfsDialog->getTranformedFrame(),
                     tr("Projective transformation"));
        QApplication::restoreOverrideCursor();
    }
    delete projTranfsDialog;
}

void MainWindow::on_Decrease_exposure_triggered() {
    if (m_tabwidget->count() <= 0) return;

    GenericViewer *g_v = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());
    HdrViewer *curr_hdr_v = qobject_cast<HdrViewer *>(g_v);
    if (curr_hdr_v != nullptr) curr_hdr_v->lumRange()->decreaseExposure();
}

void MainWindow::on_Extend_dynamic_range_triggered() {
    if (m_tabwidget->count() <= 0) return;

    GenericViewer *g_v = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());
    HdrViewer *curr_hdr_v = qobject_cast<HdrViewer *>(g_v);
    if (curr_hdr_v != nullptr) curr_hdr_v->lumRange()->extendRange();
}

void MainWindow::on_Fit_to_dynamic_range_triggered() {
    if (m_tabwidget->count() <= 0) return;

    GenericViewer *g_v = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());
    HdrViewer *curr_hdr_v = qobject_cast<HdrViewer *>(g_v);
    if (curr_hdr_v != nullptr) curr_hdr_v->lumRange()->fitToDynamicRange();
}

void MainWindow::on_Increase_exposure_triggered() {
    if (m_tabwidget->count() <= 0) return;

    GenericViewer *g_v = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());
    HdrViewer *curr_hdr_v = qobject_cast<HdrViewer *>(g_v);
    if (curr_hdr_v != nullptr) curr_hdr_v->lumRange()->increaseExposure();
}

void MainWindow::on_Shrink_dynamic_range_triggered() {
    if (m_tabwidget->count() <= 0) return;

    GenericViewer *g_v = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());
    HdrViewer *curr_hdr_v = qobject_cast<HdrViewer *>(g_v);
    if (curr_hdr_v != nullptr) curr_hdr_v->lumRange()->shrinkRange();
}

void MainWindow::on_Low_dynamic_range_triggered() {
    if (m_tabwidget->count() <= 0) return;

    GenericViewer *g_v = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());
    HdrViewer *curr_hdr_v = qobject_cast<HdrViewer *>(g_v);
    if (curr_hdr_v != nullptr) curr_hdr_v->lumRange()->lowDynamicRange();
}

// Zoom = Viewers (START)
void MainWindow::on_zoomInAct_triggered() {
    if (m_tabwidget->count() <= 0) return;

    GenericViewer *g_v = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());

    g_v->zoomIn();
}

void MainWindow::on_zoomOutAct_triggered() {
    if (m_tabwidget->count() <= 0) return;

    GenericViewer *g_v = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());

    g_v->zoomOut();
}

void MainWindow::on_fitToWindowAct_triggered() {
    if (m_tabwidget->count() <= 0) return;

    GenericViewer *g_v = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());

    g_v->fitToWindow();
}

void MainWindow::on_actionFill_to_Window_triggered() {
    if (m_tabwidget->count() <= 0) return;

    GenericViewer *g_v = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());

    g_v->fillToWindow();
}

void MainWindow::on_normalSizeAct_triggered() {
    if (m_tabwidget->count() <= 0) return;

    GenericViewer *g_v = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());

    g_v->normalSize();
}
// Zoom = Viewers (END)

void MainWindow::on_documentationAction_triggered() {
    if (sm_helpBrowser == nullptr) {
        sm_helpBrowser =
            new HelpBrowser(0, QStringLiteral(LUMINANCEDISPLAYNAME " Help"));
        sm_helpBrowser->setAttribute(Qt::WA_DeleteOnClose);
        connect(sm_helpBrowser, &QObject::destroyed, sm_helpBrowser,
                &QObject::deleteLater);
        connect(sm_helpBrowser, &HelpBrowser::closed, this,
                &MainWindow::helpBrowserClosed);
        sm_helpBrowser->show();
    } else {
        sm_helpBrowser->show();
        sm_helpBrowser->activateWindow();
        sm_helpBrowser->raise();
    }
}

void MainWindow::helpBrowserClosed() {
    sm_helpBrowser = nullptr;
}

void MainWindow::enterWhatsThis() { QWhatsThis::enterWhatsThisMode(); }

void MainWindow::openRecentFile() {
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        // emit open_hdr_frame(action->data().toString());
        QMetaObject::invokeMethod(m_IOWorker, "read_hdr_frame",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, action->data().toString()));
    }
}

void MainWindow::setupIO() {
    // progress bar
    m_ProgressBar = new QProgressBar(this);
    m_ProgressBar->hide();
    statusBar()->addWidget(m_ProgressBar);

    // Init Object/Thread
    m_IOThread = new QThread;
    m_IOWorker = new IOWorker;

    m_IOWorker->moveToThread(m_IOThread);

    // Memory Management
    connect(this, &QObject::destroyed, m_IOWorker, &QObject::deleteLater);
    connect(m_IOWorker, &QObject::destroyed, m_IOThread, &QObject::deleteLater);

    // Open
    connect(m_IOWorker, SIGNAL(read_hdr_success(pfs::Frame *, const QString &)),
            this, SLOT(load_success(pfs::Frame *, const QString &)));
    connect(m_IOWorker, &IOWorker::read_hdr_failed, this,
            &MainWindow::load_failed);

    // Save HDR
    connect(m_IOWorker,
            SIGNAL(write_hdr_success(GenericViewer *, const QString &)), this,
            SLOT(save_hdr_success(GenericViewer *, const QString &)));
    connect(m_IOWorker, &IOWorker::write_hdr_failed, this,
            &MainWindow::save_hdr_failed);
    // Save LDR
    connect(m_IOWorker,
            SIGNAL(write_ldr_success(GenericViewer *, const QString &)), this,
            SLOT(save_ldr_success(GenericViewer *, const QString &)));
    connect(m_IOWorker, &IOWorker::write_ldr_failed, this,
            &MainWindow::save_ldr_failed);

    // progress bar handling
    connect(m_IOWorker, &IOWorker::setValue, m_ProgressBar,
            &QProgressBar::setValue);
    connect(m_IOWorker, &IOWorker::setMaximum, m_ProgressBar,
            &QProgressBar::setMaximum);
    connect(m_IOWorker, &IOWorker::IO_init, this, &MainWindow::ioBegin);
    connect(m_IOWorker, &IOWorker::IO_finish, this, &MainWindow::ioEnd);

    // start thread waiting for signals (I/O requests)
    m_IOThread->start();
}

void MainWindow::ioBegin() {
    statusBar()->clearMessage();

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    //  statusBar()->addWidget(m_ProgressBar);
    m_ProgressBar->setMaximum(0);
    m_ProgressBar->show();
}

void MainWindow::ioEnd() {
    // statusBar()->removeWidget(m_ProgressBar);
    m_ProgressBar->reset();
    m_ProgressBar->hide();

    QApplication::restoreOverrideCursor();

    statusBar()->showMessage(tr("Done!"), 800);
}

void MainWindow::load_failed(const QString &errorMessage) {
    m_ProgressBar->reset();
    m_ProgressBar->hide();

    QApplication::restoreOverrideCursor();

    // TODO: use unified style?
    QMessageBox::critical(this, tr("Aborting..."), errorMessage,
                          QMessageBox::Ok, QMessageBox::NoButton);
}

void MainWindow::load_success(pfs::Frame *new_hdr_frame,
                              const QString &new_fname,
                              const QStringList &inputFileNames,
                              bool needSaving) {
    if (tm_status.is_hdr_ready) {
        MainWindow *other = new MainWindow(new_hdr_frame, new_fname,
                                           inputFileNames, needSaving);
        other->move(x() + 40, y() + 40);
        other->show();
    } else {
#ifdef QT_DEBUG
        qDebug() << "Filename: " << new_fname;
#endif
        HdrViewer *newhdr = new HdrViewer(new_hdr_frame, this, needSaving);

        newhdr->setAttribute(Qt::WA_DeleteOnClose);

        watchViewer(newhdr);
        connect(newhdr, &GenericViewer::selectionReady, this,
                &MainWindow::enableCrop);
        connect(newhdr, &GenericViewer::changed, this,
                &MainWindow::syncViewers);
        connect(newhdr, &GenericViewer::changed, this,
                &MainWindow::updateMagnificationButtons);
        connect(newhdr, &GenericViewer::reparent, this,
                &MainWindow::reparentViewer);
        connect(newhdr, &GenericViewer::goNext, this,
                &MainWindow::showNextViewer);
        connect(newhdr, &GenericViewer::goPrevious, this,
                &MainWindow::showPreviousViewer);
        connect(newhdr, &GenericViewer::syncViewers, this,
                &MainWindow::setSyncViewers);

        newhdr->setViewerMode(getCurrentViewerMode(*m_tabwidget));

        QFileInfo qfileinfo(new_fname);
        if (!qfileinfo.exists()) {
            // it doesn't exist on the file system, so I have just got back a
            // file from some creational operation (new hdr, crop...)
            m_inputFilesName = inputFileNames;
            QFileInfo fi1(m_inputFilesName.first());

            if (m_inputFilesName.first() != m_inputFilesName.last()) {
                QFileInfo fi2(m_inputFilesName.last());

                m_outputFileNamePrefix =
                    fi1.completeBaseName() + "-" + fi2.completeBaseName();
            }
            else {
                m_outputFileNamePrefix = fi1.completeBaseName();
            }
            // newhdr->setFileName(QString(tr("Untitled")));
            newhdr->setFileName(m_outputFileNamePrefix);
            m_tabwidget->addTab(newhdr, QString(new_fname).prepend("(*) "));
            m_HdrCaption = new_fname;

            setWindowModified(true);
        } else {
            m_outputFileNamePrefix = new_fname;
            // the new file exists on the file system, so I can use this value
            // to set captions and so on
            newhdr->setFileName(new_fname);
            m_tabwidget->addTab(newhdr, qfileinfo.fileName());

            setCurrentFile(new_fname);
            setWindowModified(needSaving);
        }

        tm_status.is_hdr_ready = true;
        tm_status.curr_tm_frame = newhdr;

        m_tabwidget->setCurrentWidget(newhdr);

        m_tonemapPanel->setEnabled(true);
        m_tonemapPanel->updatedHDR(new_hdr_frame);
        m_Ui->actionShowPreviewPanel->setEnabled(true);

        showPreviewPanel(m_Ui->actionShowPreviewPanel->isChecked());

        // signal: I have a new HDR open
        emit updatedHDR(newhdr->getFrame());
    }
}

void MainWindow::on_OptionsAction_triggered() {
    // the preferences are a page of the window; Save or Cancel goes back
    if (m_settingsPage) {
        m_workspace->setCurrentWidget(m_settingsPage);
        m_topBar->showUtility(TopBar::Settings);
        return;
    }
    PreferencesDialog *opts = new PreferencesDialog(nullptr);
    m_settingsPage = opts->useAsPage();
    m_workspace->addWidget(m_settingsPage);
    connect(opts, &QDialog::finished, this, [this](int result) {
        if (result == QDialog::Accepted)
            m_Ui->actionShowPreviewPanel->setChecked(
                LuminanceOptions().isPreviewPanelActive());
        QWidget *page = m_settingsPage;
        m_settingsPage = nullptr;
        const bool shown = m_workspace->currentWidget() == page;
        m_workspace->removeWidget(page);
        page->deleteLater();
        if (shown) leaveUtilityPage();
    });
    m_workspace->setCurrentWidget(m_settingsPage);
    m_topBar->showUtility(TopBar::Settings);
}

void MainWindow::leaveUtilityPage() {
    // back to the image if there is one, else to the Start page
    showWorkspace(m_tabwidget->count() > 0);
}

void MainWindow::on_Transplant_Exif_Data_action_triggered() {
    TransplantExifDialog *transplant = new TransplantExifDialog(this);
    transplant->setAttribute(Qt::WA_DeleteOnClose);
    transplant->exec();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasUrls()) {
        openFiles(convertUrlListToFilenameList(event->mimeData()->urls()));
    }
    event->acceptProposedAction();
}

void MainWindow::openFile(const QString &filename) {
    QMetaObject::invokeMethod(m_IOWorker, "read_hdr_frame",
                              Qt::QueuedConnection, Q_ARG(QString, filename));
}

void MainWindow::openFiles(const QStringList &files) {
    if (!files.isEmpty()) {
        switch (DnDOptionDialog::showDndDialog(this, files)) {
            case DnDOptionDialog::ACTION_NEW_HDR: {
                createNewHdr(files);
            } break;
            case DnDOptionDialog::ACTION_OPEN_HDR: {
                foreach (const QString &filename, files) { openFile(filename); }
            } break;
        }
    }
}

void MainWindow::Text_Under_Icons() {
    m_Ui->toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    LuminanceOptions().setMainWindowToolBarMode(Qt::ToolButtonTextUnderIcon);
}

void MainWindow::Icons_Only() {
    m_Ui->toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    LuminanceOptions().setMainWindowToolBarMode(Qt::ToolButtonIconOnly);
}

void MainWindow::Text_Alongside_Icons() {
    m_Ui->toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    LuminanceOptions().setMainWindowToolBarMode(Qt::ToolButtonTextBesideIcon);
}

void MainWindow::Text_Only() {
    m_Ui->toolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    LuminanceOptions().setMainWindowToolBarMode(Qt::ToolButtonTextOnly);
}

void MainWindow::onUpdateAvailable() {
    m_Ui->actionUpdateAvailable->setVisible(true);
}

void MainWindow::on_actionUpdateAvailable_triggered() {
    if (sm_updateChecker) {
        sm_updateChecker->trayMessageClicked();
    }
}

void MainWindow::on_actionAbout_Luminance_triggered() { UMessageBox::about(); }

/*
 * Window Menu Display and Functionalities
 */
void MainWindow::updateWindowMenu() {
    // Remove current elements inside the menuWindows
    foreach (QAction *Action_MW, openMainWindows) {
        openMainWindows.removeAll(Action_MW);
        m_Ui->menuWindows->removeAction(Action_MW);
        delete Action_MW;
    }

    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
        MainWindow *MW = qobject_cast<MainWindow *>(widget);
        if (MW != nullptr) {
            QAction *action =
                m_Ui->menuWindows->addAction(MW->getCurrentHDRName());

            action->setCheckable(true);
            action->setChecked(MW == this);
            // connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));
            connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));
            windowMapper->setMapping(action, static_cast<QObject *>(MW));

            openMainWindows.push_back(action);
        }
    }
}

/*
 * This function sets the active Main Window
 * when the file name is selected inside the "Window" menu
 */
void MainWindow::setActiveMainWindow(QWidget *w) {
    MainWindow *MW = qobject_cast<MainWindow *>(w);

    if (MW == nullptr) return;

    MW->raise();
    MW->activateWindow();
    return;
}

void MainWindow::on_actionBring_All_to_Front_triggered() {
    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
        MainWindow *MW = qobject_cast<MainWindow *>(widget);
        if (MW != nullptr) {
            MW->raise();
        }
    }
}

void MainWindow::showBatchPage(int kind) {
    // batch processing is a page of the window; it keeps working while
    // the other pages are looked at
    if (!m_batchPage) {
        m_batchPage = new BatchPage(
            QSqlDatabase::database(m_tonemapPanel->getDatabaseConnection()));
        m_workspace->addWidget(m_batchPage);
        connect(m_batchPage, &BatchPage::closed, this, [this]() {
            if (m_workspace->currentWidget() == m_batchPage)
                leaveUtilityPage();
        });
    }
    if (kind >= 0) m_batchPage->showKind(static_cast<BatchPage::Kind>(kind));
    m_workspace->setCurrentWidget(m_batchPage);
    m_topBar->showUtility(TopBar::Batch);
}

void MainWindow::on_actionBatch_HDR_triggered() {
    showBatchPage(BatchPage::Hdr);
}

void MainWindow::on_actionBatch_Tone_Mapping_triggered() {
    showBatchPage(BatchPage::Looks);
}

void MainWindow::cropToSelection() {
    if (m_tabwidget->count() <= 0) return;

    GenericViewer *curr_g_v = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());

    if (!curr_g_v->isHDR()) return;
    if (!curr_g_v->hasSelection()) return;

    QRect cropRect = curr_g_v->getSelectionRect();
    int x_ul, y_ul, x_br, y_br;
    cropRect.getCoords(&x_ul, &y_ul, &x_br, &y_br);
    disableCrop();
    pfs::Frame *original_frame = curr_g_v->getFrame();
    pfs::Frame *cropped_frame =
        pfs::cut(original_frame, x_ul, y_ul, x_br, y_br);

    emit load_success(cropped_frame, QString(tr("Cropped Image")),
                      m_inputFilesName, true);

    curr_g_v->removeSelection();
}

void MainWindow::enableCrop(bool isReady) {
    m_Ui->cropToSelectionAction->setEnabled(isReady);
    m_Ui->removeSelectionAction->setEnabled(isReady);
}

void MainWindow::disableCrop() {
    if (m_tabwidget->count() <= 0) return;

    GenericViewer *curr_g_v = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());

    if (!curr_g_v->isHDR()) return;

    curr_g_v->removeSelection();
    m_Ui->cropToSelectionAction->setEnabled(false);
    m_Ui->removeSelectionAction->setEnabled(false);
}

void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) {
        m_Ui->retranslateUi(this);
        setWindowTitle(QString(LUMINANCEDISPLAYNAME " " LUMINANCEVERSION " [*]"));
    }

    QWidget::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (!maybeSave()) {
        event->ignore();
        return;
    }
    if (m_isFullscreenViewer) {
        reparentViewer(m_g_v_fullscreen);
    }
    sm_NumMainWindows--;
    if (sm_NumMainWindows == 0) {
        if (sm_helpBrowser) {
            sm_helpBrowser->close();
        }
        // Last MainWindow is dead...
        LuminanceOptions().setValue(QStringLiteral("MainWindowState"),
                                    saveState());
        LuminanceOptions().setValue(QStringLiteral("MainWindowGeometry"),
                                    saveGeometry());
        LuminanceOptions().setValue(QStringLiteral("DevelopSplitterState"),
                                    m_centralwidget_splitter->saveState());
        LuminanceOptions().setValue(
            QStringLiteral("MainWindowSplitterGeometry"),
            m_centralwidget_splitter->saveGeometry());
        LuminanceOptions().setValue(
            QStringLiteral("MainWindowBottomSplitterState"),
            m_bottom_splitter->saveState());
        LuminanceOptions().setValue(
            QStringLiteral("MainWindowBottomSplitterGeometry"),
            m_bottom_splitter->saveGeometry());

        sm_updateChecker.reset();
    } else {
        sm_mainWindowMap.take(m_winId);
        int winId = sm_mainWindowMap.firstKey();
        MainWindow *p = sm_mainWindowMap[winId];
        disconnect(sm_updateChecker.data(), &UpdateChecker::updateAvailable, p,
                   &MainWindow::onUpdateAvailable);
        sm_updateChecker->setParent(p);
        connect(sm_updateChecker.data(), &UpdateChecker::updateAvailable, p,
                &MainWindow::onUpdateAvailable);
    }

    // Let's close all threads
    m_IOThread->quit();  // should be idle, quit() ok
    m_IOThread->wait();
    if (!m_TMProgressBar->isTerminated())  // MainWindow closed while
                                           // tonemapping is running
    {
        m_TMProgressBar->requestTermination();
    }
    m_TMThread->quit();
    m_TMThread->wait();
    m_QueueThread->quit();  // should be idle, quit() ok
    m_QueueThread->wait();

    clearRecentFileActions();
    QMainWindow::closeEvent(event);
}

void MainWindow::applyTabStyle() {
    // a style sheet on tabs replaces the native look, so the tabs get a
    // complete, quiet one: flat like the rest of the window, the shown
    // image marked by an accent line, a small close cross with room to the
    // tab's edge (in the colour that suits the scheme)
    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    m_tabwidget->setStyleSheet(
        QStringLiteral(
            "QTabBar::tab { width: 190px; height: 28px; padding: 0 4px 0 12px;"
            " margin: 6px 2px 4px 0; border: none;"
            " border-bottom: 2px solid transparent; border-radius: 4px;"
            " background: transparent; color: palette(window-text); }"
            "QTabBar::tab:hover { background: rgba(128, 128, 128, 40); }"
            "QTabBar::tab:selected { background: palette(window);"
            " border-bottom: 2px solid palette(highlight);"
            " border-bottom-left-radius: 0; border-bottom-right-radius: 0;"
            " font-weight: 600; }"
            "QTabBar::close-button { image: url(:/ui/tab-close-%1.svg);"
            " subcontrol-position: right; width: 16px; height: 16px;"
            " margin-right: 6px; border-radius: 3px; }"
            "QTabBar::close-button:hover { background: rgba(128, 128, 128, 80); }")
            .arg(dark ? QStringLiteral("dark") : QStringLiteral("light")));
}

bool MainWindow::event(QEvent *event) {
    bool result = QMainWindow::event(event);
    // the colour scheme can change while the app runs
    if (event->type() == QEvent::PaletteChange && m_tabwidget != nullptr) {
        applyTabStyle();
        refreshLineIcons();
    }
    if (event->type() == QEvent::WindowActivate && m_firstWindow == 1) {
        m_firstWindow = 2;
        if (LuminanceOptions().doShowWindowsOnWindows64Message()) {
            QMessageBox::warning(
                this, QStringLiteral(LUMINANCEDISPLAYNAME " 32-bit on 64-bit"),
                tr("It appears that you are running the 32-bit version "
                   "<strong>LumièreHDR</strong> on a 64-bit system. <br>"
                   "Please download the <strong>64-bit</strong> version from "
                   "<a "
                   "href=\"http://qtpfsgui.sourceforge.net\">http://"
                   "qtpfsgui.sourceforge.net</a> "
                   "to get the best LumièreHDR experience!"),
                QMessageBox::Ok, QMessageBox::NoButton);
        }
    }
    return result;
}

bool MainWindow::maybeSave() {
    // if no HDR is open, return true
    if (!tm_status.is_hdr_ready) return true;

    if (tm_status.curr_tm_frame->needsSaving()) {
        int ret = UMessageBox::saveDialog(
            tr("Unsaved changes..."), tr("This HDR image has unsaved "
                                         "changes.<br>Do you want to save it?"),
            this);
        switch (ret) {
            case QMessageBox::Save: {
                // if save == success return true;
                // else return false;
                // QString fname =
                // getHdrFileNameFromSaveDialog(QStringLiteral("Untitled"),
                // this);
                QString fname = getHdrFileNameFromSaveDialog(
                    m_outputFileNamePrefix + "-" + m_HdrCaption, this);

                if (!fname.isEmpty()) {
                    // TODO : can I launch a signal and wait that it gets
                    // executed
                    // fully?
                    return m_IOWorker->write_hdr_frame(
                        qobject_cast<HdrViewer *>(tm_status.curr_tm_frame),
                        fname);
                } else {
                    return false;
                }
            } break;
            case QMessageBox::Discard: {
                return true;
            } break;
            case QMessageBox::Cancel:
            default: { return false; } break;
        }
    } else
        return true;
}

void MainWindow::setupTM() {
    // TODO: Building TM Thread
    tm_status.is_hdr_ready = false;
    tm_status.curr_tm_frame = nullptr;
    tm_status.curr_tm_options = nullptr;

    m_tonemapPanel->setEnabled(false);

    m_TMProgressBar = new TMOProgressIndicator;
    m_TMProgressBar->hide();
    statusBar()->addWidget(m_TMProgressBar);

    connect(this, &QObject::destroyed, m_TMProgressBar, &QObject::deleteLater);

    m_TMWorker = new TMWorker;
    m_TMThread = new QThread;

    m_TMWorker->moveToThread(m_TMThread);

    // Memory Management
    connect(this, &QObject::destroyed, m_TMWorker, &QObject::deleteLater);
    connect(m_TMWorker, &QObject::destroyed, m_TMThread, &QObject::deleteLater);

    // get back result!
    connect(m_TMWorker, &TMWorker::tonemapSuccess, this,
            &MainWindow::addLdrFrame);
    connect(m_TMWorker, SIGNAL(tonemapFailed(QString)), this,
            SLOT(tonemapFailed(QString)));

    // progress bar handling
    connect(m_TMWorker, &TMWorker::tonemapBegin, this,
            &MainWindow::tonemapBegin);
    connect(m_TMWorker, &TMWorker::tonemapEnd, this, &MainWindow::tonemapEnd);

    connect(m_TMWorker, &TMWorker::tonemapSetValue, m_TMProgressBar,
            &TMOProgressIndicator::setValue);
    connect(m_TMWorker, &TMWorker::tonemapSetMaximum, m_TMProgressBar,
            &TMOProgressIndicator::setMaximum);
    connect(m_TMWorker, &TMWorker::tonemapSetMinimum, m_TMProgressBar,
            &TMOProgressIndicator::setMinimum);
    connect(m_TMProgressBar, &TMOProgressIndicator::terminate, m_TMWorker,
            &TMWorker::tonemapRequestTermination, Qt::DirectConnection);

    // start thread waiting for signals (I/O requests)
    m_TMThread->start();
}

void MainWindow::setupQueue() {
    m_QueueProgressBar = new TMOProgressIndicator;
    m_QueueProgressBar->hide();
    statusBar()->addWidget(m_QueueProgressBar);

    connect(this, &QObject::destroyed, m_QueueProgressBar,
            &QObject::deleteLater);

    m_QueueWorker = new TMWorker;
    m_QueueThread = new QThread;

    m_QueueWorker->moveToThread(m_QueueThread);

    // Memory Management
    connect(this, &QObject::destroyed, m_QueueWorker, &QObject::deleteLater);
    connect(m_QueueWorker, &QObject::destroyed, m_QueueThread,
            &QObject::deleteLater);

    // get back result!
    // connect(m_QueueWorker, SIGNAL(tonemapSuccess(pfs::Frame*,
    // TonemappingOptions*)),
    //    this, SLOT(addLdrFrame(pfs::Frame*, TonemappingOptions*)));
    connect(m_QueueWorker, SIGNAL(tonemapFailed(QString)), this,
            SLOT(tonemapFailed(QString)));

    // progress bar handling
    connect(m_QueueWorker, &TMWorker::tonemapBegin, this,
            &MainWindow::exportBegin);
    connect(m_QueueWorker, &TMWorker::tonemapEnd, this, &MainWindow::exportEnd);

    connect(m_QueueWorker, &TMWorker::tonemapSetValue, m_QueueProgressBar,
            &TMOProgressIndicator::setValue);
    connect(m_QueueWorker, &TMWorker::tonemapSetMaximum, m_QueueProgressBar,
            &TMOProgressIndicator::setMaximum);
    connect(m_QueueWorker, &TMWorker::tonemapSetMinimum, m_QueueProgressBar,
            &TMOProgressIndicator::setMinimum);
    connect(m_QueueProgressBar, &TMOProgressIndicator::terminate, m_QueueWorker,
            &TMWorker::tonemapRequestTermination, Qt::DirectConnection);

    m_exportQueue = new ExportQueueDialog(this);
    connect(m_QueueWorker, &TMWorker::exportJobStarted, m_exportQueue,
            &ExportQueueDialog::jobStarted);
    connect(m_QueueWorker, &TMWorker::exportJobFinished, this,
            [this](int id, const QString &file, int result) {
                m_exportQueue->jobFinished(id, file, result);
                // a written picture no longer needs its HDR kept
                if (result == 0) m_exportJobs.remove(id);
                m_tonemapPanel->setExportQueueSize(m_exportQueue->openJobs());
            });
    connect(m_exportQueue, &ExportQueueDialog::cancelRequested, this,
            [this](int id) { m_QueueWorker->cancelExportJob(id); });
    connect(m_exportQueue, &ExportQueueDialog::retryRequested, this,
            &MainWindow::queueExportJob);
    // the queue lives under the picture in the Export workspace
    m_exportQueue->setWindowFlags(Qt::Widget);
    for (const char *name : {"QueueIntro", "QueueClose"})
        if (QWidget *w = m_exportQueue->findChild<QWidget *>(name)) w->hide();
    m_exportPage->setQueue(m_exportQueue);
    connect(m_tonemapPanel, &TonemappingPanel::showExportQueue, this,
            &MainWindow::showExportWorkspace);

    // start thread waiting for signals (I/O requests)
    m_QueueThread->start();
}

void MainWindow::tonemapBegin() {
    // statusBar()->addWidget(m_TMProgressBar);
    m_TMProgressBar->setMaximum(0);
    m_TMProgressBar->show();
}

void MainWindow::tonemapEnd() {
    // statusBar()->removeWidget(m_TMProgressBar);
    m_TMProgressBar->hide();
    m_TMProgressBar->reset();
}

void MainWindow::exportBegin() {
    m_QueueProgressBar->setMaximum(0);
    m_QueueProgressBar->show();
}

void MainWindow::exportEnd() {
    m_QueueProgressBar->hide();
    m_QueueProgressBar->reset();
}

void MainWindow::tonemapImage(TonemappingOptions *opts) {
#ifdef QT_DEBUG
    qDebug() << "Start Tone Mapping";
#endif

    // while a picture is computed, only the newest request waits; a live
    // preview still running is out of date and is cut short. Before, every
    // slider change queued a full run and the preview fell behind.
    if (m_tmBusy) {
        m_tmPending = opts;
        if (m_tmBusyLive) emit m_TMWorker->tonemapRequestTermination(true);
        return;
    }

    // Warning when using size dependent TMOs with smaller sizes
    if ((opts->tmoperator == fattal) &&
        (opts->operator_options.fattaloptions.fftsolver == false) &&
        (opts->xsize != opts->origxsize) &&
        (LuminanceOptions().isShowFattalWarning())) {
        UMessageBox warningDialog(this);
        warningDialog.setText(tr("Fattal Warning"));
        warningDialog.setInformativeText(
            tr("This tonemapping operator depends on the size of the input "
               " image. Applying this operator on the full size image will "
               "most probably result in a different image. "
               "\n\nDo you want to continue?"));
        warningDialog.setStandardButtons(
            QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::No);
        warningDialog.setDefaultButton(QMessageBox::No);
        warningDialog.setIcon(QMessageBox::Warning);

        switch (warningDialog.exec()) {
            case QMessageBox::Yes: {
            } break;
            case QMessageBox::YesAll: {
                LuminanceOptions().setShowFattalWarning(false);
            } break;
            case QMessageBox::No:
            default: { return; }
        }
    }

    m_PreviewPanel->setEnabled(false);

    tm_status.curr_tm_options = opts;

    if (tm_status.curr_tm_frame->hasSelection()) {
        opts->tonemapSelection = true;

        getCropCoords(tm_status.curr_tm_frame, opts->selection_x_up_left,
                      opts->selection_y_up_left, opts->selection_x_bottom_right,
                      opts->selection_y_bottom_right);
    } else
        opts->tonemapSelection = false;

    HdrViewer *hdr_viewer = qobject_cast<HdrViewer *>(tm_status.curr_tm_frame);
    if (hdr_viewer) {
#ifdef QT_DEBUG
        qDebug() << "MainWindow(): emit getTonemappedFrame()";
#endif
        // CALL m_TMWorker->getTonemappedFrame(hdr_viewer->getHDRPfsFrame(),
        // opts);
        m_tmBusy = true;
        m_tmBusyLive = m_tonemapPanel->lastRunWasLive();
        QMetaObject::invokeMethod(
            m_TMWorker, "computeTonemap", Qt::QueuedConnection,
            Q_ARG(pfs::Frame *, hdr_viewer->getFrame()),
            Q_ARG(TonemappingOptions *, opts),
            Q_ARG(InterpolationMethod, m_interpolationMethod));
    }
}

void MainWindow::exportImage(TonemappingOptions *opts) {
    addExportJob(*opts,
                 pfsadditions::FormatHelper::getParamsFromSettings(
                     KEY_FILEFORMAT_QUEUE, false),
                 LuminanceOptions().getExportDir());
    delete opts;
}

void MainWindow::addExportJob(const TonemappingOptions &options,
                              const pfs::Params &params, const QString &dir,
                              const QString &fileName) {
    HdrViewer *hdr_viewer = qobject_cast<HdrViewer *>(tm_status.curr_tm_frame);
    if (!hdr_viewer) return;

    ExportJobPtr job = std::make_shared<ExportJob>();
    job->id = m_nextExportJob++;
    job->hdr = hdr_viewer->getFrameShared();
    job->options = options;
    job->params = params;
    job->hdrName = QFileInfo(getCurrentHDRName()).baseName();
    if (!m_inputFilesName.isEmpty()) job->inputFileName = m_outputFileNamePrefix;
    job->exportDir = dir;
    job->fileName = fileName;
    job->expoTimes = m_inputExpoTimes;
    job->method = m_interpolationMethod;
    m_exportJobs.insert(job->id, job);

    std::string format;
    job->params.get("format", format);
    QString extension;
    if (format.empty() && job->params.get("fileextension", extension))
        format = extension.toStdString();
    QString look = job->options.presetName.isEmpty()
                       ? job->options.operatorLabel
                       : job->options.presetName;
    m_exportQueue->addJob(job->id, job->hdrName, look,
                          format == "ultrahdr"   ? tr("Ultra HDR JPEG")
                          : format == "avif_hdr" ? tr("HDR AVIF")
                          : format == "jxl_hdr"  ? tr("HDR JPEG XL")
                                                 : QString::fromStdString(format)
                                                       .toUpper());
    queueExportJob(job->id);
}

void MainWindow::queueExportJob(int id) {
    const ExportJobPtr job = m_exportJobs.value(id);
    if (!job) return;
    m_tonemapPanel->setExportQueueSize(m_exportQueue->openJobs());
    QMetaObject::invokeMethod(m_QueueWorker, "runExportJob",
                              Qt::QueuedConnection, Q_ARG(ExportJobPtr, job));
}

void MainWindow::showUnedited() {
    HdrViewer *hdr = qobject_cast<HdrViewer *>(tm_status.curr_tm_frame);
    if (!hdr) return;
    pfs::Frame *source = hdr->getFrame();
    pfs::Channel *in[3];
    source->getXYZChannels(in[0], in[1], in[2]);
    if (!in[0] || !in[1] || !in[2]) return;

    // the same mapping as the HDR tab's display, kept in floating point
    pfs::Frame *picture =
        new pfs::Frame(source->getWidth(), source->getHeight());
    pfs::Channel *out[3];
    picture->createXYZChannels(out[0], out[1], out[2]);
    const auto map = pfs::utils::chain(
        pfs::colorspace::Normalizer(hdr->getMinLuminanceValue(),
                                    hdr->getMaxLuminanceValue()),
        pfs::utils::chain(pfs::utils::CLAMP_F32,
                          Remapper<float>(hdr->getLuminanceMappingMethod())));
    for (int c = 0; c < 3; ++c)
        std::transform(in[c]->begin(), in[c]->end(), out[c]->begin(), map);

    TonemappingOptions *opts = new TonemappingOptions;
    opts->operatorLabel = tr("Unedited");
    opts->presetName = tr("Unedited");
    opts->origxsize = opts->xsize = int(source->getWidth());
    // shown as it is: no automatic levels
    m_showingUnedited = true;
    addLdrFrame(picture, opts);
    m_showingUnedited = false;
}

void MainWindow::startPendingTonemap() {
    m_tmBusy = false;
    if (!m_tmPending) return;
    TonemappingOptions *next = m_tmPending;
    m_tmPending = nullptr;
    tonemapImage(next);
}

void MainWindow::addLdrFrame(pfs::Frame *frame,
                             TonemappingOptions *tm_options) {
    // a live result that a newer change already replaced is not shown
    if (!m_showingUnedited && m_tmBusyLive && m_tmPending) {
        delete frame;
        startPendingTonemap();
        return;
    }
    if (m_tonemapPanel->doAutoLevels() && !m_showingUnedited) {
        float threshold, minL, maxL, gammaL;
        threshold = m_tonemapPanel->getAutoLevelsThreshold();
        QScopedPointer<QImage> temp_qimage(fromLDRPFStoQImage(frame));
        computeAutolevels(temp_qimage.data(), threshold, minL, maxL, gammaL);
        pfs::gammaAndLevels(frame, minL, maxL, 0.f, 1.f, gammaL);
    }

    GenericViewer *n =
        static_cast<GenericViewer *>(m_tabwidget->currentWidget());
    if (m_tonemapPanel->replaceLdr() && n != nullptr && !n->isHDR()) {
        n->setFrame(frame, tm_options);
    } else {
        curr_num_ldr_open++;
        num_ldr_generated++;

        n = new LdrViewer(frame, tm_options, this, true);

        watchViewer(n);
        connect(n, &GenericViewer::changed, this, &MainWindow::syncViewers);
        connect(n, &GenericViewer::changed, this,
                &MainWindow::updateMagnificationButtons);
        connect(n, &GenericViewer::reparent, this, &MainWindow::reparentViewer);
        connect(n, &GenericViewer::goNext, this, &MainWindow::showNextViewer);
        connect(n, &GenericViewer::goPrevious, this,
                &MainWindow::showPreviousViewer);
        connect(n, &GenericViewer::syncViewers, this,
                &MainWindow::setSyncViewers);

        if (num_ldr_generated == 1)
            m_tabwidget->addTab(n, tr("Untitled"));
        else
            m_tabwidget->addTab(n, tr("Untitled %1").arg(num_ldr_generated));

        n->setViewerMode(getCurrentViewerMode(*m_tabwidget));
    }
    m_tabwidget->setCurrentWidget(n);

    m_PreviewPanel->setEnabled(true);

    if (m_Ui->actionSoft_Proofing->isChecked()) {
        LdrViewer *viewer = static_cast<LdrViewer *>(n);
        viewer->doSoftProofing(false);
    } else if (m_Ui->actionGamut_Check->isChecked()) {
        LdrViewer *viewer = static_cast<LdrViewer *>(n);
        viewer->doSoftProofing(true);
    }
    // a replaced result does not change the tab
    updateHistogram();
    if (!m_showingUnedited) startPendingTonemap();
}

void MainWindow::tonemapFailed(const QString &error_msg) {
    if (error_msg != QLatin1String("Canceled")) {
        QMessageBox::critical(this, QStringLiteral(LUMINANCEDISPLAYNAME),
                              tr("Error: %1").arg(error_msg), QMessageBox::Ok,
                              QMessageBox::NoButton);
    }
    m_tonemapPanel->setEnabled(true);
    m_PreviewPanel->setEnabled(true);
    m_TMProgressBar->hide();
    // the export queue reports here too; only the preview worker frees up
    if (sender() == m_TMWorker) startPendingTonemap();
}

/*
 * Lock Handling
 */
void MainWindow::on_actionLock_toggled(bool /*toggled*/) {
    if (m_Ui->actionLock->isChecked() && m_tabwidget->count()) {
        syncViewers(qobject_cast<GenericViewer *>(m_tabwidget->currentWidget()));
    }
}

void MainWindow::syncViewers(GenericViewer *sender) {
    if (sender == nullptr) return;
    if (!m_Ui->actionLock->isChecked()) return;

    for (int idx = 0; idx < m_tabwidget->count(); idx++) {
        GenericViewer *viewer = qobject_cast<GenericViewer *>(m_tabwidget->widget(idx));
        if (viewer == nullptr)
            return;  // this happens when a tab is removed and sync viewers is
                     // active,
                     // fixes a crash
        if (sender != viewer) {
            viewer->blockSignals(true);
            viewer->syncViewer(sender);
            viewer->blockSignals(false);
        }
    }
}

void MainWindow::showPreviewPanel(bool b) {
    // the looks replaced the operator previews
    m_tonemapPanel->setRealtimePreviews(false);
    m_looksPanel->setVisible(b);
    if (b && tm_status.is_hdr_ready)
        m_presetStrip->updatePreviews(tm_status.curr_tm_frame->getFrame());
}

void MainWindow::updatePreviews(bool b, float th) {
    if (m_Ui->actionShowPreviewPanel->isChecked()) {
        if (tm_status.is_hdr_ready) {
            m_PreviewPanel->setAutolevels(b, th);
            // ask panel to refresh itself
            m_PreviewPanel->updatePreviews(tm_status.curr_tm_frame->getFrame());
        }
    }
}

void MainWindow::updateMagnificationButtons(GenericViewer *c_v) {
    if (c_v == nullptr) return;
    bool isNormalSize = c_v->isNormalSize();
    bool isFilledToWindow = c_v->isFilledToWindow();
    bool isFittedToWindow = c_v->isFittedToWindow();

    m_Ui->zoomInAct->setChecked(isFilledToWindow || isFittedToWindow);
    m_Ui->zoomInAct->setEnabled(isFilledToWindow || isFittedToWindow);
    m_Ui->zoomOutAct->setChecked(isFittedToWindow);
    m_Ui->zoomOutAct->setEnabled(!isFittedToWindow);

    m_Ui->normalSizeAct->setChecked(isNormalSize);
    m_Ui->normalSizeAct->setEnabled(!isNormalSize);
    m_Ui->fitToWindowAct->setChecked(isFittedToWindow);
    m_Ui->fitToWindowAct->setEnabled(!isFittedToWindow);
    m_Ui->actionFill_to_Window->setChecked(isFilledToWindow);
    m_Ui->actionFill_to_Window->setEnabled(!isFilledToWindow);
}

void MainWindow::on_actionRemove_Tab_triggered() {
    removeTab(m_tabwidget->currentIndex());
}

void MainWindow::removeTab(int t) {
#ifdef QT_DEBUG
    qDebug() << "MainWindow::remove_image(" << t << ")";
#endif
    if (t < 0) return;

    GenericViewer *w = qobject_cast<GenericViewer *>(m_tabwidget->widget(t));
    w->blockSignals(true);
    if (w->isHDR()) {
        bool doClose = false;

#ifdef QT_DEBUG
        qDebug() << "Remove HDR from MainWindow";
#endif
        if (w->needsSaving()) {
            if (maybeSave()) {
                // if discard OR saved
                doClose = true;
            }
            // if FALSE, it means that the user said "Cancel"
            // or the saving operation went wrong
            // and we don't need to remove any tab
            else {
                w->blockSignals(false);
            }
        } else
            doClose = true;

        if (doClose) {
            m_tabwidget->removeTab(t);
            w->deleteLater();  // delete yourself whenever you want

            showPreviewPanel(false);
            setWindowModified(false);

            tm_status.is_hdr_ready = false;
            tm_status.curr_tm_frame = nullptr;
            tm_status.curr_tm_options = nullptr;

            m_tonemapPanel->setEnabled(false);
            // the tab is gone before the HDR state was cleared
            updateActions(m_tabwidget->currentIndex());

            m_inputFilesName.clear();
            m_inputExpoTimes.clear();

            m_PreviewscrollArea->hide();
        }
    } else {
        curr_num_ldr_open--;
        m_tabwidget->removeTab(t);
        w->deleteLater();  // delete yourself whenever you want

        if (curr_num_ldr_open == 1) m_Ui->fileSaveAllAction->setEnabled(false);
    }
}

QString MainWindow::getCurrentHDRName() {
    if (tm_status.is_hdr_ready) {
        return tm_status.curr_tm_frame->getFileName();
    } else {
        return QString(tr("Untitled HDR"));
    }
}

void MainWindow::setMainWindowModified(bool b) {
    if (b) {
        if (tm_status.is_hdr_ready) {
            tm_status.curr_tm_frame->setNeedsSaving(true);
            setWindowModified(true);
        }
    } else {
        if (tm_status.is_hdr_ready) {
            tm_status.curr_tm_frame->setNeedsSaving(false);
        }
        setWindowModified(false);
    }
}

/*
 * Recent File Handling
 */
void MainWindow::setCurrentFile(const QString &fileName) {
    QStringList files =
        LuminanceOptions().value(KEY_RECENT_FILES).toStringList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > MAX_RECENT_FILES) {
        files.removeLast();
    }

    LuminanceOptions().setValue(KEY_RECENT_FILES, files);

    // Update ALL MainWindow
    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
        MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
        if (mainWin) mainWin->updateRecentFileActions();
    }

    OsIntegration::getInstance().addRecentFile(fileName);
}

void MainWindow::updateRecentFileActions() {
    QStringList files =
        LuminanceOptions().value(KEY_RECENT_FILES).toStringList();
    if (m_startPage) m_startPage->setRecentFiles(files);

    int numRecentFiles = qMin(files.size(), (int)MAX_RECENT_FILES);
    separatorRecentFiles->setVisible(numRecentFiles > 0);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = QStringLiteral("&%1 %2").arg(i + 1).arg(
            QFileInfo(files[i]).fileName());
        recentFileActs[i]->setText(text);
        recentFileActs[i]->setData(files[i]);
        recentFileActs[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < MAX_RECENT_FILES; ++j) {
        recentFileActs[j]->setVisible(false);
    }
}

void MainWindow::initRecentFileActions() {
    separatorRecentFiles = m_Ui->menuFile->addSeparator();

    for (int i = 0; i < MAX_RECENT_FILES; ++i) {
        recentFileActs[i] = new QAction(this);
        recentFileActs[i]->setVisible(false);
        m_Ui->menuFile->addAction(recentFileActs[i]);
        connect(recentFileActs[i], &QAction::triggered, this,
                &MainWindow::openRecentFile);
    }
}

void MainWindow::clearRecentFileActions() {
    for (int i = 0; i < MAX_RECENT_FILES; ++i) {
        delete recentFileActs[i];
    }
}

void MainWindow::on_actionFix_Histogram_toggled(bool checked) {
    if (checked) {
        GenericViewer *current = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());
        if (current == nullptr) return;
        if (current->isHDR()) return;

        QScopedPointer<GammaAndLevels> g_n_l(
            new GammaAndLevels(this, current->getQImage()));

        m_Ui->actionFix_Histogram->setDisabled(true);

        connect(g_n_l.data(), SIGNAL(updateQImage(QImage)), current,
                SLOT(setQImage(QImage)));
        int exit_status = g_n_l->exec();

        if (exit_status == 1) {
#ifdef QT_DEBUG
            qDebug() << "GammaAndLevels accepted!";
#endif
            // pfs::Frame * frame = current->getFrame();
            pfs::gammaAndLevels(
                current->getFrame(), g_n_l->getBlackPointInput(),
                g_n_l->getWhitePointInput(), g_n_l->getBlackPointOutput(),
                g_n_l->getWhitePointOutput(), g_n_l->getGamma());

            // current->setFrame(frame);
        } else {
#ifdef QT_DEBUG
            qDebug() << "GammaAndLevels refused!";
#endif
            current->setQImage(g_n_l->getReferenceQImage());
        }

        m_Ui->actionFix_Histogram->setDisabled(false);
        m_Ui->actionFix_Histogram->setChecked(false);
    }
}

void MainWindow::on_actionWhite_Balance_triggered() {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_Ui->actionWhite_Balance->setEnabled(false);
    m_processingAWB = true;
    m_viewerToProcess = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());
    m_tabwidget->setTabEnabled(m_tabwidget->currentIndex(), false);

    Frame *frame = m_viewerToProcess->getFrame();

    m_futureWatcher.setFuture(QtConcurrent::run(
        boost::bind(whiteBalance, boost::ref(*frame), WB_COLORBALANCE)));
}

void MainWindow::whiteBalanceDone() {
    QApplication::restoreOverrideCursor();
    m_processingAWB = false;
    m_Ui->actionWhite_Balance->setEnabled(true);
    m_viewerToProcess->setEnabled(true);
    m_tabwidget->setTabEnabled(m_tabwidget->indexOf(m_viewerToProcess), true);
    m_tabwidget->setCurrentWidget(m_viewerToProcess);
    m_viewerToProcess->updatePixmap();
    if (m_viewerToProcess->isHDR()) {
        m_viewerToProcess->setNeedsSaving(true);
        m_presetStrip->updatePreviews(tm_status.curr_tm_frame->getFrame());
    }
}

void MainWindow::on_actionSoft_Proofing_toggled(bool doProof) {
    GenericViewer *current = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());
    if (current == nullptr) return;
    if (current->isHDR()) return;
    LdrViewer *viewer = static_cast<LdrViewer *>(current);
    if (doProof) {
#ifdef QT_DEBUG
        qDebug() << "MainWindow:: do soft proofing";
#endif
        if (m_Ui->actionGamut_Check->isChecked())
            m_Ui->actionGamut_Check->setChecked(false);
        viewer->doSoftProofing(false);
    } else {
#ifdef QT_DEBUG
        qDebug() << "MainWindow:: undo soft proofing";
#endif
        viewer->undoSoftProofing();
    }
}

void MainWindow::on_actionGamut_Check_toggled(bool doGamut) {
    GenericViewer *current = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());
    if (current == nullptr) return;
    if (current->isHDR()) return;
    LdrViewer *viewer = static_cast<LdrViewer *>(current);
    if (doGamut) {
#ifdef QT_DEBUG
        qDebug() << "MainWindow:: do gamut check";
#endif
        if (m_Ui->actionSoft_Proofing->isChecked())
            m_Ui->actionSoft_Proofing->setChecked(false);
        viewer->doSoftProofing(true);
    } else {
#ifdef QT_DEBUG
        qDebug() << "MainWindow:: undo gamut check";
#endif
        viewer->undoSoftProofing();
    }
}

void MainWindow::updateSoftProofing(int i) {
    QWidget *wgt = m_tabwidget->widget(i);
    GenericViewer *g_v = qobject_cast<GenericViewer *>(wgt);

    if (g_v == nullptr) return;
    if (!g_v->isHDR()) {
        LdrViewer *l_v = static_cast<LdrViewer *>(g_v);
        if (!m_Ui->actionSoft_Proofing->isChecked() &&
            !m_Ui->actionGamut_Check->isChecked()) {
            l_v->undoSoftProofing();
        } else if (m_Ui->actionSoft_Proofing->isChecked()) {
            l_v->doSoftProofing(false);
        } else if (m_Ui->actionGamut_Check->isChecked()) {
            l_v->doSoftProofing(true);
        }
    }
}

void MainWindow::showPreviewsOnTheRight() {
    m_PreviewscrollArea->setParent(m_centralwidget_splitter);
    LuminanceOptions().setPreviewPanelMode(0);
}

void MainWindow::showPreviewsOnTheBottom() {
    m_PreviewscrollArea->setParent(m_bottom_splitter);
    LuminanceOptions().setPreviewPanelMode(1);
}

void MainWindow::on_actionFits_Importer_triggered() {
#ifdef HAVE_CFITSIO
    FitsImporter importer;

    if (importer.exec() == QDialog::Accepted) {
        emit load_success(importer.getFrame(), tr("FITS Image"), importer.getFilenames(), true);
    }
#endif
}

void MainWindow::on_actionShow_Full_Screen_toggled(bool b) {
    b ? this->showFullScreen() : this->showMaximized();
}

void MainWindow::on_actionShow_Image_Full_Screen_triggered() {
    if (m_isFullscreenViewer) {
        return;
    }
    m_isFullscreenViewer = true;
    m_fullscreenViewerIndex = m_tabwidget->currentIndex();
    m_tabText = m_tabwidget->tabText(m_fullscreenViewerIndex);
    m_g_v_fullscreen = qobject_cast<GenericViewer *>(m_tabwidget->currentWidget());
    m_g_v_fullscreen->setParent(0);
    m_g_v_fullscreen->showFullScreen();
}

void MainWindow::reparentViewer(GenericViewer *g_v, bool close) {
    m_tabwidget->insertTab(m_fullscreenViewerIndex, g_v, m_tabText);
    m_tabwidget->setCurrentIndex(m_fullscreenViewerIndex);
    g_v->showNormal();
    m_isFullscreenViewer = false;
    if (close) {
        on_actionRemove_Tab_triggered();
    }
}

void MainWindow::showNextViewer(GenericViewer *g_v) {
    int count = m_tabwidget->count();
    if (count >= 1) {
        reparentViewer(g_v);
        int idx = m_tabwidget->currentIndex();
        int next = (idx + 1) % (count + 1);
        m_tabwidget->setCurrentIndex(next);
        on_actionShow_Image_Full_Screen_triggered();
    }
}

void MainWindow::showPreviousViewer(GenericViewer *g_v) {
    int count = m_tabwidget->count();
    if (count >= 1) {
        reparentViewer(g_v);
        int idx = m_tabwidget->currentIndex();
        int previous;
        if (idx == 0) {
            previous = count;
        } else {
            previous = (idx - 1) % (count + 1);
        }
        m_tabwidget->setCurrentIndex(previous);
        on_actionShow_Image_Full_Screen_triggered();
    }
}

void MainWindow::setSyncViewers(GenericViewer *g_v) {
    if (g_v == nullptr) return;
    m_Ui->actionLock->setChecked(!m_Ui->actionLock->isChecked());
    if (m_Ui->actionLock->isChecked() && (m_tabwidget->count() - 1)) {
        syncViewers(g_v);
    }
}

void MainWindow::on_actionSelect_Interpolation_Method_toggled(bool b) {
    m_interpolationMethod = b ? LanczosInterp : BilinearInterp;
}

void MainWindow::on_actionOnline_Documentation_triggered() {
    QDesktopServices::openUrl(
        QUrl(QStringLiteral("http://luminancehdr.readthedocs.io/en/latest/")));
}

void MainWindow::on_actionDevelopers_Website_triggered() {
    QDesktopServices::openUrl(
        QUrl(QStringLiteral(LUMINANCEWEBSITE)));
}

void MainWindow::on_actionLuminance_HDR_Website_triggered() {
    QDesktopServices::openUrl(
        QUrl(QStringLiteral("http://qtpfsgui.sourceforge.net/")));
}

void MainWindow::setRealtimePreviewsActive(bool b) {
    LuminanceOptions().setRealtimePreviewsActive(b);
}

void MainWindow::setPreviewPanelActive(bool b) {
    LuminanceOptions().setPreviewPanelActive(b);
}
