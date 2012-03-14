/*
 * Qt-based INDI wire protocol client
 * 
 * Copyright (C) 2010-2011 Bogdan Marinov
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

#include "IndiProperty.hpp"

#include <QDir>
#include <QStringList>
#include <QDebug>


const char* TagAttributes::VERSION = "version";
const char* TagAttributes::DEVICE = "device";
const char* TagAttributes::NAME = "name";
const char* TagAttributes::LABEL = "label";
const char* TagAttributes::GROUP = "group";
const char* TagAttributes::STATE = "state";
const char* TagAttributes::PERMISSION = "perm";
const char* TagAttributes::TIMEOUT = "timeout";
const char* TagAttributes::TIMESTAMP = "timestamp";
const char* TagAttributes::MESSAGE = "message";
const char* TagAttributes::RULE = "rule";

TagAttributes::TagAttributes(const QXmlStreamReader& xmlReader) : 
    areValid(false)
{
	attributes = xmlReader.attributes();
	
	// Required attributes
	device = attributes.value(DEVICE).toString();
	name = attributes.value(NAME).toString();
	if (device.isEmpty() || name.isEmpty())
	{
		qDebug() << "A required attribute is missing (device, name):"
		         << device << name;
		return;
	}
	
	areValid = true;
	
	timeoutString = attributes.value(TIMEOUT).toString();
	timestamp = readTimestampAttribute(attributes);
	message = attributes.value(MESSAGE).toString();
}

QDateTime TagAttributes::readTimestampAttribute(const QXmlStreamAttributes& attributes)
{
	QString timestampString = attributes.value(TIMESTAMP).toString();
	QDateTime timestamp;
	if (!timestampString.isEmpty())
	{
		timestamp = QDateTime::fromString(timestampString, Qt::ISODate);
		timestamp.setTimeSpec(Qt::UTC);
	}
	return timestamp;
}

BasicDefTagAttributes::BasicDefTagAttributes
(const QXmlStreamReader& xmlReader) : 
    TagAttributes(xmlReader)
{
	if (!areValid)
		return;
	
	QString stateString = attributes.value(STATE).toString();
	if (stateString == "Idle")
		state = StateIdle;
	else if (stateString == "Ok")
		state = StateOk;
	else if (stateString == "Busy")
		state = StateBusy;
	else if (stateString == "Alert")
		state = StateAlert;
	else
	{
		qDebug() << "Invalid value for required state attribute:"
		         << stateString;
		areValid = false;
		return;
	}
	
	// Other common attributes
	label = attributes.value(LABEL).toString();
	group = attributes.value(GROUP).toString();
}

StandardDefTagAttributes::StandardDefTagAttributes(const QXmlStreamReader& xmlReader) :
    BasicDefTagAttributes(xmlReader)
{
	// The basic validity should have been evaluated by now
	if (!areValid)
		return;
	
	QString permissionString = attributes.value(PERMISSION).toString();
	if (permissionString == "rw")
		permission = PermissionReadWrite;
	else if (permissionString == "wo")
		permission = PermissionWriteOnly;
	else if (permissionString == "ro")
		permission = PermissionReadOnly;
	else
	{
		qDebug() << "Invalid value for required permission attribute:"
		         << permissionString;
		areValid = false;
	}
}

DefSwitchTagAttributes::DefSwitchTagAttributes(const QXmlStreamReader& xmlReader) :
    StandardDefTagAttributes(xmlReader)
{
	// The basic validity should have been evaluated by now
	if (!areValid)
		return;
	
	QString ruleString  = attributes.value(RULE).toString();
	if (ruleString == "OneOfMany")
		rule = SwitchOnlyOne;
	else if (ruleString == "AtMostOne")
		rule = SwitchAtMostOne;
	else if (ruleString == "AnyOfMany")
		rule = SwitchAny;
	else
	{
		qDebug() << "Invalid value for rule attribute:"
		         << ruleString;
		areValid = false;
	}
}


SetTagAttributes::SetTagAttributes(const QXmlStreamReader& xmlReader) :
    TagAttributes(xmlReader),
    stateChanged(true)
{
	if (!areValid)
		return;
	
	QString stateString = attributes.value(STATE).toString();
	if (stateString == "Idle")
		state = StateIdle;
	else if (stateString == "Ok")
		state = StateOk;
	else if (stateString == "Busy")
		state = StateBusy;
	else if (stateString == "Alert")
		state = StateAlert;
	else
		stateChanged = false;
}

const char* Property::T_DEF_TEXT_VECTOR = "defTextVector";
const char* Property::T_DEF_NUMBER_VECTOR = "defNumberVector";
const char* Property::T_DEF_SWITCH_VECTOR = "defSwitchVector";
const char* Property::T_DEF_LIGHT_VECTOR = "defLightVector";
const char* Property::T_DEF_BLOB_VECTOR = "defBLOBVector";
const char* Property::T_SET_TEXT_VECTOR = "setTextVector";
const char* Property::T_SET_NUMBER_VECTOR = "setNumberVector";
const char* Property::T_SET_SWITCH_VECTOR = "setSwitchVector";
const char* Property::T_SET_LIGHT_VECTOR = "setLightVector";
const char* Property::T_SET_BLOB_VECTOR = "setBLOBVector";
const char* Property::T_NEW_TEXT_VECTOR = "newTextVector";
const char* Property::T_NEW_NUMBER_VECTOR = "newNumberVector";
const char* Property::T_NEW_SWITCH_VECTOR = "newSwitchVector";
const char* Property::T_NEW_BLOB_VECTOR = "newBLOBVector";
const char* Property::T_DEF_TEXT = "defText";
const char* Property::T_DEF_NUMBER = "defNumber";
const char* Property::T_DEF_SWITCH = "defSwitch";
const char* Property::T_DEF_LIGHT = "defLight";
const char* Property::T_DEF_BLOB = "defBLOB";
const char* Property::T_ONE_TEXT = "oneText";
const char* Property::T_ONE_NUMBER = "oneNumber";
const char* Property::T_ONE_SWITCH = "oneSwitch";
const char* Property::T_ONE_LIGHT = "oneLight";
const char* Property::T_ONE_BLOB = "oneBLOB";

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Property Methods
#endif
/* ********************************************************************* */
Property::Property(const QString& propertyName,
				   State propertyState,
				   Permission accessPermission,
				   const QString& propertyLabel,
				   const QString& propertyGroup,
				   const QDateTime& firstTimestamp)
{
	name = propertyName;
	if (propertyLabel.isEmpty())
		label = name;
	else
		label = propertyLabel;

	group = propertyGroup;
	permission = accessPermission;
	state = propertyState;
	setTimestamp(firstTimestamp);
}

Property::Property(const BasicDefTagAttributes& attributes)
{
	name = attributes.name;
	if (attributes.label.isEmpty())
		label = name;
	else
		label = attributes.label;
	
	device = attributes.device;
	group = attributes.group;
	permission = PermissionReadOnly;
	state = attributes.state;
	setTimestamp(attributes.timestamp);
}

Property::~Property()
{
}

Property::PropertyType Property::getType() const
{
	return type;
}

QString Property::getName() const
{
	return name;
}

QString Property::getLabel() const
{
	return label;
}

QString Property::getGroup() const
{
	return group;
}

QString Property::getDevice() const
{
	return device;
}

bool Property::isReadable() const
{
	return (permission == PermissionReadOnly || permission == PermissionReadWrite);
}

bool Property::isWritable() const
{
	return (permission == PermissionWriteOnly || permission == PermissionReadWrite);
}

Permission Property::getPermission() const
{
	return permission;
}

void Property::setState(State newState)
{
	state = newState;
}

State Property::getCurrentState() const
{
	return state;
}

QDateTime Property::getTimestamp() const
{
	return timestamp;
}

qint64 Property::getTimestampInMilliseconds() const
{
	return timestamp.toMSecsSinceEpoch();
}

int Property::elementCount() const
{
	return elements.count();
}
 
QStringList Property::getElementNames() const
{
	return elements.keys();
}

void Property::update(const QHash<QString, QString>& newValues,
                      SetTagAttributes attributes)
{
	QHashIterator<QString, QString> it(newValues);
	while(it.hasNext())
	{
		it.next();
		Element* element = elements.value(it.key(), 0);
		if (element)
			element->setValue(it.value());
	}
	
	if (attributes.stateChanged)
		setState(attributes.state);
	setTimestamp(attributes.timestamp);
	emit newValuesReceived();
}

void Property::send(const QHash<QString, QString>& newValues)
{
	if (type == BlobProperty)
	{
		qWarning() << name << "Writing BLOBs is not supported!";
		return;
	}
	initTagTemplates();
	
	QString timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
	QString tag = newVectorStartTag.arg(timestamp);
	tag.reserve(100 + elements.count() * 100);
	
	QHashIterator<QString, QString> it(newValues);
	while(it.hasNext())
	{
		it.next();
		Element* element = elements.value(it.key(), 0);
		if (element)
		{
			tag += oneElementTemplate.arg(it.key(), it.value());
		}
	}
	
	tag += newVectorEndTag;
	
	emit valuesToSend(tag.toAscii());
}

void Property::setTimestamp(const QDateTime& newTimestamp)
{
	if (newTimestamp.isValid())
	{
		timestamp = newTimestamp;
		//TODO: Re-interpret or convert?
		if(newTimestamp.timeSpec() != Qt::UTC)
			timestamp.setTimeSpec(Qt::UTC);
	}
	else
	{
		timestamp = QDateTime::currentDateTimeUtc();
	}
}

void Property::initTagTemplates()
{
	// Init the templates if necessary
	if (newVectorStartTag.isEmpty())
	{
		newVectorStartTag = QString("<%1 device=\"%2\" name=\"%3\" timestamp=\"%4\">").arg(newVectorTag(), device, name, "%1");
		//qDebug() << newVectorStartTag;
	}
	if (newVectorEndTag.isEmpty())
	{
		newVectorEndTag = QString("</%1>").arg(newVectorTag());
	}
	if (oneElementTemplate.isEmpty())
	{
		oneElementTemplate = QString("<%1 name=\"%2\">%3</%1>").arg(oneElementTag(), "%1", "%2");
	}
}


TextProperty::TextProperty(const QString& propertyName,
                           State propertyState,
                           Permission accessPermission,
                           const QString& propertyLabel,
                           const QString& propertyGroup,
                           const QDateTime& timestamp) :
	Property(propertyName,
	         propertyState,
	         accessPermission,
	         propertyLabel,
	         propertyGroup,
	         timestamp)
{
	type = Property::TextProperty;
}

TextProperty::TextProperty(const StandardDefTagAttributes& attributes) :
    Property(attributes)
{
	permission = attributes.permission;
	type = Property::TextProperty;
}

TextProperty::~TextProperty()
{
	qDeleteAll(elements);
}

void TextProperty::addElement(Element* element)
{
	TextElement* newElement = 0;
	if (newElement = dynamic_cast<TextElement*>(element))
		elements.insert(newElement->getName(), newElement);
}

void TextProperty::addElement(TextElement* element)
{
	elements.insert(element->getName(), element);
}

TextElement* TextProperty::getElement(const QString& name)
{
	return dynamic_cast<TextElement*>(elements.value(name));
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark NumberProperty Methods
#endif
/* ********************************************************************* */
NumberProperty::NumberProperty(const QString& propertyName,
                               State propertyState,
                               Permission accessPermission,
                               const QString& propertyLabel,
                               const QString& propertyGroup,
                               const QDateTime& timestamp) :
	Property(propertyName,
	         propertyState,
	         accessPermission,
	         propertyLabel,
	         propertyGroup,
	         timestamp)
{
	type = Property::NumberProperty;
}

NumberProperty::NumberProperty(const StandardDefTagAttributes& attributes) :
    Property(attributes)
{
	permission = attributes.permission;
	type = Property::NumberProperty;
}

NumberProperty::~NumberProperty()
{
	qDeleteAll(elements);
}

void NumberProperty::addElement(Element* element)
{
	NumberElement* newElement = 0;
	if (newElement = dynamic_cast<NumberElement*>(element))
		elements.insert(newElement->getName(), newElement);
}

void NumberProperty::addElement(NumberElement* element)
{
	elements.insert(element->getName(), element);
}

NumberElement* NumberProperty::getElement(const QString& name)
{
	return dynamic_cast<NumberElement*>(elements.value(name));
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark SwitchProperty Methods
#endif
/* ********************************************************************* */
SwitchProperty::SwitchProperty(const QString &propertyName,
                               State propertyState,
                               Permission accessPermission,
                               SwitchRule switchRule,
                               const QString &propertyLabel,
                               const QString &propertyGroup,
                               const QDateTime& timestamp) :
	Property(propertyName,
	         propertyState,
	         accessPermission,
	         propertyLabel,
	         propertyGroup,
	         timestamp),
	rule(switchRule)
{
	type = Property::SwitchProperty;
}

SwitchProperty::SwitchProperty(const DefSwitchTagAttributes& attributes) :
    Property(attributes)
{
	permission = attributes.permission;
	rule = attributes.rule;
	type = Property::SwitchProperty;
}

SwitchProperty::~SwitchProperty()
{
	qDeleteAll(elements);
}

SwitchRule SwitchProperty::getSwitchRule() const
{
	return rule;
}

void SwitchProperty::addElement(Element* element)
{
	SwitchElement* newElement = 0;
	if (newElement = dynamic_cast<SwitchElement*>(element))
		elements.insert(newElement->getName(), newElement);
}

void SwitchProperty::addElement(SwitchElement* element)
{
	elements.insert(element->getName(), element);
}

SwitchElement* SwitchProperty::getElement(const QString& name)
{
	return dynamic_cast<SwitchElement*>(elements.value(name));
}

void SwitchProperty::send(const QHash<QString, QString>& newValues)
{
	send(newValues);
}

void SwitchProperty::send(const QHash<QString, bool>& newValues)
{
	initTagTemplates();
	
	QString timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
	QString tag = newVectorStartTag.arg(timestamp);
	tag.reserve(100 + elements.count() * 100);
	
	QHashIterator<QString, bool> it(newValues);
	while(it.hasNext())
	{
		it.next();
		Element* element = elements.value(it.key(), 0);
		if (element)
		{
			const char* value = (it.value()) ? "On" : "Off";
			tag += oneElementTemplate.arg(it.key(), value);
		}
	}
	
	tag += newVectorEndTag;
	
	emit valuesToSend(tag.toAscii());
}


LightProperty::LightProperty(const QString& propertyName,
                               State propertyState,
                               const QString& propertyLabel,
                               const QString& propertyGroup,
                               const QDateTime& timestamp) :
	Property(propertyName,
	         propertyState,
	         PermissionReadOnly,
	         propertyLabel,
	         propertyGroup,
	         timestamp)
{
	type = Property::LightProperty;
}

LightProperty::LightProperty(const BasicDefTagAttributes& attributes) :
    Property(attributes)
{
	type = Property::LightProperty;
}

LightProperty::~LightProperty()
{
	qDeleteAll(elements);
}

void LightProperty::addElement(Element* element)
{
	LightElement* newElement = 0;
	if (newElement = dynamic_cast<LightElement*>(element))
		elements.insert(newElement->getName(), newElement);
}

void LightProperty::addElement(LightElement* element)
{
	elements.insert(element->getName(), element);
}

LightElement* LightProperty::getElement(const QString& name)
{
	return dynamic_cast<LightElement*>(elements.value(name));
}


BlobProperty::BlobProperty(const QString& propertyName,
                           State propertyState,
                           Permission accessPermission,
                           const QString& propertyLabel,
                           const QString& propertyGroup,
                           const QDateTime& timestamp) :
	Property(propertyName,
	         propertyState,
	         accessPermission,
	         propertyLabel,
	         propertyGroup,
	         timestamp)
{
	type = Property::BlobProperty;
}

BlobProperty::BlobProperty(const StandardDefTagAttributes& attributes) :
    Property(attributes)
{
	permission = attributes.permission;
	type = Property::BlobProperty;
}

BlobProperty::~BlobProperty()
{
	qDeleteAll(elements);
}

void BlobProperty::addElement(Element *element)
{
	BlobElement* newElement = 0;
	if (newElement = dynamic_cast<BlobElement*>(element))
		elements.insert(newElement->getName(), newElement);
}

void BlobProperty::addElement(BlobElement* element)
{
	elements.insert(element->getName(), element);
}

BlobElement* BlobProperty::getElement(const QString& name)
{
	return (dynamic_cast<BlobElement*>(elements[name]));
}

void BlobProperty::update(const QHash<QString, QString>& newValues,
                          SetTagAttributes attributes)
{
	Q_UNUSED(newValues);
	
	if (attributes.stateChanged)
		setState(attributes.state);
	setTimestamp(attributes.timestamp);
}
