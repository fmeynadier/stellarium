/*
 * Copyright (C) 2004 Fabien Ch�reau
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

// Redefine the single precision math functions if not defined
// This must be used in conjunction with the autoconf macro :
// AC_CHECK_FUNCS(sinf cosf tanf asinf acosf atanf expf logf log10f atan2f sqrtf)

#ifndef _FMATH_H_
#define _FMATH_H_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef HAVE_ACOSF
# define acosf(x) (float)(acos(x))
#endif
#ifndef HAVE_ASINF
# define asinf(x) (float)(asin(x))
#endif
#ifndef HAVE_ATAN2F
# define atan2f(x) (float)(atan2(x))
#endif
#ifndef HAVE_ATANF
# define atanf(x) (float)(atan(x))
#endif
#ifndef HAVE_COSF
# define cosf(x) (float)(cos(x))
#endif
#ifndef HAVE_EXPF
# define expf(x) (float)(exp(x))
#endif
#ifndef HAVE_POWF
# define powf(x) (float)(pow(x))
#endif
#ifndef HAVE_LOG10F
# define log10f(x) (float)(log10(x))
#endif
#ifndef HAVE_LOGF
# define logf(x) (float)(log(x))
#endif
#ifndef HAVE_SINF
# define sinf(x) (float)(sin(x))
#endif
#ifndef HAVE_SQRTF
# define sqrtf(x) (float)(sqrt(x))
#endif

#endif /*_FMATH_H_*/
