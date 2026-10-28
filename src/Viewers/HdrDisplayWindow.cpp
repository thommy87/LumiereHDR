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

// Windows headers would otherwise define min and max as macros
#ifndef NOMINMAX
#define NOMINMAX
#endif

// before anything else: a "using namespace std" in a later header would
// make the Windows headers' byte clash with std::byte
#include <d3d11.h>
#include <dxgi1_6.h>

#include "Viewers/HdrDisplayWindow.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <cstring>

#include <Libpfs/frame.h>
#include <Libpfs/manip/resize.h>

namespace {
//! Releases a COM pointer when it goes out of use
template <typename T>
struct Com {
    T *p = nullptr;
    Com() = default;
    Com(const Com &) = delete;
    Com &operator=(const Com &) = delete;
    ~Com() { reset(); }
    void reset() {
        if (p) p->Release();
        p = nullptr;
    }
    T **out() {
        reset();
        return &p;
    }
    T *operator->() const { return p; }
    explicit operator bool() const { return p != nullptr; }
};

//! IEEE half float, enough for picture values (no NaN handling needed)
uint16_t toHalf(float f) {
    uint32_t x;
    std::memcpy(&x, &f, 4);
    const uint32_t sign = (x >> 16) & 0x8000u;
    const int exponent = int((x >> 23) & 0xff) - 127 + 15;
    const uint32_t mantissa = x & 0x7fffffu;
    if (exponent <= 0) return uint16_t(sign);
    if (exponent >= 31) return uint16_t(sign | 0x7c00u);
    return uint16_t(sign | (uint32_t(exponent) << 10) | (mantissa >> 13));
}

// scRGB: 1.0 is 80 nits. Normal white is placed at 200 nits, as Windows
// does by default for SDR content on an HDR screen.
const float kNitsPerUnit = 80.f;
const float kSdrWhiteNits = 200.f;
}  // namespace

struct HdrDisplayCanvas::Direct3D {
    Com<ID3D11Device> device;
    Com<ID3D11DeviceContext> context;
    Com<IDXGISwapChain1> swapChain;
    UINT width = 0, height = 0;
    bool hdrScreen = false;
    float peakNits = kSdrWhiteNits;
};

HdrDisplayCanvas::HdrDisplayCanvas(QWidget *parent)
    : QWidget(parent), m_d3d(new Direct3D) {
    // Direct3D draws into this window; Qt must not paint over it
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setMinimumSize(320, 200);
}

HdrDisplayCanvas::~HdrDisplayCanvas() = default;

void HdrDisplayCanvas::setImage(std::shared_ptr<pfs::Frame> hdr) {
    m_hdr = std::move(hdr);
    m_fitted.reset();
    // the log-average brightness becomes mid grey, as a camera exposes it
    m_middleGrey = 0.18f;
    if (m_hdr) {
        const pfs::Channel *r, *g, *b;
        m_hdr->getXYZChannels(r, g, b);
        if (r && g && b) {
            double sum = 0;
            size_t n = 0;
            const size_t size = r->size();
            const size_t stride = std::max<size_t>(1, size / 200000);
            for (size_t i = 0; i < size; i += stride) {
                const float y =
                    0.2126f * (*r)(i) + 0.7152f * (*g)(i) + 0.0722f * (*b)(i);
                if (std::isfinite(y) && y > 1e-6f) {
                    sum += std::log(double(y));
                    ++n;
                }
            }
            if (n) m_middleGrey = float(std::exp(sum / double(n)));
        }
    }
    update();
}

void HdrDisplayCanvas::setExposure(float stops) {
    m_exposure = stops;
    update();
}

void HdrDisplayCanvas::paintEvent(QPaintEvent *) { render(); }

void HdrDisplayCanvas::resizeEvent(QResizeEvent *) {
    m_fitted.reset();
    update();
}

void HdrDisplayCanvas::moveEvent(QMoveEvent *) {
    // another screen may have other HDR abilities
    update();
}

void HdrDisplayCanvas::render() {
    Direct3D &d = *m_d3d;
    const qreal ratio = devicePixelRatioF();
    const UINT width = UINT(std::max(1, int(this->width() * ratio)));
    const UINT height = UINT(std::max(1, int(this->height() * ratio)));

    if (!d.device) {
        const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0,
                                            D3D_FEATURE_LEVEL_10_0};
        if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                     0, levels, 2, D3D11_SDK_VERSION,
                                     d.device.out(), nullptr,
                                     d.context.out()))) {
            m_status = tr("Direct3D is not available, so the HDR preview "
                          "cannot be shown.");
            emit screenStatusChanged(m_status);
            return;
        }
    }
    if (!d.swapChain) {
        Com<IDXGIDevice> dxgiDevice;
        Com<IDXGIAdapter> adapter;
        Com<IDXGIFactory2> factory;
        if (FAILED(d.device->QueryInterface(__uuidof(IDXGIDevice),
                                            reinterpret_cast<void **>(
                                                dxgiDevice.out()))) ||
            FAILED(dxgiDevice->GetAdapter(adapter.out())) ||
            FAILED(adapter->GetParent(__uuidof(IDXGIFactory2),
                                      reinterpret_cast<void **>(
                                          factory.out()))))
            return;
        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Width = width;
        desc.Height = height;
        // half floats in scRGB: linear, values above 1 are brighter than
        // normal white
        desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        if (FAILED(factory->CreateSwapChainForHwnd(
                d.device.p, reinterpret_cast<HWND>(winId()), &desc, nullptr,
                nullptr, d.swapChain.out())))
            return;
        Com<IDXGISwapChain3> swapChain3;
        if (SUCCEEDED(d.swapChain->QueryInterface(
                __uuidof(IDXGISwapChain3),
                reinterpret_cast<void **>(swapChain3.out()))))
            swapChain3->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);
        d.width = width;
        d.height = height;
    }
    if (d.width != width || d.height != height) {
        if (FAILED(d.swapChain->ResizeBuffers(0, width, height,
                                              DXGI_FORMAT_UNKNOWN, 0)))
            return;
        d.width = width;
        d.height = height;
    }

    // what the screen under the window can show
    {
        Com<IDXGIOutput> output;
        Com<IDXGIOutput6> output6;
        bool hdr = false;
        float peak = kSdrWhiteNits;
        if (SUCCEEDED(d.swapChain->GetContainingOutput(output.out())) &&
            SUCCEEDED(output->QueryInterface(
                __uuidof(IDXGIOutput6),
                reinterpret_cast<void **>(output6.out())))) {
            DXGI_OUTPUT_DESC1 desc;
            if (SUCCEEDED(output6->GetDesc1(&desc))) {
                hdr = desc.ColorSpace ==
                      DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
                if (hdr && desc.MaxLuminance > kSdrWhiteNits)
                    peak = desc.MaxLuminance;
            }
        }
        const QString status =
            hdr ? tr("HDR is on for this screen: highlights up to about %1 "
                     "nits.")
                      .arg(int(peak))
                : tr("HDR is off for this screen, so the picture is shown "
                     "without HDR brightness. Switch on \"Use HDR\" in the "
                     "Windows display settings, or move this window to an "
                     "HDR screen.");
        d.hdrScreen = hdr;
        d.peakNits = peak;
        if (status != m_status) {
            m_status = status;
            emit screenStatusChanged(m_status);
        }
    }

    // the picture at the size it is shown, centred
    std::vector<uint16_t> pixels(size_t(width) * height * 4, toHalf(0.f));
    const uint16_t background = toHalf(0.02f);
    for (size_t i = 0; i < pixels.size(); i += 4) {
        pixels[i] = pixels[i + 1] = pixels[i + 2] = background;
        pixels[i + 3] = toHalf(1.f);
    }
    if (m_hdr) {
        const float fit = std::min(float(width) / m_hdr->getWidth(),
                                   float(height) / m_hdr->getHeight());
        const int fw = std::max(1, int(m_hdr->getWidth() * fit));
        if (!m_fitted || int(m_fitted->getWidth()) != fw)
            m_fitted.reset(pfs::resize(m_hdr.get(), fw, BilinearInterp));
        const pfs::Channel *r, *g, *b;
        m_fitted->getXYZChannels(r, g, b);
        const int iw = int(m_fitted->getWidth());
        const int ih = int(m_fitted->getHeight());
        const int ox = (int(width) - iw) / 2, oy = (int(height) - ih) / 2;

        // in scRGB units: mid grey at 18 % of normal white, highlights
        // above it, rolled off softly up to the screen's peak
        const float white = d.hdrScreen ? kSdrWhiteNits / kNitsPerUnit : 1.f;
        const float peak = d.hdrScreen ? d.peakNits / kNitsPerUnit : 1.f;
        const float knee = 0.75f * peak;
        const float scale =
            0.18f * white / m_middleGrey * std::pow(2.f, m_exposure);
        for (int y = 0; y < ih; ++y) {
            const int ty = y + oy;
            if (ty < 0 || ty >= int(height)) continue;
            for (int x = 0; x < iw; ++x) {
                const int tx = x + ox;
                if (tx < 0 || tx >= int(width)) continue;
                const size_t k = size_t(y) * iw + x;
                float c[3] = {std::max((*r)(k), 0.f) * scale,
                              std::max((*g)(k), 0.f) * scale,
                              std::max((*b)(k), 0.f) * scale};
                const float lum = 0.2126f * c[0] + 0.7152f * c[1] + 0.0722f * c[2];
                if (lum > knee) {
                    const float rolled =
                        peak - (peak - knee) * std::exp(-(lum - knee) /
                                                        (peak - knee));
                    for (float &v : c) v *= rolled / lum;
                }
                uint16_t *out = &pixels[(size_t(ty) * width + tx) * 4];
                for (int ch = 0; ch < 3; ++ch)
                    out[ch] = toHalf(std::min(c[ch], peak));
            }
        }
    }

    Com<ID3D11Texture2D> backBuffer;
    if (FAILED(d.swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                      reinterpret_cast<void **>(
                                          backBuffer.out()))))
        return;
    d.context->UpdateSubresource(backBuffer.p, 0, nullptr, pixels.data(),
                                 width * 4 * sizeof(uint16_t), 0);
    d.swapChain->Present(1, 0);
}

HdrDisplayWindow::HdrDisplayWindow(std::shared_ptr<pfs::Frame> hdr,
                                   QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(tr("HDR preview"));
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
    setAttribute(Qt::WA_DeleteOnClose);
    resize(1100, 760);

    // the preview only shows more than the normal view on a real HDR screen
    QLabel *hint = new QLabel(
        tr("<b>Only on HDR screens:</b> this preview shows real HDR "
           "brightness only on an HDR monitor or HDR TV with \"Use HDR\" "
           "switched on in the Windows display settings. On a normal screen "
           "it looks like the usual view."),
        this);
    hint->setWordWrap(true);
    hint->setFrameShape(QFrame::StyledPanel);
    hint->setMargin(6);

    m_canvas = new HdrDisplayCanvas(this);
    m_canvas->setImage(std::move(hdr));

    m_exposure = new QSlider(Qt::Horizontal, this);
    m_exposure->setRange(-40, 40);  // tenths of a stop
    m_exposure->setValue(0);
    m_exposure->setToolTip(
        tr("Brighter or darker; the highlights stay within what the screen "
           "can show"));
    m_exposureLabel = new QLabel(this);
    m_exposureLabel->setMinimumWidth(80);
    m_status = new QLabel(this);
    m_status->setWordWrap(true);

    auto showExposure = [this](int tenths) {
        const float stops = tenths / 10.f;
        m_exposureLabel->setText(tr("%1 EV").arg(stops, 0, 'f', 1));
        m_canvas->setExposure(stops);
    };
    connect(m_exposure, &QSlider::valueChanged, this, showExposure);
    showExposure(0);
    connect(m_canvas, &HdrDisplayCanvas::screenStatusChanged, m_status,
            &QLabel::setText);

    QHBoxLayout *controls = new QHBoxLayout;
    controls->addWidget(new QLabel(tr("Exposure:"), this));
    controls->addWidget(m_exposure, 1);
    controls->addWidget(m_exposureLabel);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(hint);
    layout->addWidget(m_canvas, 1);
    layout->addLayout(controls);
    layout->addWidget(m_status);
}
