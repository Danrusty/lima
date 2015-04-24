/*
    Copyright 2002-2014 CEA LIST

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

 ************************************************************************
 *
 * @file       TrainerWap.h
 * @author     Grandjean Alex (alex.grandjean@cea.fr)
 * @date       Thu Mar 20 2014
 * copyright   Copyright (C) 20062014 by CEA LIST
 * Project     Lima
 * 
 * @brief      Train a model for crf with wapiti library
 * 
 * 
 ***********************************************************************/
#ifndef TRAINERWAP_H
#define TRAINERWAP_H

#include <string>

#include "model.h"
#include "AbstractTrainer.h"
#include <vector>

#include "SpecificEntitiesCRFExport.h"

namespace Lima {
   
class AnalysisContent;

  namespace LinguisticProcessing {
 
#define TrainerWap_ID "trainWap"

class TrainerWapPrivate;

class TrainerWap : public AbstractTrainer 
{

public:
  TrainerWap();
  
  ~TrainerWap();

  
  /**
   * @brief Initialize the training options
   * @param options Map with all the options and their values
   */
  void initOptions(const std::map<std::string, std::string>& options);
  
  /**
   * @brief Perform the training: create a model from input data
   */
  void training();


private:
  TrainerWapPrivate *m_tw;


};
  }
}

#endif
