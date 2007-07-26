/*
 *   Copyright (C) 2007 Ivan Cukic <ivan.cukic+kde@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
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

#include "filebrowserengine.h"

#include "plasma/datacontainer.h"

#include <QDir>
#include <KDirWatch>
#include <KDebug>
#include <KFileMetaInfo>

#define InvalidIfEmpty(A) ((A.isEmpty())?(QVariant()):(QVariant(A)))
#define forMatchingSources for (DataEngine::SourceDict::iterator it = sources.begin(); it != sources.end(); it++) \
  if (dir == QDir(it.key()))

FileBrowserEngine::FileBrowserEngine (QObject* parent, const QStringList& args)
  : Plasma::DataEngine(parent) {
  Q_UNUSED(args)

  _dirWatch = new KDirWatch(this);
  connect(_dirWatch, SIGNAL(created(const QString &)), this, SLOT(dirCreated(const QString &)));
  connect(_dirWatch, SIGNAL(deleted(const QString &)), this, SLOT(dirDeleted(const QString &)));
  connect(_dirWatch, SIGNAL(dirty(const QString &)),   this, SLOT(dirDirty(const QString &)));
};

FileBrowserEngine::~FileBrowserEngine() {};

void FileBrowserEngine::init() {
  kDebug() << "init() called" << endl;
};

bool FileBrowserEngine::sourceRequested (const QString &path) {
  kDebug() << "source requested() called: " << path << endl;
  _dirWatch->addDir(path);
  setData(path, "type", QVariant("unknown"));
  updateData (path, INIT);
  return true;
};

void FileBrowserEngine::dirDirty (const QString &path) {
  updateData(path, DIRTY);
};

void FileBrowserEngine::dirCreated (const QString &path) {
  updateData(path, CREATED);
};

void FileBrowserEngine::dirDeleted (const QString &path) {
  updateData(path, DELETED);
};

void FileBrowserEngine::updateData (const QString &path, EventType event) {
  Q_UNUSED(event)

  ObjectType type = NOTHING;
  if (QDir(path).exists()) {
    type = DIRECTORY;
  } else if (QFile::exists(path)) {
    type = FILE;
  }

  DataEngine::SourceDict sources = sourceDict();

  QDir dir(path);
  clearData(path);

  if (type == DIRECTORY) {
    kDebug() << "directory info processing: " << path << endl;
    if (dir.isReadable()) {
      QStringList visibleFiles       = dir.entryList(QDir::Files, QDir::Name);
      QStringList allFiles           = dir.entryList(QDir::Files | QDir::Hidden, QDir::Name);

      QStringList visibleDirectories = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
      QStringList allDirectories     = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden, QDir::Name);

      forMatchingSources {
        kDebug() << "MATCH" << endl;
        it.value()->setData("item.type",  QVariant("directory"));

        it.value()->setData("directories.visible",  InvalidIfEmpty(visibleDirectories));
        it.value()->setData("directories.all",      InvalidIfEmpty(allDirectories));
        it.value()->setData("files.visible",        InvalidIfEmpty(visibleFiles));
        it.value()->setData("files.all",            InvalidIfEmpty(allFiles));
      }
    }
  } else if (type == FILE) {
    kDebug() << "file info processing: " << path << endl;
    KFileMetaInfo kfmi(path, QString::null, KFileMetaInfo::Everything);
    if (kfmi.isValid()) {
      kDebug() << "METAINFO: " << kfmi.keys() << endl;

      forMatchingSources {
        kDebug() << "MATCH" << endl;
        it.value()->setData("item.type",  QVariant("file"));

        for (QHash< QString, KFileMetaInfoItem >::const_iterator i = kfmi.items().constBegin(); i != kfmi.items().constEnd(); i++) {
          it.value()->setData(i.key(), i.value().value());
        }
      }
    }
  } else {
    forMatchingSources {
      it.value()->setData("item.type",  QVariant("imaginary"));
    }
  };

  checkForUpdates();

};

void FileBrowserEngine::clearData (const QString &path) {
  QDir dir(path);
  DataEngine::SourceDict sources = sourceDict();
  for (DataEngine::SourceDict::iterator it = sources.begin(); it != sources.end(); it++) {
    if (dir == QDir(it.key())) {
      kDebug() << "matched: " << path << " " << it.key() << endl;
      it.value()->clearData();

    } else {
      kDebug() << "didn't match: " << path << " " << it.key() << endl;
    }
  }
};

#include "filebrowserengine.moc"
