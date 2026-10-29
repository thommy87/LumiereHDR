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

// before Qt: the Windows headers define min and max otherwise
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>

#include "OsIntegration/wintaskbar.h"

#include <QDir>
#include <QString>
#include <QWidget>

WinTaskbar::~WinTaskbar() {
    if (m_taskbar) m_taskbar->Release();
}

void WinTaskbar::init(QWidget *mainWindow) {
    if (!mainWindow) return;
    m_window = reinterpret_cast<void *>(mainWindow->winId());
    if (FAILED(CoCreateInstance(CLSID_TaskbarList, nullptr,
                                CLSCTX_INPROC_SERVER, IID_ITaskbarList3,
                                reinterpret_cast<void **>(&m_taskbar)))) {
        m_taskbar = nullptr;
        return;
    }
    if (FAILED(m_taskbar->HrInit())) {
        m_taskbar->Release();
        m_taskbar = nullptr;
    }
}

void WinTaskbar::setProgress(int value, int max) {
    if (!m_taskbar || !m_window) return;
    HWND window = reinterpret_cast<HWND>(m_window);
    if (value < 0 || max <= 0) {
        m_taskbar->SetProgressState(window, TBPF_NOPROGRESS);
        return;
    }
    m_taskbar->SetProgressState(window, TBPF_NORMAL);
    m_taskbar->SetProgressValue(window, ULONGLONG(value), ULONGLONG(max));
}

void WinTaskbar::addRecentFile(const QString &filename) {
    // Windows keeps the list and shows it in the jump list
    const std::wstring path =
        QDir::toNativeSeparators(filename).toStdWString();
    SHAddToRecentDocs(SHARD_PATHW, path.c_str());
}
