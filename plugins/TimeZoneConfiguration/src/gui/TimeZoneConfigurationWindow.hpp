/*
 * Time zone configuration plug-in for Stellarium
 *
 * Copyright (C) 2010 Bogdan Marinov
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TIME_ZONE_CONFIGURATION_WINDOW_HPP_
#define _TIME_ZONE_CONFIGURATION_WINDOW_HPP_

#include "StelDialog.hpp"

#include <QString>
#include <QDoubleSpinBox>

class Ui_timeZoneConfigurationWindowForm;
class TimeZoneConfiguration;
class DefineTimeZoneWindow;

class TimeZoneConfigurationWindow : public StelDialog
{
	Q_OBJECT

public:
	TimeZoneConfigurationWindow();
	~TimeZoneConfigurationWindow();
	void languageChanged();

	//! Converts a decimal fraction of hours to a string containing a signed
	//! offset in the format used in the TZ variable.
	//! The sign is inverted, as in the TZ format offset = (UTC - local time),
	//! not the traditional offset = (local time - UTC).
	static QString getTzOffsetStringFrom(QDoubleSpinBox * spinBox);

protected:
	void createDialogContent();

private:
	Ui_timeZoneConfigurationWindowForm * ui;
	DefineTimeZoneWindow * defineTimeZoneWindow;
	TimeZoneConfiguration * timeZoneConfiguration;

private slots:
	void saveTimeZoneSettings();
	void openDefineTimeZoneWindow();
	void closeDefineTimeZoneWindow(bool);
	void timeZoneDefined(QString timeZoneDefinition);

	void setTimeFormat(bool);
	void setDateFormat(bool);
	void updateDisplayFormatSwitches();
};


#endif //_TIME_ZONE_CONFIGURATION_WINDOW_HPP_
