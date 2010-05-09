/*  This file is part of the KDE project
    Copyright (C) 2006 Kevin Ottens <ervin@kde.org>
    Copyright (C) 2008 Dario Freddi <drf54321@gmail.com>

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

#ifndef SOLID_IFACES_POWERMANAGER_H
#define SOLID_IFACES_POWERMANAGER_H

#include <QtCore/QObject>
#include <QtCore/QStringList>

#include "../solid_control_export.h"

#include "../powermanager.h"

class KJob;

namespace Solid
{
namespace Control
{
namespace Ifaces
{
    /**
     * This class specifies the interface a backend will have to implement in
     * order to be used in the system.
     *
     * A power manager allows to control or query the power management features
     * or the underlying platform.
     */
    class SOLIDCONTROLIFACES_EXPORT PowerManager : public QObject
    {
        Q_OBJECT

    public:
        /**
         * Constructs a PowerManager
         */
        PowerManager(QObject *parent = 0);

        /**
         * Destructs a PowerManager object
         */
        virtual ~PowerManager();

        /**
         * Retrieves the current state of the system battery.
         *
         * @return the current battery state
         * @see Solid::Control::PowerManager::BatteryState
         */
        virtual Solid::Control::PowerManager::BatteryState batteryState() const = 0;

        /**
         * Retrieves the current charge percentage of the system batteries.
         *
         * @return the current global battery charge percentage
         */
        virtual int batteryChargePercent() const = 0;

        /**
         * Retrieves the current estimated remaining time of the system batteries
         *
         * @return the current global estimated remaining time in milliseconds
         */
        virtual int batteryRemainingTime() const = 0;

        /**
         * Retrieves the current state of the system AC adapter.
         *
         * @return the current AC adapter state
         * @see Solid::Control::PowerManager::AcAdapterState
         */
        virtual Solid::Control::PowerManager::AcAdapterState acAdapterState() const = 0;


        /**
         * Retrieves the set of suspend methods supported by the system.
         *
         * @return the suspend methods supported by this system
         * @see Solid::Control::PowerManager::SuspendMethod
         * @see Solid::Control::PowerManager::SuspendMethods
         */
        virtual Solid::Control::PowerManager::SuspendMethods supportedSuspendMethods() const = 0;

        /**
         * Requests a suspend of the system.
         *
         * @param method the suspend method to use
         * @return the job handling the operation
         */
        virtual KJob *suspend(Solid::Control::PowerManager::SuspendMethod method) const = 0;


        /**
         * Retrieves the set of CPU frequency policies supported by the system.
         *
         * @return the CPU frequency policies supported by this system
         * @see Solid::Control::PowerManager::CpuFreqPolicy
         * @see Solid::Control::PowerManager::CpuFreqPolicies
         */
        virtual Solid::Control::PowerManager::CpuFreqPolicies supportedCpuFreqPolicies() const = 0;

        /**
         * Retrieves the current CPU frequency policy of the system.
         *
         * @return the current CPU frequency policy used by the system
         * @see Solid::Control::PowerManager::CpuFreqPolicy
         */
        virtual Solid::Control::PowerManager::CpuFreqPolicy cpuFreqPolicy() const = 0;

        /**
         * Changes the current power management policy of the system.
         *
         * @param powersave if powersaving should be anabled
         * @return true if the policy change succeeded, false otherwise
         * @see Solid::Control::PowerManager::setPowerSave
         */
        virtual bool setPowerSave(bool powersave) = 0;

        /**
         * Changes the current CPU frequency policy of the system.
         *
         * @param newPolicy the new policy
         * @return true if the policy change succeeded, false otherwise
         * @see Solid::Control::PowerManager::CpuFreqPolicy
         */
        virtual bool setCpuFreqPolicy(Solid::Control::PowerManager::CpuFreqPolicy newPolicy) = 0;

        /**
         * Checks if a CPU can be disabled.
         *
         * @param cpuNum the number of the CPU we want to check
         * @return true if the given CPU can be disabled, false otherwise
         */
        virtual bool canDisableCpu(int cpuNum) const = 0;

        /**
         * Enables or disables a CPU.
         *
         * @param cpuNum the number of the CPU we want to enable or disable
         * @param enabled the new state of the CPU
         * @return true if the state change succeeded, false otherwise
         */
        virtual bool setCpuEnabled(int cpuNum, bool enabled) = 0;

        /**
         * Checks if brightness controls are enabled on this system.
         *
         * @return a list of the devices available to control
         */
        virtual Solid::Control::PowerManager::BrightnessControlsList brightnessControlsAvailable() = 0;

        /**
         * Gets the screen brightness.
         *
         * @param device the name of the device that you would like to control
         * @return the brightness of the device, as a percentage
         */
        virtual float brightness(const QString &device = QString()) = 0;

        /**
         * Sets the screen brightness.
         *
         * @param brightness the desired screen brightness, as a percentage
         * @param device the name of the device that you would like to control, as given by brightnessControlsAvailable
         * @return true if the brightness change succeeded, false otherwise
         */
        virtual bool setBrightness(float brightness, const QString &panel = QString()) = 0;

        /**
         * Should be called when the user presses a brightness key.
         *
         * @param type the type of the brightness key press
         * @see Solid::Control::PowerManager::BrightnessKeyType
         */
        virtual void brightnessKeyPressed(Solid::Control::PowerManager::BrightnessKeyType type) = 0;

    Q_SIGNALS:
        /**
         * This signal is emitted when the AC adapter is plugged or unplugged.
         *
         * @param newState the new state of the AC adapter, it's one of the
         * type @see Solid::Control::PowerManager::AcAdapterState
         */
        void acAdapterStateChanged(int newState);

        /**
         * This signal is emitted when the system battery state changed.
         *
         * @param newState the new state of the system battery, it's one of the
         * type @see Solid::Control::PowerManager::BatteryState
         */
        void batteryStateChanged(int newState);

        /**
         * This signal is emitted when a button has been pressed.
         *
         * @param buttonType the pressed button type, it's one of the
         * type @see Solid::Control::PowerManager::ButtonType
         */
        void buttonPressed(int buttonType);

        /**
         * This signal is emitted when the brightness changes.
         *
         * @param brightness the new brightness level
         */
        void brightnessChanged(float brightness);

        /**
         * This signal is emitted when the estimated battery remaining time changes.
         *
         * @param brightness the new remaining time
         */
        void batteryRemainingTimeChanged(int time);
    };
}
}
}

#endif
