/*
 * installer.cpp
 *
 * Copyright (c) 1998 Stefan Taferner <taferner@kde.org>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <qdir.h>
#include <qstrlist.h>
#include <qfiledlg.h>
#include <qbttngrp.h>
#include <qframe.h>
#include <qgrpbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbt.h>
#include <qradiobt.h>
#include <qchkbox.h>
#include <qfileinfo.h>
#include <qlistbox.h>
#include <qmultilinedit.h>
#include <kbuttonbox.h>
#include <kfiledialog.h>
#include <unistd.h>
#include <stdlib.h>

#include "installer.h"
#include "themecreator.h"
#include "global.h"
#include "newthemedlg.h"
#include <klocale.h>

static bool sSettingTheme = false;


//-----------------------------------------------------------------------------
Installer::Installer (QWidget *aParent, const char *aName, bool aInit)
  : InstallerInherited(aParent, aName)
{
  KButtonBox* bbox;

  mGui = !aInit;
  if (!mGui)
  {
    return;
  }

  mEditing = false;

  connect(theme, SIGNAL(changed()), SLOT(slotThemeChanged()));

  mGrid = new QGridLayout(this, 3, 2, 6, 6);
  mThemesList = new QListBox(this);
  connect(mThemesList, SIGNAL(highlighted(int)), SLOT(slotSetTheme(int)));
  mGrid->addMultiCellWidget(mThemesList, 0, 1, 0, 0);

  mPreview = new QLabel(this);
  mPreview->setFrameStyle(QFrame::Panel|QFrame::Sunken);
  mGrid->addWidget(mPreview, 0, 1);

  bbox = new KButtonBox(this, KButtonBox::HORIZONTAL, 0, 6);
  mGrid->addMultiCellWidget(bbox, 2, 2, 0, 1);

  mBtnNew = bbox->addButton(i18n("New..."));
  connect(mBtnNew, SIGNAL(clicked()), SLOT(slotNew()));

  mBtnRemove = bbox->addButton(i18n("Remove"));
  connect(mBtnRemove, SIGNAL(clicked()), SLOT(slotRemove()));

  mBtnImport = bbox->addButton(i18n("Import"));
  connect(mBtnImport, SIGNAL(clicked()), SLOT(slotImport()));

  mBtnExport = bbox->addButton(i18n("Export"));
  connect(mBtnExport, SIGNAL(clicked()), SLOT(slotExport()));

  mText = new QMultiLineEdit(this);
  mText->setMinimumSize(mText->sizeHint());
  mGrid->addWidget(mText, 1, 1);

  mGrid->setColStretch(0, 1);
  mGrid->setColStretch(1, 3);
  mGrid->setRowStretch(0, 3);
  mGrid->setRowStretch(1, 1);
  mGrid->setRowStretch(2, 0);
  mGrid->activate();
  bbox->layout();

  readThemesList();
  slotSetTheme(-1);
}


//-----------------------------------------------------------------------------
Installer::~Installer()
{
}


//-----------------------------------------------------------------------------
void Installer::readThemesList(void)
{
  QDir d(Theme::themesDir(), QString::null, QDir::Name, QDir::Files|QDir::Dirs);
  //d.setNameFilter("*.themerc");
  mThemesList->clear();

  // Read local themes
  QStringList entryList = d.entryList();
  QStringList::ConstIterator name;
  for(name = entryList.begin(); 
      name != entryList.end(); name++)
  {
    if ((*name)[0]=='.') continue;
    if ((*name)=="CVS" || (*name).right(8)==".themerc") continue;
    mThemesList->inSort(*name);
  }

  // Read global themes
  d.setPath(Theme::globalThemesDir());
  entryList = d.entryList();
  for(name=entryList.begin(); name != entryList.end(); name++)
  {
    if ((*name)[0]=='.') continue;
    if ((*name)=="CVS" || (*name).right(8)==".themerc") continue;
    // Dirty hack: the trailing space marks global themes ;-)
    mThemesList->inSort(*name + " ");
  }
}


//-----------------------------------------------------------------------------
void Installer::loadSettings()
{
  debug("Installer::loadSettings() called");
}


//-----------------------------------------------------------------------------
void Installer::applySettings()
{
  debug("Installer::applySettings() called");
}


//-----------------------------------------------------------------------------
void Installer::slotNew()
{
  QString name;
  NewThemeDlg dlg;

  if (!dlg.exec()) return;
  dlg.hide();

  name = dlg.fileName();
  if (!theme->create(name)) return;

  mEditing = true;

  sSettingTheme = true;
  if (findItem(name) < 0) mThemesList->inSort(name);
  mThemesList->setCurrentItem(findItem(name));
  sSettingTheme = false;

  mPreview->setText("");
  mText->setText("");
}


//-----------------------------------------------------------------------------
void Installer::slotRemove()
{
  int cur = mThemesList->currentItem();
  QString cmd, themeFile;
  int rc;
  QFileInfo finfo;

  if (cur < 0) return;
  themeFile = Theme::themesDir() + mThemesList->text(cur);
  cmd.sprintf("rm -rf \"%s\"", (const char*)themeFile);
  finfo.setFile(themeFile);
  rc = system(cmd);
  if (rc || finfo.exists())
  {
    warning(i18n("Failed to remove theme %s"), (const char*)themeFile);
    return;
  }
  mThemesList->removeItem(cur);
  if (cur >= (int)mThemesList->count()) cur--;
  mThemesList->setCurrentItem(cur);
}


//-----------------------------------------------------------------------------
void Installer::slotSetTheme(int id)
{
  bool enabled, isGlobal=false;
  QString name;

  if (sSettingTheme) return;

  if (id < 0)
  {
    mPreview->setText("");
    mText->setText("");
    enabled = false;
  }
  else
  {
    name = mThemesList->text(id);
    if (name.isEmpty()) return;

    isGlobal = (name[name.length()-1]==' ');
    if (isGlobal) name = Theme::globalThemesDir() + name.stripWhiteSpace();
    else name = Theme::themesDir() + name;

    enabled = theme->load(name);
    if (!enabled)
    {
      mPreview->setText(i18n("(no theme chosen)"));
      mText->setText("");
    }
  }

  mBtnExport->setEnabled(enabled);
  mBtnRemove->setEnabled(enabled && !isGlobal);
}


//-----------------------------------------------------------------------------
void Installer::slotImport()
{
  QString fname, fpath, cmd, theme;
  int i, rc;
  static QString path;
  if (path.isEmpty()) path = QDir::homeDirPath();

  KFileDialog dlg(path, "*.tar.gz", 0, 0, true, false);
  dlg.setCaption(i18n("Import Theme"));
  if (!dlg.exec()) return;

  path = dlg.dirPath();
  fpath = dlg.selectedFile();
  i = fpath.findRev('/');
  if (i >= 0) theme = fpath.mid(i+1, 1024);
  else theme = fpath;

  // Copy theme package into themes directory
  cmd.sprintf("cp %s %s", (const char*)fpath, 
	      (const char*)Theme::themesDir());
  rc = system(cmd);
  if (rc)
  {
    warning(i18n("Failed to copy theme %s\ninto themes directory %s"),
	    (const char*)fpath, (const char*)Theme::themesDir());
    return;
  }

  mThemesList->inSort(theme);
}


//-----------------------------------------------------------------------------
void Installer::slotExport()
{
  QString fname, fpath, cmd, themeFile, ext;
  bool isGlobal = false;
  static QString path;
  QFileInfo finfo;
  int cur, i;

  if (path.isEmpty()) path = QDir::homeDirPath();

  cur = mThemesList->currentItem();
  if (cur < 0) return;

  themeFile = mThemesList->text(cur);
  if (themeFile.isEmpty()) return;

  isGlobal = (themeFile[themeFile.length()-1]==' ');
  if (isGlobal) fpath = Theme::globalThemesDir() + themeFile.stripWhiteSpace();
  else fpath = Theme::themesDir() + themeFile;

  finfo.setFile(fpath);
  if (finfo.isDir())
  {
    themeFile += ".tar.gz";
    ext = "*.tar.gz";
  }
  else
  {
    i = themeFile.findRev('.');
    ext = '*' + themeFile.mid(i, 256);
  }

  KFileDialog dlg(path, ext, 0, 0, true, false);
  dlg.setCaption(i18n("Export Theme"));
  dlg.setSelection(themeFile);
  if (!dlg.exec()) return;

  path = dlg.dirPath();
  fpath = dlg.selectedFile();

  theme->save(fpath);
}


//-----------------------------------------------------------------------------
void Installer::slotThemeChanged()
{
  mText->setText(theme->description()); 

  mBtnExport->setEnabled(TRUE);

  if (theme->preview().isNull())
    mPreview->setText(i18n("(no preview pixmap)"));
  else mPreview->setPixmap(theme->preview());
  //mPreview->setFixedSize(theme->preview().size());
}


//-----------------------------------------------------------------------------
int Installer::findItem(const QString aText) const
{
  int id = mThemesList->count()-1;

  while (id >= 0)
  {
    if (mThemesList->text(id) == aText) return id;
    id--;
  }

  return -1;
}


//-----------------------------------------------------------------------------
#include "installer.moc"
