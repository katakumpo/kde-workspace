/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
 *   Copyright 2008 by Chani Armitage <chanika@gmail.com>
 *   Copyright 2011 Martin Gräßlin <mgraesslin@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
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

#include "savercorona.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QGraphicsLayout>
#include <QAction>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>

#include <QtDeclarative/QDeclarativeComponent>
#include <QtDeclarative/QDeclarativeEngine>

#include <KDebug>
#include <KDialog>
#include <KStandardDirs>
#include <KIcon>
#include <kdeclarative.h>

#include <Plasma/Containment>
#include <plasma/containmentactionspluginsconfig.h>

SaverCorona::SaverCorona(QObject *parent)
    : Plasma::Corona(parent)
    , m_engine(NULL)
    , m_greeterItem(NULL)
{
    init();
}

void SaverCorona::init()
{
    setPreferredToolBoxPlugin(Plasma::Containment::DesktopContainment, "org.kde.desktoptoolbox");
    setPreferredToolBoxPlugin(Plasma::Containment::CustomContainment, "org.kde.desktoptoolbox");
    setPreferredToolBoxPlugin(Plasma::Containment::PanelContainment, "org.kde.paneltoolbox");
    setPreferredToolBoxPlugin(Plasma::Containment::CustomPanelContainment, "org.kde.paneltoolbox");

    QDesktopWidget *desktop = QApplication::desktop();
    connect(desktop,SIGNAL(screenCountChanged(int)), SLOT(numScreensUpdated(int)));
    m_numScreens = desktop->numScreens();

    Plasma::ContainmentActionsPluginsConfig plugins;
    plugins.addPlugin(Qt::NoModifier, Qt::RightButton, "minimalcontextmenu");
    //should I add paste too?
    setContainmentActionsDefaults(Plasma::Containment::CustomContainment, plugins);

    bool unlocked = immutability() == Plasma::Mutable;

    QAction *lock = action("lock widgets");
    if (lock) {
        kDebug() << "unlock action";
        //rename the lock action so that corona doesn't mess with it
        addAction("unlock widgets", lock);
        //rewire the action so we can check for a password
        lock->disconnect(SIGNAL(triggered(bool)));
        connect(lock, SIGNAL(triggered()), this, SLOT(toggleLock()));
        lock->setIcon(KIcon(unlocked ? "object-locked" : "configure"));
        lock->setText(unlocked ? i18n("Lock Screen") : i18n("Configure Widgets"));
    }

    //the most important action ;)
    QAction *leave = new QAction(unlocked ? i18n("Leave Screensaver") : i18n("Unlock"), this);
    leave->setIcon(KIcon("system-lock-screen"));
    leave->setShortcut(QKeySequence("esc"));
    connect(leave, SIGNAL(triggered()), this, SLOT(unlockDesktop()));
    addAction("unlock desktop", leave);

    //updateShortcuts(); //just in case we ever get a config dialog

    // create the QML Component
    m_engine = new QDeclarativeEngine(this);
    foreach(const QString &importPath, KGlobal::dirs()->findDirs("module", "imports")) {
        m_engine->addImportPath(importPath);
    }
    KDeclarative kdeclarative;
    kdeclarative.setDeclarativeEngine(m_engine);
    kdeclarative.initialize();
    kdeclarative.setupBindings();

    connect(this, SIGNAL(immutabilityChanged(Plasma::ImmutabilityType)), SLOT(updateActions(Plasma::ImmutabilityType)));
}

void SaverCorona::loadDefaultLayout()
{
    kDebug();
    QString defaultConfig = KStandardDirs::locate("appdata", "plasma-overlay-default-layoutrc");

    if (!defaultConfig.isEmpty()) {
        kDebug() << "attempting to load the default layout from:" << defaultConfig;
        loadLayout(defaultConfig);
        return;
    }

    QDesktopWidget *desktop = QApplication::desktop();

    // create a containment for the screens
    Plasma::Containment *c;
    for(int screen=0; screen<m_numScreens; ++screen)
    {
      QRect g = desktop->screenGeometry(screen);
      kDebug() << "     screen" << screen << "geometry is" << g;
      c = addContainment("saverdesktop");
      c->setScreen(screen);
      c->setFormFactor(Plasma::Planar);
      c->flushPendingConstraintsEvents();
    }

    // a default clock
    c = containmentForScreen(desktop->primaryScreen());
    Plasma::Applet *clock =  Plasma::Applet::load("clock"/*, c->id() + 1*/);
    c->addApplet(clock, QPointF(KDialog::spacingHint(), KDialog::spacingHint()), true);
    clock->init();
    clock->flushPendingConstraintsEvents();

    //emit containmentAdded(c);
}

int SaverCorona::numScreens() const
{
    return m_numScreens;
}

QRect SaverCorona::screenGeometry(int id) const
{
    return QApplication::desktop()->screenGeometry(id);
}

void SaverCorona::updateActions(Plasma::ImmutabilityType immutability)
{
    bool unlocked = immutability == Plasma::Mutable;
    QAction *a = action("unlock widgets");
    if (a) {
        a->setIcon(KIcon(unlocked ? "object-locked" : "configure"));
        a->setText(unlocked ? i18n("Lock Screen") : i18n("Configure Widgets"));
    }
    a = action("unlock desktop");
    if (a) {
        a->setText(unlocked ? i18n("Leave Screensaver") : i18n("Unlock"));
    }
}

void SaverCorona::toggleLock()
{
    //require a password to unlock
    QDBusInterface lockprocess("org.kde.screenlocker", "/LockProcess",
            "org.kde.screenlocker.LockProcess", QDBusConnection::sessionBus(), this);
    if (immutability() == Plasma::Mutable) {
        setImmutability(Plasma::UserImmutable);
        lockprocess.call(QDBus::NoBlock, "startLock");
        kDebug() << "locking up!";
    } else if (immutability() == Plasma::UserImmutable) {
        QList<QVariant> args;
        args << i18n("Unlock widgets to configure them");
        bool sent = lockprocess.callWithCallback("checkPass", args, this, SLOT(unlock(QDBusMessage)), SLOT(dbusError(QDBusError)));
        kDebug() << sent;
    }
}

void SaverCorona::unlock(QDBusMessage reply)
{
    //assuming everything went as expected
    if (reply.arguments().isEmpty()) {
        kDebug() << "quit succeeded, I guess";
        return;
    }
    //else we were trying to unlock just the widgets
    bool success = reply.arguments().first().toBool();
    kDebug() << success;
    if (success) {
        setImmutability(Plasma::Mutable);
    }
}

void SaverCorona::dbusError(QDBusError error)
{
    kDebug() << error.errorString(error.type());
    kDebug() << "bailing out";
    //if it was the quit call and it failed, we shouldn't leave the user stuck in
    //plasma-overlay forever.
    qApp->quit();
}

void SaverCorona::unlockDesktop()
{
    if (!m_greeterItem) {
        createGreeter();
    }
    m_greeterItem->setVisible(true);
}

void SaverCorona::numScreensUpdated(int newCount)
{
    m_numScreens = newCount;
    //do something?
}

void SaverCorona::createGreeter()
{
    QDeclarativeComponent component(m_engine, QUrl::fromLocalFile(KStandardDirs::locate("data", "plasma/screenlocker/lockscreen.qml")));
    m_greeterItem = qobject_cast<QGraphicsObject *>(component.create());
    addItem(m_greeterItem);
    connect(m_greeterItem, SIGNAL(accepted()), SLOT(greeterAccepted()));
    const QRect screenRect = screenGeometry(QApplication::desktop()->primaryScreen());
    // TODO: center on screen
    m_greeterItem->setPos(screenRect.x() + screenRect.width()/2,
                          screenRect.y() + screenRect.height()/2);
}

void SaverCorona::greeterAccepted()
{
    qApp->quit();
}

#include "savercorona.moc"

