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
#ifndef LIMA_LINGUISTICPROCESSING_ANALYSISDICT_MULTILEVELANALYSISDICTIONARYITERATOR_H
#define LIMA_LINGUISTICPROCESSING_ANALYSISDICT_MULTILEVELANALYSISDICTIONARYITERATOR_H

#include "AnalysisDictExport.h"
#include "linguisticProcessing/LinguisticProcessingCommon.h"
#include "AnalysisDictionaryIterators.h"
#include "MultiLevelAnalysisDictionary.h"
#include "MultiLevelAnalysisDictionaryEntry.h"
#include "common/misc/AbstractAccessIterators.h"

namespace Lima
{

namespace LinguisticProcessing
{

namespace AnalysisDict
{

/**
@author Benoit Mathieu
*/
class LIMA_ANALYSISDICT_EXPORT MultiLevelAnalysisDictionarySubWordIterator  : public AbstractDictionarySubWordIterator
{
public:
  MultiLevelAnalysisDictionarySubWordIterator(
    const std::vector<std::pair<Common::AccessSubWordIterator,Common::AccessSubWordIterator> >& accessItr,
    const MultiLevelAnalysisDictionary& dico);

  virtual ~MultiLevelAnalysisDictionarySubWordIterator();

  virtual const std::pair<uint64_t,DictionaryEntry> operator*() const override;
  MultiLevelAnalysisDictionarySubWordIterator& operator++(int i) override;
  bool operator==(const AbstractDictionarySubWordIterator& it) const override;
  bool operator!=(const AbstractDictionarySubWordIterator& it) const override;

  virtual AbstractDictionarySubWordIterator* clone() const override;

private:

  // ce vector doit contenir un iterator par niveau dans le dictionnaire
  std::vector<std::pair<Common::AccessSubWordIterator,Common::AccessSubWordIterator> > m_accessItrs;
  const MultiLevelAnalysisDictionary& m_dico;
};

class LIMA_ANALYSISDICT_EXPORT MultiLevelAnalysisDictionarySuperWordIterator : public AbstractDictionarySuperWordIterator
{
public:

  MultiLevelAnalysisDictionarySuperWordIterator(
    const std::vector<Common::AccessSuperWordIterator>& i);
  MultiLevelAnalysisDictionarySuperWordIterator(const MultiLevelAnalysisDictionarySuperWordIterator& source);
  virtual ~MultiLevelAnalysisDictionarySuperWordIterator();

  virtual const LimaString operator*() const override;
  MultiLevelAnalysisDictionarySuperWordIterator& operator++(int i) override;
  bool operator==(const AbstractDictionarySuperWordIterator& it) const override;
  bool operator!=(const AbstractDictionarySuperWordIterator& it) const override;
  
  virtual AbstractDictionarySuperWordIterator* clone() const override;

private:

  std::vector<Common::AccessSuperWordIterator> m_accessItrs;

};

}

}

}

#endif
