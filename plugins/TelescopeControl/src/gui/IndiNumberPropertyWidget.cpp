/*
 * Device Control plug-in for Stellarium
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

#include "IndiNumberPropertyWidget.hpp"

#include <stdexcept>
#include <QDoubleValidator>
#include <QRegExp>
#include <QRegExpValidator>

IndiNumberPropertyWidget::IndiNumberPropertyWidget(const NumberPropertyP& property,
                                                   const QString& title,
                                                   QWidget* parent)
	: IndiPropertyWidget(property, title, parent),
	property(property),
	setButton(0),
	gridLayout(0)
{
	Q_ASSERT(property);

	gridLayout = new QGridLayout();
	gridLayout->setContentsMargins(0, 0, 0, 0);
	QStringList elementNames = property->getElementNames();
	int row = 0;
	foreach (const QString& elementName, elementNames)
	{
		NumberElement* element = property->getElement(elementName);
		int column = 0;

		//Labels are immutable and die with the widget
		QLabel* label = new QLabel(element->getLabel());
		gridLayout->addWidget(label, row, column, 1, 1);

		if (property->isReadable())
		{
			column++;
			QLineEdit* lineEdit = new QLineEdit();
			lineEdit->setReadOnly(true);
			lineEdit->setAlignment(Qt::AlignRight);
			lineEdit->setText(element->getFormattedValue());
			displayWidgets.insert(elementName, lineEdit);
			gridLayout->addWidget(lineEdit, row, column, 1, 1);
		}

		if (property->isWritable())
		{
			column++;
			QLineEdit* lineEdit = new QLineEdit();
			lineEdit->setAlignment(Qt::AlignRight);
			lineEdit->setText(element->getFormattedValue());//Only for init!
			inputWidgets.insert(elementName, lineEdit);
			gridLayout->addWidget(lineEdit, row, column, 1, 1);

			//Validator
			//TODO: Is there any sense in this? There will be a separate widget
			//for the standard coordinates properties.
			QRegExp indiFormat("^\\%(\\d+)\\.(\\d)\\m$");
			if (indiFormat.exactMatch(element->getFormatString()))
			{
				int precision = indiFormat.cap(2).toInt();

				QString str;
				switch (precision)
				{
					case 3:
						str = "\\s*\\-?\\d{1,3}\\:\\d{1,2}\\s*";
						break;
					case 5:
						str = "\\s*\\-?\\d{1,3}\\:\\d{1,2}(\\.\\d)?\\s*";
						break;
					case 6:
						str = "\\s*\\-?\\d{1,3}\\:\\d{1,2}\\:\\d{1,2}\\s*";
						break;
					case 8:
						str = "\\s*\\-?\\d{1,3}\\:\\d{1,2}\\:\\d{1,2}(\\.\\d)?\\s*";
						break;
					case 9:
						str = "\\s*\\-?\\d{1,3}\\:\\d{1,2}\\:\\d{1,2}(\\.\\d{1,2})?\\s*";
						break;
					default:
						str.clear();
				}

				if (!str.isEmpty())
				{
					QRegExp regExp(str);
					QRegExpValidator* validator = new QRegExpValidator(regExp,
					                                                   this);
					lineEdit->setValidator(validator);
				}
			}

			if (lineEdit->validator() == 0)
			{
				QDoubleValidator* validator = new QDoubleValidator(this);
				validator->setDecimals(4);
				double min = element->getMinValue();
				double max = element->getMaxValue();
				validator->setBottom(min);
				if (min < max)
					validator->setTop(max);
				lineEdit->setValidator(validator);
			}
		}

		row++;
	}
	mainLayout->addLayout(gridLayout);

	if (property->isWritable())
	{
		setButton = new QPushButton("Set");
		setButton->setSizePolicy(QSizePolicy::Preferred,
		                         QSizePolicy::Preferred);
		connect(setButton, SIGNAL(clicked()),
		        this, SLOT(setNewPropertyValue()));
		mainLayout->addWidget(setButton);
	}
}

IndiNumberPropertyWidget::~IndiNumberPropertyWidget()
{
	//TODO
}

void IndiNumberPropertyWidget::updateFromProperty()
{
	if (property.isNull())
		return;
	
	//State
	State newState = property->getCurrentState();
	stateWidget->setState(newState);
	
	if (property->isReadable())
	{
		QStringList elementNames = property->getElementNames();
		foreach (const QString& elementName, elementNames)
		{
			if (displayWidgets.contains(elementName))
			{
				NumberElement* element = property->getElement(elementName);
				QString value = element->getFormattedValue();
				displayWidgets[elementName]->setText(value);
			}
		}
	}
}

void IndiNumberPropertyWidget::setNewPropertyValue()
{
	QHash<QString,QString> newValues;
	QHashIterator<QString,QLineEdit*> i(inputWidgets);
	while (i.hasNext())
	{
		i.next();
		QString stringValue = i.value()->text().trimmed();
		double value = NumberElement::readDoubleFromString(stringValue);
		newValues.insert(i.key(), QString::number(value, 'f'));
	}
	stateWidget->setState(StateBusy);
	property->send(newValues);
}
