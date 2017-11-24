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

#ifndef LIMA_LINGUISTICPROCESSING_TENSORFLOWSPECIFICENTITIES_TENSORFLOWSPECIFICENTITIES_H
#define LIMA_LINGUISTICPROCESSING_TENSORFLOWSPECIFICENTITIES_TENSORFLOWSPECIFICENTITIES_H

#include "common/MediaProcessors/MediaProcessUnit.h"
#include "TensorflowSpecificEntitiesExport.h"

namespace Lima
{
namespace LinguisticProcessing
{
namespace TensorflowSpecificEntities
{
#define TENSORFLOWSPECIFICENTITIES_CLASSID "TensorflowSpecificEntities"

class LIMA_TENSORFLOWSPECIFICENTITIES_EXPORT TensorflowSpecificEntities : public MediaProcessUnit
{
public:
//   TensorflowSpecificEntities(){};
  virtual ~TensorflowSpecificEntities(){};

  void init(
    Common::XMLConfigurationFiles::GroupConfigurationStructure& unitConfiguration,
    Manager* manager) override;

  LimaStatusCode process(
    AnalysisContent& analysis) const override;  
private:
  std::string m_graph;
  std::string m_fileChars;
  std::string m_fileWords;
  std::string m_fileTags;
};

} // TensorflowSpecificEntities
} // LinguisticProcessing
} // Lima

#endif
