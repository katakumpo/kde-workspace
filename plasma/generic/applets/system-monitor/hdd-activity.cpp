/*
 *   Copyright (C) 2007 Petri Damsten <damu@iki.fi>
 *   Copyright (C) 2010 Michel Lafon-Puyo <michel.lafonpuyo@gmail.com>
 *   Copyright (C) 2011 Shaun Reich <shaun.reich@kdemail.net>
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

#include "hdd-activity.h"
#include "monitoricon.h"


#include <KDebug>
#include <KConfigDialog>
#include <KColorUtils>
#include <QFileInfo>
#include <QGraphicsLinearLayout>

#include <Plasma/Meter>
#include <Plasma/Containment>
#include <Plasma/Theme>
#include <Plasma/ToolTipManager>

Hdd_Activity::Hdd_Activity(QObject *parent, const QVariantList &args)
    : SM::Applet(parent, args)
{
    setHasConfigurationInterface(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

Hdd_Activity::~Hdd_Activity()
{
}

void Hdd_Activity::init()
{
    KGlobal::locale()->insertCatalog("plasma_applet_system-monitor");
    setEngine(dataEngine("systemmonitor"));
    setTitle(i18n("Disk Activity"), true);

    configChanged();

    /* At the time this method is running, not all source may be connected. */
    connect(engine(), SIGNAL(sourceAdded(QString)), this, SLOT(sourceChanged(QString)));
    connect(engine(), SIGNAL(sourceRemoved(QString)), this, SLOT(sourceChanged(QString)));
    foreach (const QString& source, engine()->sources()) {
        sourceChanged(source);
    }
}

void Hdd_Activity::sourceChanged(const QString& name)
{
    if (m_rx.indexIn(name) != -1) {
        //kDebug() << m_rx.cap(1);
        //kWarning() << name; // debug
        m_cpus << name;
        if (!m_sourceTimer.isActive()) {
            m_sourceTimer.start(0);
        }
    }
}

void Hdd_Activity::sourcesChanged()
{
    configChanged();
}

void Hdd_Activity::dataUpdated(const QString& source, const Plasma::DataEngine::Data &data)
{
    SM::Plotter *plotter = qobject_cast<SM::Plotter*>(visualization(source));
    if (plotter) {
        double value = data["value"].toDouble();
        QString temp = KGlobal::locale()->formatNumber(value, 1);
        plotter->addSample(QList<double>() << value);
        if (mode() == SM::Applet::Panel) {
            setToolTip(source, QString("<tr><td>%1&nbsp;</td><td>%2%</td></tr>")
            .arg(plotter->title()).arg(temp));
        }
    }
}

void Hdd_Activity::createConfigurationInterface(KConfigDialog *parent)
{
    QWidget *widget = new QWidget();
    ui.setupUi(widget);
    m_hddModel.clear();
    m_hddModel.setHorizontalHeaderLabels(QStringList() << i18n("Mount Point")
                                                       << i18n("Name"));
    QStandardItem *parentItem = m_hddModel.invisibleRootItem();
    Plasma::DataEngine::Data data;
    QString predicateString("IS StorageVolume");

    foreach (const QString& uuid, engine()->query(predicateString)[predicateString].toStringList()) {
        if (!isValidDevice(uuid, &data)) {
            continue;
        }
        QStandardItem *item1 = new QStandardItem(filePath(data));
        item1->setEditable(false);
        item1->setCheckable(true);
        item1->setData(uuid);
        if (sources().contains(uuid)) {
            item1->setCheckState(Qt::Checked);
        }
        QStandardItem *item2 = new QStandardItem(hddTitle(uuid, data));
        item2->setData(guessHddTitle(data));
        item2->setEditable(true);
        parentItem->appendRow(QList<QStandardItem *>() << item1 << item2);
    }

    connect(parent, SIGNAL(applyClicked()), this, SLOT(configAccepted()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));
    connect(ui.treeView, SIGNAL(clicked(QModelIndex)), parent, SLOT(settingsModified()));
    connect(ui.intervalSpinBox, SIGNAL(valueChanged(QString)), parent, SLOT(settingsModified()));

    QWidget *widget = new QWidget();
    ui.setupUi(widget);
    m_model.clear();
    m_model.setHorizontalHeaderLabels(QStringList() << i18n("CPU"));
    QStandardItem *parentItem = m_model.invisibleRootItem();

    foreach (const QString& cpu, m_cpus) {
        if (m_rx.indexIn(cpu) != -1) {
            QStandardItem *item1 = new QStandardItem(cpuTitle(m_rx.cap(1)));
            item1->setEditable(false);
            item1->setCheckable(true);
            item1->setData(cpu);
            if (sources().contains(cpu)) {
                item1->setCheckState(Qt::Checked);
            }
            parentItem->appendRow(QList<QStandardItem *>() << item1);
        }
    }

    ui.treeView->setModel(&m_hddModel);
    ui.treeView->resizeColumnToContents(0);
    ui.intervalSpinBox->setValue(interval() / 1000.0);
    ui.intervalSpinBox->setSuffix(i18nc("second", " s"));
    parent->addPage(widget, i18n("Partitions"), "drive-harddisk");

    connect(parent, SIGNAL(applyClicked()), this, SLOT(configAccepted()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));
    connect(ui.treeView, SIGNAL(clicked(QModelIndex)), parent, SLOT(settingsModified()));
    connect(ui.intervalSpinBox, SIGNAL(valueChanged(QString)), parent, SLOT(settingsModified()));
}

void Hdd_Activity::configChanged()
{
    //KConfigGroup cg = config();
    //QStringList sources = cg.readEntry("uuids", mounted());
    //setSources(sources);
    //setInterval(cg.readEntry("interval", 2) * 60 * 1000);
    //connectToEngine();

    KConfigGroup cg = config();
    QStringList default_cpus;

    if(m_cpus.contains("cpu/system/TotalLoad")) {
        default_cpus << "cpu/system/TotalLoad";
    } else {
        default_cpus = m_cpus;
    }

    setInterval(cg.readEntry("interval", 2.0) * 1000.0);
    setSources(cg.readEntry("cpus", default_cpus));
    connectToEngine();
}

void Hdd_Activity::configAccepted()
{
    KConfigGroup cg = config();
    KConfigGroup cgGlobal = globalConfig();

    clear();

    for (int i = 0; i < parentItem->rowCount(); ++i) {
        QStandardItem *item = parentItem->child(i, 0);
        if (item) {
            if (item->checkState() == Qt::Checked) {
                appendSource(item->data().toString());
            }
        }
    }

    cg.writeEntry("cpus", sources());

    uint interval = ui.intervalSpinBox->value();
    cg.writeEntry("interval", interval);

    emit configNeedsSaving();
}

QString Hdd_Activity::hddTitle(const QString& uuid, const Plasma::DataEngine::Data &data)
{
    KConfigGroup cg = globalConfig();
    QString label = cg.readEntry(uuid, "");

    if (label.isEmpty()) {
        label = guessHddTitle(data);
    }
    return label;
}

bool Hdd_Activity::addVisualization(const QString& source)
{

    return true;
}

#include "hdd-activity.moc"
