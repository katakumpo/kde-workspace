/* This file is part of the KDE project
   Copyright (C) 2002 Daniel Molkentin <molkentin@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qlayout.h>
#include <qwhatsthis.h>
#include <qvgroupbox.h>
#include <qpushbutton.h>
#include <qheader.h>
#include <qtimer.h>

#include <kbuttonbox.h>
#include <kapplication.h>
#include <kaboutdata.h>
#include <klistview.h>
#include <kgenericfactory.h>
#include <kstandarddirs.h>
#include <kdesktopfile.h>
#include <kmessagebox.h>

#include <dcopclient.h>


#include <kdebug.h>

#include "kxmlrpcdlg.h"
#include "kcmkded.h"
#include "kcmkded.moc"

typedef KGenericFactory<KDEDConfig, QWidget> KDEDFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_kded, KDEDFactory( "kcmkded" ) );

static const QCString KXMLRPCD("kxmlrpcd");
static const bool KXMLRPCD_DEFAULT = false;
static const QCString KALARMD("kalarmd");
static const bool KALARMD_DEFAULT = true;
static const QCString KWRITED("kwrited");
static const bool KWRITED_DEFAULT = true;


KDEDConfig::KDEDConfig(QWidget* parent, const char* name, const QStringList &) :
	KCModule( KDEDFactory::instance(), parent, name )
{
	QVBoxLayout *lay = new QVBoxLayout( this, 0, KDialog::spacingHint() );

	QGroupBox *gb = new QVGroupBox(i18n("Load-on-Demand Services"), this );
	QWhatsThis::add(gb, i18n("This is a list of available KDE services which will "
			"be started on demand. They are only listed for convenience, as you "
			"cannot manipulate these services."));
	lay->addWidget( gb );

	_lvLoD = new KListView( gb );
	_lvLoD->addColumn(i18n("Service"));
	_lvLoD->addColumn(i18n("Description"));
	_lvLoD->addColumn(i18n("Status"));
	_lvLoD->setResizeMode(QListView::LastColumn);
	_lvLoD->setAllColumnsShowFocus(true);

 	gb = new QVGroupBox(i18n("Startup Services"), this );
	QWhatsThis::add(gb, i18n("This shows all KDE services that can be loaded "
				"on KDE startup. Checked services will be invoked on next startup. "
				"Be careful with deactivation of unknown services."));
	lay->addWidget( gb );

	_lvStartup = new KListView( gb );
	_lvStartup->addColumn(i18n("Use"));
	_lvStartup->addColumn(i18n("Service"));
	_lvStartup->addColumn(i18n("Description"));
	_lvStartup->addColumn(i18n("Status"));
	_lvStartup->setResizeMode(QListView::LastColumn);
	_lvStartup->setAllColumnsShowFocus(true);

	KButtonBox *buttonBox = new KButtonBox( gb, Horizontal);
	_pbStart = buttonBox->addButton( i18n("Start"));
	_pbStop = buttonBox->addButton( i18n("Stop"));
	_pbOptions = buttonBox->addButton( i18n("Options..."));

	_pbStart->setEnabled( false );
	_pbStop->setEnabled( false );
	_pbOptions->setEnabled( false );

	connect(_pbStart, SIGNAL(clicked()), SLOT(slotStartService()));
	connect(_pbStop,  SIGNAL(clicked()), SLOT(slotStopService()));
	connect(_pbOptions, SIGNAL(clicked()), SLOT(configureService()));
	connect(_lvStartup, SIGNAL(selectionChanged(QListViewItem*)), SLOT(slotEvalItem(QListViewItem*)) );

	load();
}

void KDEDConfig::load() {

	_lvStartup->clear();
	_lvLoD->clear();

	QStringList files;
	KGlobal::dirs()->findAllResources( "services",
			QString::fromLatin1( "kded/*.desktop" ),
			true, true, files );

	QListViewItem* item = 0L;
	CheckListItem* clitem;
	for ( QStringList::ConstIterator it = files.begin(); it != files.end(); it++ ) {

		if ( KDesktopFile::isDesktopFile( *it ) ) {
			KDesktopFile file( *it, true, "services" );

			if ( file.readBoolEntry("X-KDE-Kded-autoload") ) {
				clitem = new CheckListItem(_lvStartup, QString::null);
				connect(clitem, SIGNAL(changed(QCheckListItem*)), SLOT(slotItemChecked(QCheckListItem*)));
				clitem->setOn(!file.readBoolEntry("X-KDE-Kded-nostart",false));
				item = clitem;
				item->setText(1, file.readName());
				item->setText(2, file.readComment());
				item->setText(3, i18n("Not running"));
				item->setText(4, file.readEntry("X-KDE-Library"));
			}
			else if ( file.readBoolEntry("X-KDE-Kded-load-on-demand") ) {
				item = new QListViewItem(_lvLoD, file.readName());
				item->setText(1, file.readComment());
				item->setText(2, i18n("Not running"));
				item->setText(4, file.readEntry("X-KDE-Library"));
			}
		}
	}

	// Special case: kxmlrpcd
	clitem = new CheckListItem(_lvStartup, QString::null);
	connect(clitem, SIGNAL(changed(QCheckListItem*)), SLOT(slotItemChecked(QCheckListItem*)));
        {
		KConfig config("kxmlrpcdrc", true);
		config.setGroup("General");
		clitem->setOn(config.readBoolEntry("StartServer", KXMLRPCD_DEFAULT));
        }
	item = clitem;
	item->setText(1, i18n("XML-RPC Daemon"));
	item->setText(2, QString::null);
	item->setText(3, i18n("Not running"));
	item->setText(4, QString::fromLatin1(KXMLRPCD));

	// Special case: kalarmd
	clitem = new CheckListItem(_lvStartup, QString::null);
	connect(clitem, SIGNAL(changed(QCheckListItem*)), SLOT(slotItemChecked(QCheckListItem*)));
        {
		KConfig config("kalarmdrc", true);
		config.setGroup("General");
		clitem->setOn(config.readBoolEntry("Autostart", KALARMD_DEFAULT));
        }
	item = clitem;
	item->setText(1, i18n("Alarm Daemon"));
	item->setText(2, QString::null);
	item->setText(3, i18n("Not running"));
	item->setText(4, QString::fromLatin1(KALARMD));

	// Special case: kwrited
	clitem = new CheckListItem(_lvStartup, QString::null);
	connect(clitem, SIGNAL(changed(QCheckListItem*)), SLOT(slotItemChecked(QCheckListItem*)));
        {
		KConfig config("kwritedrc", true);
		config.setGroup("General");
		clitem->setOn(config.readBoolEntry("Autostart", KWRITED_DEFAULT));
        }
	item = clitem;
	item->setText(1, i18n("KWrite Daemon"));
	item->setText(2, QString::null);
	item->setText(3, i18n("Not running"));
	item->setText(4, QString::fromLatin1(KWRITED));

	getServiceStatus();
}

void KDEDConfig::save() {
	QCheckListItem* item = 0L;

	QStringList files;
	KGlobal::dirs()->findAllResources( "services",
			QString::fromLatin1( "kded/*.desktop" ),
			true, true, files );


	for ( QStringList::ConstIterator it = files.begin(); it != files.end(); it++ ) {

		if ( KDesktopFile::isDesktopFile( *it ) ) {

			KConfig file( *it, false, false, "services" );
			file.setGroup("Desktop Entry");

			if (file.readBoolEntry("X-KDE-Kded-autoload")){

				item = static_cast<QCheckListItem *>(_lvStartup->findItem(file.readEntry("X-KDE-Library"),4));
				if (item) {
					// we found a match, now compare and see what changed
					if (item->isOn())
						file.writeEntry("X-KDE-Kded-nostart", false);
					else
						file.writeEntry("X-KDE-Kded-nostart", true);

				}
			}
		}
	}

	// Special case: kxmlrpcd
	item = static_cast<QCheckListItem *>(_lvStartup->findItem(QString::fromLatin1(KXMLRPCD),4));
	if (item) {
		KConfig config("kxmlrpcdrc", false, false);
		config.setGroup("General");
		config.writeEntry("StartServer", item->isOn());
	}

	// Special case: kwrited
	item = static_cast<QCheckListItem *>(_lvStartup->findItem(KWRITED,4));
	if (item) {
		KConfig config("kwritedrc", false, false);
		config.setGroup("General");
		config.writeEntry("Autostart", item->isOn());
	}

	// Special case: kalarmd
	item = static_cast<QCheckListItem *>(_lvStartup->findItem(KALARMD,4));
	if (item) {
		KConfig config("kalarmdrc", false, false);
		config.setGroup("General");
		config.writeEntry("Autostart", item->isOn());
	}
};


void KDEDConfig::defaults()
{
	QListViewItemIterator it( _lvStartup);
	while ( it.current() != 0 ) {
		if (it.current()->rtti()==1) {
			QCheckListItem *item = static_cast<QCheckListItem *>(it.current());
			item->setOn(false);
		}
		++it;
	}

	getServiceStatus();

        QCheckListItem* item;
	// Special case: kxmlrpcd
	item = static_cast<QCheckListItem *>(_lvStartup->findItem(QString::fromLatin1(KXMLRPCD),4));
	if (item) {
		item->setOn(KXMLRPCD_DEFAULT);
	}

	// Special case: kwrited
	item = static_cast<QCheckListItem *>(_lvStartup->findItem(KWRITED,4));
	if (item) {
		item->setOn(KWRITED_DEFAULT);
	}

	// Special case: kalarmd
	item = static_cast<QCheckListItem *>(_lvStartup->findItem(KALARMD,4));
	if (item) {
		item->setOn(KALARMD_DEFAULT);
	}
}


void KDEDConfig::getServiceStatus()
{
	QCStringList modules;
	QCString replyType;
	QByteArray replyData;


	if (!kapp->dcopClient()->call( "kded", "kded", "loadedModules()", QByteArray(),
				replyType, replyData ) ) {

		_lvLoD->setEnabled( false );
		_lvStartup->setEnabled( false );
		KMessageBox::error(this, i18n("Unable to contact KDED!"));
		return;
	}
	else {

		if ( replyType == "QCStringList" ) {
			QDataStream reply(replyData, IO_ReadOnly);
			reply >> modules;
		}
	}

	for( QListViewItemIterator it( _lvLoD); it.current() != 0; ++it )
                it.current()->setText(2, i18n("Not running"));
	for( QListViewItemIterator it( _lvStartup); it.current() != 0; ++it )
                it.current()->setText(3, i18n("Not running"));
	for ( QCStringList::Iterator it = modules.begin(); it != modules.end(); ++it )
	{
		QListViewItem *item = _lvLoD->findItem(*it, 4);
		if ( item )
		{
			item->setText(2, i18n("Running"));
		}

		item = _lvStartup->findItem(*it, 4);
		if ( item )
		{
			item->setText(3, i18n("Running"));
		}
	}

	// Special case: kxmlrpcd
	if (kapp->dcopClient()->isApplicationRegistered(KXMLRPCD))
	{
		QListViewItem *item = _lvStartup->findItem(QString::fromLatin1(KXMLRPCD), 4);
		if ( item )
		{
			item->setText(3, i18n("Running"));
		}
	}

	// Special case: kalarmd
	QListViewItem *item = _lvStartup->findItem(QString::fromLatin1(KALARMD), 4);
	if ( item )
	{
		bool running = kapp->dcopClient()->isApplicationRegistered(KALARMD);
		item->setText(3, (running ? i18n("Running") : i18n("Not running")));
	}

	// Special case: kwrited
	if (kapp->dcopClient()->isApplicationRegistered(KWRITED))
	{
		QListViewItem *item = _lvStartup->findItem(QString::fromLatin1(KWRITED), 4);
		if ( item )
		{
			item->setText(3, i18n("Running"));
		}
	}
}

void KDEDConfig::slotReload()
{
	QString current = _lvStartup->currentItem()->text(4);
	load();
	QListViewItem *item = _lvStartup->findItem(current, 4);
	if (item)
		_lvStartup->setCurrentItem(item);
}

void KDEDConfig::slotEvalItem(QListViewItem * item)
{
	if (!item)
		return;

	// Special case: kxmlrpcd
	bool options = false;
	if ( item->text(4) == QString::fromLatin1(KXMLRPCD) )
		options = true;
	_pbOptions->setEnabled(options);

	if ( item->text(3) == i18n("Running") ) {
		_pbStart->setEnabled( false );
		_pbStop->setEnabled( true );
	}
	else if ( item->text(3) == i18n("Not running") ) {
		_pbStart->setEnabled( true );
		_pbStop->setEnabled( false );
	}
	else // Error handling, better do nothing
	{
		_pbStart->setEnabled( false );
		_pbStop->setEnabled( false );
	}

	getServiceStatus();
}

void KDEDConfig::slotServiceRunningToggled()
{
	getServiceStatus();
	slotEvalItem(_lvStartup->currentItem());
}

void KDEDConfig::slotStartService()
{
	QCString service = _lvStartup->currentItem()->text(4).latin1();

	// Special case: kxmlrpcd
	if (service == KXMLRPCD)
	{
		KApplication::startServiceByDesktopName(KXMLRPCD);
		slotServiceRunningToggled();
		return;
	}

	// Special case: kalarmd
	if (service == KALARMD)
	{
		KApplication::startServiceByDesktopName(KALARMD);
		slotServiceRunningToggled();
		return;
	}

	// Special case: kwrited
	if (service == KWRITED)
	{
		KApplication::startServiceByDesktopName(KWRITED);
		slotServiceRunningToggled();
		return;
	}

	QByteArray data;
	QDataStream arg( data, IO_WriteOnly );
	arg << service;
	if (kapp->dcopClient()->send( "kded", "kded", "loadModule(QCString)", data ) ) {
		slotServiceRunningToggled();
	}
	else {
		KMessageBox::error(this, i18n("Unable to start service!"));
	}
}

void KDEDConfig::slotStopService()
{
	QCString service = _lvStartup->currentItem()->text(4).latin1();
	kdDebug() << "Stopping: " << service << endl;
	QByteArray data;
	QDataStream arg( data, IO_WriteOnly );

	// Special case: kxmlrpcd
	if (service == KXMLRPCD)
	{
		kapp->dcopClient()->send(KXMLRPCD, "qt/"+KXMLRPCD, "quit()", data);
		QTimer::singleShot(200, this, SLOT(slotServiceRunningToggled()));
		return;
	}

	// Special case: kalarmd
	if (service == KALARMD)
	{
		kapp->dcopClient()->send(KALARMD, "qt/"+KALARMD, "quit()", data);
		QTimer::singleShot(200, this, SLOT(slotServiceRunningToggled()));
		return;
	}

	// Special case: kwrited
	if (service == KWRITED)
	{
		kapp->dcopClient()->send(KWRITED, "qt/"+KWRITED, "quit()", data);
		QTimer::singleShot(200, this, SLOT(slotServiceRunningToggled()));
		return;
	}

	arg << service;
	if (kapp->dcopClient()->send( "kded", "kded", "unloadModule(QCString)", data ) ) {
		slotServiceRunningToggled();
	}
	else {
		KMessageBox::error(this, i18n("Unable to stop service!"));
	}

}

void KDEDConfig::configureService()
{
	QCString service = _lvStartup->currentItem()->text(4).latin1();

	// Special case: kxmlrpcd
	if (service == KXMLRPCD)
	{
		KXmlRpcDialog dlg(this);
		dlg.exec();
		return;
	}
}

void KDEDConfig::slotItemChecked(QCheckListItem*)
{
	emit changed(true);
}

const KAboutData* KDEDConfig::aboutData() const
{
	KAboutData *about =
		new KAboutData( I18N_NOOP( "kcmkded" ), I18N_NOOP( "KDE Service Manager" ),
				0, 0, KAboutData::License_GPL,
				I18N_NOOP( "(c) 2002 Daniel Molkentin" ) );
	about->addAuthor("Daniel Molkentin",0,"molkentin@kde.org");
	return about;

}

QString KDEDConfig::quickHelp() const
{
	return i18n("<h1>Service Manager</h1><p>This module allows you to have an overview of all plugins of the "
			"KDE Daemon, also referred to as KDE Services. Generally, there are two types of service:</p>"
			"<ul><li>Services invoked at startup</li><li>Services called on demand</li></ul>"
			"<p>The latter are only listed for convenience. The startup services can be started and stopped. "
			"In Administrator mode, you can also define whether services should be loaded at startup.</p>"
			"<p><b> Use this with care. Some services are vital for KDE. Don't deactivate services if you"
			" don't know what you are doing!</b></p>");
}



CheckListItem::CheckListItem(QListView *parent, const QString &text)
	: QObject(parent),
	  QCheckListItem(parent, text, CheckBox)
{ }

void CheckListItem::stateChange(bool on)
{
	QCheckListItem::stateChange(on);
	emit changed(this);
}
