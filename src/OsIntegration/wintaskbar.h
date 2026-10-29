/*
 * This file is a part of LumiereHDR package.
 * ----------------------------------------------------------------------
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * ----------------------------------------------------------------------
 */

//! \brief Progress on the Windows taskbar button and recent files in its
//! jump list, through the Windows API itself (Qt 6 has no QtWinExtras)

#ifndef WINTASKBAR_H
#define WINTASKBAR_H

class QString;
class QWidget;
struct ITaskbarList3;

class WinTaskbar {
   public:
    WinTaskbar() = default;
    ~WinTaskbar();

    void init(QWidget *mainWindow);
    //! \a value < 0 hides the progress
    void setProgress(int value, int max);
    void addRecentFile(const QString &filename);

   private:
    ITaskbarList3 *m_taskbar = nullptr;
    void *m_window = nullptr;  // HWND
};

#endif  // WINTASKBAR_H
