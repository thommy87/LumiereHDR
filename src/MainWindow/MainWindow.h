/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006-2007 Giuseppe Rota
 * Copyright (C) 2011-2012 Davide Anastasia
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
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDockWidget>
#include <QFutureWatcher>
#include <QHash>
#include <memory>
#include <QMainWindow>
#include <QMap>
#include <QProgressBar>
#include <QScopedPointer>
#include <QScrollArea>
#include <QSignalMapper>
#include <QSplitter>
#include <QString>
#include <QStringList>
#include <QTabWidget>
#include <QThread>

#include "Common/LuminanceOptions.h"
#include "Common/global.h"

#define MAX_RECENT_FILES (5)

// Forward declaration
namespace Ui {
class MainWindow;
}

namespace pfs {
class Frame;  // #include "Libpfs/frame.h"
}

class IOWorker;  // #include "Core/IOWorker.h"
class GenericViewer;
class LdrViewer;
class HdrViewer;
class PreviewPanel;          // #include "PreviewPanel/PreviewPanel.h"
class HelpBrowser;           // #include "HelpBrowser/helpbrowser.h"
class TMOProgressIndicator;  // #include
                             // "TonemappingPanel/TMOProgressIndicator.h"
class TonemappingPanel;      // #include "TonemappingPanel/TonemappingPanel.h"
class HomePage;             // #include "UI/HomePage.h"
class TopBar;                // #include "UI/TopBar.h"
class QStackedWidget;
class PresetStrip;
class QUndoGroup;
class QUndoView;
class FilmStrip;
class QButtonGroup;
class QAbstractButton;
class QTimer;
class QDockWidget;
class HistogramPanel;
class TonemappingOptions;    // #include "Core/TonemappingOptions.h"
class TMWorker;

class UpdateChecker;  // #include "MainWindow/UpdateChecker.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

   public:
    MainWindow(QWidget *parent = 0);
    // Constructor loading file inside
    MainWindow(pfs::Frame *curr_frame, const QString &new_fname,
               const QStringList &inputFileNames, bool needSaving = false,
               QWidget *parent = 0);
    ~MainWindow();

   public Q_SLOTS:

    // I/O
    void save_hdr_success(GenericViewer *saved_hdr, const QString &fname);
    void save_hdr_failed(const QString &fname);
    void save_ldr_success(GenericViewer *saved_ldr, const QString &fname);
    void save_ldr_failed(const QString &fname);

    void load_failed(const QString &);
    void load_success(pfs::Frame *new_hdr_frame, const QString &new_fname,
                      const QStringList &inputFileNames = QStringList(),
                      bool needSaving = false);

    void ioBegin();
    void ioEnd();

    void setMainWindowModified(bool b);

    void openFile(const QString &file);
    void openFiles(const QStringList &files);

   protected Q_SLOTS:

    void on_fileNewAction_triggered();
    void createNewHdr(const QStringList &files);

    // for File->Open, it then calls loadFile()
    void on_fileOpenAction_triggered();
    void on_fileSaveAsAction_triggered();
    void on_fileSaveAllAction_triggered();
    void on_actionSave_Hdr_Preview_triggered();

    void on_rotateccw_triggered();
    void on_rotatecw_triggered();

    void on_actionResizeHDR_triggered();
    void on_action_Projective_Transformation_triggered();

    void on_actionBatch_HDR_triggered();
    void on_actionBatch_Tone_Mapping_triggered();

    void on_actionExportToHTML_triggered();
    void on_actionSupported_Cameras_triggered();

    void on_Increase_exposure_triggered();
    void on_Decrease_exposure_triggered();
    void on_Extend_dynamic_range_triggered();
    void on_Shrink_dynamic_range_triggered();
    void on_Fit_to_dynamic_range_triggered();
    void on_Low_dynamic_range_triggered();

    // Viewers
    void on_zoomInAct_triggered();
    void on_zoomOutAct_triggered();
    void on_fitToWindowAct_triggered();
    void on_actionFill_to_Window_triggered();
    void on_normalSizeAct_triggered();
    void updateMagnificationButtons(GenericViewer *);

    void on_documentationAction_triggered();
    void enterWhatsThis();

    void on_OptionsAction_triggered();
    void on_Transplant_Exif_Data_action_triggered();

    void on_actionFix_Histogram_toggled(bool checked);
    void on_actionWhite_Balance_triggered();
    void whiteBalanceDone();

    // Tool Bar Handling
    void Text_Under_Icons();
    void Icons_Only();
    void Text_Alongside_Icons();
    void Text_Only();
    void showPreviewsOnTheRight();
    void showPreviewsOnTheBottom();

    // Window Menu Display and Functionalities
    void updateWindowMenu();
    void on_actionBring_All_to_Front_triggered();

    void openRecentFile();
    void setCurrentFile(const QString &fileName);

    void on_actionAbout_Luminance_triggered();

    void updateActions(int w);
    void setActiveMainWindow(QWidget *w);

    void cropToSelection();
    void enableCrop(bool);
    void disableCrop();

    void helpBrowserClosed();
    void on_actionDonate_triggered();

    void onUpdateAvailable();
    void on_actionUpdateAvailable_triggered();

    // TM
    void tonemapBegin();
    void tonemapEnd();
    void tonemapImage(TonemappingOptions *opts);
    void exportImage(TonemappingOptions *opts);
    void addLdrFrame(pfs::Frame *, TonemappingOptions *);
    // void addLDRResult(QImage*, quint16*);
    void tonemapFailed(const QString &);

    // Export queue
    void exportBegin();
    void exportEnd();

    // lock functionalities
    void on_actionLock_toggled(bool);
    void syncViewers(GenericViewer *);

    void showPreviewPanel(bool b);

    // QTabWidget
    void removeTab(int);
    void on_actionRemove_Tab_triggered();

    void on_actionSoft_Proofing_toggled(bool);
    void on_actionGamut_Check_toggled(bool);
    void updateSoftProofing(int);

    void on_actionFits_Importer_triggered();

    void on_actionShow_Full_Screen_toggled(bool);
    void on_actionShow_Image_Full_Screen_triggered();
    void on_actionSelect_Interpolation_Method_toggled(bool);

    void updatePreviews(bool, float);

    void reparentViewer(GenericViewer *g_vi, bool = false);
    void showNextViewer(GenericViewer *g_v);
    void showPreviousViewer(GenericViewer *g_v);
    void setSyncViewers(GenericViewer *);

    // Online Docs, Website, GitHub
    void on_actionOnline_Documentation_triggered();
    void on_actionDevelopers_Website_triggered();
    void on_actionLuminance_HDR_Website_triggered();

   Q_SIGNALS:
    // update HDR
    void updatedHDR(pfs::Frame *);

   protected:
    QSplitter *m_centralwidget_splitter;
    QSplitter *m_bottom_splitter;

    QTabWidget *m_tabwidget = nullptr;

    QSignalMapper *windowMapper;
    QDialog *splash;

    // Recent Files Management
    QAction *recentFileActs[MAX_RECENT_FILES];
    QAction *separatorRecentFiles;

    // Open MainWindows Handling
    QList<QAction *> openMainWindows;

    //! \brief contains the file names of the images loaded by the wizard,
    //! they are used to copy EXIF tags to saved LDR images
    QStringList m_inputFilesName;

    //! \brief contain output file name prefix in the form 1st bracketed file
    //! name
    //! "-" last bracketed file name
    QString m_outputFileNamePrefix;

    //! \brief contain Hdr creation model to use as suffix in HDR output file
    //! name
    QString m_HdrCaption;

    //! \brief this contains the exposure times of the images to write to LDR
    //! as EXIF comment
    QVector<float> m_inputExpoTimes;

    //! \group Event handler
    virtual void dragEnterEvent(QDragEnterEvent *);
    virtual void dropEvent(QDropEvent *);
    virtual void changeEvent(QEvent *event);
    virtual void closeEvent(QCloseEvent *);
    virtual bool event(QEvent *event);
    //!

    void dispatchrotate(bool clockwise);

    void updateRecentFileActions();
    void initRecentFileActions();
    void clearRecentFileActions();

    struct {
        bool is_hdr_ready;
        GenericViewer *curr_tm_frame;
        TonemappingOptions *curr_tm_options;
    } tm_status;
    int num_ldr_generated;
    int curr_num_ldr_open;

    void init();
    void createUI();
    void loadOptions();
    void createMenus();
    void createToolBar();
    void createCentralWidget();
    void createStatusBar();
    void setupIO();
    void setupTM();
    void setupQueue();
    void createConnections();

    void updatePreviousNextActions();

    QString getCurrentHDRName();

    bool maybeSave();

    void setRealtimePreviewsActive(bool);
    void setPreviewPanelActive(bool b);

    // Preview Panel
    QScrollArea *m_PreviewscrollArea;
    PreviewPanel *m_PreviewPanel;

   private:
    // UI declaration
    QScopedPointer<Ui::MainWindow> m_Ui;

    // keep track of fullscreen image viewer
    QString m_tabText;
    int m_fullscreenViewerIndex;
    bool m_isFullscreenViewer;
    GenericViewer *m_g_v_fullscreen;

    // Tone Mapping Panel
    TonemappingPanel *m_tonemapPanel;
    //! workspaces, search, batch and settings across the top
    TopBar *m_topBar = nullptr;
    //! the Start workspace, or the image with its panels
    QStackedWidget *m_workspace;
    HomePage *m_startPage = nullptr;
    int m_lastTabCount = 0;
    void createShell();
    void showWorkspace(bool images);
    void openCommandPalette();
    //! actions drawn with the line icons, which follow the colour scheme
    QList<QPair<QAction *, QString>> m_lineIcons;
    QList<QPair<QAbstractButton *, QString>> m_lineIconButtons;
    //! the looks and the history, left of the image
    QWidget *createLooksPanel();
    QWidget *m_looksPanel = nullptr;
    FilmStrip *m_filmStrip = nullptr;
    QButtonGroup *m_looksSwitch = nullptr;
    QStackedWidget *m_looksPages = nullptr;
    QUndoView *m_historyView = nullptr;
    void refreshLineIcons();
    QAction *m_compareAct;
    void applyTabStyle();
    bool m_resultSaved = false;  // Export done until the next change
    PresetStrip *m_presetStrip;
    // undo/redo of HDR edits, one stack per open image
    QUndoGroup *m_undoGroup;
    void applyHdrEdit(GenericViewer *viewer, pfs::Frame *edited,
                      const QString &text);
    void markModified(GenericViewer *viewer);
    // live histogram of the shown image
    HistogramPanel *m_histogramPanel;
    QTimer *m_histogramTimer;
    // dynamic range of the last HDR shown, computed once per frame
    const pfs::Frame *m_rangeFrame = nullptr;
    double m_rangeStops = 0.0;
    void watchViewer(GenericViewer *viewer);
    void updateHistogram();

    // I/O
    QThread *m_IOThread;
    IOWorker *m_IOWorker;
    QProgressBar *m_ProgressBar;

    // TM thread
    QThread *m_TMThread;
    TMWorker *m_TMWorker;
    // one tone mapping at a time; while it runs only the newest request
    // waits, older ones are dropped
    bool m_tmBusy = false;
    bool m_tmBusyLive = false;
    TonemappingOptions *m_tmPending = nullptr;
    void startPendingTonemap();
    // the export queue: its window and every job, kept to try again
    class ExportQueueDialog *m_exportQueue = nullptr;
    QHash<int, std::shared_ptr<struct ExportJob>> m_exportJobs;
    int m_nextExportJob = 1;
    void queueExportJob(int id);
    //! the HDR exactly as the HDR tab shows it, as a picture to export
    void showUnedited();
    bool m_showingUnedited = false;
    TMOProgressIndicator *m_TMProgressBar;

    // Export queue
    QThread *m_QueueThread;
    TMWorker *m_QueueWorker;
    TMOProgressIndicator *m_QueueProgressBar;

    int m_exportQueueSize;

    //
    InterpolationMethod m_interpolationMethod;

    QFutureWatcher<void> m_futureWatcher;
    GenericViewer *m_viewerToProcess;
    bool m_processingAWB;
    int m_firstWindow;
    int m_winId;  // unique MainWindow identifier

    static int sm_NumMainWindows;  // number of active MainWindows
    static int sm_counter;         // Always increases
    static QScopedPointer<UpdateChecker> sm_updateChecker;
    static QMap<int, MainWindow *>
        sm_mainWindowMap;  // maps m_winId with MainWindow "this" pointer
    static HelpBrowser *sm_helpBrowser;
};

#endif  // MAINWINDOW_H
