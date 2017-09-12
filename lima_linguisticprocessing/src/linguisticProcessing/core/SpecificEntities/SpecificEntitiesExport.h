/*
    Copyright 2002-2013 CEA LIST

    This file is part of LIMA.

    LIMA is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LIMA is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with LIMA.  If not, see <http://www.gnu.org/licenses/>
*/
/***************************************************************************
 *   Copyright (C) 2004-2012 by CEA LIST                               *
 *                                                                         *
 ***************************************************************************/

#include <common/LimaCommon.h>

#ifndef LIMA_SPECIFICENTITIESEXPORT_H
#define LIMA_SPECIFICENTITIESEXPORT_H

#ifdef WIN32

#undef min
#undef max

#ifdef LIMA_SPECIFICENTITIES_EXPORTING
   #define LIMA_SPECIFICENTITIES_EXPORT    __declspec(dllexport)
#else
   #define LIMA_SPECIFICENTITIES_EXPORT    __declspec(dllimport)
#endif

#ifdef LIMA_SPECIFICENTITIES_DATETIME_EXPORTING
   #define LIMA_SPECIFICENTITIES_DATETIME_EXPORT    __declspec(dllexport)
#else
   #define LIMA_SPECIFICENTITIES_DATETIME_EXPORT    __declspec(dllimport)
#endif

#ifdef LIMA_SPECIFICENTITIES_NUMBER_EXPORTING
   #define LIMA_SPECIFICENTITIES_NUMBER_EXPORT    __declspec(dllexport)
#else
   #define LIMA_SPECIFICENTITIES_NUMBER_EXPORT    __declspec(dllimport)
#endif

#ifdef LIMA_SPECIFICENTITIES_PERSON_EXPORTING
   #define LIMA_SPECIFICENTITIES_PERSON_EXPORT    __declspec(dllexport)
#else
   #define LIMA_SPECIFICENTITIES_PERSON_EXPORT    __declspec(dllimport)
#endif

#else // Not WIN32

#define LIMA_SPECIFICENTITIES_EXPORT
#define LIMA_SPECIFICENTITIES_DATETIME_EXPORT
#define LIMA_SPECIFICENTITIES_NUMBER_EXPORT
#define LIMA_SPECIFICENTITIES_PERSON_EXPORT

#endif

#endif
