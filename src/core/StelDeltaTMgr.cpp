/*
 * Stellarium
 * Copyright (C) 2014  Bogdan Marinov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "StelDeltaTMgr.hpp"
#include "DeltaTAlgorithm.hpp"

#include <QDebug>

using namespace DeltaT;

StelDeltaTMgr::StelDeltaTMgr() :
    currentAlgorithm(0),
    defaultAlgorithm(0),
    zeroAlgorithm(0),
    customAlgorithm(0)
{
	zeroAlgorithm = new WithoutCorrection();
	currentAlgorithm = zeroAlgorithm;
	customAlgorithm = new Custom();
	DeltaTAlgorithm* newAlgorithm = 0;

	algorithms.insert(zeroAlgorithm->getId(), zeroAlgorithm);
	algorithms.insert(customAlgorithm->getId(), customAlgorithm);
	// TODO

	// TODO: Add the actual one
	defaultAlgorithm = zeroAlgorithm;
}

StelDeltaTMgr::~StelDeltaTMgr()
{
	currentAlgorithm = 0;
	defaultAlgorithm = 0;
	zeroAlgorithm = 0;
	customAlgorithm = 0;
	qDeleteAll(algorithms);
	algorithms.clear();
}

void
StelDeltaTMgr::setCurrentAlgorithm(const QString& id)
{
	// TODO: Or should it revert to the default instead?
	currentAlgorithm = algorithms.value(id, zeroAlgorithm);
	if (currentAlgorithm->getId != id)
		qWarning() << "WARNING: Unable to find DeltaT algorithm" << id << "; "
		           << "using" << currentAlgorithm->getId() << "instead.";
}

QString
StelDeltaTMgr::getCurrentAlgorithmId() const
{
	return currentAlgorithm->getId();
}

QList<QString>
StelDeltaTMgr::getAvailableAlgorithmIds() const
{
	return algorithms.keys();
}

QStandardItemModel
StelDeltaTMgr::getAvailableAlgorithmsModel()
{
	return QStandardItemModel();
}

double
StelDeltaTMgr::calculateDeltaT(const double& jdUtc, QString* outputString)
{
	int year, month, day;
	// TODO
	return currentAlgorithm->calculateDeltaT(jdUtc,
	                                         year, month, day,
	                                         outputString);
}

void
StelDeltaTMgr::setCustomAlgorithmParams(const float& year,
                                        const float& ndot,
                                        const float& a,
                                        const float& b,
                                        const float& c)
{
	Custom* algorithm = dynamic_cast<Custom*> customAlgorithm;
	if (algorithm)
		algorithm->setParameters(year, ndot, a, b, c);
}
