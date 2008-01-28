/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
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

#ifndef _LANDSCAPE_H_
#define _LANDSCAPE_H_

#include <QMap>
#include "vecmath.h"
#include "ToneReproducer.hpp"
#include "Projector.hpp"
#include "Navigator.hpp"
#include "Fader.hpp"
#include "StelUtils.hpp"
#include "STextureTypes.hpp"

class QSettings;

// Class which manages the displaying of the Landscape
class Landscape
{
public:
	enum LANDSCAPE_TYPE
	{
		OLD_STYLE,
		FISHEYE,
		SPHERICAL
	};

	Landscape(float _radius = 2.);
	virtual ~Landscape();
	virtual void load(const QString& file_name, const QString& landscapeId) = 0;
	
	//! Set the brightness of the landscape
	void setBrightness(float b) {sky_brightness = b;}
	
	//! Set whether landscape is displayed (does not concern fog)
	void setFlagShow(bool b) {land_fader=b;}
	//! Get whether landscape is displayed (does not concern fog)
	bool getFlagShow() const {return (bool)land_fader;}
	//! Set whether fog is displayed
	void setFlagShowFog(bool b) {fog_fader=b;}
	//! Get whether fog is displayed
	bool getFlagShowFog() const {return (bool)fog_fader;}
	//! Get landscape name
	QString getName() const {return name;}
	//! Get landscape author name
	QString getAuthorName() const {return author;}
	//! Get landscape description
	QString getDescription() const {return description;}
	
	void update(double deltaTime) {land_fader.update((int)(deltaTime*1000)); fog_fader.update((int)(deltaTime*1000));}
	virtual void draw(ToneReproducer * eye, const Projector* prj, const Navigator* nav) = 0;

	//! Return the (English) planet name for the landscape
	QString getPlanet(void) { return planet; }	
	//! Return the latitude for the landscape
	double getLatitude(void) { return latitude; }
	//! Return the longitude for the landscape
	double getLongitude(void) { return longitude; }
	//! Return the altitude for the landscape
	double getAltitude(void) { return altitude; }

protected:
	//! Load attributes common to all landscapes
	//! @param A reference to an existant QSettings object which describes the landscape
	//! @param The name of the directory for the landscape files (e.g. "ocean")
	void loadCommon(const QSettings& landscapeIni, const QString& landscapeId);
	
	//! search for a texture in landscape directory, else global textures directory
	//! @param basename The name of a texture file, e.g. "fog.png"
	//! @param landscapeId The landscape ID (directory name) to which the texture belongs
	//! @exception misc possibility of throwing boost:filesystem or "file not found" exceptions
	const QString getTexturePath(const QString& basename, const QString& landscapeId);
	float radius;
	QString name;
	float sky_brightness;
	bool valid_landscape;   // was a landscape loaded properly?
	LinearFader land_fader;
	LinearFader fog_fader;
	QString author;
	QString description;
	QString planet;
	double latitude;
	double longitude;
	double altitude;
	
typedef struct
{
	STextureSP tex;
	float tex_coords[4];
} landscape_tex_coord;
};


class LandscapeOldStyle : public Landscape
{
public:
	LandscapeOldStyle(float _radius = 2.);
    virtual ~LandscapeOldStyle();
	virtual void load(const QString& fileName, const QString& landscapeId);
	virtual void draw(ToneReproducer * eye, const Projector* prj, const Navigator* nav);
	void create(bool _fullpath, QMap<QString, QString> param);
private:
	void draw_fog(ToneReproducer * eye, const Projector* prj, const Navigator* nav) const;
	void draw_decor(ToneReproducer * eye, const Projector* prj, const Navigator* nav) const;
	void draw_ground(ToneReproducer * eye, const Projector* prj, const Navigator* nav) const;
	STextureSP* side_texs;
	int nb_side_texs;
	int nb_side;
	landscape_tex_coord* sides;
	STextureSP fog_tex;
	landscape_tex_coord fog_tex_coord;
	STextureSP ground_tex;
	landscape_tex_coord ground_tex_coord;
	int nb_decor_repeat;
	float fog_alt_angle;
	float fog_angle_shift;
	float decor_alt_angle;
	float decor_angle_shift;
	float decor_angle_rotatez;
	float ground_angle_shift;
	float ground_angle_rotatez;
	int draw_ground_first;
	bool tanMode;	// Whether the angles should be converted using tan instead of sin 
};

class LandscapeFisheye : public Landscape
{
public:
	LandscapeFisheye(float _radius = 1.);
	virtual ~LandscapeFisheye();
	virtual void load(const QString& fileName, const QString& landscapeId);
	virtual void draw(ToneReproducer * eye, const Projector* prj, const Navigator* nav);
	void create(const QString _name, bool _fullpath, const QString& _maptex,
	            double _texturefov, double angle_rotatez);
private:

	STextureSP map_tex;
	float tex_fov;
	float angle_rotatez;
};


class LandscapeSpherical : public Landscape
{
public:
	LandscapeSpherical(float _radius = 1.);
	virtual ~LandscapeSpherical();
	virtual void load(const QString& fileName, const QString& landscapeId);
	virtual void draw(ToneReproducer * eye, const Projector* prj, const Navigator* nav);
	void create(const QString _name, bool _fullpath,
	            const QString& _maptex, double angle_rotatez);
private:

	STextureSP map_tex;
	float angle_rotatez;
};

#endif // _LANDSCAPE_H_
