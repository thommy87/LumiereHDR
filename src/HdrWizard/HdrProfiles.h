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

//! \brief Plain names and explanations for the six predefined HDR creation
//! profiles (predef_confs), shared by the HDR wizard and Batch HDR.

#ifndef HDRPROFILES_H
#define HDRPROFILES_H

#include <QComboBox>
#include <QCoreApplication>
#include <QFileInfo>
#include <QString>
#include <QStringList>

namespace hdrprofiles {

// in the order of predef_confs: weights triangular, plateau, gaussian,
// each with the linear (raw) and the gamma (JPEG/TIFF) response
inline QString name(int index) {
    static const char *names[6] = {
        QT_TRANSLATE_NOOP("HdrProfiles", "Standard, for raw files"),
        QT_TRANSLATE_NOOP("HdrProfiles", "Standard, for JPEG/TIFF"),
        QT_TRANSLATE_NOOP("HdrProfiles", "Smooth, less noise, for raw files"),
        QT_TRANSLATE_NOOP("HdrProfiles", "Smooth, less noise, for JPEG/TIFF"),
        QT_TRANSLATE_NOOP("HdrProfiles",
                          "Careful with highlights, for raw files"),
        QT_TRANSLATE_NOOP("HdrProfiles",
                          "Careful with highlights, for JPEG/TIFF")};
    static const char *technical[6] = {
        "Debevec, Triangular, Linear", "Debevec, Triangular, Gamma",
        "Debevec, Plateau, Linear",    "Debevec, Plateau, Gamma",
        "Debevec, Gaussian, Linear",   "Debevec, Gaussian, Gamma"};
    if (index < 0 || index > 5) return QString();
    return QStringLiteral("%1 (%2)").arg(
        QCoreApplication::translate("HdrProfiles", names[index]),
        QLatin1String(technical[index]));
}

inline QString help(int index) {
    static const char *weights[3] = {
        QT_TRANSLATE_NOOP("HdrProfiles",
                          "Balanced: every photo counts most in its "
                          "well-exposed mid-tones. A good default."),
        QT_TRANSLATE_NOOP("HdrProfiles",
                          "Uses a wider range of each photo, which gives "
                          "smoother results with less noise."),
        QT_TRANSLATE_NOOP("HdrProfiles",
                          "Trusts only the safest mid-tones, which helps "
                          "when bright areas are blown out or shadows are "
                          "noisy.")};
    static const char *responses[2] = {
        QT_TRANSLATE_NOOP("HdrProfiles",
                          "For raw files, whose data is linear."),
        QT_TRANSLATE_NOOP("HdrProfiles",
                          "For JPEG or TIFF photos from the camera, which "
                          "are already gamma encoded.")};
    if (index < 0 || index > 5) return QString();
    return QCoreApplication::translate("HdrProfiles", weights[index / 2]) +
           QLatin1Char(' ') +
           QCoreApplication::translate("HdrProfiles", responses[index % 2]);
}

//! names and tooltips for the first six entries of a profile list
inline void label(QComboBox *combo) {
    for (int i = 0; i < 6 && i < combo->count(); ++i) {
        combo->setItemText(i, name(i));
        combo->setItemData(i, help(i), Qt::ToolTipRole);
    }
}

//! the standard profile that suits the files: raw or JPEG/TIFF
inline int standardFor(const QStringList &files) {
    static const QStringList raw = {
        QStringLiteral("cr2"), QStringLiteral("cr3"), QStringLiteral("crw"),
        QStringLiteral("nef"), QStringLiteral("nrw"), QStringLiteral("arw"),
        QStringLiteral("srf"), QStringLiteral("sr2"), QStringLiteral("dng"),
        QStringLiteral("raf"), QStringLiteral("orf"), QStringLiteral("rw2"),
        QStringLiteral("pef"), QStringLiteral("srw"), QStringLiteral("x3f"),
        QStringLiteral("3fr"), QStringLiteral("mrw"), QStringLiteral("kdc"),
        QStringLiteral("dcr"), QStringLiteral("raw"), QStringLiteral("ptx"),
        QStringLiteral("mef"), QStringLiteral("mos"), QStringLiteral("erf")};
    for (const QString &file : files)
        if (raw.contains(QFileInfo(file).suffix().toLower())) return 0;
    return 1;
}

}  // namespace hdrprofiles

#endif
