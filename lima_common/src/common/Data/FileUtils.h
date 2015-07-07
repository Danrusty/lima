/*
    Copyright 2015 CEA LIST

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
/************************************************************************
 * @file       FileUtils.h
 * @author     Gael de Chalendar
 * @date       Tue Jul  7 2015
 * copyright   Copyright (C) 2015 by CEA LIST
 ***********************************************************************/

#ifndef LIMA_COMMON_MISC_FILEUTILS_H
#define LIMA_COMMON_MISC_FILEUTILS_H

#include "common/LimaCommon.h"

#include <iostream>
#include <QtCore/QFile>

namespace Lima {
namespace Common {
namespace Misc {

/** 
 * Count the number of lines in the given file from the current position
 * 
 * If the last line has no character (no character after the last line break)' it is not counted.
 * After this function, the file is in the same good state and at the same position.
 * 
 * @param file the file to count the lines of
 * 
 * @return the number of lines of the file
 */
LIMA_DATA_EXPORT uint64_t countLines(std::istream& file);

/** 
 * Count the number of lines in the given file from the current position
 * 
 * If the last line has no character (no character after the last line break)' it is not counted.
 * After this function, the file is at the same position.
 * 
 * @param file the file to count the lines of
 * 
 * @return the number of lines of the file
 */
LIMA_DATA_EXPORT uint64_t countLines(QFile& file);


} // end namespace
} // end namespace
} // end namespace

#endif
