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

#include "StelApp.hpp"
#include "Navigator.hpp"
#include "StelUtils.hpp"
#include "SolarSystem.hpp"
#include "Observer.hpp"
#include "Planet.hpp"
#include "StelObjectMgr.hpp"
#include "StelCore.hpp"

#include <QSettings>
#include <QStringList>
#include <QDateTime>
#include <QDebug>

////////////////////////////////////////////////////////////////////////////////
Navigator::Navigator(Observer* obs) : time_speed(JD_SECOND), JDay(0.), position(obs)
{
	if (!position)
	{
		qCritical() << "ERROR : Can't create a Navigator without a valid Observator";
		exit(1);
	}
	local_vision=Vec3d(1.,0.,0.);
	equ_vision=Vec3d(1.,0.,0.);
	J2000_equ_vision=Vec3d(1.,0.,0.);  // not correct yet...
	viewing_mode = VIEW_HORIZON;  // default
}

Navigator::~Navigator()
{}

const Planet *Navigator::getHomePlanet(void) const
{
	return position->getHomePlanet();
}

void Navigator::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);

	setTimeNow();
	setLocalVision(Vec3f(1,1e-05,0.2));
	// Compute transform matrices between coordinates systems
	updateTransformMatrices();
	updateModelViewMat();
	QString tmpstr = conf->value("navigation/viewing_mode", "horizon").toString();
	if (tmpstr=="equator")
		setViewingMode(Navigator::VIEW_EQUATOR);
	else
	{
		if (tmpstr=="horizon")
			setViewingMode(Navigator::VIEW_HORIZON);
		else
		{
			qDebug() << "ERROR : Unknown viewing mode type : " << tmpstr;
			assert(0);
		}
	}
	
	initViewPos = StelUtils::str_to_vec3f(conf->value("navigation/init_view_pos").toString());
	setLocalVision(initViewPos);
	
	// Navigation section
	PresetSkyTime 		= conf->value("navigation/preset_sky_time",2451545.).toDouble();
	StartupTimeMode 	= conf->value("navigation/startup_time_mode", "actual").toString().toLower();
	if (StartupTimeMode=="preset")
		setJDay(PresetSkyTime - StelUtils::get_GMT_shift_from_QT(PresetSkyTime) * JD_HOUR);
	else if (StartupTimeMode=="today")
		setTodayTime(getInitTodayTime());
	else 
		setTimeNow();
}

//! Set stellarium time to current real world time
void Navigator::setTimeNow()
{
	setJDay(StelUtils::getJDFromSystem());
}

void Navigator::setTodayTime(const QTime& target)
{
	QDateTime dt = QDateTime::currentDateTime();
	if (target.isValid())
	{
		dt.setTime(target);
		// don't forget to adjust for timezone / daylight savings.
		setJDay(StelUtils::qDateTimeToJd(dt)-(StelUtils::get_GMT_shift_from_QT(StelUtils::getJDFromSystem()) * JD_HOUR));
	}
	else
	{
		qWarning() << "WARNING - time passed to Navigator::setTodayTime is not valid. The system time will be used." << target;
		setTimeNow();
	}
}

//! Get whether the current stellarium time is the real world time
bool Navigator::getIsTimeNow(void) const
{
	// cache last time to prevent to much slow system call
	static double lastJD = getJDay();
	static bool previousResult = (fabs(getJDay()-StelUtils::getJDFromSystem())<JD_SECOND);
	if (fabs(lastJD-getJDay())>JD_SECOND/4)
	{
		lastJD = getJDay();
		previousResult = (fabs(getJDay()-StelUtils::getJDFromSystem())<JD_SECOND);
	}
	return previousResult;
}

QTime Navigator::getInitTodayTime(void)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);
	return QTime::fromString(conf->value("navigation/today_time", "22:00").toString(), "hh:mm");
}

void Navigator::setInitTodayTime(const QTime& t)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);
	conf->setValue("navigation/today_time", t.toString("hh:mm"));
} 

QDateTime Navigator::getInitDateTime(void)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);
	return StelUtils::jdToQDateTime(conf->value("navigation/preset_sky_time",2451545.).toDouble() - StelUtils::get_GMT_shift_from_QT(PresetSkyTime) * JD_HOUR);
}

void Navigator::setInitDateTime(const QDateTime& dt)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);
	conf->setValue("navigation/preset_sky_time", StelUtils::qDateTimeToJd(dt));
}

void Navigator::addSolarDays(double d)
{
	setJDay(getJDay() + d);
}

void Navigator::addSiderealDays(double d)
{
	const Planet* home = position->getHomePlanet();
	if (home->getEnglishName() != "Solar System Observer")
	d *= home->getSiderealDay();
	setJDay(getJDay() + d);
}

void Navigator::moveObserverToSelected(void)
{
	if (StelApp::getInstance().getStelObjectMgr().getWasSelected())
	{
		Planet* pl = dynamic_cast<Planet*>(StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0].get());
		if (pl) 
			StelApp::getInstance().getCore()->getObservatory()->setHomePlanet(pl);
	}
}

void Navigator::setInitViewDirectionToCurrent(void)
{
	QString dirStr = QString("%1,%2,%3").arg(local_vision[0]).arg(local_vision[1]).arg(local_vision[2]);
	StelApp::getInstance().getSettings()->setValue("navigation/init_view_pos", dirStr);
}

//! Increase the time speed
void Navigator::increaseTimeSpeed()
{
	double s = getTimeSpeed();
	if (s>=JD_SECOND) s*=10.;
	else if (s<-JD_SECOND) s/=10.;
	else if (s>=0. && s<JD_SECOND) s=JD_SECOND;
	else if (s>=-JD_SECOND && s<0.) s=0.;
	setTimeSpeed(s);
}

//! Decrease the time speed
void Navigator::decreaseTimeSpeed()
{
	double s = getTimeSpeed();
	if (s>JD_SECOND) s/=10.;
	else if (s<=-JD_SECOND) s*=10.;
	else if (s>-JD_SECOND && s<=0.) s=-JD_SECOND;
	else if (s>0. && s<=JD_SECOND) s=0.;
	setTimeSpeed(s);
}
	
////////////////////////////////////////////////////////////////////////////////
void Navigator::setLocalVision(const Vec3d& _pos)
{
	local_vision = _pos;
	equ_vision=local_to_earth_equ(local_vision);
	J2000_equ_vision = mat_earth_equ_to_j2000*equ_vision;
}

////////////////////////////////////////////////////////////////////////////////
void Navigator::setEquVision(const Vec3d& _pos)
{
	equ_vision = _pos;
	J2000_equ_vision = mat_earth_equ_to_j2000*equ_vision;
	local_vision = earth_equ_to_local(equ_vision);
}

////////////////////////////////////////////////////////////////////////////////
void Navigator::setJ2000EquVision(const Vec3d& _pos)
{
	J2000_equ_vision = _pos;
	equ_vision = mat_j2000_to_earth_equ*J2000_equ_vision;
	local_vision = earth_equ_to_local(equ_vision);
}

////////////////////////////////////////////////////////////////////////////////
// Increment time
void Navigator::updateTime(double deltaTime)
{
	JDay+=time_speed*deltaTime;

	// Fix time limits to -100000 to +100000 to prevent bugs
	if (JDay>38245309.499988) JDay = 38245309.499988;
	if (JDay<-34803211.500012) JDay = -34803211.500012;
}

////////////////////////////////////////////////////////////////////////////////
// The non optimized (more clear version is available on the CVS : before date 25/07/2003)
// see vsop87.doc:

const Mat4d mat_j2000_to_vsop87(Mat4d::xrotation(-23.4392803055555555556*(M_PI/180)) * Mat4d::zrotation(0.0000275*(M_PI/180)));
const Mat4d mat_vsop87_to_j2000(mat_j2000_to_vsop87.transpose());

void Navigator::updateTransformMatrices(void)
{
	mat_local_to_earth_equ = position->getRotLocalToEquatorial(JDay);
	mat_earth_equ_to_local = mat_local_to_earth_equ.transpose();

	mat_earth_equ_to_j2000 = mat_vsop87_to_j2000 * position->getRotEquatorialToVsop87();
	mat_j2000_to_earth_equ = mat_earth_equ_to_j2000.transpose();
	mat_j2000_to_local = mat_earth_equ_to_local*mat_j2000_to_earth_equ;
	
	mat_helio_to_earth_equ = mat_j2000_to_earth_equ * mat_vsop87_to_j2000 * Mat4d::translation(-position->getCenterVsop87Pos());

	// These two next have to take into account the position of the observer on the earth
	Mat4d tmp = mat_j2000_to_vsop87 * mat_earth_equ_to_j2000 * mat_local_to_earth_equ;

	mat_local_to_helio =  Mat4d::translation(position->getCenterVsop87Pos()) * tmp *
	                      Mat4d::translation(Vec3d(0.,0., position->getDistanceFromCenter()));

	mat_helio_to_local =  Mat4d::translation(Vec3d(0.,0.,-position->getDistanceFromCenter())) * tmp.transpose() *
	                      Mat4d::translation(-position->getCenterVsop87Pos());
}

void Navigator::setStartupTimeMode(const QString& s)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);
	StartupTimeMode = s;
	conf->setValue("navigation/startup_time_mode", StartupTimeMode);
}

////////////////////////////////////////////////////////////////////////////////
// Update the modelview matrices
void Navigator::updateModelViewMat(void)
{
	Vec3d f;

	if( viewing_mode == VIEW_EQUATOR)
	{
		// view will use equatorial coordinates, so that north is always up
		f = equ_vision;
	}
	else
	{
		// view will correct for horizon (always down)
		f = local_vision;
	}

	f.normalize();
	Vec3d s(f[1],-f[0],0.);

	if( viewing_mode == VIEW_EQUATOR)
	{
		// convert everything back to local coord
		f = local_vision;
		f.normalize();
		s = earth_equ_to_local( s );
	}

	Vec3d u(s^f);
	s.normalize();
	u.normalize();

	mat_local_to_eye.set(s[0],u[0],-f[0],0.,
	                     s[1],u[1],-f[1],0.,
	                     s[2],u[2],-f[2],0.,
	                     0.,0.,0.,1.);

	mat_earth_equ_to_eye = mat_local_to_eye*mat_earth_equ_to_local;
	mat_helio_to_eye = mat_local_to_eye*mat_helio_to_local;
	mat_j2000_to_eye = mat_earth_equ_to_eye*mat_j2000_to_earth_equ;
}


////////////////////////////////////////////////////////////////////////////////
// Return the observer heliocentric position
Vec3d Navigator::getObserverHelioPos(void) const
{
	static const Vec3d v(0.,0.,0.);
	return mat_local_to_helio*v;
}


////////////////////////////////////////////////////////////////////////////////
// Set type of viewing mode (align with horizon or equatorial coordinates)
void Navigator::setViewingMode(ViewingModeType view_mode)
{
	viewing_mode = view_mode;

	// TODO: include some nice smoothing function trigger here to rotate between
	// the two modes
}
