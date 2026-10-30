/**
 * This file is a part of Luminance HDR package.
 * ----------------------------------------------------------------------
 * Copyright (C) 2006,2007 Giuseppe Rota
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
 *
 */

#ifndef TONEMAPPINGWIDGET_H
#define TONEMAPPINGWIDGET_H

#include <QStatusBar>

#include <Core/TonemappingOptions.h>
#include <Libpfs/frame.h>
#include <PreviewPanel/PreviewPanel.h>
#include <TonemappingPanel/ThresholdWidget.h>

class Gang;
class QtWaitingSpinner;

class QLabel;
class QCheckBox;
class QTimer;
class QSlider;
class QMenu;

namespace Ui {
class TonemappingPanel;
}

class TonemappingPanel : public QWidget {
    Q_OBJECT

   protected:
    Gang
        // mantiuk06
        *contrastfactorGang,
        *saturationfactorGang, *detailfactorGang,
        // mantiuk08
        *colorSaturationGang, *contrastEnhancementGang, *luminanceLevelGang,
        // fattal02
        *alphaGang, *betaGang, *saturation2Gang, *noiseGang,
        // *oldFattalGang,
        *fftSolverGang,
        // ferrands11
        *rhoGang, *inv_alphaGang,
        // ashikhmin02
        *contrastGang, *simpleGang, *eq2Gang,
        // drago03
        *biasGang,
        // durand02
        *spatialGang, *rangeGang, *baseGang,
        // pattanaik00
        *multiplierGang, *coneGang, *rodGang, *autoYGang, *pattalocalGang,
        // reinhard02
        *keyGang, *phiGang, *range2Gang, *lowerGang, *upperGang, *usescalesGang,
        // reinhard05
        *brightnessGang, *chromaticGang, *lightGang,
        // ferwerda96
        *ferwerdamultiplierGang, *adaptationGang,
        // kimkautz08
        *kimkautzc1Gang,
        *kimkautzc2Gang,
        // vanhateren06
        *vanhaterenPupilAreaGang,
        // lischinski06
        *lischinskiAlphaGang,
        //
        *pregammaGang,
        //
        *postgammaGang,
        //
        *postsaturationGang;

    TMOperator m_currentTmoOperator;
    TonemappingOptions *m_toneMappingOptions;
    QList<TonemappingOptions *> m_toneMappingOptionsToDelete;
    QVector<int> sizes;
    void fillToneMappingOptions(bool exportMode);
    void setupUndo();
    void fromGui2Txt(QString destination);  // i.e. WRITE tmo settings to text
                                            // file
    // QString recentPathLoadSaveTmoSettings;
    QString tmoSettingsFilename;
    int out_ldr_cs;

    float heightToWidthRatio;
    bool adding_custom_size;
    PreviewPanel *m_previewPanel;

    void createDatabase();
    int m_mainWinNumber;

    pfs::Frame *m_currentFrame;
    float m_autolevelThreshold;
    QScopedPointer<ThresholdWidget> m_thd;
    QScopedPointer<Ui::TonemappingPanel> m_Ui;
    QMenu *m_presetMenu;
    QLabel *m_operatorHelp = nullptr;
    QLabel *m_presetLabel = nullptr;
    // live preview: re-run tone mapping shortly after a change
    QCheckBox *m_liveCheck = nullptr;
    QTimer *m_liveTimer = nullptr;
    bool m_lastRunWasLive = false;
    void scheduleLivePreview();
    void runLivePreview();
    // basic sliders that act on the result of any operator
    Gang *postbrightnessGang = nullptr;
    Gang *postcontrastGang = nullptr;
    Gang *postwarmthGang = nullptr;
    QSlider *m_postbrightnessSlider = nullptr;
    QSlider *m_postcontrastSlider = nullptr;
    QSlider *m_postwarmthSlider = nullptr;
    QString m_currentPreset;
    bool m_applyingPreset = false;
    void applyOperatorNames();

   protected Q_SLOTS:
    virtual void changeEvent(QEvent *event);
    void on_pregammadefault_clicked();
    void on_postgammadefault_clicked();
    void on_postsaturationdefault_clicked();
    void on_defaultButton_clicked();
    void on_applyButton_clicked();
    void on_undoButton_clicked();
    void on_redoButton_clicked();
    void on_savesettingsbutton_clicked();
    void on_loadsettingsbutton_clicked();
    void fillPresetMenu();
    void applyPreset(const QString &fileName, const QString &name);
    void savePreset();
    void markCustomSettings();
    void on_queueButton_clicked();
    void on_lblOpenQueue_linkActivated(const QString &link);

    // APPLY tmo settings from text file
    void fromTxt2Gui();
    // user wants a custom size.
    void on_addCustomSizeButton_clicked();
    void fillCustomSizeComboBox();
    void updateCurrentTmoOperator(int);
    void updateUndoState();
    void loadParameters();
    void saveParameters();
    void execMantiuk06Query(bool, float, float, float, QString);
    void execMantiuk08Query(float, float, float, bool, QString);
    void execAshikhminQuery(bool, bool, float, QString);
    void execDragoQuery(float, QString);
    void execDurandQuery(float, float, float, QString);
    void execFattalQuery(float, float, float, float, bool, QString);
    void execFerradansQuery(float, float, QString);
    void execFerwerdaQuery(float, float, QString);
    void execKimKautzQuery(float, float, QString);
    void execPattanaikQuery(bool, bool, float, float, float, QString);
    void execReinhard02Query(bool, float, float, int, int, int, QString);
    void execReinhard05Query(float, float, float, QString);
    void execVanHaterenQuery(float, QString);
    void execLischinskiQuery(float, QString);
    void execMaiQuery(QString);

    void updatePreviews(double);
    void updatePreviewsCB(bool);
    void updatePreviewsRB(bool);

    void on_pattalocal_toggled(bool);
    void on_toolButtonThreshold_clicked();
    void thresholdReady();

   public:
    TonemappingPanel(int mainWinNumber, PreviewPanel *p = 0,
                     QWidget *parent = 0);
    ~TonemappingPanel();
    void setSizes(int, int);
    bool replaceLdr();
    //! file name of the "Unedited" preset, which has no settings file
    static QString uneditedPreset() { return QStringLiteral("unedited:"); }
    bool lastRunWasLive() const { return m_lastRunWasLive; }
    bool doAutoLevels();
    float getAutoLevelsThreshold();
    void setExportQueueSize(int);
    QString & getDatabaseConnection();

    //! name and settings file of the built-in and the user's presets
    //! (name, file) of every preset, built in and own
    static QList<QPair<QString, QString>> presets();
    //! the folder of the user's own looks
    static QString ownPresetFolder();
    //! for the Looks panel
    void saveCurrentLook() { savePreset(); }
    void loadLookFile() { on_loadsettingsbutton_clicked(); }
    void saveLookFile() { on_savesettingsbutton_clicked(); }
    void loadFromDatabase() { loadParameters(); }
    void saveToDatabase() { saveParameters(); }

    //! all settings as text, for the history
    QString settingsSnapshot();
    //! puts settings of the history back and shows them; an empty
    //! snapshot is the HDR as it came
    void restoreSnapshot(const QString &snapshot, const QString &preset);
    //! a new picture: the history starts again from here
    void resetHistory();

   public Q_SLOTS:
    void setEnabled(bool);
    //! sets operator and sliders from a preset file and tone maps
    void applyPresetFile(const QString &fileName, const QString &name);
    void updatedHDR(pfs::Frame *);
    void updateTonemappingParams(TonemappingOptions *opts);
    void setRealtimePreviews(bool);
    void autoLevels(bool b);
    //! Same as the Tonemap button; does nothing while the panel is disabled.
    void tonemap();

   signals:
    //! a preset was saved, so the list of presets changed
    void presetsChanged();
    //! the preset now applied; empty once a setting is changed by hand
    void currentPresetChanged(const QString &name);
    void startTonemapping(TonemappingOptions *);
    //! the "Unedited" preset: take the HDR as the HDR tab shows it
    void uneditedRequested();
    //! a changed look was shown: what changed, and the settings before
    //! and after (for the history)
    void lookStepped(const QString &label, const QString &before,
                     const QString &after, const QString &preset);
    void startExport(TonemappingOptions *);
    //! show the export queue
    void showExportQueue();
    void autoLevels(bool, float);

   private:
    void onUndoRedo(bool undo);
    //! tells the history about the settings that are tone mapped now
    void recordStep();
    QString describeChange(QObject *changed) const;

    QString m_lastSnapshot;
    QString m_lastChange;
    bool m_restoring = false;

    QtWaitingSpinner *m_spinner;
    static int sm_counter;
    QString m_databaseconnection;
};

#endif
