/* This file is part of the KDE Display Manager Configuration package
    Copyright (C) 1997-1998 Thomas Tanghus (tanghus@earthling.net)

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/  

#include <qdir.h>
#include <qcombobox.h>
#include <kdbtn.h>
#include "utils.h"
#include <klined.h>
#include <klocale.h>
#include <kstddirs.h>
#include "kdm-sess.moc"



// Destructor
KDMSessionsWidget::~KDMSessionsWidget()
{
  if(gui)
  {
    delete restart_lined;
    delete shutdown_lined;
    delete session_lined;
    delete sdcombo;
    delete sessionslb;
  }
}


KDMSessionsWidget::KDMSessionsWidget(QWidget *parent, const char *name, bool init)
  : KConfigWidget(parent, name)
{
      gui = !init;
      loadSettings();      
      if(gui)
        setupPage(parent);
}

void KDMSessionsWidget::setupPage(QWidget *)
{
      QGroupBox *group0 = new QGroupBox( i18n("Allow to shutdown"), this );

      sdcombo = new QComboBox( FALSE, group0 );
      connect(sdcombo, SIGNAL(highlighted(int)), SLOT(slotSetAllowShutdown(int)));
      sdcombo->insertItem(i18n("None"), 0);
      sdcombo->insertItem(i18n("All"), 1);
      sdcombo->insertItem(i18n("Root Only"), 2);
      sdcombo->insertItem(i18n("Console Only"), 3);
      sdcombo->setCurrentItem(sdMode);
      sdcombo->setFixedSize(sdcombo->sizeHint());

      QGroupBox *group1 = new QGroupBox( i18n("Commands"), this );
      QLabel *shutdown_label = new QLabel(i18n("Shutdown"), group1);
      shutdown_label->setFixedSize(shutdown_label->sizeHint());
      shutdown_lined = new KLineEdit(group1);
      shutdown_lined->setFixedHeight(shutdown_lined->sizeHint().height());
      shutdown_lined->setText(shutdownstr);

      QLabel *restart_label = new QLabel(i18n("Restart"), group1);
      restart_label->setFixedSize(restart_label->sizeHint());

      restart_lined = new KLineEdit(group1);
      restart_lined->setFixedHeight(shutdown_lined->height());
      restart_lined->setText(restartstr);

      QLabel *console_label = new QLabel(i18n("Console mode"), group1);
      console_label->setFixedSize(console_label->sizeHint());

      console_lined = new KLineEdit(group1);
      console_lined->setFixedHeight(console_lined->height());
      console_lined->setText(consolestr);

      QGroupBox *group2 = new QGroupBox( i18n("Session types"), this );
      
      QLabel *type_label = new QLabel(i18n("New type"), group2);
      type_label->setFixedSize(type_label->sizeHint());

      session_lined = new KLineEdit(group2);
      session_lined->setFixedHeight(session_lined->sizeHint().height());
      connect(session_lined, SIGNAL(textChanged(const QString&)),
              SLOT(slotCheckNewSession(const QString&)));
      connect(session_lined, SIGNAL(returnPressed()),
              SLOT(slotAddSessionType()));

      QLabel *types_label = new QLabel(i18n("Available types"), group2);
      types_label->setFixedSize(types_label->sizeHint());

      sessionslb = new MyListBox(group2);
      connect(sessionslb, SIGNAL(highlighted(int)), SLOT(slotSessionHighlighted(int)));
      sessionslb->insertStrList(&sessions);

      btnrm = new QPushButton( i18n("Remove"), group2 );
      btnrm->setFixedSize(btnrm->sizeHint());
      btnrm->setEnabled(false);
      connect( btnrm, SIGNAL( clicked() ), SLOT( slotRemoveSessionType() ) );

      btnadd = new QPushButton( i18n("Add"), group2 );
      btnadd->setFixedSize(btnadd->sizeHint());
      btnadd->setEnabled(false);
      connect( btnadd, SIGNAL( clicked() ), SLOT( slotAddSessionType() ) );

      btnup = new KDirectionButton(UpArrow, group2);
      btnup->setFixedSize(20, 20);
      btnup->setEnabled(false);
      connect(btnup, SIGNAL( clicked() ), SLOT( slotSessionUp() ));
      btndown = new KDirectionButton(DownArrow, group2);
      btndown->setFixedSize(20, 20);
      btndown->setEnabled(false);
      connect(btndown,SIGNAL( clicked() ), SLOT( slotSessionDown() ));

      QBoxLayout *main = new QVBoxLayout( this, 10 );
      QBoxLayout *lgroup0 = new QVBoxLayout( group0, 10 );

      QBoxLayout *lgroup1 = new QVBoxLayout( group1, 10 );
      QBoxLayout *lgroup1a = new QHBoxLayout();
      QBoxLayout *lgroup1b = new QHBoxLayout();
      QBoxLayout *lgroup1c = new QHBoxLayout();
      QBoxLayout *lgroup2 = new QVBoxLayout( group2, 10 );
      QBoxLayout *lgroup2sub = new QHBoxLayout();
      QBoxLayout *lgroup2a = new QVBoxLayout();
      QBoxLayout *lgroup2b = new QVBoxLayout();
      QBoxLayout *lgroup2c = new QVBoxLayout();

      main->addWidget(group0);
      main->addWidget(group1);
      main->addWidget(group2, 2);

      lgroup0->addSpacing(10);
      lgroup0->addWidget(sdcombo);

      lgroup1->addSpacing(group1->fontMetrics().height()/2);
      lgroup1->addLayout(lgroup1a);
      lgroup1->addLayout(lgroup1b);
      lgroup1->addLayout(lgroup1c);
      lgroup1a->addWidget(shutdown_label);
      lgroup1a->addWidget(shutdown_lined);
      lgroup1b->addWidget(restart_label);
      lgroup1b->addWidget(restart_lined);
      lgroup1c->addWidget(console_label);
      lgroup1c->addWidget(console_lined);
      lgroup1->activate();

      lgroup2->addSpacing(group2->fontMetrics().height()/2);
      lgroup2->addLayout(lgroup2sub);
      lgroup2sub->addLayout(lgroup2a, 2);
      lgroup2sub->addLayout(lgroup2b, 2);
      lgroup2sub->addLayout(lgroup2c);
      lgroup2a->addWidget(type_label, 10, AlignLeft);
      lgroup2a->addWidget(session_lined);
      lgroup2a->addWidget(btnrm, 10, AlignRight);
      lgroup2a->addWidget(btnadd, 10, AlignRight);
      lgroup2b->addWidget(types_label, 10, AlignLeft);
      lgroup2b->addWidget(sessionslb);
      lgroup2c->addStretch(1);
      lgroup2c->addWidget(btnup);
      lgroup2c->addWidget(btndown);
      lgroup2c->addStretch(1);
      lgroup2->activate();

      main->activate();
}

void KDMSessionsWidget::slotSessionHighlighted(int s)
{
  session_lined->setText(sessionslb->text(s));
  btnup->setEnabled(s > 0);
  btndown->setEnabled(s < (int)sessionslb->count()-1);
  btnrm->setEnabled(sessionslb->currentItem() > -1);
  if(!sessionslb->isItemVisible(s))
    sessionslb->centerCurrentItem();
}

void KDMSessionsWidget::slotCheckNewSession(const QString& str)
{
  btnadd->setEnabled(!str.isEmpty());
}

void KDMSessionsWidget::slotSessionUp()
{
  moveSession(-1);
}

void KDMSessionsWidget::slotSessionDown()
{
  moveSession(1);
}

void KDMSessionsWidget::moveSession(int d)
{
  int id = sessionslb->currentItem();
  QString str = sessionslb->text(id);
  sessionslb->removeItem(id);
  sessionslb->insertItem(str, id+d);
  sessionslb->setCurrentItem(id+d);
}

void KDMSessionsWidget::slotAddSessionType()
{
  if(!session_lined->text().isEmpty())
  {
    sessionslb->insertItem(session_lined->text());
    session_lined->setText("");
  }
}

void KDMSessionsWidget::slotRemoveSessionType()
{
  int i = sessionslb->currentItem();
  if(i > -1)
    sessionslb->removeItem(i);
}

void KDMSessionsWidget::slotSetAllowShutdown(int s)
{
  sdMode = s;
}

void KDMSessionsWidget::applySettings()
{
  //debug("KDMSessionsWidget::applySettings()");
  KSimpleConfig *c = new KSimpleConfig(locate("config", "kdmrc"));

  c->setGroup("KDM");

  if(!shutdown_lined->text().isEmpty())
    c->writeEntry("ShutDown", shutdown_lined->text(), true);
  if(!restart_lined->text().isEmpty())
    c->writeEntry("Restart", restart_lined->text(), true);
  if(!console_lined->text().isEmpty())
    c->writeEntry("ConsoleMode", console_lined->text(), true);

  // write shutdown auth
  switch ( sdMode )
  {
    case Non:
	c->writeEntry( "ShutDownButton", "None" );
	break;
    case All:
	c->writeEntry( "ShutDownButton", "All" );
	break;
    case RootOnly:
	c->writeEntry( "ShutDownButton", "RootOnly" );
	break;
    case ConsoleOnly:
	c->writeEntry( "ShutDownButton", "ConsoleOnly" );
	break;
    default:
	break;
  }

  if(sessionslb->count() > 0)
  {
    QString sesstr;
    for(uint i = 0; i < sessionslb->count(); i++)
    {
      sesstr.append(sessionslb->text(i));
      sesstr.append(";");
    }
    c->writeEntry( "SessionTypes", sesstr );
  }

  delete c;
}

void KDMSessionsWidget::loadSettings()
{
  QString str;
  
  // Get config object
  KSimpleConfig *c = new KSimpleConfig(locate("config", "kdmrc"));
  c->setGroup("KDM");

  // read restart and shutdown cmds
  restartstr = c->readEntry("Restart", "/sbin/reboot");
  shutdownstr = c->readEntry("Shutdown", "/sbin/halt");
  consolestr = c->readEntry("ConsoleMode", "/sbin/init 3");

  str = c->readEntry("ShutDownButton", "All");
  if(str == "All")
    sdMode = All;
  else if(str == "None")
    sdMode = Non;
  else if(str == "RootOnly")
    sdMode = RootOnly;
  else
    sdMode = ConsoleOnly;

  str = c->readEntry( "SessionTypes");
  if(!str.isEmpty())
    semsplit( str, sessions);	  
  //for(uint i = 0; i < sessions.count(); i++)
    //debug("session type: %s", sessions.at(i));

  delete c;
}



