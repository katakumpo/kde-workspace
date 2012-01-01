/*
 *   Copyright (C) 2007 Petri Damsten <damu@iki.fi>
 *   Copyright (C) 2010 Michel Lafon-Puyo <michel.lafonpuyo@gmail.com>
 *   Copyright (C) 2011 Shaun Reich <shaun.reich@kdemail.net>
 *
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

#ifndef HDD_ACTIVITY_HEADER
#define HDD_ACTIVITY_HEADER

#include "ui_hdd-activity-config.h"
#include "applet.h"

#include <QStandardItemModel>
#include <QRegExp>

#include <Plasma/DataEngine>

namespace Plasma {
}

class QGraphicsLinearLayout;

class Hdd_Activity : public SM::Applet
{
    Q_OBJECT
public:
    Hdd_Activity(QObject *parent, const QVariantList &args);
    ~Hdd_Activity();

    virtual void init();

    virtual bool addVisualization(const QString& source);
    virtual void createConfigurationInterface(KConfigDialog *parent);

public slots:
    void dataUpdated(const QString &name, const Plasma::DataEngine::Data &data);
    void sourceChanged(const QString &name);
    void sourcesChanged();

    void configAccepted();
    void configChanged();

protected:
    QString hddTitle(const QString& uuid, const Plasma::DataEngine::Data &data);
    QString guessHddTitle(const Plasma::DataEngine::Data &data);

private:
    Ui::config ui;
    QStandardItemModel m_hddModel;
    QRegExp m_regexp;
};

K_EXPORT_PLASMA_APPLET(sm_hdd_activity, Hdd_Activity)

#endif
