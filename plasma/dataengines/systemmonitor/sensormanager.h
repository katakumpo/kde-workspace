/*
    KSysGuard, the KDE System Guard

    Copyright (c) 1999, 2000 Chris Schlaeger <cs@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef KSG_SENSORMANAGER_H
#define KSG_SENSORMANAGER_H

#include <kconfig.h>

#include <QtCore/QEvent>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QStringList>

#include "sensoragent.h"

namespace KSGRD {

class SensorManagerIterator;

/**
  The SensorManager handles all interaction with the connected
  hosts. Connections to a specific hosts are handled by
  SensorAgents. Use engage() to establish a connection and
  disengage() to terminate the connection.
 */
class SensorManager : public QObject
{
  Q_OBJECT

  friend class SensorManagerIterator;

  public:
    class MessageEvent : public QEvent
    {
      public:
        MessageEvent( const QString &message );

        QString message() const;

      private:
        QString m_message;
    };

    explicit SensorManager();
    ~SensorManager();

    bool engage( const QString &hostName, const QString &shell = "ssh",
                 const QString &command = "", int port = -1 );
    /* Returns true if we are connected or trying to connect to the host given
     */
    bool isConnected( const QString &hostName );
    void requestDisengage( const SensorAgent *agent );
    bool disengage( const SensorAgent *agent );
    bool disengage( const QString &hostName );
    bool resynchronize( const QString &hostName );
    void hostLost( const SensorAgent *agent );
    void notify( const QString &msg ) const;

    void setBroadcaster( QWidget *wdg );

    virtual bool event( QEvent *event );

    bool sendRequest( const QString &hostName, const QString &request,
                      SensorClient *client);

    const QString hostName( const SensorAgent *sensor ) const;
    bool hostInfo( const QString &host, QString &shell,
                   QString &command, int &port );

    QString translateUnit( const QString &unit ) const;
    QString translateSensorPath( const QString &path ) const;
    QString translateSensorType( const QString &type ) const;
    QString translateSensor(const QString& u) const;

    void readProperties( const KConfigGroup& cfg );
    void saveProperties( KConfigGroup& cfg );

    void disconnectClient( SensorClient *client );

  public Q_SLOTS:
    void reconfigure( const SensorAgent *agent );

  Q_SIGNALS:
    void update();
    void hostConnectionLost( const QString &hostName );

  protected:
    QHash<QString, SensorAgent*> m_agents;

  private:
    /**
      These dictionary stores the localized versions of the sensor
      descriptions and units.
     */
    QHash<QString, QString> m_units;
    QHash<QString, QString> m_dict;
    QHash<QString, QString> m_types;

    /** Store the data from the config file to pass to the MostConnector dialog box*/
    QStringList m_hostList;
    QStringList m_commandList;

    QWidget* m_broadcaster;
};

extern SensorManager* SensorMgr;

class SensorManagerIterator : public QHashIterator<QString, SensorAgent*>
{
  public:
    explicit SensorManagerIterator( const SensorManager *sm )
      : QHashIterator<QString, SensorAgent*>( sm->m_agents ) { }

    ~SensorManagerIterator() { }
};

}

#endif
