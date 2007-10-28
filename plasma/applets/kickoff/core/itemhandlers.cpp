/*  
    Copyright 2007 Robert Knight <robertknight@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

// Own
#include "core/itemhandlers.h"

// Qt
#include <QUrl>
#include <QtDebug>
#include <QTimer>

// KDE
#include <KService>
#include <KToolInvocation>
#include <solid/powermanagement.h>

// KDE Base
#include <kworkspace.h>

// Local
#include "core/recentapplications.h"

// DBus
#include "screensaver_interface.h"

using namespace Kickoff;

bool ServiceItemHandler::openUrl(const QUrl& url)
{
    int result = KToolInvocation::startServiceByDesktopPath(url.toString(),QStringList(),0,0,0,"",true);

    if (result == 0) {
        KService::Ptr service = KService::serviceByDesktopPath(url.toString());

        if (!service.isNull()) {
            RecentApplications::self()->add(service);
        } else {
            qWarning() << "Failed to find service for" << url;
            return false;
        }
    }

    return result == 0;
}

bool LeaveItemHandler::openUrl(const QUrl& url)
{
    m_logoutAction = url.path().remove('/');

    if (m_logoutAction == "sleep") {
        Solid::PowerManagement::requestSleep(Solid::PowerManagement::SuspendState,0,0);
        return true;
    } else if (m_logoutAction == "hibernate") {
        Solid::PowerManagement::requestSleep(Solid::PowerManagement::HibernateState,0,0);
        return true;
    } else if (m_logoutAction == "lock") {
        // decouple dbus call, otherwise we'll run into a dead-lock
        QTimer::singleShot(0, this, SLOT(lock()));
        return true;
    } else if (m_logoutAction == "logout" || m_logoutAction == "switch" ||
               m_logoutAction == "restart" || m_logoutAction == "shutdown" ) {

        // decouple dbus call, otherwise we'll run into a dead-lock
        QTimer::singleShot(0, this, SLOT(logout()));
        return true;
    }

    return false;
}

void LeaveItemHandler::logout()
{
    KWorkSpace::ShutdownConfirm confirm = KWorkSpace::ShutdownConfirmDefault;
    KWorkSpace::ShutdownType type = KWorkSpace::ShutdownTypeNone;

    if (m_logoutAction == "logout") {
        type = KWorkSpace::ShutdownTypeNone;
    } else if (m_logoutAction == "lock") {
        qDebug() << "Locking screen"; 
    } else if (m_logoutAction == "switch") {
        qDebug() << "Switching user";
    } else if (m_logoutAction == "restart") {
        type = KWorkSpace::ShutdownTypeReboot;
    } else if (m_logoutAction == "shutdown") {
        type = KWorkSpace::ShutdownTypeHalt;
    }

    KWorkSpace::requestShutDown(confirm,type);
}

void LeaveItemHandler::lock()
{
    QString interface("org.freedesktop.ScreenSaver");
    org::freedesktop::ScreenSaver screensaver(interface, "/ScreenSaver",
                                              QDBusConnection::sessionBus());
    if (screensaver.isValid()) {
        screensaver.Lock();
    }
}
