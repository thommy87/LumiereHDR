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
 * This class hold the Tonemapping Options
 *
 * Original Work
 * @author Giuseppe Rota <grota@users.sourceforge.net>
 * @author Franco Comida <fcomida@users.sourceforge.net>
 *
 * New Design based on class rather then struct
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#ifndef TONEMAPPINGOPTIONS_H
#define TONEMAPPINGOPTIONS_H

#include <QObject>
#include <QString>

#include "Libpfs/tm/TonemapParams.h"

class TonemappingOptions : public TonemapParams {
   public:
    // as shown in the tone mapping panel, for readable file names
    QString operatorLabel;  // e.g. "Natural contrast (Mantiuk 2006)"
    QString presetName;     // empty for own settings

    // default constructor
    TonemappingOptions() { setDefaultParameters(); }
    const QString getPostfix();

    /** returns the translated description of the TMO operator**/
    const QString getCaption(bool pregamma = true,
                             QString separator = QStringLiteral(" ~ "));
    void setDefaultTonemapParameters();
    void setDefaultParameters();

    char getRatingForOperator();
};

/*
 * TODO: Davide Anastasia 2011.11.19
 *
 * This class depends strongly on the TonemappingOptions
 * However, most of these functionality can be brought inside TonemappingOptions
 * and get rid of this class
 */

class TMOptionsOperations {
   public:
    TMOptionsOperations(const TonemappingOptions *opts);
    static TonemappingOptions *parseFile(const QString &file);
    static TonemappingOptions *getDefaultTMOptions();
    QString getExifComment();

   private:
    const TonemappingOptions *opts;
};

#endif  // TONEMAPPINGOPTIONS_H
