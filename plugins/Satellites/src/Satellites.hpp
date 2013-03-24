/*
 * Copyright (C) 2009, 2012 Matthew Gates
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

#ifndef _SATELLITES_HPP_
#define _SATELLITES_HPP_ 1

#include "StelObjectModule.hpp"
#include "Satellite.hpp"
#include "StelFader.hpp"
#include "StelGui.hpp"
#include "StelDialog.hpp"
#include "StelLocation.hpp"

#include <QDateTime>
#include <QFile>
#include <QSharedPointer>
#include <QVariantMap>

class StelButton;
class Planet;
class QNetworkAccessManager;
class QNetworkReply;
class QPixmap;
class QProgressBar;
class QSettings;
class QTimer;
class SatellitesDialog;

typedef QSharedPointer<Satellite> SatelliteP;

//! Data structure containing unvalidated TLE set as read from a TLE list file.
struct TleData
{
	//! NORAD catalog number, as extracted from the TLE set.
	QString id;
	//! Human readable name, as extracted from the TLE title line.
	QString name;
	QString first;
	QString second;
};

typedef QList<TleData> TleDataList;
typedef QHash<QString, TleData> TleDataHash ;


/*! @mainpage notitle
@section overview Plugin Overview

The %Satellites plugin displays the positions of artifical satellites in Earth
orbit based on a catalog of orbital data.

The Satellites class is the main class of the plug-in. It manages a collection
of Satellite objects and takes care of loading, saving and updating the
satellite catalog. It allows automatic updates from online sources and manages
a list of update file URLs.

To calculate satellite positions, the plugin uses an implementation of
the SGP4/SDP4 algorithms (J.L. Canales' gsat library).

@section satprop Satellite Properties

@subsection ident Name and identifiers
Each satellite has a name. It's displayed as a label of the satellite hint and in the list of satellites. Names are not unique though, so they are used only
for presentation purposes.

In the @ref satcat satellites are uniquely identified by their NORAD number, which is encoded in TLEs.

@subsection groups Grouping
A satellite can belong to one or more groups such as "amateur", "geostationary" or "navigation". They have no other function but to help the user organize the satellite collection.

Group names are arbitrary strings defined in the @ref satcat for each satellite and are more similar to the concept of "tags" than a hierarchical grouping. A satellite may not belong to any group at all.

By convention, group names are in lowercase. The GUI translates some of the groups used in the default catalog.

@section satcat Satellite Catalog
The satellite catalog is stored on the disk in [JSON](http://www.json.org/)
format, in a file named "satellites.json". A default copy is embedded in the
plug-in at compile time. A working copy is kept in the user data directory.

@section config Configuration
The plug-ins' configuration data is stored in Stellarium's main configuration
file.
*/


//! @class Satellites
//! Main class of the %Satellites plugin.
class Satellites : public StelObjectModule
{
	Q_OBJECT
public:
	//! @enum UpdateState
	//! Used for keeping track of the download/update status
	enum UpdateState
	{
		Updating,             //!< Update in progress
		CompleteNoUpdates,    //!< Update completed, there we no updates
		CompleteUpdates,      //!< Update completed, there were updates
		DownloadError,        //!< Error during download phase
		OtherError            //!< Other error
	};

	//! Flags used to filter the satellites list according to their status.
	enum Status
	{
		Visible,
		NotVisible,
		Both,
		NewlyAdded,
		OrbitError
	};

	Satellites();
	virtual ~Satellites();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void deinit();
	virtual void update(double deltaTime);
	virtual void draw(StelCore* core, class StelRenderer* renderer);
	virtual void drawPointer(StelCore* core, class StelRenderer* renderer);
	virtual double getCallOrder(StelModuleActionName actionName) const;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectManager class
	//! Used to get a list of objects which are near to some position.
	//! @param v a vector representing the position in th sky around which to search for nebulae.
	//! @param limitFov the field of view around the position v in which to search for satellites.
	//! @param core the StelCore to use for computations.
	//! @return an list containing the satellites located inside the limitFov circle around position v.
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;

	//! Return the matching satellite object's pointer if exists or NULL.
	//! @param nameI18n The case in-sensistive satellite name
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;

	//! Return the matching satellite if exists or NULL.
	//! @param name The case in-sensistive standard program name
	virtual StelObjectP searchByName(const QString& name) const;
	
	//! Return the satellite with the given catalog number.
	//! Used as a helper function by searchByName() and
	//! searchByNameI18n().
	//! @param noradNumber search string in the format "NORAD XXXX".
	//! @returns a null pointer if no such satellite is found.
	StelObjectP searchByNoradNumber(const QString& noradNumber) const;

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @return a list of matching object name by order of relevance, or an empty list if nothing match
	virtual QStringList listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem=5) const;

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object English name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @return a list of matching object name by order of relevance, or an empty list if nothing match
	virtual QStringList listMatchingObjects(const QString& objPrefix, int maxNbItem=5) const;

	virtual QStringList listAllObjects(bool inEnglish) const;

	virtual QString getName() const { return "Satellites"; }

	//! Implment this to tell the main Stellarium GUi that there is a GUI element to configure this
	//! plugin. 
	virtual bool configureGui(bool show=true);

	//! Set up the plugin with default values.  This means clearing out the Satellites section in the
	//! main config.ini (if one already exists), and populating it with default values.  It also 
	//! creates the default satellites.json file from the resource embedded in the plugin lib/dll file.
	void restoreDefaults(void);

	//! Read (or re-read) the plugin's settings from the configuration file.
	//! This will be called from init() and also when restoring defaults
	//! (i.e. from the configuration dialog / restore defaults button).
	void loadSettings();

	//! Save the plugin's settings to the main configuration file.
	void saveSettings();

	//! Get the groups used in the currently loaded satellite collection.
	//! See @ref groups for details. Use getGroupIdList() if you need a list.
	QSet<QString> getGroups() const;
	//! Get a sorted list of group names.
	//! See @ref groups for details. Use getGroups() if you don't need a list.
	QStringList getGroupIdList() const;

	//! get satellite objects filtered by group.  If an empty string is used for the
	//! group name, return all satallites
	QHash<QString,QString> getSatellites(const QString& group=QString(), Status vis=Both);

	//! Get a satellite object by its identifier (i.e. NORAD number).
	SatelliteP getById(const QString& id);
	
	//! Returns a list of all satellite IDs.
	QStringList listAllIds();
	
	//! Add the given satellites.
	//! The changes are not saved to file.
	void add(const TleDataList& newSatellites);
	
	//! Remove the selected satellites.
	//! The changes are not saved to file.
	void remove(const QStringList& idList);

	//! get whether or not the plugin will try to update TLE data from the internet
	//! @return true if updates are set to be done, false otherwise
	bool getUpdatesEnabled(void) {return updatesEnabled;}
	//! set whether or not the plugin will try to update TLE data from the internet
	//! @param b if true, updates will be enabled, else they will be disabled
	void setUpdatesEnabled(bool b) {updatesEnabled=b;}

	//! get the date and time the TLE elements were updated
	QDateTime getLastUpdate(void) {return lastUpdate;}

	//! get the update frequency in hours
	int getUpdateFrequencyHours(void) {return updateFrequencyHours;}
	void setUpdateFrequencyHours(int hours) {updateFrequencyHours = hours;}

	//! get the number of seconds till the next update
	int getSecondsToUpdate(void);

	//! get the update frequency in hours
	//void setUpdateFrequencyHours(int hours);

	//! Get the current updateState
	UpdateState getUpdateState(void) {return updateState;}

	//! Get a list of URLs which are sources of TLE data.
	QStringList getTleSources(void) {return updateUrls;}

	//! Set the list of URLs which are sources of TLE data.
	void setTleSources(QStringList tleSources);
	
	//! Returns the module-specific style sheet.
	//! The main StelStyle instance should be passed.
	// TODO: Plugin-specific styles are no longer necessary?
	const StelStyle getModuleStyleSheet(const StelStyle& style);

	//! Reads update file(s) in celestrak's .txt format, and updates
	//! the TLE elements for exisiting satellites from them.
	//! emits signals updateStateChanged and tleUpdateComplete
	//! @param paths a list of paths to update files
	//! @param deleteFiles if set, the update files are deleted after
	//!        they are used, else they are left alone
	void updateFromFiles(QStringList paths, bool deleteFiles=false);
	
	//! Reads a TLE list from a file to the supplied hash.
	//! If an entry with the same ID exists in the given hash, its contents
	//! are overwritten with the new values.
	//! \param openFile a reference to an \b open file.
	//! \param tleList a hash with satellite IDs (catalog numbers) as keys.
	static void parseTleFile(QFile& openFile, TleDataHash& tleList);

signals:
	//! emitted when the update status changes, e.g. when 
	//! an update starts, completes and so on.  Note that
	//! on completion of an update, tleUpdateComplete is also
	//! emitted with the number of updates done.
	//! @param state the new update state.
	void updateStateChanged(Satellites::UpdateState state);

	//! Emitted after an update has run.
	//! @param updates the number of satellites updated.
	//! @param total the total number of satellites in the JSON data.
	//! @param missing the number of satellites in the JSON data but not found
	//! in update data.
	void tleUpdateComplete(int updates, int total, int missing);
	

public slots:
	void setFlagHints(bool b) {hintFader=b;}
	bool getFlagHints(void) {return hintFader;}

	//! get the label font size
	//! @return the pixel size of the font
	int getLabelFontSize(void) {return labelFont.pixelSize();}
	//! set the label font size
	//! @param size the pixel size of the font
	void setLabelFontSize(int size) {labelFont.setPixelSize(size);}

	bool getFlagLabels(void);
	void setFlagLabels(bool b);

	//! Download TLEs from web recources described in the module section of the
	//! module.ini file and update the TLE values for any satellites for which
	//! there is new TLE data.
	void updateTLEs(void);

	//! Choose whether or not to draw orbit lines.  Each satellite has its own setting
	//! as well, but this can be used to turn on/off all those satellites which elect to
	//! have orbit lines all in one go.
	//! @param b - true to turn on orbit lines, false to turn off
	void setOrbitLinesFlag(bool b);

	//! Get the current status of the orbit line rendering flag
	bool getOrbitLinesFlag(void);

	void recalculateOrbitLines(void);

	//! Display a message on the screen for a few seconds.
	//! This is used for plugin-specific warnings and such.
	void displayMessage(const QString& message, const QString hexColor="#999999");
	//! Hide all messages.
	void hideMessages();

	//! Save the current satellite catalog to disk.
	void saveCatalog(QString path=QString());

private slots:
	void setStelStyle(const QString& section);

private:
	//! Delete Satellites section in main config.ini, then create with default values.
	void restoreDefaultSettings();

	//! Replace the catalog file with the default one.
	void restoreDefaultCatalog();

	//! Load the satellites from the catalog file.
	//! Removes existing satellites first if there are any.
	//! this will be done once at init, and also if the defaults are reset.
	void loadCatalog();

	//! Creates a backup of the satellites.json file called satellites.json.old
	//! @param deleteOriginal if true, the original file is removed, else not
	//! @return true on OK, false on failure
	bool backupCatalog(bool deleteOriginal=false);

	//! Get the version from the "creator" value in the satellites.json file.
	//! @return version string, e.g. "0.6.1"
	const QString getCatalogVersion();

	//! Save a structure representing a satellite catalog to a JSON file.
	//! If no path is specified, catalogPath is used.
	//! @see createDataMap()
	bool saveDataMap(const QVariantMap& map, QString path=QString());
	//! Load a structure representing a satellite catalog from a JSON file.
	//! If no path is specified, catalogPath is used.
	QVariantMap loadDataMap(QString path=QString());
	//! Parse a satellite catalog structure into internal satellite data.
	void setDataMap(const QVariantMap& map);
	//! Make a satellite catalog structure from current satellite data.
	//! @return a representation of a JSON file.
	QVariantMap createDataMap();

	//! Path to the satellite catalog file.
	QString catalogPath;
	QList<SatelliteP> satellites;
	
	QSet<QString> groups;
	
	LinearFader hintFader;
	class StelTextureNew* hintTexture;
	class StelTextureNew* texPointer;
	
	//! @name Bottom toolbar button
	//@{
	QPixmap* pxmapGlow;
	QPixmap* pxmapOnIcon;
	QPixmap* pxmapOffIcon;
	StelButton* toolbarButton;	
	//@}
	// FIXME: Possible bug with the Solar System recreated by the SSEditor.
	QSharedPointer<Planet> earth;
	Vec3f defaultHintColor;
	Vec3f defaultOrbitColor;
	QFont labelFont;
	
	//! @name Updater module
	//@{
	UpdateState updateState;
	QNetworkAccessManager* downloadMgr;
	QStringList updateUrls;
	QStringList updateFiles;
	QProgressBar* progressBar;
	int currentUpdateUrlIdx;
	int numberDownloadsComplete;
	QTimer* updateTimer;
	QTimer* messageTimer;
	QList<int> messageIDs;
	bool updatesEnabled;
	QDateTime lastUpdate;
	int updateFrequencyHours;
	//@}

	// GUI
	SatellitesDialog* configDialog;
	QByteArray normalStyleSheet;
	QByteArray nightStyleSheet;

private slots:
	//! check to see if an update is required.  This is called periodically by a timer
	//! if the last update was longer than updateFrequencyHours ago then the update is
	//! done.
	void checkForUpdate(void);
	void updateDownloadComplete(QNetworkReply* reply);
	void observerLocationChanged(StelLocation loc);

};


#include "fixx11h.h"
#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class SatellitesStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*_SATELLITES_HPP_*/

