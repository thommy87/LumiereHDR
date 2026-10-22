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

#ifndef STEPBAR_H
#define STEPBAR_H

#include <QWidget>

class QAction;
class QToolButton;

//! \brief Workflow bar above the image area: Import/Merge -> Tone -> Export.
//! Highlights the step the user is at and runs that step on a click.
class StepBar : public QWidget {
    Q_OBJECT
   public:
    enum Step { Import = 0, Tone = 1, Export = 2 };

    explicit StepBar(QWidget *parent = nullptr);

    //! Import runs \a newHdr; \a open is offered in its drop-down menu.
    void setImportActions(QAction *newHdr, QAction *open);
    //! Export runs \a saveAs.
    void setExportAction(QAction *saveAs);

    void setCurrentStep(Step step);
    void setToneEnabled(bool enabled);
    void setExportEnabled(bool enabled);

   signals:
    void toneRequested();

   protected:
    void changeEvent(QEvent *event) override;

   private:
    void retranslate();
    void refresh();

    QToolButton *m_buttons[3];
    Step m_current;
};

#endif
