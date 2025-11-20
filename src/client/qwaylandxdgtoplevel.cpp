/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the config.tests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwaylandxdgtoplevel_p.h"

#include "qwaylanddisplay_p.h"
#include "qwaylandwindow_p.h"
#include "qwaylandinputdevice_p.h"
#include "qwaylandabstractdecoration_p.h"
#include "qwaylandscreen_p.h"
#include "qwaylandextendedsurface_p.h"
#include "qwaylandxdgwmbase_p.h"


QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandXdgToplevel::QWaylandXdgToplevel(QWaylandXdgWmBase *shell, QWaylandWindow *window)
    : QWaylandShellSurface(window)
    , QtWayland::xdg_surface(shell->get_xdg_surface(window->object()))
    , QtWayland::xdg_toplevel(get_toplevel())
    , m_window(window)
    , m_shell(shell)
    , m_maximized(false)
    , m_minimized(false)
    , m_fullscreen(false)
    , m_active(false)
    , m_extendedWindow(Q_NULLPTR)
{
    if (window->display()->windowExtension())
        m_extendedWindow = new QWaylandExtendedSurface(window);
}

QWaylandXdgToplevel::~QWaylandXdgToplevel()
{
    if (m_active)
        window()->display()->handleWindowDeactivated(m_window);

    QtWayland::xdg_toplevel::destroy();
    QtWayland::xdg_surface::destroy();
    delete m_extendedWindow;
}

void QWaylandXdgToplevel::resize(QWaylandInputDevice *inputDevice, enum wl_shell_surface_resize edges)
{
    // May need some conversion if types get incompatibles, ATM they're identical
    enum resize_edge const * const arg = reinterpret_cast<enum resize_edge const * const>(&edges);
    resize(inputDevice, *arg);
}

void QWaylandXdgToplevel::resize(QWaylandInputDevice *inputDevice, enum resize_edge edges)
{
    resize(inputDevice->wl_seat(),
           inputDevice->serial(),
           edges);
}

void QWaylandXdgToplevel::move(QWaylandInputDevice *inputDevice)
{
    move(inputDevice->wl_seat(),
         inputDevice->serial());
}

void QWaylandXdgToplevel::setMaximized()
{
    if (!m_maximized)
        set_maximized();
}

void QWaylandXdgToplevel::setFullscreen()
{
    if (!m_fullscreen)
        set_fullscreen(Q_NULLPTR);
}

void QWaylandXdgToplevel::setNormal()
{
    if (m_fullscreen || m_maximized  || m_minimized) {
        if (m_maximized) {
            unset_maximized();
        }
        if (m_fullscreen) {
            unset_fullscreen();
        }

        m_fullscreen = m_maximized = m_minimized = false;
    }
}

void QWaylandXdgToplevel::setMinimized()
{
    m_minimized = true;
    set_minimized();
}

void QWaylandXdgToplevel::setTopLevel()
{
    // There's no xdg_shell_surface API for this, ignoring
}

void QWaylandXdgToplevel::updateTransientParent(QWindow *parent)
{
    QWaylandWindow *parent_wayland_window = static_cast<QWaylandWindow *>(parent->handle());
    if (!parent_wayland_window)
        return;
    auto parentXdgToplevel = qobject_cast<QWaylandXdgToplevel *>(parent_wayland_window->shellSurface());
    Q_ASSERT(parentXdgToplevel);
    set_parent(parentXdgToplevel->object());
}

void QWaylandXdgToplevel::setTitle(const QString & title)
{
    return set_title(title);
}

void QWaylandXdgToplevel::setAppId(const QString & appId)
{
    return set_app_id(appId);
}

void QWaylandXdgToplevel::raise()
{
    if (m_extendedWindow)
        m_extendedWindow->raise();
}

void QWaylandXdgToplevel::lower()
{
    if (m_extendedWindow)
        m_extendedWindow->lower();
}

void QWaylandXdgToplevel::setContentOrientationMask(Qt::ScreenOrientations orientation)
{
    if (m_extendedWindow)
        m_extendedWindow->setContentOrientationMask(orientation);
}

void QWaylandXdgToplevel::setWindowFlags(Qt::WindowFlags flags)
{
    if (m_extendedWindow)
        m_extendedWindow->setWindowFlags(flags);
}

void QWaylandXdgToplevel::sendProperty(const QString &name, const QVariant &value)
{
    if (m_extendedWindow)
        m_extendedWindow->updateGenericProperty(name, value);
}

void QWaylandXdgToplevel::xdg_toplevel_configure(int32_t width, int32_t height, struct wl_array *states)
{
    uint32_t *state = reinterpret_cast<uint32_t*>(states->data);
    size_t numStates = states->size / sizeof(uint32_t);
    bool aboutToMaximize = false;
    bool aboutToFullScreen = false;
    bool aboutToActivate = false;

    for (size_t i = 0; i < numStates; i++) {
        switch (state[i]) {
        case state_maximized:
            aboutToMaximize = ((width > 0) && (height > 0));
            break;
        case state_fullscreen:
            aboutToFullScreen = true;
            break;
        case state_resizing:
            m_normalSize = QSize(width, height);
            break;
        case state_activated:
            aboutToActivate = true;
            break;
        default:
            qWarning() << "implement missing top level state";
            break;
        }
    }

    if (!m_active && aboutToActivate) {
        m_active = true;
        window()->display()->handleWindowActivated(m_window);
    } else if (m_active && !aboutToActivate) {
        m_active = false;
        window()->display()->handleWindowDeactivated(m_window);
    }

    if (!m_fullscreen && aboutToFullScreen) {
        if (!m_maximized)
            m_normalSize = m_window->window()->frameGeometry().size();
        m_fullscreen = true;
        m_window->window()->showFullScreen();
    } else if (m_fullscreen && !aboutToFullScreen) {
        m_fullscreen = false;
        if ( m_maximized ) {
            m_window->window()->showMaximized();
        } else {
            m_window->window()->showNormal();
        }
    } else if (!m_maximized && aboutToMaximize) {
        if (!m_fullscreen)
            m_normalSize = m_window->window()->frameGeometry().size();
        m_maximized = true;
        m_window->window()->showMaximized();
    } else if (m_maximized && !aboutToMaximize) {
        m_maximized = false;
        m_window->window()->showNormal();
    }

    if (width <= 0 || height <= 0) {
        if (!m_normalSize.isEmpty())
            m_window->configure(0, m_normalSize.width(), m_normalSize.height());
    } else {
        m_window->configure(0, width, height);
    }
}

void QWaylandXdgToplevel::xdg_surface_configure(uint32_t serial)
{
    ack_configure(serial);
}

void QWaylandXdgToplevel::xdg_toplevel_close()
{
    m_window->window()->close();
}

}

QT_END_NAMESPACE
