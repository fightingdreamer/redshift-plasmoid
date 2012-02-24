/**************************************************************************
*   Copyright (C) 2012 by Simone Gaiarin <simgunz@gmail.com>              *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
**************************************************************************/

#include "redshiftcontroller.h"

RedshiftController::RedshiftController() : m_state(false)
{
    m_process = new KProcess();    
    readConfig();
    if(m_autolaunch)
        start();
}

RedshiftController::~RedshiftController()
{
    stop();
}

void RedshiftController::start()
{        
    m_process->waitForFinished();
    QString command = QString("redshift -c /dev/null -l %1:%2 -t %3:%4 -g %5:%6:%7").arg(m_latitude,0,'f',1).arg(m_longitude,0,'f',1).arg(m_dayTemp).arg(m_nightTemp).arg(m_gammaR,0,'f',2).arg(m_gammaG,0,'f',2).arg(m_gammaB,0,'f',2);
    if(!m_smooth)
        command.append(" -r");
    m_process->setShellCommand(command);
    m_process->start();
    emit statusChanged(1);
}

void RedshiftController::stop()
{
    m_process->terminate();
    emit statusChanged(0);
}

void RedshiftController::toggle()
{
    if(!m_process->state())
    {
        start();
        m_state = true;
    }
    else
    {
        stop();
        m_state = false;
    }
}

void RedshiftController::restart()
{
    readConfig();
    stop();
    if(m_state)
        start();
}

void RedshiftController::readConfig()
{
    RedshiftSettings::self()->readConfig();
    m_latitude = RedshiftSettings::latitude();
    m_longitude = RedshiftSettings::longitude();
    m_dayTemp = RedshiftSettings::dayTemp();
    m_nightTemp = RedshiftSettings::nightTemp();
    m_gammaR = RedshiftSettings::gammaR();
    m_gammaG = RedshiftSettings::gammaG();
    m_gammaB = RedshiftSettings::gammaB();
    m_smooth = RedshiftSettings::smooth();
    m_autolaunch = RedshiftSettings::autolaunch();    
}

