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
#ifndef LIMA_LINGUISTICPROCESSINGFAKEHANDLER_H
#define LIMA_LINGUISTICPROCESSINGFAKEHANDLER_H

#include "AnalysisHandlersExport.h"
#include "common/Handler/AbstractAnalysisHandler.h"
#include "common/Data/genericDocumentProperties.h"
#include "linguisticProcessing/client/AnalysisHandlers/AbstractTextualAnalysisHandler.h"

namespace Lima
{

namespace LinguisticProcessing
{

/**
@author Benoit Mathieu
*/
class LIMA_ANALYSISHANDLERS_EXPORT FakeHandler : public AbstractTextualAnalysisHandler
{
  Q_OBJECT
public:
  FakeHandler();

  virtual ~FakeHandler();

  virtual void endAnalysis();
  virtual void endDocument();
  virtual void handle(const char* buf, int length) ;
  virtual void startAnalysis();
  virtual void startDocument(const Common::Misc::GenericDocumentProperties& props);

};

}

}

#endif
