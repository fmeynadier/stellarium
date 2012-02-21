/*
* Stellarium
* Copyright (C) 2005 Fabien Chereau
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
* Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#ifndef _STELTRANSLATOR_HPP_
#define _STELTRANSLATOR_HPP_

//! @file StelTranslator.hpp
//! Define some translation macros.

#include <QMap>
#include <QString>
#include "StelUtils.hpp"

// These macro are used as global function replacing standard gettext operation
#include "gettext.h"

//! @def q_(str)
//! Return the gettext translated english text @a str using the current global translator.
//! The returned value is a localized QString.
#define q_(str) StelTranslator::globalTranslator.qtranslate(str)

//! @def N_(str)
//! A pseudo function call that serves as a marker for the automated extraction of messages.
//! A call to N_() doesn't translate.
#define N_(str) gettext_noop(str)

//! Class used to translate strings to any language.
//! Implements a nice interface to gettext which is UTF-8 compliant and is somewhat multiplateform
//! All its operations do not modify the global locale.
//! The purpose of this class is to remove all non-OO C locale functions from stellarium.
//! @author Fabien Chereau
class StelTranslator
{
public:

	//! Create a translator from a language name.
	//! If the passed locale name cannot be handled by the system, default value will be used.
	//! The passed language name is a language code string like "fr" or "fr_FR".
	//! This class wrap gettext to simulate an object oriented multiplateform gettext UTF8 translator
	//! @param adomain The name of the domain to use for translation
	//! @param amoDirectory The directory where to look for the domain.mo translation files.
	//! @param alangName The C locale name or language name like "fr" or "fr_FR". If string is "" or "system" it will use the system locale.
	StelTranslator(const QString& adomain, const QString& amoDirectory, const QString& alangName) :
			domain(adomain), moDirectory(amoDirectory), langName(alangName)
	{
		StelTranslator::lastUsed = NULL;
	}
	
	//! Translate input message and return it as a QString.
	//! @param s input string in english.
	//! @return The translated QString
	QString qtranslate(const QString& s)
	{
		if (s.isEmpty()) return QString();
		reload();
		return QString::fromUtf8(gettext(s.toUtf8().constData()));
	}
	
	//! Get true translator locale name. Actual locale, never "system".
	//! @return Locale name e.g "fr_FR"
	const QString& getTrueLocaleName(void) const
	{
		if (langName=="system" || langName=="system_default")
			return StelTranslator::systemLangName;
		else
			return langName;
	}

	//! Used as a global translator by the whole app
	static StelTranslator globalTranslator;

	//! Get available language name in native language from passed locales directory
	QStringList getAvailableLanguagesNamesNative(const QString& localeDir="") const;	

	//! Convert from ISO639-1 langage code to native language name
	//! @param languageCode the code to look up
	static QString iso639_1CodeToNativeName(const QString& languageCode);
	
	//! Convert from native language name to ISO639-1 2 letters langage code 
	static QString nativeNameToIso639_1Code(const QString& languageName);
	
	//! Initialize Translation
	//! @param fileName file containing the list of language codes
	static void init(const QString& fileName);
	
private:
	//! Initialize the languages code list from the passed file
	//! @param fileName file containing the list of language codes
	static void initIso639_1LanguageCodes(const QString& fileName);
	
	//! Get available language codes from passed locales directory
	QStringList getAvailableIso639_1Codes(const QString& localeDir="") const;
	
	//! Reload the current locale info so that gettext use them
	void reload();

	//! The domain name
	QString domain;

	//! The directory where the locale file tree stands
	QString moDirectory;

	//! The two letter string defining the current language name
	QString langName;

	//! Keep in memory which one was the last used transator to prevent reloading it at each tranlate() call
	static StelTranslator* lastUsed;

	//! Try to determine system language from system configuration
	static void initSystemLanguage(void);
	
	//! Store the system default language name as taken from LANGUAGE environement variable
	static QString systemLangName;
	
	//! Contains the list of all iso639 languages codes
	static QMap<QString, QString> iso639codes;
};

#endif // _STELTRANSLATOR_HPP_

