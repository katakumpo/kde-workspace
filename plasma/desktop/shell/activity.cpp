/*
 *   Copyright 2010 Chani Armitage <chani@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2,
 *   or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "desktopcorona.h"
#include "plasma-shell-desktop.h"
#include "kactivitycontroller.h"
#include "activitymanager/kidenticongenerator.h"

#include <QPixmap>
#include <QString>
#include <QSize>
#include <QFile>

#include <KConfig>
#include <KIcon>
#include <KStandardDirs>
#include <KWindowSystem>
#include <kephal/screens.h>

#include <Plasma/Containment>
#include <Plasma/Context>
#include <Plasma/Corona>

#include "plasmaapp.h"

#include "activity.h"

Activity::Activity(const QString &id, QObject *parent)
    : QObject(parent),
      m_id(id),
      m_info(new KActivityInfo(id, this))
{
    connect(m_info, SIGNAL(infoChanged()), this, SLOT(activityChanged()));
    connect(m_info, SIGNAL(stateChanged(KActivityInfo::State)), this, SLOT(activityStateChanged(KActivityInfo::State)));
    connect(m_info, SIGNAL(started()), this, SLOT(opened()));
    connect(m_info, SIGNAL(stopped()), this, SLOT(closed()));
    connect(m_info, SIGNAL(removed()), this, SLOT(removed()));

    if (m_info) {
        m_name = m_info->name();
        m_icon = m_info->icon();
        m_state = m_info->state();
    } else {
        m_name = m_id;
        m_icon = QString();
        m_state = KActivityInfo::Invalid;
    }

    m_corona = qobject_cast<DesktopCorona*>(PlasmaApp::self()->corona());

    //find your containments
    foreach (Plasma::Containment *cont, m_corona->containments()) {
        if ((cont->containmentType() == Plasma::Containment::DesktopContainment ||
             cont->containmentType() == Plasma::Containment::CustomContainment) &&
            !m_corona->offscreenWidgets().contains(cont) && cont->context()->currentActivityId() == id) {
            insertContainment(cont);
        }
    }

    //kDebug() << m_containments.size();
}

Activity::~Activity()
{
}

void Activity::activityChanged()
{
    setName(m_info->name());
    setIcon(m_info->icon());
}

void Activity::activityStateChanged(KActivityInfo::State state)
{
    m_state = state;
    emit stateChanged();
}

QString Activity::id()
{
    return m_id;
}

QString Activity::name()
{
    return m_name;
}

QPixmap Activity::pixmap(const QSize &size)
{
    if (m_info->isValid() && !m_info->icon().isEmpty()) {
        return KIcon(m_info->icon()).pixmap(size);
    } else {
        return KIdenticonGenerator::self()->generatePixmap(size.width(), m_id);
    }
}

bool Activity::isCurrent()
{
    KActivityConsumer c;
    return m_id == c.currentActivity();
    //TODO maybe plasmaapp should cache the current activity to reduce dbus calls?
}

KActivityInfo::State Activity::state()
{
    return m_state;
}

void Activity::remove()
{
    KActivityController().removeActivity(m_id);
}

void Activity::removed()
{
    if (! m_containments.isEmpty()) {
        //FIXME only m_corona has authority to remove properly
        kDebug() << "!!!!! if your widgets are locked you've hit a BUG now";
        foreach (Plasma::Containment *c, m_containments) {
            c->destroy(false);
        }
    }

    const QString name = "activities/" + m_id;
    QFile::remove(KStandardDirs::locateLocal("appdata", name));
}

Plasma::Containment* Activity::containmentForScreen(int screen, int desktop)
{
    //desktop -1 and 0 should share the same containment (for when PVD is changed)
    if (desktop == -1) {
        desktop = 0;
    }

    Plasma::Containment *c = m_containments.value(QPair<int,int>(screen, desktop));
    if (!c) {
        //TODO check if there are saved containments once we start saving them
        kDebug() << "@@@@@adding containment for" << screen << desktop;
        c = addContainment(screen, desktop);
    }

    return c;
}

Plasma::Containment* Activity::addContainment(int screen, int desktop)
{
    Plasma::Containment *containment = 0;
    foreach (Plasma::Containment *c, m_corona->containments()) {
        if ((c->containmentType() == Plasma::Containment::DesktopContainment ||
             c->containmentType() == Plasma::Containment::CustomContainment) &&
            !m_corona->offscreenWidgets().contains(c) &&
            c->context()->currentActivityId().isEmpty() &&
            m_containments.key(c,QPair<int,int>(-2,-2)) == QPair<int,int>(-2,-2)) {
            containment = c;
            containment->setScreen(screen, desktop);
            break;
        }
    }

    if (!containment) {
        containment = m_corona->addContainment(m_plugin);
    }

    insertContainment(containment, screen, desktop);
    m_corona->requestConfigSync();
    return containment;
}

void Activity::activateContainment(int screen, int desktop)
{
    Plasma::Containment *c = containmentForScreen(screen, desktop);
    c->setScreen(screen, desktop);
}

void Activity::activate()
{
    KActivityController().setCurrentActivity(m_id);
}

void Activity::ensureActive()
{
    if (m_containments.isEmpty()) {
        opened();
    }

    //ensure there's a containment for every screen & desktop.
    int numScreens = Kephal::ScreenUtils::numScreens();
    int numDesktops = 0;
    if (AppSettings::perVirtualDesktopViews()) {
        numDesktops = KWindowSystem::numberOfDesktops();
    }
    for (int screen = 0; screen < numScreens; ++screen) {
        if (numDesktops) {
            for (int desktop = 0; desktop < numDesktops; ++desktop) {
                activateContainment(screen, desktop);
            }
        } else {
            activateContainment(screen, -1);
        }
    }
}

void Activity::setName(const QString &name)
{
    if (m_name == name) {
        return;
    }

    m_name = name;
    KActivityController().setActivityName(m_id, name);
    emit infoChanged();

    foreach (Plasma::Containment *c, m_containments) {
        c->context()->setCurrentActivity(name);
    }
}

void Activity::setIcon(const QString &icon)
{
    if (m_icon == icon) {
        return;
    }

    m_icon = icon;

    KActivityController().setActivityIcon(m_id, icon);
    emit infoChanged();
}

void Activity::updateActivityName(Plasma::Context *context)
{
    if (context->currentActivityId() != m_id) {
        kDebug() << "can't happen!";
        return;
    }
    setName(context->currentActivity());
}

void Activity::save(KConfig &external)
{
    foreach (const QString &group, external.groupList()) {
        KConfigGroup cg(&external, group);
        cg.deleteGroup();
    }

    //TODO: multi-screen saving/restoring, where each screen can be
    // independently restored: put each screen's containments into a
    // different group, e.g. [Screens][0][Containments], [Screens][1][Containments], etc
    KConfigGroup dest(&external, "Containments");
    KConfigGroup dummy;
    foreach (Plasma::Containment *c, m_containments) {
        c->save(dummy);
        KConfigGroup group(&dest, QString::number(c->id()));
        c->config().copyTo(&group);
    }

    external.sync();
}

void Activity::close()
{
    KActivityController().stopActivity(m_id);
}

void Activity::closed()
{
    const QString name = "activities/" + m_id;
    KConfig external(name, KConfig::SimpleConfig, "appdata");

    //apparently this magic turns a kconfig into a kconfiggroup
    KConfigGroup group = external.group(QString());
    m_corona->exportLayout(group, m_containments.values());

    //hrm, shouldn't the containments' deleted signals have done this for us?
    if (! m_containments.isEmpty()) {
        kDebug() << "isn't close supposed to *remove* containments??";
        m_containments.clear();
    }
}

void Activity::replaceContainment(Plasma::Containment* containment)
{
    insertContainment(containment, true);
}

void Activity::insertContainment(Plasma::Containment* cont, bool force)
{
    int screen = cont->lastScreen();
    int desktop = cont->lastDesktop();

    //desktop -1 and 0 should share the same containment (for when PVD is changed)
    if (desktop == -1) {
        desktop = 0;
        //kDebug() << "desktop was -1";
    }

    kDebug() << screen << desktop;
    if (screen == -1) {
        //the migration can't set lastScreen, so maybe we need to assign the containment here
        kDebug() << "found a lost one";
        screen = 0;
    }

    if (!force && m_containments.contains(QPair<int,int>(screen, desktop))) {
        //this almost certainly means someone has been meddling where they shouldn't
        //but we should protect them from harm anyways
        kDebug() << "@!@!@!@!@!@@@@rejecting containment!!!";
        cont->context()->setCurrentActivityId(QString());
        return;
    }

    insertContainment(cont, screen, desktop);
}

void Activity::insertContainment(Plasma::Containment* containment, int screen, int desktop)
{
    //ensure it's hooked up
    Plasma::Context *context = containment->context();
    context->setCurrentActivityId(m_id);
    context->setCurrentActivity(m_name);
    //hack to keep the name in sync while KActivity* are in kdebase
    connect(context, SIGNAL(activityChanged(Plasma::Context*)), this, SLOT(updateActivityName(Plasma::Context*)), Qt::UniqueConnection);

    m_containments.insert(QPair<int,int>(screen, desktop), containment);
    connect(containment, SIGNAL(destroyed(QObject *)), this, SLOT(containmentDestroyed(QObject *)));
}

void Activity::containmentDestroyed(QObject *object)
{
    //safe here because we are not accessing it
    Plasma::Containment *deletedCont = static_cast<Plasma::Containment *>(object);

    QHash<QPair<int,int>, Plasma::Containment*>::iterator i;
    for (i = m_containments.begin(); i != m_containments.end(); ++i) {
        Plasma::Containment *cont = i.value();
        if (cont == deletedCont) {
            m_containments.remove(i.key());
            break;
        }
    }
}

void Activity::open()
{
    KActivityController().startActivity(m_id);
}

void Activity::opened()
{
    if (! m_containments.isEmpty()) {
        kDebug() << "already open!";
        return;
    }
    QString fileName = "activities/";
    fileName += m_id;
    KConfig external(fileName, KConfig::SimpleConfig, "appdata");

    foreach (Plasma::Containment *newContainment, m_corona->importLayout(external.group(QByteArray()))) {
        insertContainment(newContainment);
        //ensure it's hooked up (if something odd happened we don't want orphan containments)
        Plasma::Context *context = newContainment->context();
        context->setCurrentActivityId(m_id);
        connect(context, SIGNAL(activityChanged(Plasma::Context*)), this, SLOT(updateActivityName(Plasma::Context*)), Qt::UniqueConnection);
    }

    KConfigGroup configs(&external, "Containments");
    configs.deleteGroup();

    if (m_containments.isEmpty()) {
        //TODO check if we need more for screens/desktops
        kDebug() << "open failed (bad file?). creating new containment";
        addContainment(0, 0);
    }

    m_corona->requireConfigSync();
    external.sync();
}

void Activity::setDefaultPlugin(const QString &plugin)
{
    m_plugin = plugin;
    //FIXME save&restore this setting
}

const KActivityInfo * Activity::info() const
{
    return m_info;
}

#include "activity.moc"

// vim: sw=4 sts=4 et tw=100
