/*
 *    Copyright 2002-2013 CEA LIST
 * 
 *    This file is part of LIMA.
 * 
 *    LIMA is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 * 
 *    LIMA is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 * 
 *    You should have received a copy of the GNU Affero General Public License
 *    along with LIMA.  If not, see <http://www.gnu.org/licenses/>
 */
#ifndef LIMA_PELF_DICTIONNARYENTRY_H
#define LIMA_PELF_DICTIONNARYENTRY_H

#include <QtCore/QtDebug>
#include <QtCore/QStringList>

#include "AbstractResourceEntry.h"

namespace Lima {
namespace Pelf {

class DictionnaryEntry : public AbstractResourceEntry
{

public:

    static int columnCountPerEntry;
    static QStringList columnHeaders;
    static QString allCategoryName;
    static QString emptyCategoryName;

    static DictionnaryEntry* factory (QString s1, QString s2, int i);
    static DictionnaryEntry* factoryEndFile () { return 0; };

    QString null1;
    QString null2;
    QString lemma;
    QString normalization;
    QString validation;

    DictionnaryEntry (QString s = "");
    bool decodeSourceString () override;
    bool matches (QStringList args) override;
    QString encodeToString () override;
    QString getColumnData(int column) override;

};

} // End namespace Lima
} // End namespace Pelf

#endif // LIMA_PELF_DICTIONNARYENTRY_H
