/***************************************************************************
 *   Copyright (C) 2007 by Stephen Leaf                                    *
 *   smileaf@gmail.com                                                     *
 *   Copyright (C) 2008 by Montel Laurent <montel@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA          *
 ***************************************************************************/

#include "adddialog.h"
#include <KLineEdit>
#include <QCheckBox>
#include <QVBoxLayout>
#include <klocale.h>
#include <KUrlRequester>
#include <QLabel>
#include <KMessageBox>

AddDialog::AddDialog (QWidget* parent)
    : KDialog( parent ) {
    QWidget *w = new QWidget( this );
    setButtons( Cancel|Ok );
    QVBoxLayout *lay= new QVBoxLayout;
    w->setLayout( lay );
    QLabel *lab = new QLabel( i18n( "Shell script:" ), w );
    lay->addWidget( lab );
    m_url = new KUrlRequester( w );
    m_url->setFilter( "*.sh" );
    lay->addWidget( m_url );
    m_symlink = new QCheckBox( i18n( "Create as symlink" ), w ); //TODO fix text
    m_symlink->setChecked( true );
    lay->addWidget( m_symlink );
    connect( m_url->lineEdit(), SIGNAL(textChanged(const QString&)), SLOT(textChanged(const QString&)) );
    m_url->lineEdit()->setFocus();
    enableButtonOk(false);

    setMainWidget( w );
}

AddDialog::~AddDialog()
{}

void AddDialog::textChanged(const QString &text) {
    enableButtonOk(!text.isEmpty());
}

KUrl AddDialog::importUrl() const {
    return m_url->lineEdit()->text();
}

bool AddDialog::symLink() const {
    return m_symlink->isChecked();
}

void AddDialog::accept()
{
    if ( !m_url->lineEdit()->text().endsWith( ".sh" ) )
        KMessageBox::error( this, i18n( "KDE can autostart just script file with sh extension." ) );
    else
        KDialog::accept();
}

#include "adddialog.moc"
