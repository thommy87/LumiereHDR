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

//! \brief Shows an HDR image with real HDR brightness on an HDR screen
//! (Windows: Direct3D 11, scRGB)

#ifndef HDRDISPLAYWINDOW_H
#define HDRDISPLAYWINDOW_H

#include <QDialog>
#include <QWidget>

#include <memory>
#include <vector>

class QLabel;
class QSlider;

namespace pfs {
class Frame;
}

//! the picture itself, drawn by Direct3D into an HDR swap chain
class HdrDisplayCanvas : public QWidget {
    Q_OBJECT

   public:
    explicit HdrDisplayCanvas(QWidget *parent = nullptr);
    ~HdrDisplayCanvas() override;

    void setImage(std::shared_ptr<pfs::Frame> hdr);
    void setExposure(float stops);
    //! the screen the picture is on: HDR or not, and its peak brightness
    QString screenStatus() const { return m_status; }

    QPaintEngine *paintEngine() const override { return nullptr; }

   Q_SIGNALS:
    void screenStatusChanged(const QString &status);

   protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void moveEvent(QMoveEvent *) override;

   private:
    void render();

    struct Direct3D;
    std::unique_ptr<Direct3D> m_d3d;
    std::shared_ptr<pfs::Frame> m_hdr;
    std::unique_ptr<pfs::Frame> m_fitted;  // the image at the window's size
    float m_middleGrey = 0.18f;            // log-average of the picture
    float m_exposure = 0.f;
    QString m_status;
};

//! a window with the HDR picture and an exposure slider
class HdrDisplayWindow : public QDialog {
    Q_OBJECT

   public:
    HdrDisplayWindow(std::shared_ptr<pfs::Frame> hdr, QWidget *parent = nullptr);

   private:
    HdrDisplayCanvas *m_canvas;
    QSlider *m_exposure;
    QLabel *m_exposureLabel;
    QLabel *m_status;
};

#endif  // HDRDISPLAYWINDOW_H
