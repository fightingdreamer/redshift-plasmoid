/***************************************************************************
 *   Copyright (C) 2012 by Simone Gaiarin <simgunz@gmail.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 **************************************************************************/

#include "redshiftapplet.h"

#include <QGraphicsLinearLayout>

#include <Plasma/Svg>
#include <Plasma/Theme>
#include <Plasma/DataEngine>
#include <Plasma/ToolTipContent>
#include <Plasma/ToolTipManager>

#include <KLocale>
#include <KConfigDialog>
#include <KComboBox>

#include "redshiftsettings.h"

RedshiftApplet::RedshiftApplet(QObject *parent, const QVariantList &args)
    : Plasma::Applet(parent, args),
      m_icon("redshift"),
      m_theme(this)
{
    setBackgroundHints(StandardBackground);
    setAspectRatioMode(Plasma::ConstrainedSquare);
    setHasConfigurationInterface(true);
}

void RedshiftApplet::init()
{
    m_tooltip.setMainText(i18n("Redshift"));
    m_tooltip.setSubText(i18n("Click to toggle it on"));
    m_tooltip.setImage(KIcon("redshift-status-off"));
    Plasma::ToolTipManager::self()->setContent(this, m_tooltip);

    m_button = new Plasma::IconWidget(this);
    m_button->setIcon(KIcon("redshift-status-off"));
    m_layout = new QGraphicsGridLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->addItem(m_button, 0, 0);
    m_engine = dataEngine("redshift");
    m_engine->connectSource("Controller", this);
    connect(m_button, SIGNAL(clicked()), this, SLOT(toggle()));

    QAction *action = new QAction(this);
    //action->setToolTip(i18nc("tooltip on the config button in the popup", "Configure Power Management..."));
    action->setIcon(KIcon("system-reboot"));
    action->setText(i18n("Restart Redshift"));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(configChanged()));
    addAction("restart_redshift", action);
}

void RedshiftApplet::dataUpdated(const QString &sourceName, const Plasma::DataEngine::Data &data)
{
    if (sourceName == "Controller") {
        if (data["Status"].toString() == "Running") {
            m_button->setIcon(KIcon("redshift-status-on"));
            m_tooltip.setSubText(i18n("Click to toggle it off"));
            m_tooltip.setImage(KIcon("redshift-status-on"));
        } else {
            m_button->setIcon(KIcon("redshift-status-off"));
            m_tooltip.setSubText(i18n("Click to toggle it on"));
            m_tooltip.setImage(KIcon("redshift-status-off"));
        }
        Plasma::ToolTipManager::self()->setContent(this, m_tooltip);
    }
}

void RedshiftApplet::createConfigurationInterface(KConfigDialog *parent)
{
    RedshiftSettings::self()->readConfig();
    const QStringList alwaysOnActivities = RedshiftSettings::alwaysOnActivities();
    const QStringList alwaysOffActivities = RedshiftSettings::alwaysOffActivities();

    QWidget *redshiftInterface = new QWidget(parent);
    m_redshiftUi.setupUi(redshiftInterface);
    parent->addPage(redshiftInterface, RedshiftSettings::self(),
                    i18nc("Redshift main configuration page","General"), "redshift");

    QWidget *activitiesInterface = new QWidget(parent);
    m_activitiesUi.setupUi(activitiesInterface);

    Plasma::DataEngine *activities_engine = dataEngine("org.kde.activities");
    QStringList activities = activities_engine->sources();
    activities.removeLast();

    QString act;
    foreach(act, activities) {
        Plasma::DataEngine::Data data = activities_engine->query(act);
        QTreeWidgetItem *listItem = new QTreeWidgetItem(m_activitiesUi.activities);
        KComboBox *itemCombo = new KComboBox(m_activitiesUi.activities);
        listItem->setText(0, data["Name"].toString());
        listItem->setIcon(0, KIcon(data["Icon"].toString()));
        listItem->setFlags(Qt::ItemIsEnabled);
        listItem->setData(0, Qt::UserRole, act);

        itemCombo->addItem(i18nc("Redshift state is set manually in this activity", "Manual"));
        itemCombo->addItem(i18nc("Redshift is forced to be active in this activity", "Always Active"));
        itemCombo->addItem(i18nc("Redshift is forced to be disabled in this activity", "Always Disabled"));

        if (alwaysOnActivities.contains(act)) {
            itemCombo->setCurrentIndex(1);
        } else if (alwaysOffActivities.contains(act)) {
            itemCombo->setCurrentIndex(2);
        } else {
            itemCombo->setCurrentIndex(0);
        }

        m_activitiesUi.activities->setItemWidget(listItem, 1, itemCombo);
        connect(itemCombo, SIGNAL(currentIndexChanged(int)), parent, SLOT(settingsModified()));
    }
    m_activitiesUi.activities->resizeColumnToContents(0);

    connect(parent, SIGNAL(applyClicked()), this, SLOT(configAccepted()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));

    parent->addPage(activitiesInterface, i18nc("Redshift activities behaviour configuration page", "Activities"),
                    "preferences-activities");
}

void RedshiftApplet::toggle()
{
    Plasma::Service *service = m_engine->serviceForSource("Controller");
    service->startOperationCall(service->operationDescription("toggle"));
}

void RedshiftApplet::configAccepted()
{
    QStringList alwaysOnActivities;
    QStringList alwaysOffActivities;

    QTreeWidget *activitiesList = m_activitiesUi.activities;
    for (int i = 0; i < activitiesList->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = activitiesList->topLevelItem(i);
        KComboBox *itemCombo = static_cast<KComboBox *>(activitiesList->itemWidget(item, 1));
        const QString act = item->data(0, Qt::UserRole).toString();
        if (itemCombo->currentIndex() == 1) {
            alwaysOnActivities << act;
        } else if (itemCombo->currentIndex() == 2) {
            alwaysOffActivities << act;
        }
    }
    RedshiftSettings::setAlwaysOnActivities(alwaysOnActivities);
    RedshiftSettings::setAlwaysOffActivities(alwaysOffActivities);
    RedshiftSettings::self()->writeConfig();
}

void RedshiftApplet::configChanged()
{
    Plasma::Service *service = m_engine->serviceForSource("Controller");
    service->startOperationCall(service->operationDescription("restart"));
}

QList<QAction*> RedshiftApplet::contextualActions()
{
    QList<QAction *> rv;
    QAction *act = action("restart_redshift");
    rv << act;
    return rv;
}

K_EXPORT_PLASMA_APPLET(redshift, RedshiftApplet)

#include "redshiftapplet.moc"
