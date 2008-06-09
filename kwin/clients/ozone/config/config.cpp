/*
 * Oxygen KWin client configuration module
 *
 * Copyright (C) 2008 Lubos Lunak <l.lunak@kde.org>
 *
 * Based on the Quartz configuration module,
 *     Copyright (c) 2001 Karol Szwed <gallium@kde.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the license, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
 
#include <kglobal.h>
#include <klocale.h>

#include <QCheckBox>

#include "config.moc"

extern "C"
{
    KDE_EXPORT QObject* allocate_config( KConfig* conf, QWidget* parent )
    {
        return ( new Ozone::Oxygen::OxygenConfig( conf, parent ) );
    }
}

namespace Ozone {
namespace Oxygen {

OxygenConfig::OxygenConfig( KConfig*, QWidget* parent )
	: QObject( parent )
{
	KGlobal::locale()->insertCatalog("kwin_clients");
	c = new KConfig( "oxygenrc" );
	KConfigGroup cg(c, "Windeco");
	ui = new OxygenConfigUI( parent );
	connect( ui->showStripes,   SIGNAL(clicked()), SIGNAL(changed()) );
	connect( ui->blendTitlebarColors,   SIGNAL(clicked()), SIGNAL(changed()) );

	load( cg );
	ui->show();
}


OxygenConfig::~OxygenConfig()
{
	delete ui;
	delete c;
}


// Loads the configurable options from the kwinrc config file
// It is passed the open config from kwindecoration to improve efficiency
void OxygenConfig::load( const KConfigGroup&  )
{
	KConfigGroup cg(c, "Windeco");
	ui->showStripes->setChecked( cg.readEntry("ShowStripes", true) );
	ui->blendTitlebarColors->setChecked( cg.readEntry("BlendTitlebarColors", true) );
}


// Saves the configurable options to the kwinrc config file
void OxygenConfig::save( KConfigGroup& )
{
	KConfigGroup cg(c, "Windeco");
	cg.writeEntry( "ShowStripes", ui->showStripes->isChecked() );
	cg.writeEntry( "BlendTitlebarColors", ui->blendTitlebarColors->isChecked() );
	c->sync();
}


// Sets UI widget defaults which must correspond to style defaults
void OxygenConfig::defaults()
{
	ui->showStripes->setChecked( true );
	ui->blendTitlebarColors->setChecked( true );
	
	emit changed();
}

} //namespace Oxygen
} //namespace Ozone
