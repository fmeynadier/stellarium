/*
 * Stellarium
 * Copyright (C) 2008 Matthew Gates
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

#include "StelScriptMgr.hpp"
#include "StelApp.hpp"
#include "StelFileMgr.hpp"

#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QStringList>
#include <QDateTime>
#include <QRegExp>
#include <QVariant>
#include <QTemporaryFile>

QMap<QString, QString> StelScriptMgr::mappify(const QStringList& args, bool lowerKey)
{
	QMap<QString, QString> map;
	for(int i=0; i+1<args.size(); i++)
		if (lowerKey)
			map[args.at(i).toLower()] = args.at(i+1);
		else
			map[args.at(i)] = args.at(i+1);
	return map;
}

bool StelScriptMgr::strToBool(const QString& str)
{
	if (str.toLower() == "off" || str.toLower() == "no")
		return false;
	else if (str.toLower() == "on" || str.toLower() == "yes")
		return true;
	return QVariant(str).toBool();
}

bool StelScriptMgr::preprocessScript(QFile& input, QFile& output, const QString& scriptDir)
{
	while (!input.atEnd())
	{
		QString line = QString::fromUtf8(input.readLine());
		QRegExp includeRe("^include\\s*\\(\\s*\"([^\"]+)\"\\s*\\)\\s*;\\s*(//.*)?$");
		if (includeRe.exactMatch(line))
		{
			QString fileName = includeRe.capturedTexts().at(1);
			QString path;

			// Search for the include file.  Rules are:
			// 1. If path is absolute, just use that
			// 2. If path is relative, look in scriptDir + included filename
			if (QFileInfo(fileName).isAbsolute())
				path = fileName;
			else
			{
				try
				{
					path = StelApp::getInstance().getFileMgr().findFile(scriptDir + "/" + fileName);
				}
				catch(std::runtime_error& e)
				{
					qWarning() << "WARNING: script include:" << fileName << e.what();
					return false;
				}
			}

			QFile fic(path);
			bool ok = fic.open(QIODevice::ReadOnly);
			if (ok)
			{
				qDebug() << "script inlude: " << path;
				preprocessScript(fic, output, scriptDir);
			}
			else
			{
				qWarning() << "WARNING: could not open script include file for reading:" << path;
				return false;
			}
		}
		else
		{
			output.write(line.toUtf8());
		}
	}
	return true;
}

bool StelScriptMgr::preprocessStratoScript(QFile& input, QFile& output, const QString& scriptDir)
{
	int n=0;
	qDebug() << "Translating stratoscript:";
	while (!input.atEnd())
	{
		QString line = QString::fromUtf8(input.readLine());
		line.replace(QRegExp("#.*$"), "");
		QStringList args = line.split(QRegExp("\\s+"));

		if (args.at(0) == "script")
		{
			if (args.at(1) == "filename")
			{
				QString fileName = args.at(2);
				QString path;

				// Search for the include file.  Rules are:
				// 1. If path is absolute, just use that
				// 2. If path is relative, look in scriptDir + included filename
				if (QFileInfo(fileName).isAbsolute())
					path = fileName;
				else
				{
					try
					{
						path = StelApp::getInstance().getFileMgr().findFile(scriptDir + "/" + fileName);
					}
					catch(std::runtime_error& e)
					{
						qWarning() << "WARNING: script include:" << fileName << e.what();
						return false;
					}
				}

				QFile fic(path);
				bool ok = fic.open(QIODevice::ReadOnly);
				if (ok)
				{
					qDebug() << "script inlude: " << path;
					preprocessScript(fic, output, scriptDir);
				}
				else
				{
					qWarning() << "WARNING: could not open script include file for reading:" << path;
					return false;
				}
			}
			else
				line = "// untranslated stratoscript (script): " + line;
		}
		else if (args.at(0) == "landscape")
		{
			if (args.at(1) == "load")
				line = QString("LandscapeMgr.setCurrentLandscapeID(\"%1\");").arg(args.at(2));
			else
				line = "// untranslated stratoscript (landscape): " + line;
		}
		else if (args.at(0) == "clear")
		{
			QString state = "natural";
			if (args.at(1) == "state")
				state = args.at(2);

			line = QString("core.clear(\"%1\");").arg(state);
		}
		else if (args.at(0) == "date")
		{
			if (args.at(1).toLower() == "utc")
				line = QString("core.setDate(\"%1\"); ").arg(args.at(2));
			else if (args.at(1) == "local")
				line = QString("core.setDate(\"%1\", \"local\"); ").arg(args.at(2));
			else
				line = "// untranslated stratoscript (date): " + line;
		}
		else if (args.at(0) == "flag")
		{
			if (args.at(1) == "atmosphere")
				line = QString("LandscapeMgr.setFlagAtmosphere(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "azimuthal_grid")
				line = QString("GridLinesMgr.setFlagAzimuthalGrid(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "cardinal_points")
				line = QString("LandscapeMgr.setFlagCardinalsPoints(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "constellation_art")
				line = QString("ConstellationMgr.setFlagArt(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "constellation_boundaries")
				line = QString("ConstellationMgr.setFlagBoundaries(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "constellation_drawing" || args.at(1) == "constellations")
				line = QString("ConstellationMgr.setFlagLines(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "constellation_names")
				line = QString("ConstellationMgr.setFlagLabels(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "constellation_pick")
				line = QString("ConstellationMgr.setFlagIsolateSelected(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "ecliptic_line")
				line = QString("GridLinesMgr.setFlagEclipticLine(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "equator_line")
				line = QString("GridLinesMgr.setFlagEquatorLine(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "equator_grid")
				line = QString("GridLinesMgr.setFlagEquatorGrid(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "fog")
				line = QString("LandscapeMgr.setFlagFog(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "gravity_labels") // TODO when in new script API
				line = "// untranslated stratoscropt (flag gravity_labels): " + line;
			else if (args.at(1) == "moon_scaled")
				line = QString("SolarSystem.setFlagMoonScale(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "landscape")
				line = QString("LandscapeMgr.setFlagLandscape(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "landscape_sets_location")
				line = QString("LandscapeMgr.setFlagLandscapeSetsLocation(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "meridian_line")
				line = QString("GridLinesMgr.setFlagMeridianLine(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "milky_way")
				line = QString("MilkyWay.setFlagShow(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "nebulae")
				line = QString("NebulaMgr.setFlagShow(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "nebula_names")
				line = QString("NebulaMgr.setFlagNames(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "night") // TODO when in new script API
				line = "// untranslated stratoscropt (flag night): " + line;
			else if (args.at(1) == "object_trails")
				line = QString("SolarSystem.setFlagTrails(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "planets")
				line = QString("SolarSystem.setFlagTrails(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "planet_names")
				line = QString("SolarSystem.setFlagLabels(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "planet_orbits")
				line = QString("SolarSystem.setFlagOrbits(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "star_names")
				line = QString("StarMgr.setFlagLabels(%1);").arg(strToBool(args.at(2)));
			else if (args.at(1) == "star_twinkle") // TODO when in new script API
				line = "// untranslated stratoscropt (flag star_twinkle): " + line;
			else if (args.at(1) == "stars")
				line = QString("StarMgr.setFlagStars(%1);").arg(strToBool(args.at(2)));
			else
				line = "// untranslated stratoscript (flag): " + line;
		}
		else if (args.at(0) == "deselect")
			line = "core.selectObjectByName(\"\", false);";
		else if (args.at(0) == "select")
		{
			args.removeFirst();
			QMap<QString, QString> map = mappify(args, true);
			bool pointer = false;
			QString objectName;
			if (map.contains("pointer"))
				pointer = QVariant(map["pointer"]).toBool();
			if (map.contains("planet"))
				objectName = map["planet"];
			else if (map.contains("hp"))
				objectName = "HP" + map["hp"];
			else if (map.contains("constellation"))
				objectName = map["constellation"];
			else if (map.contains("constellation_star"))
				objectName = map["constellation_star"];
			else if (map.contains("nebula"))
				objectName = map["nebula"];

			line = QString("core.selectObjectByName(\"%1\", %2);").arg(objectName).arg(pointer);
		}
		else if (args.at(0) == "wait")
		{
			if (args.at(1) == "duration")
				line = QString("core.wait(\"%1\");").arg(args.at(2));
			else
				line = "// untranslated stratoscript (wait): " + line;
		}
		else if (args.at(0) == "zoom")
		{
			bool ok;
			args.removeFirst();
			QMap<QString, QString> map = mappify(args);
			double duration = 1.0;
			if (map.contains("duration"))
				duration = QVariant(map["duration"]).toDouble(&ok);
			if (!ok)
				duration = 1.0;

			if (map.contains("auto"))
			{
				if (map["auto"].toLower() == "in")
					line = QString("StelMovementMgr.autoZoomIn(%1);").arg(duration);
				else if (map["auto"].toLower() == "out")
					line = QString("StelMovementMgr.autoZoomOut(%1);").arg(duration);
				else if (map["auto"].toLower() == "initial")
					line = QString("StelMovementMgr.zoomTo(StelMovementMgr.getInitFov(), %1);").arg(duration);
			}
			else if (map.contains("fov"))
			{
				double fov;
				fov = QVariant(map["fov"]).toDouble(&ok);
				if (ok)
					line = QString("StelMovementMgr.zoomTo(%1, %2);").arg(fov).arg(duration);
				else
					line = "// untranslated stratoscript (zoom fov): " + line;
			}
			else if (map.contains("delta_fov"))
			{
				double dfov;
				dfov = QVariant(map["delta_fov"]).toDouble(&ok);
				if (ok)
					line = QString("StelMovementMgr.zoomTo(%1, %2);").arg(dfov).arg(duration);
				else
					line = "// untranslated stratoscript (zoom delta_fov): " + line;
			}
			else
				line = "// untranslated stratoscript (zoom): " + line;
		}
		else if (args.at(0) == "timerate")
		{
			bool ok;
			if (args.at(1) == "rate")
			{
				double rate = QVariant(args.at(2)).toDouble(&ok);
				if (ok)
					line = QString("core.setTimeRate(%1);").arg(rate);
				else
					line = "// untranslated stratoscript (timerate rate): " + line;
			}
			else
				line = "// untranslated stratoscript (timerate): " + line;
		}
		else
		{
			line = "// untranslated stratoscript: " + line;
		}
		qDebug() << QString("%1: ").arg(++n, 4) + line;
		line += "\n";
		output.write(line.toUtf8());
	}
	return true;
}


