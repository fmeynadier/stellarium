/*
 * Stellarium
 * Copyright (C) 2011 Timothy Reaves
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _TESTINDI_HPP_
#define _TESTINDI_HPP_

#include <QObject>
#include <QtTest>

class TestNumberElement : public QObject
{
Q_OBJECT
private slots:
	void testReadDoubleFromString_EmptyString_Returns0();
	void testReadDoubleFromString_NonNumber_Returns0();
	void testReadDoubleFromString_ReturnsNumber();
	void testReadDoubleFromString_SexagesimalInputTwoElementWithSpaces();
	void testReadDoubleFromString_SexagesimalInputThreeElementsWithColons();
	void testReadDoubleFromString_SexagesimalConsistence();
	void testReadDoubleFromString_SexagesimalConsistenceHighPrecision();
	void testNumberElementSetValue_MoreThanMaximumShouldBeIgnored();
	void testNumberElementSetValue_LessThanMinimumShouldBeIgnored();
	void testNumberElementSetValue_CorrectStepIncrement();
	void testNumberElementSetValue_NotStepIncrement_ShouldIgnoreValue();

private:
};

#endif // _TESTINDI_HPP_

