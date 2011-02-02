/*
 * Qt-based INDI wire protocol client
 * 
 * Copyright (C) 2011 Bogdan Marinov
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

#ifndef _INDI_SWITCH_PROPERTY_WIDGET_HPP_
#define _INDI_SWITCH_PROPERTY_WIDGET_HPP_

#include <QObject>
#include <QAbstractButton>
#include <QVBoxLayout>

#include "IndiPropertyWidget.hpp"

//! Widget representing a Switch property in the control panel window.
//! It contains an indicator displaying its current state (IndiStateWidget) and
//! a column of buttons. The exact class of the buttons depends on the
//! SwitchRule of the original SwitchProperty:
//!  - SwitchAny uses QCheckBox;
//!  - SwitchAtMostOne uses QPushButton;
//!  - SwitchOnlyOne uses QRadioButton.
//! \todo Better description.
//! \author Bogdan Marinov
class IndiSwitchPropertyWidget : public IndiPropertyWidget
{
	Q_OBJECT

public:
	IndiSwitchPropertyWidget(SwitchProperty* property,
	                         const QString& title,
	                         QWidget* parent = 0);
	~IndiSwitchPropertyWidget();

	//Slot implementation:
	void updateProperty(Property* property);

private slots:
	//! Called when one of the buttons is clicked.
	//! Reads the current states of the buttons and emits newPropertyValue().
	void setNewPropertyValue();

private:
	QVBoxLayout* buttonsLayout;
	QHash<QString,QAbstractButton*> buttons;
	SwitchRule switchRule;
};

#endif//_INDI_SWITCH_PROPERTY_WIDGET_HPP_
