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

#ifndef EXPORTPAGE_H
#define EXPORTPAGE_H

#include <QWidget>

#include <Libpfs/params.h>

class QAbstractButton;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QGridLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSlider;
class QVBoxLayout;

//! \brief The Export workspace: the picture on the left with the export
//! queue under it; on the right what to save, the format as tiles, its few
//! options and where it goes.
class ExportPage : public QWidget {
    Q_OBJECT
   public:
    explicit ExportPage(QWidget *parent = nullptr);

    //! what there is to export: the edited picture (if any) and the HDR
    void setSources(const QPixmap &result, const QString &resultCaption,
                    const QString &resultName, const QPixmap &hdr,
                    const QString &hdrCaption, const QString &hdrName);
    //! the size of the HDR: the saved picture can have it or be smaller
    void setSizes(int fullWidth, int fullHeight);
    //! the chosen width of the saved picture, 0 when there is no choice
    int resultWidth() const;
    //! the queue list, shown under the picture
    void setQueue(QWidget *queue);

    //! true when the HDR itself is saved, not the edited picture
    bool exportsHdr() const;
    //! true for the "Web page" tile, which has a dialog of its own
    bool exportsWebPage() const;
    //! the writer settings of the chosen tile and options
    pfs::Params params() const;
    //! folder, name and extension together
    QString filePath() const;
    QString folder() const;
    //! the name without folder and extension
    QString fileName() const;

   Q_SIGNALS:
    void exportRequested();
    void queueRequested();

   protected:
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

   private:
    struct Format {
        QString id;  // writer name, or "html"
        QString name;
        QString hint;
        QString extension;
        bool quality;
    };
    void buildTiles();
    void formatChanged();
    void updatePreview();
    void scalePreview();
    void chooseFolder();
    void fillSizes(int selected);
    const Format *currentFormat() const;
    QList<Format> formats(bool hdr) const;

    QLabel *m_preview;
    QLabel *m_caption;
    QVBoxLayout *m_queueLayout;
    QRadioButton *m_sourceResult;
    QRadioButton *m_sourceHdr;
    QWidget *m_tiles;
    QGridLayout *m_tileLayout;
    QButtonGroup *m_tileGroup;
    QList<Format> m_formats;
    QWidget *m_qualityRow;
    QSlider *m_quality;
    QLabel *m_qualityValue;
    QCheckBox *m_glow;
    QCheckBox *m_ultraHdr;
    QWidget *m_tiffRow;
    QComboBox *m_tiffMode;
    QWidget *m_sizeRow;
    QComboBox *m_size;
    QList<int> m_widths;
    float m_ratio = 1.f;
    QLabel *m_folder;
    QLineEdit *m_name;
    QLabel *m_extension;
    QPushButton *m_queueButton;
    QPushButton *m_exportButton;

    QPixmap m_resultPixmap, m_hdrPixmap;
    QString m_resultCaption, m_hdrCaption, m_resultName, m_hdrName;
    QString m_lastResultFormat, m_lastHdrFormat;
};

#endif  // EXPORTPAGE_H
