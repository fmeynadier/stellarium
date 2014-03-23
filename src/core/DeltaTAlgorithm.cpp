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

#include "DeltaTAlgorithm.hpp"

#include "StelLocaleMgr.hpp"


QString DeltaTAlgorithm::symbol()
{
	//
}


QPair<unsigned int, unsigned int> DeltaTAlgorithm::getRange() const
{
	return QPair<unsigned int, unsigned int>(startYear, endYear);
}

QString DeltaTAlgorithm::getRangeDescription() const
{
	// TODO: Copy-edit the description(s).
	// TRANSLATORS: Range of use of a DeltaT calculation algorithm.
	return q_("Valid range of usage: between years %1 and %2.")
	        .arg(startYear)
	        .arg(endYear);
}

double DeltaTAlgorithm::calculateSecularAcceleration(const int& year) const
{
	//
}

double DeltaTAlgorithm::calculateStandardError(const double& jdUtc, const int& year)
{
	//
}
