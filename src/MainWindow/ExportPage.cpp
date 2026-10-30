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

#include "ExportPage.h"

#include <algorithm>
#include <functional>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSlider>
#include <QVBoxLayout>

#include "Common/LuminanceOptions.h"
#include "Libpfs/io/framewriterfactory.h"

namespace {
const QString KEY_RESULT_FORMAT = QStringLiteral("ExportPage/ResultFormat");
const QString KEY_HDR_FORMAT = QStringLiteral("ExportPage/HdrFormat");
const QString KEY_QUALITY = QStringLiteral("ExportPage/Quality");
const QString KEY_GLOW = QStringLiteral("ExportPage/Glow");

QLabel *sectionLabel(const QString &text) {
    QLabel *label = new QLabel(text.toUpper());
    label->setObjectName(QStringLiteral("SectionLabel"));
    return label;
}

bool writerAvailable(const char *format) {
    return pfs::io::FrameWriterFactory::isSupported(format);
}
}  // namespace

namespace {
//! a format tile: as large as its name and description need, in any
//! language, wrapping the description to the width it gets
class FormatTile : public QPushButton {
   public:
    FormatTile() {
        QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        policy.setHeightForWidth(true);
        setSizePolicy(policy);
    }
    QSize sizeHint() const override {
        return layout() ? layout()->sizeHint().expandedTo(QSize(0, 64))
                        : QPushButton::sizeHint();
    }
    QSize minimumSizeHint() const override {
        return layout() ? layout()->minimumSize().expandedTo(QSize(0, 64))
                        : QPushButton::minimumSizeHint();
    }
    bool hasHeightForWidth() const override {
        return layout() && layout()->hasHeightForWidth();
    }
    int heightForWidth(int width) const override {
        return layout() ? qMax(64, layout()->totalHeightForWidth(width))
                        : QPushButton::heightForWidth(width);
    }
};
}  // namespace

ExportPage::ExportPage(QWidget *parent) : QWidget(parent) {
    setObjectName(QStringLiteral("ExportPage"));
    setAttribute(Qt::WA_StyledBackground);

    // left: the picture, the queue under it
    QWidget *stage = new QWidget;
    stage->setObjectName(QStringLiteral("ExportStage"));
    stage->setAttribute(Qt::WA_StyledBackground);
    m_preview = new QLabel;
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setMinimumSize(240, 160);
    m_preview->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_caption = new QLabel;
    m_caption->setObjectName(QStringLiteral("HintLabel"));
    m_caption->setAlignment(Qt::AlignCenter);
    QWidget *queueBox = new QWidget;
    queueBox->setObjectName(QStringLiteral("ExportQueueBox"));
    queueBox->setAttribute(Qt::WA_StyledBackground);
    m_queueLayout = new QVBoxLayout(queueBox);
    m_queueLayout->setContentsMargins(16, 12, 16, 12);
    m_queueLayout->setSpacing(6);
    m_queueLayout->addWidget(sectionLabel(tr("Queue")));
    QVBoxLayout *stageLayout = new QVBoxLayout(stage);
    stageLayout->setContentsMargins(0, 0, 0, 0);
    stageLayout->setSpacing(0);
    QVBoxLayout *pictureLayout = new QVBoxLayout;
    pictureLayout->setContentsMargins(24, 24, 24, 16);
    pictureLayout->setSpacing(12);
    pictureLayout->addWidget(m_preview, 1);
    pictureLayout->addWidget(m_caption);
    stageLayout->addLayout(pictureLayout, 3);
    stageLayout->addWidget(queueBox, 2);

    // right: what, format, options, where
    QWidget *panel = new QWidget;
    panel->setObjectName(QStringLiteral("ExportPanel"));
    QVBoxLayout *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(20, 18, 20, 18);
    panelLayout->setSpacing(10);

    panelLayout->addWidget(sectionLabel(tr("What")));
    m_sourceResult = new QRadioButton;
    m_sourceHdr = new QRadioButton;
    panelLayout->addWidget(m_sourceResult);
    panelLayout->addWidget(m_sourceHdr);
    connect(m_sourceResult, &QRadioButton::toggled, this, [this]() {
        buildTiles();
        updatePreview();
    });

    panelLayout->addSpacing(10);
    panelLayout->addWidget(sectionLabel(tr("Format")));
    m_tiles = new QWidget;
    m_tileLayout = new QGridLayout(m_tiles);
    m_tileLayout->setContentsMargins(0, 0, 0, 0);
    m_tileLayout->setSpacing(8);
    for (int column = 0; column < 3; ++column)
        m_tileLayout->setColumnStretch(column, 1);
    m_tileGroup = new QButtonGroup(this);
    m_tileGroup->setExclusive(true);
    connect(m_tileGroup, &QButtonGroup::idClicked, this,
            &ExportPage::formatChanged);
    panelLayout->addWidget(m_tiles);

    // the options of the chosen format
    m_qualityRow = new QWidget;
    QHBoxLayout *qualityLayout = new QHBoxLayout(m_qualityRow);
    qualityLayout->setContentsMargins(0, 6, 0, 0);
    m_quality = new QSlider(Qt::Horizontal);
    m_quality->setRange(1, 100);
    m_quality->setValue(
        LuminanceOptions().value(KEY_QUALITY, 92).toInt());
    m_qualityValue = new QLabel;
    m_qualityValue->setMinimumWidth(28);
    m_qualityValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_qualityValue->setNum(m_quality->value());
    connect(m_quality, &QSlider::valueChanged, this, [this](int value) {
        m_qualityValue->setNum(value);
        LuminanceOptions().setValue(KEY_QUALITY, value);
    });
    qualityLayout->addWidget(new QLabel(tr("Quality")));
    qualityLayout->addWidget(m_quality, 1);
    qualityLayout->addWidget(m_qualityValue);
    panelLayout->addWidget(m_qualityRow);

    m_glow = new QCheckBox(tr("Include screen HDR"));
    m_glow->setToolTip(
        tr("Keeps the highlights of the HDR in the file: they glow on HDR "
           "screens (TV, monitor, Android, Chrome); on normal screens it "
           "depends on the viewer."));
    m_glow->setChecked(LuminanceOptions().value(KEY_GLOW, false).toBool());
    connect(m_glow, &QCheckBox::toggled, this,
            [](bool on) { LuminanceOptions().setValue(KEY_GLOW, on); });
    panelLayout->addWidget(m_glow);
    // JPEG as Ultra HDR: the same file, plus glow on HDR screens
    m_ultraHdr = new QCheckBox(tr("Ultra HDR JPEG"));
    m_ultraHdr->setToolTip(
        tr("A normal JPEG that also glows on HDR screens (Android, Chrome, "
           "HDR monitors); everywhere else it looks as usual."));
    m_ultraHdr->setChecked(
        LuminanceOptions().value(QStringLiteral("ExportPage/UltraHdr"), false)
            .toBool());
    connect(m_ultraHdr, &QCheckBox::toggled, this, [](bool on) {
        LuminanceOptions().setValue(QStringLiteral("ExportPage/UltraHdr"), on);
    });
    panelLayout->addWidget(m_ultraHdr);
#ifndef HAVE_LIBUHDR
    m_ultraHdr->hide();
#endif

    m_tiffRow = new QWidget;
    QHBoxLayout *tiffLayout = new QHBoxLayout(m_tiffRow);
    tiffLayout->setContentsMargins(0, 6, 0, 0);
    m_tiffMode = new QComboBox;
    tiffLayout->addWidget(new QLabel(tr("Depth")));
    tiffLayout->addWidget(m_tiffMode, 1);
    panelLayout->addWidget(m_tiffRow);

    // the size of the saved picture; Develop works on a smaller copy
    m_sizeRow = new QWidget;
    QHBoxLayout *sizeLayout = new QHBoxLayout(m_sizeRow);
    sizeLayout->setContentsMargins(0, 6, 0, 0);
    m_size = new QComboBox;
    m_size->setToolTip(
        tr("Width and height of the saved picture. Other sizes than the one "
           "shown are made again from the HDR."));
    connect(m_size, QOverload<int>::of(&QComboBox::activated), this,
            [this](int index) {
                if (m_size->itemData(index).toInt() >= 0) return;
                // "Other width...": asks, then adds it to the list
                bool ok = false;
                const int width = QInputDialog::getInt(
                    this, tr("Other width"),
                    tr("Width of the saved picture in pixels:"),
                    m_widths.value(0, 1000), 16, 100000, 1, &ok);
                if (ok && !m_widths.contains(width)) {
                    m_widths.append(width);
                    std::sort(m_widths.begin(), m_widths.end(),
                              std::greater<int>());
                }
                fillSizes(ok ? width : m_widths.value(0));
            });
    sizeLayout->addWidget(new QLabel(tr("Size")));
    sizeLayout->addWidget(m_size, 1);
    panelLayout->addWidget(m_sizeRow);

    panelLayout->addSpacing(10);
    panelLayout->addWidget(sectionLabel(tr("Where")));
    QHBoxLayout *folderLayout = new QHBoxLayout;
    m_folder = new QLabel;
    m_folder->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_folder->setMinimumWidth(80);
    m_folder->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    QPushButton *change = new QPushButton(tr("Change..."));
    change->setObjectName(QStringLiteral("QuietButton"));
    change->setCursor(Qt::PointingHandCursor);
    connect(change, &QPushButton::clicked, this, &ExportPage::chooseFolder);
    folderLayout->addWidget(m_folder, 1);
    folderLayout->addWidget(change);
    panelLayout->addLayout(folderLayout);
    QHBoxLayout *nameLayout = new QHBoxLayout;
    m_name = new QLineEdit;
    m_name->setPlaceholderText(tr("File name"));
    m_extension = new QLabel;
    m_extension->setObjectName(QStringLiteral("HintLabel"));
    nameLayout->addWidget(m_name, 1);
    nameLayout->addWidget(m_extension);
    panelLayout->addLayout(nameLayout);
    panelLayout->addStretch(1);

    m_folder->setText(QDir::toNativeSeparators(LuminanceOptions().getExportDir()));

    QScrollArea *scroll = new QScrollArea;
    scroll->setObjectName(QStringLiteral("ExportScroll"));
    scroll->setWidget(panel);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *actions = new QWidget;
    actions->setObjectName(QStringLiteral("ExportActions"));
    actions->setAttribute(Qt::WA_StyledBackground);
    QHBoxLayout *actionLayout = new QHBoxLayout(actions);
    actionLayout->setContentsMargins(20, 14, 20, 14);
    m_queueButton = new QPushButton(tr("Add to queue"));
    m_queueButton->setToolTip(
        tr("Makes the picture in the background, so you can go on editing; "
           "it is saved to the folder above"));
    m_exportButton = new QPushButton(tr("Export"));
    m_exportButton->setProperty("primary", true);
    m_exportButton->setDefault(true);
    m_exportButton->setMinimumHeight(40);
    m_queueButton->setMinimumHeight(40);
    actionLayout->addWidget(m_queueButton);
    actionLayout->addWidget(m_exportButton, 1);
    connect(m_queueButton, &QPushButton::clicked, this,
            &ExportPage::queueRequested);
    connect(m_exportButton, &QPushButton::clicked, this,
            &ExportPage::exportRequested);
    connect(m_name, &QLineEdit::returnPressed, this,
            &ExportPage::exportRequested);

    QWidget *side = new QWidget;
    side->setObjectName(QStringLiteral("ExportSide"));
    side->setAttribute(Qt::WA_StyledBackground);
    side->setFixedWidth(440);
    QVBoxLayout *sideLayout = new QVBoxLayout(side);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(0);
    sideLayout->addWidget(scroll, 1);
    sideLayout->addWidget(actions);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(stage, 1);
    layout->addWidget(side);

    m_sourceResult->setChecked(true);
}

QList<ExportPage::Format> ExportPage::formats(bool hdr) const {
    QList<Format> list;
    auto add = [&list](const char *writer, const QString &name,
                       const QString &hint, const QString &extension,
                       bool quality) {
        if (!QString::fromLatin1(writer).isEmpty() && !writerAvailable(writer))
            return;
        list.append({QString::fromLatin1(writer), name, hint, extension,
                     quality});
    };
    if (!hdr) {
        add("jpg", tr("JPEG"), tr("Opens everywhere"), QStringLiteral("jpg"),
            true);
        add("avif", tr("AVIF"), tr("Small, HDR possible"),
            QStringLiteral("avif"), true);
        add("jxl", tr("JPEG XL"), tr("Small, HDR possible"),
            QStringLiteral("jxl"), true);
        add("webp", tr("WebP"), tr("For the web"), QStringLiteral("webp"),
            true);
        add("png", tr("PNG"), tr("Lossless"), QStringLiteral("png"), false);
        add("tiff", tr("TIFF"), tr("Lossless, 8 or 16 bit"),
            QStringLiteral("tif"), false);
        // the rest Qt writes itself
        list.append({QStringLiteral("bmp"), tr("BMP"), tr("Uncompressed"),
                     QStringLiteral("bmp"), false});
        list.append({QStringLiteral("ppm"), tr("PPM / PBM"),
                     tr("Plain, for other tools"), QStringLiteral("ppm"),
                     false});
    } else {
        add("exr", tr("OpenEXR"), tr("HDR data for other programs"),
            QStringLiteral("exr"), false);
        add("hdr", tr("Radiance"), tr("HDR data, small"),
            QStringLiteral("hdr"), false);
        add("tiff", tr("TIFF"), tr("32 bit float or LogLuv"),
            QStringLiteral("tif"), false);
        add("avif_hdr", tr("AVIF HDR"), tr("Glows on HDR screens"),
            QStringLiteral("avif"), true);
        add("jxl_hdr", tr("JPEG XL HDR"), tr("Glows on HDR screens"),
            QStringLiteral("jxl"), true);
        add("pfs", tr("PFS"), tr("For pfstools"), QStringLiteral("pfs"),
            false);
        list.append({QStringLiteral("html"), tr("Web page"),
                     tr("With an HDR viewer"), QStringLiteral("html"),
                     false});
    }
    return list;
}

void ExportPage::buildTiles() {
    const bool hdr = exportsHdr();
    for (QAbstractButton *button : m_tileGroup->buttons()) {
        m_tileGroup->removeButton(button);
        button->deleteLater();
    }
    m_formats = formats(hdr);
    QString wanted =
        LuminanceOptions()
            .value(hdr ? KEY_HDR_FORMAT : KEY_RESULT_FORMAT,
                   hdr ? QStringLiteral("exr") : QStringLiteral("jpg"))
            .toString();
    // Ultra HDR was a tile of its own: now JPEG with the box ticked
    if (wanted == QLatin1String("ultrahdr")) {
        wanted = QStringLiteral("jpg");
        m_ultraHdr->setChecked(true);
    }
    int checked = 0;
    for (int i = 0; i < m_formats.size(); ++i) {
        const Format &format = m_formats.at(i);
        QPushButton *tile = new FormatTile;
        tile->setObjectName(QStringLiteral("FormatTile"));
        tile->setCheckable(true);
        tile->setCursor(Qt::PointingHandCursor);
        tile->setAccessibleName(format.name);
        tile->setToolTip(format.hint);
        QVBoxLayout *tileLayout = new QVBoxLayout(tile);
        tileLayout->setContentsMargins(12, 10, 12, 10);
        tileLayout->setSpacing(2);
        QLabel *name = new QLabel(format.name);
        name->setObjectName(QStringLiteral("FormatName"));
        QLabel *hint = new QLabel(format.hint);
        hint->setObjectName(QStringLiteral("FormatHint"));
        hint->setWordWrap(true);
        for (QLabel *label : {name, hint})
            label->setAttribute(Qt::WA_TransparentForMouseEvents);
        tileLayout->addWidget(name);
        tileLayout->addWidget(hint);
        tileLayout->addStretch();
        m_tileGroup->addButton(tile, i);
        m_tileLayout->addWidget(tile, i / 3, i % 3);
        if (format.id == wanted) checked = i;
    }
    if (QAbstractButton *tile = m_tileGroup->button(checked))
        tile->setChecked(true);
    formatChanged();
}

const ExportPage::Format *ExportPage::currentFormat() const {
    const int id = m_tileGroup->checkedId();
    return id >= 0 && id < m_formats.size() ? &m_formats.at(id) : nullptr;
}

void ExportPage::formatChanged() {
    const Format *format = currentFormat();
    if (!format) return;
    const bool hdr = exportsHdr();
    LuminanceOptions().setValue(hdr ? KEY_HDR_FORMAT : KEY_RESULT_FORMAT,
                                format->id);
    m_qualityRow->setVisible(format->quality);
    m_glow->setVisible(!hdr && (format->id == QLatin1String("avif") ||
                                format->id == QLatin1String("jxl")));
#ifdef HAVE_LIBUHDR
    m_ultraHdr->setVisible(!hdr && format->id == QLatin1String("jpg"));
#endif
    const bool tiff = format->id == QLatin1String("tiff");
    m_tiffRow->setVisible(tiff);
    if (tiff) {
        const QString key = hdr ? QStringLiteral("ExportPage/TiffHdr")
                                : QStringLiteral("ExportPage/TiffLdr");
        m_tiffMode->disconnect(this);
        m_tiffMode->clear();
        if (hdr) {
            m_tiffMode->addItem(tr("32 bit float"), 2);
            m_tiffMode->addItem(tr("LogLuv (smaller)"), 3);
        } else {
            m_tiffMode->addItem(tr("8 bit"), 0);
            m_tiffMode->addItem(tr("16 bit"), 1);
            m_tiffMode->addItem(tr("32 bit float"), 2);
        }
        m_tiffMode->setCurrentIndex(
            qBound(0, LuminanceOptions().value(key, hdr ? 0 : 1).toInt(),
                   m_tiffMode->count() - 1));
        connect(m_tiffMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [key](int index) { LuminanceOptions().setValue(key, index); });
    }
    const bool web = format->id == QLatin1String("html");
    m_extension->setText(QStringLiteral(".") + format->extension);
    m_queueButton->setVisible(!hdr);
    m_sizeRow->setVisible(!hdr && !web && !m_widths.isEmpty());
    m_exportButton->setText(web ? tr("Export...") : tr("Export"));
}

void ExportPage::setSizes(int fullWidth, int fullHeight) {
    if (fullWidth <= 0 || fullHeight <= 0) {
        m_widths.clear();
        fillSizes(0);
        return;
    }
    const bool sameHdr = !m_widths.isEmpty() && m_widths.first() == fullWidth &&
                         qFuzzyCompare(m_ratio, float(fullHeight) / fullWidth);
    const int keep = sameHdr ? resultWidth() : fullWidth;
    m_ratio = float(fullHeight) / fullWidth;
    if (!sameHdr) {
        // the full width, then halves and three quarters down to 256 px
        m_widths = {fullWidth};
        for (int x = fullWidth / 2; x >= 256; x /= 2) {
            if (3 * x / 2 < fullWidth) m_widths.append(3 * x / 2);
            m_widths.append(x);
        }
        std::sort(m_widths.begin(), m_widths.end(), std::greater<int>());
        m_widths.erase(std::unique(m_widths.begin(), m_widths.end()),
                       m_widths.end());
    }
    fillSizes(keep);
}

void ExportPage::fillSizes(int selected) {
    const QSignalBlocker block(m_size);
    m_size->clear();
    for (int width : std::as_const(m_widths)) {
        const QString size = QStringLiteral("%1 × %2").arg(width).arg(
            qRound(width * m_ratio));
        m_size->addItem(width == m_widths.first()
                            ? tr("%1 (full size)").arg(size)
                            : size,
                        width);
    }
    if (!m_widths.isEmpty()) m_size->addItem(tr("Other width..."), -1);
    const int index = m_size->findData(selected);
    m_size->setCurrentIndex(index >= 0 ? index : 0);
}

int ExportPage::resultWidth() const {
    const int width = m_size->currentData().toInt();
    return width > 0 ? width : 0;
}

void ExportPage::setSources(const QPixmap &result, const QString &resultCaption,
                            const QString &resultName, const QPixmap &hdr,
                            const QString &hdrCaption, const QString &hdrName) {
    m_resultPixmap = result;
    m_resultCaption = resultCaption;
    m_resultName = resultName;
    m_hdrPixmap = hdr;
    m_hdrCaption = hdrCaption;
    m_hdrName = hdrName;
    const bool hasResult = !resultName.isEmpty();
    const bool hasHdr = !hdrName.isEmpty();
    m_sourceResult->setText(hasResult ? tr("This picture · %1").arg(resultCaption)
                                      : tr("This picture"));
    m_sourceResult->setEnabled(hasResult);
    m_sourceHdr->setText(tr("The HDR itself (unedited)"));
    m_sourceHdr->setEnabled(hasHdr);
    const bool wasHdr = m_sourceHdr->isChecked();
    {
        const QSignalBlocker block(m_sourceResult);
        if (!hasResult)
            m_sourceHdr->setChecked(true);
        else if (!hasHdr || !wasHdr)
            m_sourceResult->setChecked(true);
    }
    buildTiles();
    updatePreview();
}

void ExportPage::updatePreview() {
    const bool hdr = exportsHdr();
    m_caption->setText(hdr ? m_hdrCaption : m_resultCaption);
    m_name->setText(hdr ? m_hdrName : m_resultName);
    scalePreview();
}

void ExportPage::scalePreview() {
    const QPixmap &pixmap = exportsHdr() ? m_hdrPixmap : m_resultPixmap;
    if (pixmap.isNull()) {
        m_preview->clear();
        return;
    }
    const QSize box = m_preview->size() * devicePixelRatioF();
    QPixmap scaled = pixmap.scaled(box.expandedTo(QSize(1, 1)),
                                   Qt::KeepAspectRatio, Qt::SmoothTransformation);
    scaled.setDevicePixelRatio(devicePixelRatioF());
    m_preview->setPixmap(scaled);
}

void ExportPage::setQueue(QWidget *queue) {
    m_queueLayout->addWidget(queue, 1);
}

bool ExportPage::exportsHdr() const { return m_sourceHdr->isChecked(); }

bool ExportPage::exportsWebPage() const {
    const Format *format = currentFormat();
    return format && format->id == QLatin1String("html");
}

pfs::Params ExportPage::params() const {
    pfs::Params params;
    const Format *format = currentFormat();
    if (!format) return params;
    std::string writer = format->id.toStdString();
    if (!exportsHdr() && m_glow->isChecked() &&
        (writer == "avif" || writer == "jxl"))
        writer += "_hdr";
#ifdef HAVE_LIBUHDR
    if (!exportsHdr() && writer == "jpg" && m_ultraHdr->isChecked())
        writer = "ultrahdr";
#endif
    // Qt writes BMP and PPM itself, by the file name
    if (writer != "bmp" && writer != "ppm") params.set("format", writer);
    params.set("fileextension", format->extension);
    if (format->quality) params.set("quality", (size_t)m_quality->value());
    else if (writer == "jpg" || writer == "png") params.set("quality", (size_t)100);
    if (writer == "tiff") params.set("tiff_mode", m_tiffMode->currentData().toInt());
    return params;
}

QString ExportPage::folder() const {
    return QDir::fromNativeSeparators(m_folder->text());
}

QString ExportPage::fileName() const {
    const QString name = m_name->text().trimmed();
    return name.isEmpty() ? QStringLiteral("LumiereHDR") : name;
}

QString ExportPage::filePath() const {
    const Format *format = currentFormat();
    return QDir(folder()).filePath(
        fileName() +
        (format ? QStringLiteral(".") + format->extension : QString()));
}

void ExportPage::chooseFolder() {
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Where to save"), folder());
    if (dir.isEmpty()) return;
    LuminanceOptions().setExportDir(dir);
    m_folder->setText(QDir::toNativeSeparators(dir));
}

void ExportPage::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    scalePreview();
}

void ExportPage::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) buildTiles();
    QWidget::changeEvent(event);
}
