/*
 * Copyright (C) 2003 Fabien Ch�reau
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

// Class which converts tones in function of the eye adaptation to luminance.
// The aim is to get on the screen something which is perceptualy accurate,
// ie. to compress high dynamic range luminance to CRT display range.
// Partial implementation of the algorithm from the paper :
// "Tone Reproduction for Realistic Images", Tumblin and Rushmeier,
// IEEE Computer Graphics & Application, November 1993

// Important : you must call set_display_adaptation_luminance()
// and set_world_adaptation_luminance() before any call to xyY_to_RGB()
// or adapt_luminance

#ifndef _TONE_REPRODUCTOR_H_
#define _TONE_REPRODUCTOR_H_

class tone_reproductor
{
public:
    tone_reproductor();
    virtual ~tone_reproductor();

	// Set the eye adaptation luminance for the display and precompute what can be
	// Usual luminance range is 1-100 cd/m^2 for a CRT screen
	// default value = 50 cd/m^2
	set_display_adaptation_luminance(float display_adaptation_luminance);

	// Set the eye adaptation luminance for the world and precompute what can be
	// default value = 100000 cd/m^2
	// Star Light      : 0.001  cd/m^2
	// Moon Light      : 0.1    cd/m^2
	// Indoor Lighting : 100    cd/m^2
	// Sun Light       : 100000 cd/m^2
	set_world_adaptation_luminance(float world_adaptation_luminance);

	// Set the maximum display luminance : default value = 100 cd/m^2
	// This value is used to scale the RGB range
	set_max_display_luminance(float _MaxdL) {MaxdL = MaxdL};

	// Return adapted luminance from world to display
	inline float adapt_luminance(float world_luminance);

	// Convert from xyY color system to RGB
	void xyY_to_RGB(float[3]);

private:
	float Lda;		// Display luminance adaptation
	float Lwa;		// World   luminance adaptation
	float MaxdL;	// Display maximum luminance
	// Precomputed variables
	float alpha_da;
	float beta_da;
	float alpha_wa;
	float beta_wa;
	float alpha_aw_over_alpha_ad;
	float term2;
};

#endif // _TONE_REPRODUCTOR_H_
