/*
 * Copyright (c) 1998 Stefan Taferner <taferner@kde.org>
 * Copyright (c) 2000 Waldo Bastian <bastian@kde.org>
 */
#include <qframe.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbt.h>
#include <qchkbox.h>
#include <qlistbox.h>
#include <qcombo.h>
#include <kapp.h>
#include <kconfig.h>
#include <qframe.h>
#include <qfont.h>

#include "about.h"
#include "themecreator.h"
#include "global.h"
#include "version.h"
#include <klocale.h>


//-----------------------------------------------------------------------------
About::About (QWidget * aParent, const char *aName, bool aInit)
  : AboutInherited(aParent, aName)
{
  QBoxLayout* box;
  QLabel* lbl;
  QFrame* frm;
  QString str;
  QFont fnt;

  if (aInit) return;

  connect(theme, SIGNAL(changed()), SLOT(slotThemeChanged()));

  box = new QVBoxLayout(this, 20, 6);

  lblTheme = new QLabel(" ", this);
  fnt = lblTheme->font();
  fnt.setPointSize(fnt.pointSize() * 1.2);
  lblTheme->setFont(fnt);
  box->addWidget(lblTheme);

  lblVersion = new QLabel(" ", this);
  box->addWidget(lblVersion);

  lblAuthor = new QLabel(" ", this);
  box->addWidget(lblAuthor);

  lblHomepage = new QLabel(" ", this);
  box->addWidget(lblHomepage);

  frm = new QFrame(this);
  frm->setFrameStyle(QFrame::HLine|QFrame::Raised);
  box->addSpacing(5);
  box->addWidget(frm);
  box->addSpacing(5);

  lbl = new QLabel(i18n("KDE Theme Manager"), this);
  lbl->setFont(fnt);
  box->addWidget(lbl);

  str = i18n("Version %1\n\n"
		   "Copyright (C) 1998-2000 by\n%2%3\n\n"
		   "Gnu Public License (GPL)")
	      .arg(KTHEME_VERSION)
	      .arg("Stefan Taferner <taferner@kde.org>")
              .arg("Waldo Bastian <bastian@kde.org>");
  lbl = new QLabel(str, this);
  lbl->setMinimumSize(lbl->sizeHint());
  box->addWidget(lbl);
  box->addStretch(1000);
}


//-----------------------------------------------------------------------------
About::~About()
{
}


//-----------------------------------------------------------------------------
void About::slotThemeChanged()
{
  QString str, value;

  // Theme name
  value = theme->name();
  str = i18n("%1 Theme").arg(value);
  lblTheme->setText(str);

  // Version
  value = theme->version();
  if (value.isEmpty()) 
     str = "";
  else
     str = i18n("Version %1").arg(value);
  lblVersion->setText(str);

  // Author and email address
  value = theme->author();
  if (value.isEmpty()) value = i18n("Unknown");
  str = i18n("by %2").arg(value);

  value = theme->email();
  if (!value.isEmpty())
  {
    if (value.find('<') >= 0 && value.find('>') >= 0)
      str += " (" + value + ')';
    else str += " <" + value + '>';
  }
  lblAuthor->setText(str);

  // Homepage
  value = theme->homepage();
  lblHomepage->setText(value);
}

//-----------------------------------------------------------------------------
#include "about.moc"
