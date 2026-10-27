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
 * Original Work
 * @author Davide Anastasia <davideanastasia@users.sourceforge.net>
 *
 */

#ifndef TMWORKER_H
#define TMWORKER_H

#include <QMetaType>
#include <QMutex>
#include <QObject>
#include <QSet>
#include <QString>
#include <QVector>

#include <memory>

#include <Common/global.h>
#include <Core/TonemappingOptions.h>
#include <Libpfs/params.h>

// Forward declaration
namespace pfs {
class Frame;
}

class ProgressHelper;

//! one picture of the export queue, with everything needed to make it again
struct ExportJob {
    int id = 0;
    std::shared_ptr<pfs::Frame> hdr;  // kept alive while the job exists
    TonemappingOptions options;
    pfs::Params params;
    QString exportDir;
    QString hdrName;
    QString inputFileName;
    QVector<float> expoTimes;
    InterpolationMethod method = BilinearInterp;
};
typedef std::shared_ptr<ExportJob> ExportJobPtr;
Q_DECLARE_METATYPE(ExportJobPtr)

class TMWorker : public QObject {
    Q_OBJECT

   public:
    TMWorker(QObject *parent = 0);
    ~TMWorker();

   public Q_SLOTS:
    //!
    //!  This function creates a copy of the input frame, tonemap the copy
    //!  and then returns it
    //!
    pfs::Frame *computeTonemap(/* const */ pfs::Frame *, TonemappingOptions *,
                               InterpolationMethod m);

    //! makes and saves one picture of the export queue
    void runExportJob(ExportJobPtr job);

    //!
    //! This function tonemap the input frame
    //!
    void tonemapFrame(pfs::Frame *, TonemappingOptions *);

   public:
    //! stops job \a id: skipped if still waiting, cut short if running.
    //! Safe to call from any thread.
    void cancelExportJob(int id);

   private:
    pfs::Frame *preprocessFrame(pfs::Frame *, TonemappingOptions *,
                                InterpolationMethod m);
    void postprocessFrame(pfs::Frame *, TonemappingOptions *);

   Q_SIGNALS:
    void tonemapSuccess(pfs::Frame *, TonemappingOptions *);
    void tonemapFailed(QString);

    void tonemapBegin();
    void tonemapEnd();
    void tonemapSetMaximum(int);
    void tonemapSetMinimum(int);
    void tonemapSetValue(int);
    void tonemapRequestTermination(bool);

    void exportJobStarted(int id);
    //! \a result: 0 written, 1 failed, 2 cancelled
    void exportJobFinished(int id, QString file, int result);

   private:
    ProgressHelper *m_Callback;

    QMutex m_jobMutex;
    QSet<int> m_canceledJobs;
    int m_runningJob = -1;

    // the HDR reduced to the result size, kept for the next run: live
    // previews would otherwise scale the full photo down on every change
    std::unique_ptr<pfs::Frame> m_reduced;
    const pfs::Frame *m_reducedFrom = nullptr;
    int m_reducedWidth = 0;
    int m_reducedMethod = -1;
    double m_reducedFingerprint = 0;
};

#endif  // TMWORKER_H
