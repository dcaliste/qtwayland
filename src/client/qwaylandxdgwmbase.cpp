/****************************************************************************
**
** Copyright (C) 2014 Eurogiciel, author: <philippe.coval@eurogiciel.fr>
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

#include "qwaylandxdgwmbase_p.h"

#include "qwaylanddisplay_p.h"
#include "qwaylandwindow_p.h"
#include "qwaylandxdgpopup_p.h"
#include "qwaylandxdgtoplevel_p.h"

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

QWaylandXdgWmBase::QWaylandXdgWmBase(struct ::xdg_wm_base *shell)
    : QtWayland::xdg_wm_base(shell)
{
}

QWaylandXdgWmBase::QWaylandXdgWmBase(struct ::wl_registry *registry, uint32_t id, uint32_t version)
    : QtWayland::xdg_wm_base(registry, id, version)
{
}

QWaylandXdgWmBase::~QWaylandXdgWmBase()
{
    destroy();
}

QWaylandXdgToplevel *QWaylandXdgWmBase::createXdgToplevel(QWaylandWindow *window)
{
    return new QWaylandXdgToplevel(this, window);
}

QWaylandXdgPopup *QWaylandXdgWmBase::createXdgPopup(QWaylandWindow *window)
{
    QtWayland::xdg_surface *parentSurface = nullptr;
    QWaylandXdgToplevel *xdgToplevel = qobject_cast<QWaylandXdgToplevel *>(window->transientParent()->shellSurface());
    if (xdgToplevel) {
        parentSurface = static_cast<QtWayland::xdg_surface*>(xdgToplevel);
    } else {
        QWaylandXdgPopup *xdgPopup = qobject_cast<QWaylandXdgPopup *>(window->transientParent()->shellSurface());
        if (xdgPopup)
            parentSurface = static_cast<QtWayland::xdg_surface*>(xdgPopup);
    }
    if (parentSurface) {
        return new QWaylandXdgPopup(this, parentSurface, window);
    } else {
        return nullptr;
    }
}

void QWaylandXdgWmBase::xdg_wm_base_ping(uint32_t serial)
{
    pong(serial);
}

}

QT_END_NAMESPACE
