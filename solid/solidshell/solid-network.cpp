/*  This file is part of the KDE project
    Copyright (C) 2006 Kevin Ottens <ervin@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include "solid-network.h"


#include <QString>
#include <QStringList>
#include <QMetaProperty>
#include <QMetaEnum>
#include <QTimer>

#include <kcomponentdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <k3socketaddress.h>
#include <kdebug.h>

#include <solid/device.h>
#include <solid/genericinterface.h>
#include <solid/storageaccess.h>
#include <solid/opticaldrive.h>

#include <solid/control/ifaces/authentication.h>
#include <solid/control/networkmanager.h>
#include <solid/control/networkinterface.h>
#include <solid/control/network.h>
#include <solid/control/wirelessnetwork.h>

#include <kjob.h>


#include <iostream>
using namespace std;

static const char appName[] = "solid-network";
static const char programName[] = I18N_NOOP("solid-network");

static const char description[] = I18N_NOOP("KDE tool for querying and controlling your network interfaces from the command line");

static const char version[] = "0.1";

std::ostream &operator<<(std::ostream &out, const QString &msg)
{
    return (out << msg.toLocal8Bit().constData());
}

std::ostream &operator<<(std::ostream &out, const QVariant &value)
{
    switch (value.type())
    {
    case QVariant::StringList:
    {
        out << "{";

        QStringList list = value.toStringList();

        QStringList::ConstIterator it = list.begin();
        QStringList::ConstIterator end = list.end();

        for (; it!=end; ++it)
        {
            out << "'" << *it << "'";

            if (it+1!=end)
            {
                out << ", ";
            }
        }

        out << "}  (string list)";
        break;
    }
    case QVariant::Bool:
        out << (value.toBool()?"true":"false") << "  (bool)";
        break;
    case QVariant::Int:
        out << value.toString()
            << "  (0x" << QString::number(value.toInt(), 16) << ")  (int)";
        break;
    default:
        out << "'" << value.toString() << "'  (string)";
        break;
    }

    return out;
}

std::ostream &operator<<(std::ostream &out, const Solid::Device &device)
{
    out << "  parent = " << QVariant(device.parentUdi()) << endl;
    out << "  vendor = " << QVariant(device.vendor()) << endl;
    out << "  product = " << QVariant(device.product()) << endl;

    int index = Solid::DeviceInterface::staticMetaObject.indexOfEnumerator("Type");
    QMetaEnum typeEnum = Solid::DeviceInterface::staticMetaObject.enumerator(index);

    for (int i=0; i<typeEnum.keyCount(); i++)
    {
        Solid::DeviceInterface::Type type = (Solid::DeviceInterface::Type)typeEnum.value(i);
        const Solid::DeviceInterface *interface = device.asDeviceInterface(type);

        if (interface)
        {
            const QMetaObject *meta = interface->metaObject();

            for (int i=meta->propertyOffset(); i<meta->propertyCount(); i++)
            {
                QMetaProperty property = meta->property(i);
                out << "  " << QString(meta->className()).mid(7) << "." << property.name()
                    << " = ";

                QVariant value = property.read(interface);

                if (property.isEnumType()) {
                    QMetaEnum metaEnum = property.enumerator();
                    out << "'" << metaEnum.valueToKeys(value.toInt()).constData() << "'"
                        << "  (0x" << QString::number(value.toInt(), 16) << ")  ";
                    if (metaEnum.isFlag()) {
                        out << "(flag)";
                    } else {
                        out << "(enum)";
                    }
                    out << endl;
                } else {
                    out << value << endl;
                }
            }
        }
    }

    return out;
}

std::ostream &operator<<(std::ostream &out, const QMap<QString,QVariant> &properties)
{
    foreach (QString key, properties.keys())
    {
        out << "  " << key << " = " << properties[key] << endl;
    }

    return out;
}

std::ostream &operator<<(std::ostream &out, const Solid::Control::NetworkInterface &networkdevice)
{
    out << "  UNI =                " << QVariant(networkdevice.uni()) << endl;
    out << "  Type =               " << (networkdevice.type() == Solid::Control::NetworkInterface::Ieee8023 ? "Wired" : "802.11 Wireless") << endl;
    out << "  Active =             " << (networkdevice.isActive() ? "Yes" : "No") << endl;
    //out << "  HW Address =         " << networkdevice.  // TODO add to solid API.
    out << "\n  Capabilities:" << endl;
    out << "    Supported =        " << (networkdevice.capabilities()  & Solid::Control::NetworkInterface::IsManageable ? "Yes" : "No") << endl;
    out << "    Speed =            " << networkdevice.designSpeed() << endl;
    if (networkdevice.type() == Solid::Control::NetworkInterface::Ieee8023)
        out << "    Carrier Detect =   " << (networkdevice.capabilities()  & Solid::Control::NetworkInterface::SupportsCarrierDetect ? "Yes" : "No") << endl;
    else
        out << "    Wireless Scan =    " << (networkdevice.capabilities()  & Solid::Control::NetworkInterface::SupportsWirelessScan ? "Yes" : "No") << endl;
    out << "    Link Up =          " << (networkdevice.isLinkUp() ? "Yes" : "No") << endl;

    return out;
}

std::ostream &operator<<(std::ostream &out, const Solid::Control::Network &network)
{
    out << "  UNI =                " << QVariant(network.uni()) << endl;
    out << "  Addresses:" << endl;
    foreach (QNetworkAddressEntry addr, network.addressEntries())
    {
        out << "    (" << addr.ip().toString() << "," << addr.broadcast().toString() << "," << addr.ip().toString() << ")" << endl;
    }
    if (network.addressEntries().isEmpty())
        out << "    none" << endl;
    out << "  Route:               " << QVariant(network.route()) <<  endl;
    out << "  DNS Servers:" << endl;
    int i = 1;
    foreach (QHostAddress addr, network.dnsServers())
    {
        out << "  " << i++ << ": " << addr.toString() << endl;
    }
    if (network.dnsServers().isEmpty())
        out << "    none" << endl;
    out << "  Active =             " << (network.isActive() ? "Yes" : "No") << endl;

    return out;
}

std::ostream &operator<<(std::ostream &out, const Solid::Control::WirelessNetwork &network)
{
    out << "  ESSID =                " << QVariant(network.essid()) << endl;
    out << "  Mode =                 ";
    switch (network.mode())
    {
    case Solid::Control::WirelessNetwork::Unassociated:
        cout << "Unassociated" << endl;
        break;
    case Solid::Control::WirelessNetwork::Adhoc:
        cout << "Ad-hoc" << endl;
        break;
    case Solid::Control::WirelessNetwork::Managed:
        cout << "Infrastructure" << endl;
        break;
    case Solid::Control::WirelessNetwork::Master:
        cout << "Master" << endl;
        break;
    case Solid::Control::WirelessNetwork::Repeater:
        cout << "Repeater" << endl;
        break;
    default:
        cout << "Unknown" << endl;
        cerr << "Unknown network operation mode: " << network.mode() << endl;
        break;
    }
    out << "  Frequency =            " << network.frequency() << endl;
    out << "  Rate =                 " << network.bitrate() << endl;
    out << "  Strength =             " << network.signalStrength() << endl;
    if (network.isEncrypted())
    {
        out << "  Encrypted =            Yes (";
        Solid::Control::WirelessNetwork::Capabilities cap = network.capabilities();
        if (cap  & Solid::Control::WirelessNetwork::Wep)
            out << "WEP,";
        if (cap  & Solid::Control::WirelessNetwork::Wpa)
            out << "WPA,";
        if (cap  & Solid::Control::WirelessNetwork::Wpa2)
            out << "WPA2,";
        if (cap  & Solid::Control::WirelessNetwork::Psk)
            out << "PSK,";
        if (cap  & Solid::Control::WirelessNetwork::Ieee8021x)
            out << "Ieee8021x,";
        if (cap  & Solid::Control::WirelessNetwork::Wep40)
            out << "WEP40,";
        if (cap  & Solid::Control::WirelessNetwork::Wep104)
            out << "WEP104,";
        if (cap  & Solid::Control::WirelessNetwork::Wep192)
            out << "WEP192,";
        if (cap  & Solid::Control::WirelessNetwork::Wep256)
            out << "WEP256,";
        if (cap  & Solid::Control::WirelessNetwork::WepOther)
            out << "WEP-Other,";
        if (cap  & Solid::Control::WirelessNetwork::Tkip)
            out << "TKIP";
        if (cap  & Solid::Control::WirelessNetwork::Ccmp)
            out << "CCMP";
        out << ")" << endl;
    }
    else
        out << "  Encrypted =            No" << endl;

    return out;
}

void checkArgumentCount(int min, int max)
{
    int count = KCmdLineArgs::parsedArgs()->count();

    if (count < min)
    {
        cerr << i18n("Syntax Error: Not enough arguments") << endl;
        ::exit(1);
    }

    if ((max > 0) && (count > max))
    {
        cerr << i18n("Syntax Error: Too many arguments") << endl;
        ::exit(1);
    }
}

int main(int argc, char **argv)
{
  KCmdLineArgs::init(argc, argv, appName, 0, ki18n(programName), version, ki18n(description), false);


  KCmdLineOptions options;

  options.add("commands", ki18n("Show available commands by domains"));

  options.add("+command", ki18n("Command (see --commands)"));

  options.add("+[arg(s)]", ki18n("Arguments for command"));

  KCmdLineArgs::addCmdLineOptions(options);

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  KComponentData componentData(appName);

  if (args->isSet("commands"))
  {
      KCmdLineArgs::enable_i18n();

      cout << endl << i18n("Syntax:") << endl << endl;

      cout << "  solid-network listdevices" << endl;
      cout << i18n("             # List the network devices present.\n") << endl;

      cout << "  solid-network listnetworks 'uni'" << endl;
      cout << i18n("             # List the networks known to the device specified by 'uni'.\n") << endl;

      cout << "  solid-network query (status|wireless)|(interface 'uni')|(network 'device-uni' 'network-uni')" << endl;
      cout << i18n("             # Query whether networking features are active or not.\n"
                    "             # - If the 'status' option is given, return whether\n"
                    "             # networking is enabled for the system\n"
                    "             # - If the 'wireless' option is is given, return whether\n"
                    "             # wireless is enabled for the system\n"
                    "             # - If the 'interface' option is given, print the\n"
                    "             # properties of the network interface that 'uni' refers to.\n"
                    "             # - If the 'network' option is given, print the\n"
                    "             # properties of the network on 'device-uni' that 'network-uni' refers to.\n") << endl;

      cout << "  solid-network set wireless (enabled|disabled)" << endl;
      cout << i18n("             # Enable or disable networking on this system.\n") << endl;

      cout << "  solid-network set networking (enabled|disabled)" << endl;
      cout << i18n("             # Enable or disable networking on this system.\n") << endl;

      cout << "  solid-network set network 'device-uni' 'network-uni' [authentication 'key']" << endl;
      cout << i18n("             # Activate the network 'network-uni' on 'device-uni'.\n"
                    "             # Optionally, use WEP128, open-system encryption with hex key 'key'. (Hardcoded)\n"
                    "             # Where 'authentication' is one of:\n"
                    "             # wep hex64|ascii64|hex128|ascii128|passphrase64|passphrase128 'key' [open|shared]\n"
                    "             # wpapsk wpa|wpa2 tkip|ccmp-aes password\n"
                    "             # wpaeap UNIMPLEMENTED IN SOLIDSHELL\n") << endl;

      cout << endl;

      return 0;
  }

  return SolidNetwork::doIt() ? 0 : 1;
}

bool SolidNetwork::doIt()
{
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    checkArgumentCount(1, 0);

    QString command(args->arg(0));

    int fake_argc = 0;
    char **fake_argv = 0;
    SolidNetwork shell(fake_argc, fake_argv);

    if (command == "query")
    {
        checkArgumentCount(2, 4);
        QString what(args->arg(1));
        if (what == "status")
            return shell.netmgrNetworkingEnabled();
        else if (what == "wireless")
            return shell.netmgrWirelessEnabled();
        else if (what == "interface")
        {
            checkArgumentCount(3, 3);
            QString uni(args->arg(2));
            return shell.netmgrQueryNetworkInterface(uni);
        }
        else if (what == "network")
        {
            checkArgumentCount(4, 4);
            QString dev(args->arg(2));
            QString uni(args->arg(3));
            return shell.netmgrQueryNetwork(dev, uni);
        }
        else
            cerr << i18n("Syntax Error: Unknown option '%1'", what) << endl;
    }
    else if (command == "set")
    {
        checkArgumentCount(3, 9);
        QString what(args->arg(1));
        QString how(args->arg(2));
        if (what == "networking")
        {
            bool enabled;
            if (how == "enabled")
            {
                enabled = true;
            }
            else if (how == "disabled")
            {
                enabled = false;
            }
            else
            {
                cerr << i18n("Syntax Error: Unknown option '%1'", how) << endl;
                return false;
            }
            shell.netmgrChangeNetworkingEnabled(enabled);
            return true;
        }
        else if (what == "wireless")
        {
            bool enabled;
            if (how == "enabled")
            {
                enabled = true;
            }
            else if (how == "disabled")
            {
                enabled = false;
            }
            else
            {
                cerr << i18n("Syntax Error: Unknown option '%1'", how) << endl;
                return false;
            }
            shell.netmgrChangeWirelessEnabled(enabled);
            return true;
        }
    /*cout << "  solid-network set network 'device-uni' 'network-uni' [authentication 'key']" << endl; */
        /*wep hex64|ascii64|hex128|ascii128|passphrase 'key' [open|shared] */
        /* wpaeap UNIMPLEMENTED */
        else if (what == "network")
        {
            checkArgumentCount(4, 9);
            QString dev(args->arg(2));
            QString uni(args->arg(3));
            Solid::Control::Authentication * auth = 0;
            QMap<QString,QString> secrets;

            if (KCmdLineArgs::parsedArgs()->count() > 4)
            {
                QString hasAuth = args->arg(4);
                if (hasAuth == "authentication")
                {
                    //encrypted network
                    QString authScheme = args->arg(5);
                    if (authScheme == "wep")
                    {
                        Solid::Control::AuthenticationWep *wepAuth = new Solid::Control::AuthenticationWep();
                        QString keyType = args->arg(6);
                        if (keyType == "hex64")
                        {
                            wepAuth->setType(Solid::Control::AuthenticationWep::WepHex);
                            wepAuth->setKeyLength(64);
                        }
                        else if (keyType == "ascii64")
                        {
                            wepAuth->setType(Solid::Control::AuthenticationWep::WepAscii);
                            wepAuth->setKeyLength(64);
                        }
                        else if (keyType == "hex128")
                        {
                            wepAuth->setType(Solid::Control::AuthenticationWep::WepHex);
                            wepAuth->setKeyLength(128);
                        }
                        else if (keyType == "ascii128")
                        {
                            wepAuth->setType(Solid::Control::AuthenticationWep::WepAscii);
                            wepAuth->setKeyLength(128);
                        }
                        else if (keyType == "passphrase64")
                        {
                            wepAuth->setType(Solid::Control::AuthenticationWep::WepPassphrase);
                            wepAuth->setKeyLength(64);
                        }
                        else if (keyType == "passphrase128")
                        {
                            wepAuth->setType(Solid::Control::AuthenticationWep::WepPassphrase);
                            wepAuth->setKeyLength(128);
                        }
                        else
                        {
                            cerr << i18n("Unrecognised WEP type '%1'", keyType) << endl;
                            delete wepAuth;
                            return false;
                        }
    
                        QString key = args->arg(7);
                        secrets.insert("key", key);
                        wepAuth->setSecrets(secrets);
    
                        QString method = args->arg(8);
                        if (method == "open")
                            wepAuth->setMethod(Solid::Control::AuthenticationWep::WepOpenSystem);
                        else if (method == "shared")
                            wepAuth->setMethod(Solid::Control::AuthenticationWep::WepSharedKey);
                        else
                        {
                            cerr << i18n("Unrecognised WEP method '%1'", method) << endl;
                            delete wepAuth;
                            return false;
                        }
                        auth = wepAuth;
                    }
                    else if (authScheme == "wpapsk")
                    {
                        /* wpapsk wpa|wpa2 tkip|ccmp-aes password */
                        Solid::Control::AuthenticationWpaPersonal *wpapAuth = new Solid::Control::AuthenticationWpaPersonal();
                        QString version = args->arg(6);
                        if (version == "wpa")
                            wpapAuth->setVersion(Solid::Control::AuthenticationWpaPersonal::Wpa1);
                        else if (version == "wpa2")
                            wpapAuth->setVersion(Solid::Control::AuthenticationWpaPersonal::Wpa1);
                        else
                        {
                            cerr << i18n("Unrecognised WPA version '%1'", version) << endl;
                            delete wpapAuth;
                            return false;
                        }
                        QString protocol = args->arg(7);
                        if (protocol == "tkip")
                            wpapAuth->setProtocol(Solid::Control::AuthenticationWpaPersonal::WpaTkip);
                        else if (protocol == "ccmp-aes")
                            wpapAuth->setProtocol(Solid::Control::AuthenticationWpaPersonal::WpaCcmpAes);
                        else
                        {
                            cerr << i18n("Unrecognised WPA encryption protocol '%1'", protocol) << endl;
                            delete wpapAuth;
                            return false;
                        }
                        QString key = args->arg(8);
                        secrets.insert("key", key);
                        wpapAuth->setSecrets(secrets);
                        auth = wpapAuth;
                    }
                    else
                    {
                        cerr << i18n("Unimplemented auth scheme '%1'", args->arg(5)) << endl;
                        return false;
                    }
                }
            }
            else
            {
                //unencrypted network
                auth = new Solid::Control::AuthenticationNone;
            }

            return shell.netmgrActivateNetwork(dev, uni, auth);
            delete auth;
        }
        else
        {
            cerr << i18n("Syntax Error: Unknown object '%1'", what) << endl;
            return false;
        }
    }
    else if (command == "listdevices")
    {
        return shell.netmgrList();
    }
    else if (command == "listnetworks")
    {
        checkArgumentCount(2, 2);
        QString device(args->arg(1));
        return shell.netmgrListNetworks(device);
    }
    else
    {
        cerr << i18n("Syntax Error: Unknown command '%1'" , command) << endl;
    }

    return false;
}

bool SolidNetwork::netmgrNetworkingEnabled()
{
    if (Solid::Control::NetworkManager::isNetworkingEnabled())
        cout << i18n("networking: is enabled")<< endl;
    else
        cout << i18n("networking: is not enabled")<< endl;
    return Solid::Control::NetworkManager::isNetworkingEnabled();
}

bool SolidNetwork::netmgrWirelessEnabled()
{
    if (Solid::Control::NetworkManager::isWirelessEnabled())
        cout << i18n("wireless: is enabled")<< endl;
    else
        cout << i18n("wireless: is not enabled")<< endl;
    return Solid::Control::NetworkManager::isWirelessEnabled();
}

bool SolidNetwork::netmgrChangeNetworkingEnabled(bool enabled)
{
    Solid::Control::NetworkManager::setNetworkingEnabled(enabled);
    return true;
}

bool SolidNetwork::netmgrChangeWirelessEnabled(bool enabled)
{
    Solid::Control::NetworkManager::setWirelessEnabled(enabled);
    return true;
}

bool SolidNetwork::netmgrList()
{
    const Solid::Control::NetworkInterfaceList all = Solid::Control::NetworkManager::networkInterfaces();

    cerr << "debug: network interface list contains: " << all.count() << " entries" << endl;
    foreach (const Solid::Control::NetworkInterface device, all)
    {
        cout << "UNI = '" << device.uni() << "'" << endl;
    }
    return true;
}

bool SolidNetwork::netmgrListNetworks(const QString  & deviceUni)
{
    Solid::Control::NetworkInterface device = Solid::Control::NetworkManager::findNetworkInterface(deviceUni);

    Solid::Control::NetworkList networks = device.networks();
    foreach (const Solid::Control::Network * net, networks)
    {
        cout << "NETWORK UNI = '" << net->uni() << "'" << endl;
    }

    return true;
}

bool SolidNetwork::netmgrQueryNetworkInterface(const QString  & deviceUni)
{
    cerr << "SolidNetwork::netmgrQueryNetworkInterface()" << endl;
    Solid::Control::NetworkInterface device = Solid::Control::NetworkManager::findNetworkInterface(deviceUni);
    cout << device << endl;
    return true;
}

bool SolidNetwork::netmgrQueryNetwork(const QString  & deviceUni, const QString  & networkUni)
{
    cerr << "SolidNetwork::netmgrQueryNetwork()" << endl;
    Solid::Control::NetworkInterface device = Solid::Control::NetworkManager::findNetworkInterface(deviceUni);
    Solid::Control::Network * network = device.findNetwork(networkUni);
    cout << *network << endl;
    Solid::Control::WirelessNetwork * wlan = qobject_cast<Solid::Control::WirelessNetwork *>(network);
    if (wlan)
    {
        cout << *wlan << endl;
    }
    return true;
}

bool SolidNetwork::netmgrActivateNetwork(const QString  & deviceUni, const QString  & networkUni, Solid::Control::Authentication * auth)
{
    Solid::Control::NetworkInterface device = Solid::Control::NetworkManager::findNetworkInterface(deviceUni);
    Solid::Control::Network * network = device.findNetwork(networkUni);
    Solid::Control::WirelessNetwork * wlan = 0;
    if (( wlan = qobject_cast<Solid::Control::WirelessNetwork *>(network)))
    {
        wlan->setAuthentication(auth);
        wlan->setActivated(true);
    }
    else
        network->setActivated(true);
    return true;
}

void SolidNetwork::connectJob(KJob *job)
{
    connect(job, SIGNAL(result(KJob *)),
             this, SLOT(slotResult(KJob *)));
    connect(job, SIGNAL(percent(KJob *, unsigned long)),
             this, SLOT(slotPercent(KJob *, unsigned long)));
    connect(job, SIGNAL(infoMessage(KJob *, const QString &, const QString &)),
             this, SLOT(slotInfoMessage(KJob *, const QString &)));
}

void SolidNetwork::slotPercent(KJob */*job */, unsigned long percent)
{
    cout << i18n("Progress: %1%" , percent) << endl;
}

void SolidNetwork::slotInfoMessage(KJob */*job */, const QString &message)
{
    cout << i18n("Info: %1" , message) << endl;
}

void SolidNetwork::slotResult(KJob *job)
{
    m_error = 0;

    if (job->error())
    {
        m_error = job->error();
        m_errorString = job->errorString();
    }

    m_loop.exit();
}

void SolidNetwork::slotStorageResult(Solid::ErrorType error, const QVariant &errorData)
{
    if (error) {
        m_error = 1;
        m_errorString = errorData.toString();
    }
    m_loop.exit();
}

#include "solid-network.moc"
