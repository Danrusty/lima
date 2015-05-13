/***************************************************************************
 *   Copyright (C) 2014 by CEA LIST                              *
 *                                                                         *
 ***************************************************************************/
#include "CrfSEDumper.h"
#include "TextDumper.h" // for lTokenPosition comparison function to order tokens

#include "common/time/traceUtils.h"
#include "common/Data/strwstrtools.h"
#include "common/XMLConfigurationFiles/xmlConfigurationFileExceptions.h"
#include "common/AbstractFactoryPattern/SimpleFactory.h"
#include "linguisticProcessing/core/LinguisticProcessors/LinguisticMetaData.h"
#include "linguisticProcessing/core/TextSegmentation/SegmentationData.h"

#include <boost/graph/properties.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <deque>
#include <queue>

using namespace std;
//using namespace boost;
using namespace boost::tuples;
using namespace Lima::Common::XMLConfigurationFiles;
using namespace Lima::Common::MediaticData;
using namespace Lima::Common::AnnotationGraphs;
using namespace Lima::LinguisticProcessing::LinguisticAnalysisStructure;
using namespace Lima::LinguisticProcessing::SyntacticAnalysis;
using namespace Lima::LinguisticProcessing::SpecificEntities;

namespace Lima {
namespace LinguisticProcessing {
namespace AnalysisDumpers {

SimpleFactory<MediaProcessUnit,CrfSEDumper> crfSEDumperFactory(CRFSEDUMPER_CLASSID);

class CrfSEDumperPrivate {

  friend class CrfSEDumper;

  CrfSEDumperPrivate();
  ~CrfSEDumperPrivate();
  CrfSEDumperPrivate(const CrfSEDumperPrivate& sp);
  CrfSEDumperPrivate& operator= ( const CrfSEDumperPrivate& sp );
  
private:
  MediaId m_language;
  std::string m_graph;
  WordFeatures m_features; //!< use dedicated class for feature storage (easy initialization functions)
  std::map<AbstractFeatureExtractor*,std::string> m_featuresMap; //!< use additional map to associate features with XML tags (only pointers)
  std::map<std::string,std::string> m_defaultFeatures;
  bool m_outputSentenceBoundaries;
  std::map<std::string, std::string> m_NEauthorized;
  
 void clearFeatures();
  void initializeFeatures(const std::map<std::string,std::string>& features,
                          const std::deque<std::string>& featureOrder=std::deque<std::string>());
  
  void xmlOutput(std::ostream& out,
                 AnalysisContent& analysis,
                 LinguisticAnalysisStructure::AnalysisGraph* anagraph,
                 LinguisticAnalysisStructure::AnalysisGraph* posgraph,
                 const Common::AnnotationGraphs::AnnotationData* annotationData) const;
  
  void xmlOutputVertices(std::ostream& out,
                         LinguisticAnalysisStructure::AnalysisGraph* anagraph,
                         LinguisticAnalysisStructure::AnalysisGraph* posgraph,
                         const Common::AnnotationGraphs::AnnotationData* annotationData,
                         const LinguisticGraphVertex begin,
                         const LinguisticGraphVertex end,
                         const FsaStringsPool& sp,
                         const uint64_t offset) const;

  /**
   * @brief display Vertex Infos
   */
  void xmlOutputVertex(std::ostream& out, 
                       LinguisticGraphVertex v,
                       LinguisticAnalysisStructure::AnalysisGraph* anagraph,
                       LinguisticAnalysisStructure::AnalysisGraph* posgraph,
                       const Common::AnnotationGraphs::AnnotationData* annotationData,
                       const FsaStringsPool& sp,
                       uint64_t offset) const;

  /**
   * @brief Display Vertex Infos if several interpretation of the token
   */
  void xmlOutputVertexInfos(std::ostream& out, 
                            LinguisticGraphVertex v,
                            LinguisticAnalysisStructure::AnalysisGraph* anagraph,
                            uint64_t offset) const;


  // string manipulation functions to protect XML entities
  std::string xmlString(const std::string& str) const;
  void replace(std::string& str, const std::string& toReplace, const std::string& newValue) const;



};


CrfSEDumperPrivate::CrfSEDumperPrivate(): 
m_graph("PosGraph"),
m_features(),
m_featuresMap(),
m_defaultFeatures(),
m_outputSentenceBoundaries(true)
  {
  // default features
  //m_defaultFeatures["p"]="position";
  m_defaultFeatures["inf"]="word";
  m_defaultFeatures["pos"]="property:MICRO";
  m_defaultFeatures["se"]="specificEntity";
  //m_defaultFeatures["lemma"]="lemma";
}

CrfSEDumperPrivate::~CrfSEDumperPrivate() {
  clearFeatures();
}



CrfSEDumper::CrfSEDumper():
  AbstractTextualAnalysisDumper()
{
  m_d=new CrfSEDumperPrivate();

}

//@todo : copy constructor: deal with copy of pointers

CrfSEDumper::~CrfSEDumper()
{
  delete m_d;
}


void CrfSEDumperPrivate::clearFeatures() {
  for (WordFeatures::const_iterator it=m_features.begin(),it_end=m_features.end();it!=it_end;it++) {
    if (*it!=0) {
      delete *it;
    }
  }
  m_features.clear();
  m_featuresMap.clear();
}


void CrfSEDumper::init(
  Common::XMLConfigurationFiles::GroupConfigurationStructure& unitConfiguration,
  Manager* manager)

{
  AbstractTextualAnalysisDumper::init(unitConfiguration,manager);

  m_d->m_language = manager->getInitializationParameters().media;
  
  try
  {
    m_d->m_graph=unitConfiguration.getParamsValueAtKey("graph");
  }
  catch (NoSuchParam& ) {} // keep default value

  m_d->m_features.setLanguage(m_d->m_language);
  try { 
    // if some features are specified, all must be specified: do not keep any default ones
    map<string,string> featuresMap=unitConfiguration.getMapAtKey("features"); 
    try {
      // order can be specified (not in map: map is unordered)
      deque<string> featureOrder=unitConfiguration.getListsValueAtKey("featureOrder"); 
      m_d->initializeFeatures(featuresMap,featureOrder);
    }
    catch (NoSuchList& ) { // no order specified: keep order from map
      m_d->initializeFeatures(featuresMap);
    }
  }
  catch (NoSuchMap& ) {
    //initialize with default values
    m_d->initializeFeatures(m_d->m_defaultFeatures);
  }

  std::map<std::string, std::string> mpNEauthorized;
  try
  {
    mpNEauthorized=unitConfiguration.getMapAtKey("NECategoryAuthorized");
  }
  catch (NoSuchParam& ) {} // keep default value 
  
  static_cast<FeatureSpecificEntity*> (m_d->m_features.back())->setNEauthorized(mpNEauthorized);
 
}

void CrfSEDumperPrivate::initializeFeatures(const map<string,string>& featuresMap, 
                                          const std::deque<std::string>& featureOrder) 
{
  clearFeatures();
  deque<string> features,featureTags;
  bool useMapOrder(false);
  if (! featureOrder.empty()) {
    // use specified order 
    DUMPERLOGINIT;
    LDEBUG << "CrfSEDumper: initialize features: use order" << LENDL;
    featureTags=featureOrder;
    for (deque<string>::const_iterator it=featureOrder.begin(),it_end=featureOrder.end();it!=it_end;it++) {
      LDEBUG << "CrfSEDumper: --"<< (*it) << LENDL;
      const std::string& featureTag=(*it);
      map<string,string>::const_iterator f=featuresMap.find(featureTag);
      if (f==featuresMap.end()) {
        DUMPERLOGINIT;
        LWARN << "CrfSEDumper: 'featureOrder' parameter mentions a feature '" << featureTag
              << "' not in feature map parameter: order ignored" << LENDL;
        useMapOrder=true;
        features.clear();
        featureTags.clear();
        break;
      }
      features.push_back((*f).second);
    }
  }
  else {
    useMapOrder=true;
  }
  
  if (useMapOrder) {
    for (map<string,string>::const_iterator it=featuresMap.begin(),it_end=featuresMap.end();it!=it_end; it++) {
      const std::string& featureName=(*it).second;
      const std::string& featureTag=(*it).first;
      features.push_back(featureName);
      featureTags.push_back(featureTag);
    }
  }
  m_features.initialize(features);
  if (m_features.size()!=featureTags.size()) {
    DUMPERLOGINIT;
    LERROR << "CrfSEDumper: error: failed to initialize all features" << LENDL;
    throw InvalidConfiguration();
  }
  for (unsigned int i=0,end=m_features.size();i!=end;i++) {
      m_featuresMap[m_features[i]]=featureTags[i];
  }
}

LimaStatusCode CrfSEDumper::
process(AnalysisContent& analysis) const
{
  TimeUtils::updateCurrentTime();
  DUMPERLOGINIT;
  LDEBUG << "CrfSEDumper::process" << LENDL;
  
  LinguisticMetaData* metadata=static_cast<LinguisticMetaData*>(analysis.getData("LinguisticMetaData"));
  if (metadata == 0)
  {
    LERROR << "no LinguisticMetaData ! abort" << LENDL;
    return MISSING_DATA;
  }

  AnalysisGraph* anagraph=static_cast<AnalysisGraph*>(analysis.getData("AnalysisGraph"));
  if (anagraph==0)
  {
    LERROR << "no graph 'AnaGraph' available !" << LENDL;
    return MISSING_DATA;
  }
  AnalysisGraph* posgraph=static_cast<AnalysisGraph*>(analysis.getData("PosGraph"));
  if (posgraph==0)
  {
    LERROR << "no graph 'PosGraph' available !" << LENDL;
    return MISSING_DATA;
  }
  AnnotationData* annotationData = static_cast< AnnotationData* >(analysis.getData("AnnotationData"));
  if (annotationData==0)
  {
    LERROR << "no annotation graph available !" << LENDL;
    return MISSING_DATA;
  }

  static_cast<FeatureSpecificEntity*> (m_d->m_features.back())->annot=annotationData;
 
   DumperStream* dstream=initialize(analysis);
   m_d->xmlOutput(dstream->out(), analysis, anagraph, posgraph, annotationData);
   delete dstream;

   TimeUtils::logElapsedTime("CrfSEDumper");
   return SUCCESS_ID;
}
 

void CrfSEDumperPrivate::
xmlOutput(std::ostream& out,
          AnalysisContent& analysis,
          AnalysisGraph* anagraph,
          AnalysisGraph* posgraph,
          const Common::AnnotationGraphs::AnnotationData* annotationData) const
{
  DUMPERLOGINIT;
  
  LinguisticMetaData* metadata=static_cast<LinguisticMetaData*>(analysis.getData("LinguisticMetaData"));

  const FsaStringsPool& sp=Common::MediaticData::MediaticData::single().stringsPool(m_language);


  LinguisticGraph* graph=anagraph->getGraph();
  LinguisticGraphVertexIt it,itEnd;
  VertexTokenPropertyMap tokenMap=get(vertex_token,*graph);
  boost::tie(it,itEnd)=vertices(*graph);
  LinguisticGraphVertex v;
  LinguisticAnalysisStructure::Token* t;
  for (;it!=itEnd;it++)
  {
    v=*it;
    t=get(vertex_token,*graph,v);
    Token* currentToken=tokenMap[*it];
    if (currentToken!=0) {
      MorphoSyntacticData* data=get(vertex_data,*(anagraph->getGraph()),v);
    }
   
  }


  SegmentationData* sb(0);
  if (m_outputSentenceBoundaries) {
    SegmentationData* sb=static_cast<SegmentationData*>(analysis.getData("SentenceBoundaries"));
     if (sb==0) {
      LWARN << "no SentenceBoundaries" << LENDL;
    }
  }
  if (sb==0)
  {
    // no sentence bounds : dump all text at once
    xmlOutputVertices(out,
                      anagraph,
                      posgraph,
                      annotationData,
                      anagraph->firstVertex(),
                      anagraph->lastVertex(),
                      sp,
                      metadata->getStartOffset());
  }
  else
  {
    uint64_t nbSentences((sb->getSegments()).size());
    LDEBUG << "CrfSEDumper: "<< nbSentences << " sentences found" << LENDL;
    for (uint64_t i=0; i<nbSentences; i++)
    {
     
      LinguisticGraphVertex sentenceBegin=(sb->getSegments())[i].getFirstVertex();
      LinguisticGraphVertex sentenceEnd=(sb->getSegments())[i].getLastVertex();
      
      LDEBUG << "dump sentence between " << sentenceBegin << " and " << sentenceEnd << LENDL;
      LDEBUG << "dump simple terms for this sentence" << LENDL;
      
      ostringstream oss;
      xmlOutputVertices(oss,
                        anagraph,
                        posgraph,
                        annotationData,
                        sentenceBegin,
                        sentenceEnd,
                        sp,
                        metadata->getStartOffset());
      string str=oss.str();
      if (str.empty()) {
        LDEBUG << "nothing to dump in this sentence" << LENDL;
      }
      else {
        out << str;
      }
    }
  }
}


 
void CrfSEDumperPrivate::
xmlOutputVertices(std::ostream& out,
                  AnalysisGraph* anagraph,
                  AnalysisGraph* posgraph,
                  const Common::AnnotationGraphs::AnnotationData* annotationData,
                  const LinguisticGraphVertex begin,
                  const LinguisticGraphVertex end,
                  const FsaStringsPool& sp,
                  const uint64_t offset) const
{

  DUMPERLOGINIT;
  LDEBUG << "CrfSEDumper: ========================================" << LENDL;
  LDEBUG << "CrfSEDumper: outputXml from vertex "  << begin << " to vertex " << end << LENDL;
  
  LinguisticGraph* graph=posgraph->getGraph();
  LinguisticGraphVertex lastVertex=posgraph->lastVertex();
  
  map<Token*, vector<LinguisticGraphVertex>, lTokenPosition> sortedTokens;

  std::queue<LinguisticGraphVertex> toVisit;
  std::set<LinguisticGraphVertex> visited;
  
  LinguisticGraphOutEdgeIt outItr,outItrEnd;
 
  // output vertices between begin and end,
  // but do not include begin (beginning of text or previous end of sentence) and include end (end of sentence)
  toVisit.push(begin);

  bool first=true;
  bool last=false;
  while (!toVisit.empty()) {
    LinguisticGraphVertex v=toVisit.front();
    toVisit.pop();
    if (last || v == lastVertex) {
      continue;
    }
    if (v == end) {
      last=true;
    }
    
    for (boost::tie(outItr,outItrEnd)=out_edges(v,*graph); outItr!=outItrEnd; outItr++) 
    {
      LinguisticGraphVertex next=target(*outItr,*graph);
      if (visited.find(next)==visited.end())
      {
        visited.insert(next);
        toVisit.push(next);
      }
    }
    
    if (first) {
      first=false;
    }
    else {
      Token* t=get(vertex_token,*graph,v);
      if( t!=0) {
        sortedTokens[t].push_back(v);
      }
    }
  }
  
  for (map< Token*,vector<LinguisticGraphVertex>,lTokenPosition >::const_iterator
         it=sortedTokens.begin(),it_end=sortedTokens.end(); it!=it_end; it++)
  {
    if ((*it).second.size()==0) {
      continue;
    }
    
    // if several interpretation of the token (i.e several LinguisticGraphVertex associated),
    // it is not a specific entity, => then just print all interpretations
    else if ((*it).second.size()>1) {
      for (vector<LinguisticGraphVertex>::const_iterator  d=(*it).second.begin(),
             d_end=(*it).second.end(); d!=d_end; d++) {
	xmlOutputVertexInfos(out,*d,posgraph,offset);
      }
    }
    else {
      xmlOutputVertex(out,(*it).second[0],anagraph,posgraph,annotationData,sp,offset);
    }
  }
}
 
void CrfSEDumperPrivate::
xmlOutputVertex(std::ostream& out, 
                LinguisticGraphVertex v,
                AnalysisGraph* anagraph,
                AnalysisGraph* posgraph,
                const Common::AnnotationGraphs::AnnotationData* annotationData,
                const FsaStringsPool& sp,
                uint64_t offset) const
{

  std::string value;
  std::vector<std::string> line;
  std::vector<std::string> line2;
  AbstractFeatureExtractor *abExtr;
  abExtr=m_features.front();
  value=xmlString(abExtr->getValue(posgraph,v));
 
  // extract grouped Specific Entities
  std::set< AnnotationGraphVertex > anaVertices = annotationData->matches("PosGraph",v,"AnalysisGraph");
  // note: anaVertices size should be 0 or 1
  for (std::set< AnnotationGraphVertex >::const_iterator anaVerticesIt = anaVertices.begin();
       anaVerticesIt != anaVertices.end(); anaVerticesIt++)
    {
      std::set< AnnotationGraphVertex > matches = annotationData->matches("AnalysisGraph",*anaVerticesIt,"annot");
      for (std::set< AnnotationGraphVertex >::const_iterator it = matches.begin();
	   it != matches.end(); it++)
	{
	  AnnotationGraphVertex vx=*it;
	  if (annotationData->hasAnnotation(vx, Common::Misc::utf8stdstring2limastring("SpecificEntity")))
	    {
	      const SpecificEntityAnnotation* se =
		annotationData->annotation(vx, Common::Misc::utf8stdstring2limastring("SpecificEntity")).
		pointerValue<SpecificEntityAnnotation>();
	      if (se == 0) {
		DUMPERLOGINIT;
		LERROR << "missing specific entity annotation" << LENDL;
	      }
	      for (std::vector< LinguisticGraphVertex>::const_iterator m(se->m_vertices.begin());
		   m != se->m_vertices.end(); m++)
		{
		  const Token* token = get(vertex_token, *(anagraph->getGraph()), *m);
		  if (token != 0) {
		    
		    uint64_t position=token->position() + offset;
		  
		    std::string resline = Common::Misc::limastring2utf8stdstring(token->stringForm()) ;
		   
		    line2.push_back(resline);
		   
		   
		  }
		}
	    }
	}
    }
  
  if (line2.size()>1) {
    line=line2;
  } else {
    boost::replace_all(value, " ", "_");
    line.push_back(value);
  }
  
  
  for (uint k=0; k<line.size(); k++) {
    out << line[k] << " " ;
    for (uint j=1; j<m_features.size(); j++) {
      std::string val;
      if (m_features[j]->getName()=="position") {
	unsigned int pos=atoi(m_features[j]->getValue(posgraph,v).c_str());
	pos+=offset;
	ostringstream oss;
	oss << pos;
	val=oss.str();
      }
      else {
	val=xmlString(m_features[j]->getValue(posgraph,v));
	if (val=="") {
	  val="NAN";
	}
      }
      
      // add B- I- to the Specific Entities
      if (j==m_features.size()-1 && val!="NAN") {
	istringstream iss( val ); 
	std::string mot; 
	std::vector<std::string> resvalue;
	while ( std::getline( iss, mot, '.' ) ) {
	  resvalue.push_back(mot); 
	} 
	if (k==0) {
	  val=resvalue[0]+".B-"+resvalue[1];
	} else {
	  val=resvalue[0]+".I-"+resvalue[1];
	}
	
      }
      out << val << " ";
    }
    out << endl;
  }
  
}


void CrfSEDumperPrivate::xmlOutputVertexInfos(std::ostream& out, 
                                           LinguisticGraphVertex v,
                                           LinguisticAnalysisStructure::AnalysisGraph* graph,
                                           uint64_t offset) const
{
  for (vector<AbstractFeatureExtractor*>::const_iterator it=m_features.begin(),
       it_end=m_features.end(); it!=it_end; it++) {
    std::string value;
    // for position, correct with offset : hard coded name
    if ((*it)->getName()=="position") {
      unsigned int pos=atoi((*it)->getValue(graph,v).c_str());
      pos+=offset;
      ostringstream oss;
      oss << pos;
      value=oss.str();
    }
    else {
      value=xmlString((*it)->getValue(graph,v));
    }
    out << value << " ";
  }
  out  << endl;
}        



std::string CrfSEDumperPrivate::xmlString(const std::string& inputStr) const
{
  // protect XML entities
  std::string str(inputStr);
 
  return str;
}


void CrfSEDumperPrivate::replace(std::string& str, 
                              const std::string& toReplace, 
                              const std::string& newValue) const
{
  string::size_type oldLen=toReplace.size();
  string::size_type newLen=newValue.size();
  string::size_type i=str.find(toReplace);
  while (i!=string::npos) {
    str.replace(i,oldLen,newValue);
    i+=newLen;
    i=str.find(toReplace,i);
  }
}


} // AnalysisDumper
} // LinguisticProcessing
} // Lima
