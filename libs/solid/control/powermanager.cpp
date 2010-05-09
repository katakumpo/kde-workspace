/*  This file is part of the KDE project
    Copyright (C) 2006-2007 Kevin Ottens <ervin@kde.org>

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

#include "powermanager.h"
#include "powermanager_p.h"

#include "soliddefs_p.h"
#include "managerbase_p.h"
#include "ifaces/powermanager.h"
#include <kdebug.h>
#include <kglobal.h>
#include <QX11Info>

K_GLOBAL_STATIC(Solid::Control::PowerManagerPrivate, globalPowerManager)

Solid::Control::PowerManagerPrivate::PowerManagerPrivate()
{
    loadBackend("Power Management", "SolidPowerManager", "Solid::Control::Ifaces::PowerManager");

    if (managerBackend()!=0) {
        connect(managerBackend(), SIGNAL(acAdapterStateChanged(int)),
                this, SIGNAL(acAdapterStateChanged(int)));
        connect(managerBackend(), SIGNAL(batteryStateChanged(int)),
                this, SIGNAL(batteryStateChanged(int)));
        connect(managerBackend(), SIGNAL(buttonPressed(int)),
                this, SIGNAL(buttonPressed(int)));
        connect(managerBackend(), SIGNAL(brightnessChanged(float)),
                this, SIGNAL(brightnessChanged(float)));
        connect(managerBackend(), SIGNAL(batteryRemainingTimeChanged(int)),
                this, SIGNAL(batteryRemainingTimeChanged(int)));
    }
}

Solid::Control::PowerManagerPrivate::~PowerManagerPrivate()
{
}

bool Solid::Control::PowerManager::setPowerSave(bool powersave)
{
    return_SOLID_CALL(Ifaces::PowerManager *, globalPowerManager->managerBackend(),
                      false, setPowerSave(powersave));
}

Solid::Control::PowerManager::BatteryState Solid::Control::PowerManager::batteryState()
{
    return_SOLID_CALL(Ifaces::PowerManager *, globalPowerManager->managerBackend(),
                      NoBatteryState, batteryState());
}

int Solid::Control::PowerManager::batteryChargePercent()
{
    return_SOLID_CALL(Ifaces::PowerManager *, globalPowerManager->managerBackend(),
                      -1, batteryChargePercent());
}

int Solid::Control::PowerManager::batteryRemainingTime()
{
    return_SOLID_CALL(Ifaces::PowerManager *, globalPowerManager->managerBackend(),
                      -1, batteryRemainingTime());
}

Solid::Control::PowerManager::AcAdapterState Solid::Control::PowerManager::acAdapterState()
{
    return_SOLID_CALL(Ifaces::PowerManager *, globalPowerManager->managerBackend(),
                      Plugged, acAdapterState());
}

Solid::Control::PowerManager::SuspendMethods Solid::Control::PowerManager::supportedSuspendMethods()
{
    return_SOLID_CALL(Ifaces::PowerManager *, globalPowerManager->managerBackend(),
                      UnknownSuspendMethod, supportedSuspendMethods());
}

KJob *Solid::Control::PowerManager::suspend(SuspendMethod method)
{
    return_SOLID_CALL(Ifaces::PowerManager *, globalPowerManager->managerBackend(),
                      0, suspend(method));
}

Solid::Control::PowerManager::CpuFreqPolicies Solid::Control::PowerManager::supportedCpuFreqPolicies()
{
    return_SOLID_CALL(Ifaces::PowerManager *, globalPowerManager->managerBackend(),
                      UnknownCpuFreqPolicy, supportedCpuFreqPolicies());
}

Solid::Control::PowerManager::CpuFreqPolicy Solid::Control::PowerManager::cpuFreqPolicy()
{
    return_SOLID_CALL(Ifaces::PowerManager *, globalPowerManager->managerBackend(),
                      UnknownCpuFreqPolicy, cpuFreqPolicy());
}

bool Solid::Control::PowerManager::setCpuFreqPolicy(CpuFreqPolicy newPolicy)
{
    return_SOLID_CALL(Ifaces::PowerManager *, globalPowerManager->managerBackend(),
                      false, setCpuFreqPolicy(newPolicy));
}

bool Solid::Control::PowerManager::canDisableCpu(int cpuNum)
{
    return_SOLID_CALL(Ifaces::PowerManager *, globalPowerManager->managerBackend(),
                      false, canDisableCpu(cpuNum));
}

bool Solid::Control::PowerManager::setCpuEnabled(int cpuNum, bool enabled)
{
    return_SOLID_CALL(Ifaces::PowerManager *, globalPowerManager->managerBackend(),
                      false, setCpuEnabled(cpuNum, enabled));
}

Solid::Control::PowerManager::BrightnessControlsList Solid::Control::PowerManager::brightnessControlsAvailable()
{
    return_SOLID_CALL(Ifaces::PowerManager *, globalPowerManager->managerBackend(),
                      Solid::Control::PowerManager::BrightnessControlsList(), brightnessControlsAvailable());
}

extern float xrandr_brightlight(Display *dpy, long new_value = -1);

bool Solid::Control::PowerManager::setBrightness(float brightness, const QString &device)
{
    if(device.isEmpty())
    {
        Solid::Control::PowerManager::BrightnessControlsList controls = brightnessControlsAvailable();
        if(controls.keys(Solid::Control::PowerManager::Screen).isEmpty())
        {
#ifdef Q_WS_WIN
            return false;
#else
            return ( xrandr_brightlight( QX11Info::display(), brightness ) >= 0 );
#endif
        }
        else
        {
            foreach(const QString &device, controls.keys())
            {
                SOLID_CALL(Ifaces::PowerManager *, globalPowerManager->managerBackend(), setBrightness(brightness, device));
            }
            //TODO - This should be done better, it will return true even if one of the calls returns false. SOLID_CALL does not allow us to get the return value.
            return true;
        }
    }
    else
    {
        return_SOLID_CALL(Ifaces::PowerManager *, globalPowerManager->managerBackend(),
                      false, setBrightness(brightness, device));
    }
}

float Solid::Control::PowerManager::brightness(const QString &device)
{
    if(device.isEmpty())
    {
        Solid::Control::PowerManager::BrightnessControlsList controls = brightnessControlsAvailable();
        if(controls.keys(Solid::Control::PowerManager::Screen).isEmpty())
        {
#ifdef Q_WS_WIN
            return false;
#else
            return xrandr_brightlight( QX11Info::display() );
#endif
        }
        else
        {
            return_SOLID_CALL(Ifaces::PowerManager *, globalPowerManager->managerBackend(),
                      false, brightness(controls.keys(Solid::Control::PowerManager::Screen).at(0)));
        }
    }
    else
    {
        return_SOLID_CALL(Ifaces::PowerManager *, globalPowerManager->managerBackend(),
                      false, brightness(device));
    }
}

void Solid::Control::PowerManager::brightnessKeyPressed(Solid::Control::PowerManager::BrightnessKeyType type)
{
    SOLID_CALL(Ifaces::PowerManager *, globalPowerManager->managerBackend(),
               brightnessKeyPressed(type));
}

Solid::Control::PowerManager::Notifier *Solid::Control::PowerManager::notifier()
{
    return globalPowerManager;
}

#include "powermanager_p.moc"
#include "powermanager.moc"

