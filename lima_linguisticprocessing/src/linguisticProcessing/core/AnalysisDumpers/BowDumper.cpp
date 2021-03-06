/*
    Copyright 2002-2015 CEA LIST

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

#include "BowDumper.h"

#include "linguisticProcessing/common/BagOfWords/bowDocument.h"
#include "common/MediaProcessors/HandlerStreamBuf.h"
#include "common/time/timeUtilsController.h"
#include "common/Data/strwstrtools.h"
#include "common/Data/genericDocumentProperties.h"
#include "common/XMLConfigurationFiles/xmlConfigurationFileExceptions.h"
#include "common/MediaticData/mediaticData.h"
#include "linguisticProcessing/common/BagOfWords/bowToken.h"
#include "linguisticProcessing/common/BagOfWords/BoWRelation.h"
#include "linguisticProcessing/common/BagOfWords/bowNamedEntity.h"
#include "linguisticProcessing/common/BagOfWords/bowTerm.h"
#include "linguisticProcessing/common/BagOfWords/bowText.h"
#include "linguisticProcessing/common/BagOfWords/bowBinaryReaderWriter.h"
#include "common/AbstractFactoryPattern/SimpleFactory.h"
#include "linguisticProcessing/common/annotationGraph/AnnotationData.h"
#include "linguisticProcessing/core/LinguisticProcessors/LinguisticMetaData.h"
#include "linguisticProcessing/core/LinguisticResources/LinguisticResources.h"
#include "linguisticProcessing/core/Automaton/recognizerMatch.h"
#include "linguisticProcessing/core/LinguisticAnalysisStructure/AnalysisGraph.h"
#include "linguisticProcessing/core/TextSegmentation/SegmentationData.h"
#include "linguisticProcessing/core/LinguisticAnalysisStructure/MorphoSyntacticData.h"
#include "linguisticProcessing/core/LinguisticAnalysisStructure/Token.h"
#include "linguisticProcessing/core/SyntacticAnalysis/SyntacticData.h"
#include "linguisticProcessing/core/AnalysisDumpers/BowGeneration.h"
#include "linguisticProcessing/core/SemanticAnalysis/SemanticRelationAnnotation.h"
#include "common/Handler/AbstractAnalysisHandler.h"
#include "common/MediaProcessors/MediaAnalysisDumper.h"
#include "linguisticProcessing/client/AnalysisHandlers/AbstractTextualAnalysisHandler.h"
#include "common/MediaProcessors/DumperStream.h"

#include <boost/graph/properties.hpp>

#include <fstream>
#include <deque>
#include <queue>
#include <iostream>

using namespace Lima::Common::XMLConfigurationFiles;
using namespace Lima::Common::MediaticData;
using namespace Lima::Common::BagOfWords;
using namespace Lima::Common::AnnotationGraphs;
using namespace Lima::LinguisticProcessing::Automaton;
using namespace Lima::LinguisticProcessing::Compounds;
using namespace Lima::LinguisticProcessing::LinguisticAnalysisStructure;
using namespace Lima::LinguisticProcessing::SpecificEntities;
using namespace Lima::LinguisticProcessing::SyntacticAnalysis;
using namespace Lima::LinguisticProcessing::SemanticAnalysis;
using namespace std;
namespace Lima
{

namespace LinguisticProcessing
{

namespace AnalysisDumpers
{

SimpleFactory<MediaProcessUnit,BowDumper> bowDumperFactory(BOWDUMPER_CLASSID);

typedef boost::color_traits<boost::default_color_type> Color;

BowDumper::BowDumper():
   AbstractTextualAnalysisDumper(),
    m_bowGenerator(new Compounds::BowGenerator()),
    m_handler(),
    m_graph()
{
}

BowDumper::~BowDumper()
{
  delete m_bowGenerator;
}

void BowDumper::init(
  Common::XMLConfigurationFiles::GroupConfigurationStructure& unitConfiguration,
  Manager* manager)
{
  AbstractTextualAnalysisDumper::init(unitConfiguration,manager);
  
  MediaId language = manager->getInitializationParameters().media;
  try
  {
    m_graph=unitConfiguration.getParamsValueAtKey("graph");
  }
  catch (NoSuchParam& )
  {
    m_graph=std::string("PosGraph");
  }
  try
  {
    m_handler=unitConfiguration.getParamsValueAtKey("handler");
  }
  catch (NoSuchParam& )
  {
    DUMPERLOGINIT;
    LERROR << "Missing parameter handler in BowDumper configuration";
    throw InvalidConfiguration();
  }
  
  m_bowGenerator->init(unitConfiguration, language);
}

LimaStatusCode BowDumper::process(
  AnalysisContent& analysis) const
{
  TimeUtilsController timer("BowDumper");
  DUMPERLOGINIT;

  LinguisticMetaData* metadata=static_cast<LinguisticMetaData*>(analysis.getData("LinguisticMetaData"));
  if (metadata == 0)
  {
    LERROR << "no LinguisticMetaData ! abort";
    return MISSING_DATA;
  }


  AnalysisHandlerContainer* h = static_cast<AnalysisHandlerContainer*>(analysis.getData("AnalysisHandlerContainer"));

  AbstractTextualAnalysisHandler* handler = static_cast<AbstractTextualAnalysisHandler*>(h->getHandler(m_handler));

#ifdef DEBUG_LP
  LDEBUG << "BowDumper handler will be: " << m_handler << (void*)handler;
#endif
  if (handler==0)
  {
    LERROR << "BowDumper::process: handler " << m_handler << " has not been given to the core client";
    return MISSING_DATA;
  }

  AnalysisGraph* anagraph=static_cast<AnalysisGraph*>(analysis.getData("AnalysisGraph"));
  if (anagraph==0)
  {
    LERROR << "BowDumper::process: no graph 'AnaGraph' available !";
    return MISSING_DATA;
  }
  AnalysisGraph* posgraph=static_cast<AnalysisGraph*>(analysis.getData("PosGraph"));
  if (posgraph==0)
  {
    LERROR << "BowDumper::process: no graph 'PosGraph' available !";
    return MISSING_DATA;
  }
  AnnotationData* annotationData = static_cast< AnnotationData* >(analysis.getData("AnnotationData"));
  if (annotationData==0)
  {
    LERROR << "BowDumper::process: no annotation graph available !";
    return MISSING_DATA;
  }
  SyntacticData* syntacticData=static_cast<SyntacticData*>(analysis.getData("SyntacticData"));
/*  if (syntacticData==0)
  {
    LERROR << "BowDumper::process: no SyntacticData ! abort";
    return MISSING_DATA;
  }*/
  if (syntacticData==0)
  {
    syntacticData=new SyntacticAnalysis::SyntacticData(static_cast<AnalysisGraph*>(analysis.getData(m_graph)),0);
    syntacticData->setupDependencyGraph();
    analysis.setData("SyntacticData",syntacticData);
  }


  // build BoWText from the result of the analysis
  BoWText bowText;
  bowText.lang=metadata->getMetaData("Lang");
  buildBoWText(annotationData, syntacticData, bowText,analysis,anagraph,posgraph);

  // Exclude from the shift list XML entities preceding the offset and 
  // readjust positions regarding the beginning of the node being analyzed
  uint64_t offset = metadata->getStartOffset();
  QMap<uint64_t, uint64_t> localShiftFrom;
  const auto& globalShiftFrom = handler->shiftFrom();
#ifdef DEBUG_LP
  LDEBUG << "BowDumper::process offset:" << offset;
  LDEBUG << "BowDumper::process globalShiftFrom:" << globalShiftFrom;
#endif
  if (!globalShiftFrom.isEmpty())
  {
    uint64_t diff = 0;
    // start first loop at second position
    auto it=globalShiftFrom.constBegin()+1;
    for (; it!=globalShiftFrom.constEnd(); ++it)
    {
#ifdef DEBUG_LP
      LDEBUG << "BowDumper::process it.key():"<<it.key()
              <<"; (it-1).value():"<<(it-1).value()
              <<"; offset:"<<offset<<"; diff:"<<diff;
#endif
      if (it.key()+(it-1).value() >= offset)
        break;
      diff = it.value();
    }
#ifdef DEBUG_LP
    LDEBUG << "BowDumper::process after shiftFrom loop, diff is:" << diff;
#endif
    // rewind by one to not miss the first entity and then 
    // continue from where we stoped the shift corrections
    for (it = it -1; it!=globalShiftFrom.constEnd(); ++it)
    {
#ifdef DEBUG_LP
      LDEBUG << "BowDumper::process it.key():"<<it.key()
              <<"; it.value():"<<it.value()
              <<"; offset:"<<offset<<"; diff:"<<diff;
#endif
      if (it.key()+diff >= offset && it.value() > diff)
      {
        // empirical correction but seems to work
        localShiftFrom.insert(it.key()+diff, it.value()-diff); 
      }
    }
  }
#ifdef DEBUG_LP
  LDEBUG << "BowDumper::process localShiftFrom:" << localShiftFrom;
#endif
  BoWBinaryWriter writer(localShiftFrom);
  DumperStream* dstream=initialize(analysis);

#ifdef DEBUG_LP
  LDEBUG << "BowDumper::process writing BoW text on" << &(dstream->out());
#endif
  writer.writeBoWText(dstream->out(),bowText);
  delete dstream;
  return SUCCESS_ID;
}

void BowDumper::buildBoWText(
    const Common::AnnotationGraphs::AnnotationData* annotationData,
    const SyntacticData* syntacticData,
    BoWText& bowText,
    AnalysisContent& analysis,
    AnalysisGraph* anagraph,
    AnalysisGraph* posgraph) const
{
#ifdef DEBUG_LP
  DUMPERLOGINIT;
#endif

  LinguisticMetaData* metadata=static_cast<LinguisticMetaData*>(analysis.getData("LinguisticMetaData"));

  SegmentationData* sb=static_cast<SegmentationData*>(analysis.getData("SentenceBoundaries"));
  if (sb==0)
  {
    DUMPERLOGINIT;
    LWARN << "no SentenceBounds";
  }

  if (sb==0)
  {
    // no sentence bounds : there can be specific entities,
    // but no compounds (syntactic analysis depend on sentence bounds)
    // dump whole text at once
    addVerticesToBoWText(
        annotationData,
        anagraph,
        posgraph,
        syntacticData,
        anagraph->firstVertex(),
        anagraph->lastVertex(),
        metadata->getStartOffset(),
        bowText);

  }
  else
  {
    for (auto boundItr=sb->getSegments().begin(); boundItr!=sb->getSegments().end(); boundItr++)
    {
      LinguisticGraphVertex sentenceBegin=boundItr->getFirstVertex();
      LinguisticGraphVertex sentenceEnd=boundItr->getLastVertex();

#ifdef DEBUG_LP
      LDEBUG << "BowDumper::buildBoWText dump sentence between " << sentenceBegin << " and " << sentenceEnd;

      LDEBUG << "BowDumper::buildBoWText dump simple terms for this sentence";
#endif
      addVerticesToBoWText(annotationData,
                           anagraph,
                           posgraph,
                           syntacticData,
                           sentenceBegin,
                           sentenceEnd,
                           metadata->getStartOffset(),
                           bowText);

    }
  }

  // look at all edges for relations
  AnnotationGraphEdgeIt it,it_end;
  const AnnotationGraph& annotGraph=annotationData->getGraph();
  boost::tie(it, it_end) = boost::edges(annotGraph);
  for (; it != it_end; it++)
  {
#ifdef DEBUG_LP
    LDEBUG << "BowDumper::buildBoWText on annotation edge "
           << source(*it,annotGraph) << "->" << target(*it,annotGraph);
#endif
    if (annotationData->hasAnnotation(*it,Common::Misc::utf8stdstring2limastring("SemanticRelation")))
    {
#ifdef DEBUG_LP
      LDEBUG << "found semantic relation";
#endif
      try
      {
        AnnotationGraphVertex agvs = source(*it,annotGraph);
        AnnotationGraphVertex agvt = target(*it,annotGraph);
        std::set< LinguisticGraphVertex > anaGraphVertices = annotationData->matches("annot", agvs, "AnalysisGraph");
        if  (anaGraphVertices.empty())
        {
          DUMPERLOGINIT;
          LERROR << "Found no analysis graph vertex associated to the annotation graph vertex" << agvs << ". It will crash";
        }
        LinguisticGraphVertex lgvs = *anaGraphVertices.begin();
        std::set< LinguisticGraphVertex > visited;
        bool keepAnyway=true;

        const SemanticRelationAnnotation& annot = annotationData->annotation(
          *it,Common::Misc::utf8stdstring2limastring("SemanticRelation"))
              .value<SemanticRelationAnnotation>();
        boost::shared_ptr< BoWPredicate > predicate = m_bowGenerator->createPredicate(
                                        lgvs, agvs, agvt, annot,
                                        annotationData,
                                        *anagraph->getGraph(),
                                        *posgraph->getGraph(),
                                        metadata->getStartOffset(), visited,
                                        keepAnyway);
        bowText.push_back(predicate);
      }
      catch (const boost::bad_any_cast& e)
      {
        DUMPERLOGINIT;
        LERROR << "This annotation is not a SemanticAnnotation";
        continue;
      }
    }
  }


}

void BowDumper::addVerticesToBoWText(
    const Common::AnnotationGraphs::AnnotationData* annotationData,
    AnalysisGraph* anagraph,
    AnalysisGraph* posgraph,
    const SyntacticData* syntacticData,
    const LinguisticGraphVertex begin,
    const LinguisticGraphVertex end,
    const uint64_t offset,
    BoWText& bowText) const
{

#ifdef DEBUG_LP
  DUMPERLOGINIT;
  LDEBUG << "BowDumper::addVerticesToBoWText from"  << begin << "to" << end << "; offset:" << offset;
#endif

  const LinguisticGraph& beforePoSGraph=*(anagraph->getGraph());
  const LinguisticGraph& graph=*(posgraph->getGraph());
  
  // go through the graph, add BoWTokens that are not in complex terms
  // Don't use visitor to avoid throwing exceptions

  LinguisticGraphVertex firstVx = anagraph->firstVertex();
  LinguisticGraphVertex lastVx = anagraph->lastVertex();

  std::set< std::string > alreadyStored;
  std::set<LinguisticGraphVertex> visited;
  //std::set<uint32_t> alreadyStoredVertices; compatibilite 32 64 bits
  std::set<LinguisticGraphVertex> alreadyStoredVertices;

  std::queue<LinguisticGraphVertex> toVisit;
  toVisit.push(begin);

  while (!toVisit.empty())
  {
    LinguisticGraphVertex v=toVisit.front();
// #ifdef DEBUG_LP
//     LDEBUG << "BowDumper::addVerticesToBoWText visiting" << v;
// #endif

    toVisit.pop();
    if (v == end) {
// #ifdef DEBUG_LP
//       LDEBUG << "BowDumper::addVerticesToBoWText on end";
// #endif
      continue;
    }

    LinguisticGraphOutEdgeIt outItr,outItrEnd;
    for (boost::tie(outItr,outItrEnd)=out_edges(v,graph);
         outItr!=outItrEnd;
         outItr++)
    {
      LinguisticGraphVertex next=target(*outItr,graph);
      if (visited.find(next)==visited.end())
      {
        visited.insert(next);
        toVisit.push(next);
      }
    }

    if (v != firstVx && v != lastVx)
    {
      /// @todo replace v in LDEBUGs below by matching annotation vertices
//       LDEBUG << "BowDumper::addVerticesToBoWText: hasAnnotation("<<v<<", CompoundTokenAnnotation): "
//         << annotationData->hasAnnotation(v, Common::Misc::utf8stdstring2limastring("CompoundTokenAnnotation"));
//       LDEBUG << "BowDumper::addVerticesToBoWText: hasAnnotation("<<v<<", SpecificEntity): "
//         << annotationData->hasAnnotation(v, Common::Misc::utf8stdstring2limastring("SpecificEntity"));
      std::set< AnnotationGraphVertex > cpdsHeads = annotationData->matches("PosGraph", v, "cpdHead");
      std::set< AnnotationGraphVertex > cpdsExts = annotationData->matches("PosGraph", v, "cpdExt");
      if (!cpdsHeads.empty())
      {
// #ifdef DEBUG_LP
//         LDEBUG << "BowDumper::addVerticesToBoWText" << v << "is a compound head";
// #endif
        auto cpdsHeadsIt = cpdsHeads.begin(), cpdsHeadsIt_end = cpdsHeads.end();
        for (; cpdsHeadsIt != cpdsHeadsIt_end; cpdsHeadsIt++)
        {
          AnnotationGraphVertex agv  = *cpdsHeadsIt;
// #ifdef DEBUG_LP
//           LDEBUG << "BowDumper::addVerticesToBoWText" << v << "calling buildTermFor on" << agv;
// #endif
          std::vector<std::pair< boost::shared_ptr< BoWRelation >, boost::shared_ptr< BoWToken > > > bowTokens = m_bowGenerator->buildTermFor(agv, agv, beforePoSGraph, graph, offset, syntacticData, annotationData, visited);
// #ifdef DEBUG_LP
//           LDEBUG << "BowDumper::addVerticesToBoWText" << v << "buildTermFor on" << agv << "got nb tokens:" << bowTokens.size();
// #endif
          for (auto bowItr=bowTokens.begin(); bowItr!=bowTokens.end(); bowItr++)
          {
            std::string elem = (*bowItr).second->getIdUTF8String();
            if (alreadyStored.find(elem) != alreadyStored.end())
            { // already stored
// #ifdef DEBUG_LP
//               LDEBUG << "BowDumper::addVerticesToBoWText" << elem << "already stored. Skipping it.";
// #endif
            }
            else
            {             
              bowText.push_back((*bowItr).second); // copy pointer
              std::set<uint64_t> bowTokenVertices = (*bowItr).second->getVertices();
              //std::set<LinguisticGraphVertex> bowTokenVertices = (*bowItr)->getVertices();
              alreadyStoredVertices.insert(bowTokenVertices.begin(), bowTokenVertices.end());
              alreadyStored.insert(elem);
              
// #ifdef DEBUG_LP
//               std::ostringstream oss;
//               //std::set<uint32_t>::const_iterator asvit, asvit_end;
//               std::set<LinguisticGraphVertex>::const_iterator asvit, asvit_end;
//               asvit = alreadyStoredVertices.begin(); asvit_end = alreadyStoredVertices.end();
//               for (; asvit != asvit_end; asvit++)
//               {
//                 oss << *asvit << ", ";
//               }
//               LDEBUG << "BowDumper::addVerticesToBoWText" << v << "added" << elem << "to bowText; alreadyStoredVertices are: " << oss.str();
// #endif
            }
          }
        }
      }
      else if (!cpdsExts.empty())
      {
        // Do nothing for compound extensions. Will be  handled by the head
// #ifdef DEBUG_LP
//         LDEBUG << "BowDumper::addVerticesToBoWText Do nothing for compound extensions. Will be  handled by the head";
// #endif
      }
      else if (alreadyStoredVertices.find(v) == alreadyStoredVertices.end())
      {
// Commented out code below was handling a bug causing to dump as a simple term 
// a token member of a compound. As it is better handled by setting a correct 
// annotation to the token, this code is removed
//         bool isInCompound = false;
//         DependencyGraphVertex dgv = syntacticData->depVertexForTokenVertex(v);
//         DependencyGraphOutEdgeIt dgoutItr,dgoutItrEnd;
//         for (boost::tie(dgoutItr,dgoutItrEnd)=boost::out_edges(dgv,*syntacticData->dependencyGraph());
//               dgoutItr!=dgoutItrEnd;
//               dgoutItr++)
//         {
//           auto relTypeMap = get(edge_deprel_type, *syntacticData->dependencyGraph());
// 
//           Common::MediaticData::SyntacticRelationId relType=relTypeMap[*dgoutItr];
//           std::string relName = static_cast<const Common::MediaticData::LanguageData&>(Common::MediaticData::MediaticData::single().mediaData(m_language)).getSyntacticRelationName(relType);
// 
//           LDEBUG << "Relation name" << relName;
// 
//           if (static_cast<const Common::MediaticData::LanguageData&>(Common::MediaticData::MediaticData::single().mediaData(m_language)).isACompoundRel(relName))
//           {
//             isInCompound = true;
//             break;
//           }
//         }
// 
//         
//         if   (!isInCompound)
        {
// #ifdef DEBUG_LP
//           LDEBUG << "BowDumper::addVerticesToBoWText" << v << "isn't a compound head";
// #endif
          std::vector<std::pair<boost::shared_ptr< BoWRelation>, boost::shared_ptr< AbstractBoWElement>> > bowTokens=
            m_bowGenerator->createAbstractBoWElement(v, beforePoSGraph, graph, offset, annotationData, visited);
// #ifdef DEBUG_LP
//           LDEBUG << "BowDumper::addVerticesToBoWText"<<v<<" createAbstractBoWElement returned a vector of size" << bowTokens.size();
// #endif

          for (auto bowItr=bowTokens.begin();
              bowItr!=bowTokens.end();
              bowItr++)
          {
            std::string elem = (*bowItr).second->getIdUTF8String();
// #ifdef DEBUG_LP
//             LDEBUG << "BowDumper::addVerticesToBoWText"<<v<<"created BoWToken" << elem;
// #endif
            if (alreadyStored.find(elem) != alreadyStored.end())
            { // already stored
// #ifdef DEBUG_LP
//               LDEBUG << "BowDumper::addVerticesToBoWText BoWToken already stored. Skipping it.";
// #endif
            }
            else
            {
// #ifdef DEBUG_LP
//               LDEBUG << "BowDumper::addVerticesToBoWText pushing" << elem << "to bowText";
// #endif
              bowText.push_back((*bowItr).second); // copy pointer
              std::set<uint64_t> bowTokenVertices = (*bowItr).second->getVertices();
              alreadyStoredVertices.insert(bowTokenVertices.begin(), bowTokenVertices.end());
              alreadyStored.insert(elem);
            
// #ifdef DEBUG_LP
//               std::ostringstream oss;
//               //std::set<uint32_t>::const_iterator asvit, asvit_end;
//               std::set<LinguisticGraphVertex>::const_iterator asvit, asvit_end;
//               asvit = alreadyStoredVertices.begin(); asvit_end = alreadyStoredVertices.end();
//               for (; asvit != asvit_end; asvit++)
//               {
//                 oss << *asvit << ", ";
//               }
//               std::string elem = (*bowItr).second->getIdUTF8String();
//               LDEBUG << "BowDumper::addVerticesToBoWText for" << v << elem << "; alreadyStoredVertices are:" << oss.str();
// #endif
            }
          }
        }
      }
// #ifdef DEBUG_LP
//       else
//       {
//         LDEBUG << "BowDumper::addVerticesToBoWText" << v << "is already stored.";
//       }
// #endif
    }
  }
}

} // AnalysisDumper

} // LinguisticProcessing

} // Lima
