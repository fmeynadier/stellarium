/*
 * Stellarium
 * Copyright (C) 2014 Marcos Cardinot
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

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringBuilder>

#include "StelAddOnDAO.hpp"
#include "StelFileMgr.hpp"

StelAddOnDAO::StelAddOnDAO(QSqlDatabase database)
	: m_db(database)
{
}

bool StelAddOnDAO::init()
{
	// Init database
	StelFileMgr::Flags flags = (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable);
	m_sAddonPath = StelFileMgr::findFile("addon/", flags);
	m_db.setHostName("localhost");
	m_db.setDatabaseName(m_sAddonPath % "addon.sqlite");
	bool ok = m_db.open();
	qDebug() << "Add-On Database status:" << m_db.databaseName() << "=" << ok;
	if (m_db.lastError().isValid())
	{
	    qDebug() << "Error loading Add-On database:" << m_db.lastError();
	    return false;
	}

	// creating tables
	if (!createAddonTables() ||
	    !createTableLicense() ||
	    !createTableAuthor())
	{
		return false;
	}

	return true;
}

bool StelAddOnDAO::createAddonTables()
{
	QStringList addonTables;

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_ADDON % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"category TEXT, "
			"title TEXT UNIQUE, "
			"description TEXT, "
			"version TEXT, "
			"compatibility TEXT, "
			"author1 INTEGER, "
			"author2 INTEGER, "
			"license INTEGER, "
			"installed TEXT, "
			"directory TEXT, "
			"url TEXT, "
			"filename TEXT, "
			"download_size TEXT, "
			"checksum TEXT, "
			"last_update TEXT)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_CATALOG % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"addon INTEGER UNIQUE, "
			"type TEXT)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_PLUGIN_CATALOG % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"catalog INTEGER UNIQUE)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_STAR_CATALOG % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"catalog INTEGER UNIQUE, "
			"count INTEGER, "
			"mag_range TEXT)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_LANDSCAPE % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"addon INTEGER UNIQUE, "
			"thumbnail TEXT)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_LANGUAGE_PACK % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"addon INTEGER UNIQUE)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_SCRIPT % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"addon INTEGER UNIQUE)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_STARLORE % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"addon INTEGER UNIQUE)";

	addonTables << "CREATE TABLE IF NOT EXISTS " % TABLE_TEXTURE % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"addon INTEGER UNIQUE)";

	QSqlQuery query(m_db);
	foreach (QString table, addonTables)
	{
		query.prepare(table);
		if (!query.exec())
		{
			qDebug() << "Add-On Manager : unable to create the addon table."
				 << m_db.lastError();
			return false;
		}
	}
	return true;
}

bool StelAddOnDAO::createTableLicense()
{
	QSqlQuery query(m_db);
	query.prepare(
		"CREATE TABLE IF NOT EXISTS " % TABLE_LICENSE % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"name TEXT, "
			"url TEXT)"
	);
	if (!query.exec())
	{
		qDebug() << "Add-On Manager : unable to create the license table."
			 << m_db.lastError();
		return false;
	}
	return true;
}

bool StelAddOnDAO::createTableAuthor()
{
	QSqlQuery query(m_db);
	query.prepare(
		"CREATE TABLE IF NOT EXISTS " % TABLE_AUTHOR % " ("
			"id INTEGER primary key AUTOINCREMENT, "
			"name TEXT, "
			"email TEXT, "
			"url TEXT)"
	);
	if (!query.exec())
	{
		qDebug() << "Add-On Manager : unable to create the author table."
			 << m_db.lastError();
		return false;
	}
	return true;
}
