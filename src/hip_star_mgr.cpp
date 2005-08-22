/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chï¿½eau
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

// class used to manage groups of Stars

#include <string>
#include "hip_star_mgr.h"
#include "s_texture.h"
#include "grid.h"
#include "bytes.h"
#include "stellarium.h"
#include "navigator.h"

#define RADIUS_STAR 1.

// construct and load all data
Hip_Star_mgr::Hip_Star_mgr() :
	starZones(NULL),
	StarArray(NULL),
	StarArraySize(0),
	starTexture(NULL), 
	starFont(NULL),
	limiting_mag(6.5f)
{
	starZones = new vector<Hip_Star*>[HipGrid.getNbPoints()];
}


Hip_Star_mgr::~Hip_Star_mgr()
{
	delete [] starZones;
	starZones=NULL;
	delete [] StarArray;
	StarArray=NULL;
	delete [] StarFlatArray;
	StarFlatArray=NULL;
	delete starTexture;
	starTexture=NULL;
	delete starFont;
	starFont=NULL;
}

void Hip_Star_mgr::init(const string& font_fileName, const string& hipCatFile,
	const string& commonNameFile, const string& sciNameFile, LoadingBar& lb)
{
	load_data(hipCatFile, lb);
    load_common_names(commonNameFile);
	load_sci_names(sciNameFile);
	
	starTexture = new s_texture("star16x16",TEX_LOAD_TYPE_PNG_SOLID);  // Load star texture
    starFont = new s_font(11.f,"spacefont", font_fileName); // load Font
    if (!starFont)
    {
	    printf("Can't create starFont\n");
        exit(-1);
    }	
}

// Load from file ( create the stream and call the Read function )
void Hip_Star_mgr::load_data(const string& hipCatFile, LoadingBar& lb)
{
	char tmpstr[512];
	
    printf(_("Loading Hipparcos star data "));
    FILE * hipFile;
    hipFile = NULL;

    hipFile=fopen(hipCatFile.c_str(),"rb");
    if (!hipFile)
    {
        printf("ERROR %s NOT FOUND\n",hipCatFile.c_str());
        exit(-1);
    }

	// Read number of stars in the Hipparcos catalog
    unsigned int catalogSize=0;
    fread((char*)&catalogSize,4,1,hipFile);
    LE_TO_CPU_INT32(catalogSize,catalogSize);

    StarArraySize = catalogSize;//120417;

    // Create the sequential array
    StarArray = new Hip_Star[StarArraySize];
	
	StarFlatArray = new Hip_Star*[StarArraySize];
	for (int i=0;i<StarArraySize;++i)
	{
		StarFlatArray[i] = NULL;
	}
	
	// Read binary file Hipparcos catalog  
	unsigned int data_drop =0;
    Hip_Star * e = NULL;
    for(int i=0;i<StarArraySize;i++)
    {
        // Tony - added "|| (i == StarArraySize-1)"
		if (!(i%2000) || (i == StarArraySize-1))
		{
			// Draw loading bar
            // Tony - added "+1"
			snprintf(tmpstr, 512, _("Loading Hipparcos catalog: %d/%d"), i+1, StarArraySize);
			lb.SetMessage(tmpstr);
			lb.Draw((float)i/StarArraySize);
		}
		
		e = &(StarArray[i]);
		e->HP=(unsigned int)i;
        if (!e->read(hipFile))
        {
			data_drop++;
        	continue;
        }
        starZones[HipGrid.GetNearest(e->XYZ)].push_back(e);
		StarFlatArray[e->HP]=e;
    }
    fclose(hipFile);

	printf("(%d stars loaded).\n", StarArraySize-data_drop);

    // sort stars by magnitude for faster rendering
    for(int i=0;i < HipGrid.getNbPoints();i++) {
      std::sort( starZones[i].begin(), starZones[i].end(), std::not2(Hip_Star_Mag_Comparer()));
    }
}


// Load common names from file 
int Hip_Star_mgr::load_common_names(const string& commonNameFile)
{
	// clear existing names (would be faster if they were in separate array
	// since relatively few are named)
    for (int i=0; i<StarArraySize; ++i)
      {
	StarArray[i].CommonName = "";
      }
	
    FILE *cnFile;
    cnFile=fopen(commonNameFile.c_str(),"r");
    if (!cnFile)
    {   
        // printf("WARNING %s NOT FOUND\n",commonNameFile.c_str());
        return 0;
    }

    // Tony - Because this is called twice???
    if (!lstCommonNames.empty())
    {
       lstCommonNames.clear();
       lstCommonNamesHP.clear();
    }
	// Assign names to the matching stars, now support spaces in names
    unsigned int tmp;
    char line[256];
    Hip_Star *star;
	fgets(line, 256, cnFile);
	do
	{
		sscanf(line,"%u",&tmp);
		star = search(tmp);
		if (star)
		{
			char c=line[0];
			int i=0;
			while(c!='|' && i<256){c=line[i];++i;}
			star->CommonName =  &(line[i]);
			// remove newline
			star->CommonName.erase(star->CommonName.length()-1, 1);
			lstCommonNames.push_back(star->CommonName);
			lstCommonNamesHP.push_back(tmp);
		}
	} while(fgets(line, 256, cnFile));

    fclose(cnFile);
    return 1;
}

unsigned int Hip_Star_mgr::getCommonNameHP(string _commonname)
{
    unsigned int i = 0;

	while ( i < lstCommonNames.size())
    {
        if (fcompare(_commonname,lstCommonNames[i]) == 0)
           return lstCommonNamesHP[i];
        i++;
    }
    return 0;
}   


// Load scientific names from file 
void Hip_Star_mgr::load_sci_names(const string& sciNameFile)
{
	// clear existing names (would be faster if they were in separate arrays
	// since relatively few are named)
    for (int i=0; i<StarArraySize; i++)
	{
		StarArray[i].SciName = "";
    }

	FILE *snFile;
    snFile=fopen(sciNameFile.c_str(),"r");
    if (!snFile)
    {   
        printf("WARNING %s NOT FOUND\n",sciNameFile.c_str());
        return;
    }

	// Assign names to the matching stars, now support spaces in names
    unsigned int tmp;
    char line[256];
    Hip_Star *star;
	fgets(line, 256, snFile);
	do
	{
		sscanf(line,"%u",&tmp);
		star = search(tmp);
		if (star)
		{
			char c=line[0];
			int i=0;
			while(c!='|' && i<256){c=line[i];++i;}
			star->SciName = &(line[i]);
			star->SciName[star->SciName.size()-1] = '\0';
		}
	} while(fgets(line, 256, snFile));

    fclose(snFile);
}

// Draw all the stars
void Hip_Star_mgr::draw(float _star_scale, float _star_mag_scale, float _twinkle_amount,
						float maxMagStarName, Vec3f equ_vision,
						tone_reproductor* _eye, Projector* prj, bool _gravity_label)
{
	Hip_Star::twinkle_amount = _twinkle_amount;
	Hip_Star::star_scale = _star_scale;
	Hip_Star::star_mag_scale = _star_mag_scale;
	Hip_Star::eye = _eye;
	Hip_Star::proj = prj;
	Hip_Star::gravity_label = _gravity_label;
	Hip_Star::names_brightness = names_fader.get_interstate();
	
	glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture (GL_TEXTURE_2D, starTexture->getID());
	glBlendFunc(GL_ONE, GL_ONE);

	// Find the star zones which are in the screen
	int nbZones=0;
	nbZones = HipGrid.Intersect(equ_vision, prj->get_fov()*M_PI/180.f*1.2f);
	static int * zoneList = HipGrid.getResult();
	float maxMag = limiting_mag-1 + 60.f/prj->get_fov();

    prj->set_orthographic_projection();	// set 2D coordinate

	// Print all the stars of all the selected zones
	static vector<Hip_Star *>::iterator end;
	static vector<Hip_Star *>::iterator iter;
	Hip_Star* h;
	for(int i=0;i<nbZones;++i)
	{
		end = starZones[zoneList[i]].end();
	    for(iter = starZones[zoneList[i]].begin(); iter!=end; ++iter)
		{
			h=*iter;
			// If too small, skip and Compute the 2D position and check if in screen
			if(h->Mag>maxMag) break;
			if(!prj->project_prec_earth_equ_check(h->XYZ, h->XY)) continue;
			h->draw();
			if (!h->CommonName.empty() && names_fader.get_interstate() && h->Mag<maxMagStarName)
			{
				h->draw_name(starFont);
				glBindTexture (GL_TEXTURE_2D, starTexture->getID());
			}
		}
	}
	
	prj->reset_perspective_projection();
}

// Draw all the stars
void Hip_Star_mgr::draw_point(float _star_scale, float _star_mag_scale, float _twinkle_amount, float maxMagStarName, Vec3f equ_vision, tone_reproductor* _eye, Projector* prj, bool _gravity_label)
{
	Hip_Star::twinkle_amount = _twinkle_amount;
	Hip_Star::star_scale = _star_scale;
	Hip_Star::star_mag_scale = _star_mag_scale;
	Hip_Star::eye = _eye;
	Hip_Star::proj = prj;
	Hip_Star::gravity_label = _gravity_label;
	Hip_Star::names_brightness = names_fader.get_interstate();

	glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBindTexture (GL_TEXTURE_2D, starTexture->getID());
	glBlendFunc(GL_ONE, GL_ONE);

	// Find the star zones which are in the screen
	int nbZones=0;
	nbZones = HipGrid.Intersect(equ_vision, prj->get_fov()*M_PI/180.f*1.2f);
	int * zoneList = HipGrid.getResult();
	float maxMag = 5.5f+60.f/prj->get_fov();

    prj->set_orthographic_projection();	// set 2D coordinate

	// Print all the stars of all the selected zones
	static vector<Hip_Star *>::iterator end;
	static vector<Hip_Star *>::iterator iter;
	Hip_Star* h;
	for(int i=0;i<nbZones;++i)
	{
		end = starZones[zoneList[i]].end();
		for(iter = starZones[zoneList[i]].begin(); iter!=end; ++iter)
		{
			h=*iter;
			// If too small, skip and Compute the 2D position and check if in screen
			if(h->Mag>maxMag) break;
			if(!prj->project_prec_earth_equ_check(h->XYZ, h->XY)) continue;
			h->draw_point();
			if (!h->CommonName.empty() && names_fader.get_interstate() && h->Mag<maxMagStarName)
			{
				h->draw_name(starFont);
				glBindTexture (GL_TEXTURE_2D, starTexture->getID());
			}
		}
	}

    prj->reset_perspective_projection();
}

// Look for a star by XYZ coords
Hip_Star * Hip_Star_mgr::search(Vec3f Pos)
{
    Pos.normalize();
    Hip_Star * nearest=NULL;
    float angleNearest=0.;

    for(int i=0; i<StarArraySize; i++)
    {
		if (!StarFlatArray[i]) continue;
		if 	(StarFlatArray[i]->XYZ[0]*Pos[0] + StarFlatArray[i]->XYZ[1]*Pos[1] +
			StarFlatArray[i]->XYZ[2]*Pos[2]>angleNearest)
    	{
			angleNearest = StarFlatArray[i]->XYZ[0]*Pos[0] +
				StarFlatArray[i]->XYZ[1]*Pos[1] + StarFlatArray[i]->XYZ[2]*Pos[2];
        	nearest=StarFlatArray[i];
    	}
    }
    if (angleNearest>RADIUS_STAR*0.9999)
    {   
	    return nearest;
    }
    else return NULL;
}

// Return a stl vector containing the nebulas located inside the lim_fov circle around position v
vector<stel_object*> Hip_Star_mgr::search_around(Vec3d v, double lim_fov)
{
	vector<stel_object*> result;
    v.normalize();
	double cos_lim_fov = cos(lim_fov * M_PI/180.);
	//	static Vec3d equPos;

	for(int i=0; i<StarArraySize; i++)
    {
		if (!StarFlatArray[i]) continue;
		if (StarFlatArray[i]->XYZ[0]*v[0] + StarFlatArray[i]->XYZ[1]*v[1] + StarFlatArray[i]->XYZ[2]*v[2]>=cos_lim_fov)
    	{
			result.push_back(StarFlatArray[i]);
    	}
    }
	return result;
}

// Search the star by HP number
Hip_Star * Hip_Star_mgr::search(unsigned int _HP)
{
	if (StarFlatArray[_HP] && StarFlatArray[_HP]->HP == _HP)
		return StarFlatArray[_HP];
    return NULL;
}

/*
void Hip_Star_mgr::Save(void)
{
	FILE * fic = fopen("cat.fab","wb");
    fwrite((char*)&StarArraySize,4,1,fic);

    for(int i=0;i<StarArraySize;i++)
    {
    	Hip_Star * s = StarArray[i];
    	float RAf=0, DEf=0;
    	unsigned short int mag=0;
    	unsigned short int type=0;

    	if (s)
    	{
    		RAf=s->r; DEf=s->d;
    		mag = s->magp;
    		type = s->typep;
    	}
    	fwrite((char*)&RAf,4,1,fic);
    	fwrite((char*)&DEf,4,1,fic);
    	fwrite((char*)&mag,2,1,fic);
    	fwrite((char*)&type,2,1,fic);
    }
   	fclose(fic);
}*/
