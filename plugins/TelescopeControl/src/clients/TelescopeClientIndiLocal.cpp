/*
 * Stellarium Telescope Control plug-in
 * Copyright (C) 2010  Bogdan Marinov
 * Copyright (C) 2011  Timothy Reaves
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TelescopeClientIndiLocal.hpp"

#include <QFile>
#include <QFileInfo>

TelescopeClientIndiLocal::TelescopeClientIndiLocal(const QString& name, const QString& driverName, Equinox eq):
	TelescopeClientIndi(name, eq),
	driverProcess(0)
{
	qDebug() << "Creating INDI local telescope client:" << name;

	//TODO: Again, use exception instead of this.
	if (driverName.isEmpty()
		 || !QFile::exists("/usr/bin/" + driverName)
		 || !QFileInfo("/usr/bin/" + driverName).isExecutable())
		return;

	driverProcess = new QProcess(this);
	//TODO: Fix this!
	//TODO: Pass the full path instead of only the filename?
	QString driverPath = "/usr/bin/" + driverName;
	QStringList driverArguments;
	qDebug() << driverPath;
	driverProcess->start(driverPath, driverArguments);
	connect(driverProcess, SIGNAL(error(QProcess::ProcessError)),
			  this, SLOT(handleDriverError(QProcess::ProcessError)));

	indiClient = new IndiClient(name, driverProcess);
}

TelescopeClientIndiLocal::~TelescopeClientIndiLocal()
{
	if (indiClient)
	{
		delete indiClient;
	}

	//TODO: Disconnect stuff
	if (driverProcess)
	{
		disconnect(driverProcess, SIGNAL(error(QProcess::ProcessError)),
					  this, SLOT(handleDriverError(QProcess::ProcessError)));
		//TODO: There was some problems on Windows with QProcess - one of the
		//termination methods didn't work. This is not a problem at the moment.
		driverProcess->terminate();
		driverProcess->waitForFinished();
		driverProcess->deleteLater();
	}
}

bool TelescopeClientIndiLocal::isInitialized() const
{
	//TODO: Improve the checks.
	if (indiClient &&
	    driverProcess->state() == QProcess::Running &&
	    driverProcess->isOpen())
		return true;
	else
		return false;
}

bool TelescopeClientIndiLocal::isConnected() const
{
	//TODO: Should include a check for the CONNECTION property
	if (!isInitialized())
		return false;
	else
		return true;
}

bool TelescopeClientIndiLocal::prepareCommunication()
{
	if (!isInitialized())
		return false;

	//TODO: Request properties?
	//TODO: Try to connect

	return true;
}

void TelescopeClientIndiLocal::performCommunication()
{
	//
}

void TelescopeClientIndiLocal::handleDriverError(QProcess::ProcessError error)
{
	//TODO
	qDebug() << error;
	qDebug() << driverProcess->errorString();
}
