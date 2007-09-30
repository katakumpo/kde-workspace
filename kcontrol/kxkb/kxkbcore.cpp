/*
    Copyright (C) 2001, S.R.Haque <srhaque@iee.org>.
	Copyright (C) 2006, Andriy Rysin <rysin@kde.org>. Derived from an
    original by Matthias H�zer-Klpfel released under the QPL.
    This file is part of the KDE project

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

DESCRIPTION

    KDE Keyboard Tool. Manages XKB keyboard mappings.
*/
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include <QDesktopWidget>
#include <QProcess>

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kglobal.h>
#include <kglobalaccel.h>
#include <klocale.h>
#include <kwindowsystem.h>
#include <kstandarddirs.h>
#include <kaction.h>
#include <kdebug.h>
#include <kconfig.h>
#include <ktoolinvocation.h>
#include <kglobalsettings.h>
#include <kactioncollection.h>
#include <kapplication.h>

//#include "kxkb_adaptor.h"

#include "x11helper.h"
#include "extension.h"
#include "rules.h"
#ifdef HAVE_XKLAVIER
#include "xklavier_adaptor.h"
#endif
#include "kxkbconfig.h"
#include "layoutmap.h"
#include "kxkbwidget.h"

#include "kxkbcore.h"

#include "kxkbcore.moc"


/*
    This is dummy class just to handle x11 events
*/
class DummyWidget: public QWidget
{
  KxkbCore* kxkb;
public:
    DummyWidget(KxkbCore* kxkb_):
	  kxkb(kxkb_)
	{ }
protected:
    bool x11Event( XEvent * e) { return kxkb->x11EventFilter(e); }
};

KxkbCore::KxkbCore(KxkbWidget* kxkbWidget, int mode):
//     m_prevWinId(X11Helper::UNKNOWN_WINDOW_ID),
    m_mode(mode),
    m_rules(NULL),
    m_kxkbWidget(kxkbWidget)
{
    m_extension = new XKBExtension();
    if( !m_extension->init() ) {
        kDebug() << "xkb initialization failed, exiting...";
        ::exit(1);
    }

    KApplication::kApplication()->installX11EventFilter(new DummyWidget(this));
    
#ifdef HAVE_XKLAVIER
    XKlavierAdaptor::getInstance(QX11Info::display())->startListening();
#endif

  // TODO: keyboard bindings
    //globalKeys = KGlobalAccel::self();
    //KActionCollection* actionCollection = collection;
    //QAction* a = 0L;

//	#include "kxkbbindings.cpp"
    //keys->readSettings();
    //keys->updateConnections();

    m_layoutOwnerMap = new LayoutMap(m_kxkbConfig);

//    connect( KGlobalSettings::self(), SIGNAL(settingsChanged(int)),
//             this, SLOT(slotSettingsChanged(int)) );

    //TODO: don't do this if kxkb does not become a daemon
//     new KXKBAdaptor( this );
//	if( !m_kxkbWidget )
//	{
 		connect(m_kxkbWidget, SIGNAL(menuTriggered(QAction*)), this, SLOT(iconMenuTriggered(QAction*)));
		connect(m_kxkbWidget, SIGNAL(iconToggled()), this, SLOT(iconToggled()));
//	}
}


KxkbCore::~KxkbCore()
{
    delete m_keys;
    delete m_kxkbWidget;
    delete m_rules;
    delete m_extension;
    delete m_layoutOwnerMap;
}

int KxkbCore::newInstance()
{
    m_extension->reset();

    if( settingsRead() )
		layoutApply();

    return 0;
}

bool KxkbCore::settingsRead()
{
    m_kxkbConfig.load( KxkbConfig::LOAD_ACTIVE_OPTIONS );

    if( m_kxkbConfig.m_enableXkbOptions ) {
	kDebug() << "Setting XKB options " << m_kxkbConfig.m_options;
	if( !m_extension->setXkbOptions(m_kxkbConfig.m_options, m_kxkbConfig.m_resetOldOptions) ) {
            kDebug() << "Setting XKB options failed!";
        }
    }

    if ( m_kxkbConfig.m_useKxkb == false ) {
	kWarning() << "Kxkb is disabled, exiting...";
        emit quit(); //qApp->quit();
        return false;
    }

// 	m_prevWinId = X11Helper::UNKNOWN_WINDOW_ID;

	if( m_kxkbConfig.m_switchingPolicy == SWITCH_POLICY_GLOBAL ) {
		disconnect(KWindowSystem::self(), SIGNAL(activeWindowChanged(WId)), this, SLOT(windowChanged(WId)));
	}
	else {
		QDesktopWidget desktopWidget;
		if( desktopWidget.numScreens() > 1 && desktopWidget.isVirtualDesktop() == false ) {
			kWarning() << "With non-virtual desktop only global switching policy supported on non-primary screens" ;
			//TODO: find out how to handle that
		}

		disconnect(KWindowSystem::self(), SIGNAL(activeWindowChanged(WId)), this, SLOT(windowChanged(WId)));
		connect(KWindowSystem::self(), SIGNAL(activeWindowChanged(WId)), this, SLOT(windowChanged(WId)));
/*		int m_prevWinId = kWinModule->activeWindow();
		kDebug() << "Active window " << m_prevWinId;*/
	}

	m_layoutOwnerMap->reset();
	m_layoutOwnerMap->setCurrentWindow( X11Helper::UNKNOWN_WINDOW_ID ); //TODO

	if( m_rules == NULL )
		m_rules = new XkbRules(false);

	if( m_mode == NORMAL ) {
	    m_currentLayout = m_kxkbConfig.getDefaultLayout();
	    setLayoutGroups();
	}
	else {
	    updateGroupsFromServer();
	}
	
	if( m_kxkbConfig.m_layouts.count() == 1 ) {
	    if( m_kxkbConfig.m_showSingle == false ) {
		kWarning() << "Kxkb is disabled for single layout, exiting...";
		emit quit(); //qApp->quit();
		return false;
	    }
	}

	initTray();

//	KGlobal::config()->reparseConfiguration(); // kcontrol modified kdeglobals
	//TODO:
//	keys->readSettings();
	//keys->updateConnections();

	return true;
}

void KxkbCore::setLayoutGroups()
{
	if( m_kxkbConfig.m_layouts.count() == 1 ) {
		const LayoutUnit& currentLayout = m_kxkbConfig.m_layouts[0];
		QString layoutName = currentLayout.layout;
		QString variantName = currentLayout.variant;

/*		if( !m_extension->setLayout(kxkbConfig.m_model, layoutName, variantName, includeName, false)
				   || !m_extension->setGroup( group ) ) {*/
		if( ! m_extension->setLayoutGroups(layoutName, variantName) ) {
			kDebug() << "Error switching to single layout " << currentLayout.toPair();
			// TODO: alert user
		}

 	}
	else {
		QString layouts;
		QString variants;
		for(int ii=0; ii<(int)m_kxkbConfig.m_layouts.count(); ii++) {
			LayoutUnit& layoutUnit = m_kxkbConfig.m_layouts[ii];
			layouts += layoutUnit.layout;
			variants += layoutUnit.variant;
			if( ii < m_kxkbConfig.m_layouts.count()-1 ) {
				layouts += ',';
				variants += ',';
			}
		}
		kDebug() << "initing " << "-layout " << layouts << " -variants " << variants;
		m_extension->setLayoutGroups(layouts, variants);
	}
}

void KxkbCore::initTray()
{
    kDebug() << "initing tray";

    m_kxkbWidget->setShowFlag(m_kxkbConfig.m_showFlag);
    m_kxkbWidget->initLayoutList(m_kxkbConfig.m_layouts, *m_rules);
    m_kxkbWidget->setCurrentLayout(m_kxkbConfig.m_layouts[m_currentLayout]);
}

// This function activates the keyboard layout specified by the
// configuration members (m_currentLayout)
void KxkbCore::layoutApply()
{
    setLayout(m_currentLayout);
}

// DBUS
bool KxkbCore::setLayout(const QString& layoutPair)
{
 	const LayoutUnit layoutUnitKey(layoutPair);
 	if( m_kxkbConfig.m_layouts.contains(layoutUnitKey) ) {
 		int ind = m_kxkbConfig.m_layouts.indexOf(layoutUnitKey);
 		return setLayout( ind );
 	}
 	return false;
}


// Activates the keyboard layout specified by 'layoutUnit'
bool KxkbCore::setLayout(int layout)
{
	bool res = false;

// 	res = m_extension->setLayout(kxkbConfig.m_model,
//  					layoutUnit.layout, layoutUnit.variant,
//  					layoutUnit.includeGroup);

	res = m_extension->setGroup(layout); // not checking for ret - not important

	updateIndicator(layout, res);

    return res;
}

void KxkbCore::updateIndicator(int layout, int res)
{
    if( res ) {
  	m_currentLayout = layout;
	int winId = KWindowSystem::activeWindow();
 	m_layoutOwnerMap->setCurrentWindow(winId);
	m_layoutOwnerMap->setCurrentLayout(layout);
    }

    if( m_kxkbWidget ) {
	const QString& layoutName = m_kxkbConfig.m_layouts[layout].toPair(); //displayName;
	if( res )
	    m_kxkbWidget->setCurrentLayout(layoutName);
	else
	    m_kxkbWidget->setError(layoutName);
    }
}

void KxkbCore::iconToggled()
{
//    if ( reason != QSystemTrayIcon::Trigger )
//        return;
//     const LayoutUnit& layout = m_layoutOwnerMap->getNextLayout().layoutUnit;
    int layout = m_layoutOwnerMap->getNextLayout();
    setLayout(layout);
}

void KxkbCore::iconMenuTriggered(QAction* action)
{
	int id = action->data().toInt();

    if( KxkbWidget::START_MENU_ID <= id
        && id < KxkbWidget::START_MENU_ID + (int)m_kxkbConfig.m_layouts.count() )
    {
//         const LayoutUnit& layout = kxkbConfig.m_layouts[id - KxkbWidget::START_MENU_ID];
        int layout = id - KxkbWidget::START_MENU_ID;
        m_layoutOwnerMap->setCurrentLayout( layout );
        setLayout( layout );
    }
    else if (id == KxkbWidget::CONFIG_MENU_ID)
    {
        QStringList lst;
        lst<< "keyboard_layout";
	QProcess::startDetached("kcmshell",lst);
    }
    else if (id == KxkbWidget::HELP_MENU_ID)
    {
        KToolInvocation::invokeHelp(0, "kxkb");
    }
    else
    {
        emit quit();
    }
}

// TODO: we also have to handle deleted windows
void KxkbCore::windowChanged(WId winId)
{
	if( m_kxkbConfig.m_switchingPolicy == SWITCH_POLICY_GLOBAL ) { // should not happen actually
		kDebug() << "windowChanged() signal in GLOBAL switching policy";
		return;
	}

// 	int group = m_extension->getGroup();

// 	kDebug() << "old WinId: " << m_prevWinId << ", new WinId: " << winId;

//  	if( m_prevWinId != X11Helper::UNKNOWN_WINDOW_ID ) {	// saving layout/group from previous window
//  		kDebug() << "storing " << m_currentLayout.toPair() << ":" << group << " for " << m_prevWinId;
//  		m_layoutOwnerMap->setCurrentWindow(m_prevWinId);
// // 		m_layoutOwnerMap->setCurrentLayout(m_currentLayout);
// 		m_layoutOwnerMap->setCurrentGroup(group);
//  	}

// 	m_prevWinId = winId;

	if( winId != X11Helper::UNKNOWN_WINDOW_ID ) {
		m_layoutOwnerMap->setCurrentWindow(winId);
		int layoutState = m_layoutOwnerMap->getCurrentLayout();

		if( layoutState != m_currentLayout ) {
			setLayout(layoutState);
		}
	}
}


void KxkbCore::slotSettingsChanged(int category)
{
    if ( category != KGlobalSettings::SETTINGS_SHORTCUTS)
		return;

    KGlobal::config()->reparseConfiguration(); // kcontrol modified kdeglobals
    m_keys->readSettings();
	//TODO:
	//keys->updateConnections();
}


bool KxkbCore::x11EventFilter ( XEvent * event )
{
#ifdef HAVE_XKLAVIER
    XKlavierAdaptor::getInstance(QX11Info::display())->filterEvents(event);
#endif

  if( m_extension->isXkbEvent(event) ) {
//    qApp->x11ProcessEvent ( event );

    if( XKBExtension::isGroupSwitchEvent(event) ) {
      // group changed
  	  int group = m_extension->getGroup();
	  if( group != m_currentLayout ) {
	    kDebug() << "got event: group chagned to " << group;
	    if( group < m_kxkbConfig.m_layouts.count() ) {
		updateIndicator(group, 1);
	    }
	    else {
		kWarning() << "new group is out of my layouts list range";
	    }
	  }
    }
    else if( XKBExtension::isLayoutSwitchEvent(event) ) {
	  kDebug() << "got event: layouts chagned";
	  updateGroupsFromServer();
    }
    else {
	kDebug() << "other xkb event: ";// + ((XkbEvent*)event)->any.xkb_type;
    }
  }
  return false;
}

int
KxkbCore::updateGroupsFromServer()
{
    kDebug() << "updating groups from server";

#ifdef HAVE_XKLAVIER
	  QList<LayoutUnit> lus = XKlavierAdaptor::getInstance(QX11Info::display())->getGroupNames();
	  if( lus.count() > 0 ) {
	    if( lus != m_kxkbConfig.m_layouts ) {
		m_kxkbConfig.setConfiguredLayouts(lus);
		m_layoutOwnerMap->reset();
		m_layoutOwnerMap->setCurrentWindow( X11Helper::UNKNOWN_WINDOW_ID ); //TODO
		initTray();
	    }
	    else {
		kDebug() << "no change in layouts - ignoring event";
	    }

  	    int group = m_extension->getGroup();
	    kDebug() << "got group from server:" << group;
	    updateIndicator(group, 1);
	}
#endif
	
    return 0;
}
