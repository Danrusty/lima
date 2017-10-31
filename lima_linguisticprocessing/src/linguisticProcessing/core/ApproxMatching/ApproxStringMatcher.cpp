/*
    Copyright 2002-2017 CEA LIST

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

#include "ApproxStringMatcher.h"

#include "linguisticProcessing/LinguisticProcessingCommon.h"
#include "common/MediaticData/mediaticData.h"
#include "common/time/timeUtilsController.h"
#include "common/XMLConfigurationFiles/xmlConfigurationFileExceptions.h"
#include "linguisticProcessing/common/annotationGraph/AnnotationData.h"
#include "linguisticProcessing/core/LinguisticAnalysisStructure/Token.h"
#include "linguisticProcessing/core/LinguisticAnalysisStructure/AnalysisGraph.h"
#include "linguisticProcessing/core/LinguisticResources/LinguisticResources.h"
#include "linguisticProcessing/core/AnalysisDict/AbstractDictionaryEntry.h"
#include "common/AbstractFactoryPattern/SimpleFactory.h"
#include "common/misc/fsaStringsPool.h"
#include "common/misc/AbstractAccessIterators.h"
#include "common/Data/strwstrtools.h"
#include <iostream>
#include <tre/regex.h>

using namespace std;
using namespace Lima::Common::AnnotationGraphs;
using namespace Lima::Common::MediaticData;
using namespace Lima::Common::XMLConfigurationFiles;
using namespace Lima::LinguisticProcessing::LinguisticAnalysisStructure;
using namespace Lima::LinguisticProcessing::AnalysisDict;

namespace Lima
{
namespace LinguisticProcessing
{
namespace MorphologicAnalysis
{

SimpleFactory<MediaProcessUnit,ApproxStringMatcher> ApproxStringMatcherFactory(APPROX_STRING_MATCHER_CLASSID);

ApproxStringMatcher::ApproxStringMatcher() :
    m_nbMaxError(0)
{}

ApproxStringMatcher::~ApproxStringMatcher()
{
  // delete m_reader;
}

void ApproxStringMatcher::init(
  Common::XMLConfigurationFiles::GroupConfigurationStructure& unitConfiguration,
  Manager* manager)
{
  MORPHOLOGINIT;
  MediaId language = manager->getInitializationParameters().media;
  // m_sp=&Common::MediaticData::MediaticData::changeable().stringsPool(language);
  string dico;
  try
  {
    dico=unitConfiguration.getParamsValueAtKey("dictionary");
  }
  catch (NoSuchParam& )
  {
    LERROR << "no param 'dictionary' in ApproxStringMatcher group for language " << (int) language;
    throw InvalidConfiguration();
  }
  try
  {
    std::string nbMaxErrorStr=unitConfiguration.getParamsValueAtKey("nbMaxError");
    std::istringstream iss(nbMaxErrorStr);
    iss >> m_nbMaxError;
  }
  catch (NoSuchParam& )
  {
    LERROR << "no param 'nbMaxError' in ApproxStringMatcher group for language " << (int) language;
    throw InvalidConfiguration();
  }

  AbstractResource* res=LinguisticResources::single().getResource(language,dico);
  AbstractAccessResource* lexicon = lexicon=static_cast<AbstractAccessResource*>(res);
  m_lexicon = lexicon->getAccessByString();
}


LimaStatusCode ApproxStringMatcher::process(
  AnalysisContent& analysis) const
{
  Lima::TimeUtilsController timer("ApproxStringMatcher");
  MORPHOLOGINIT;
  LINFO << "starting process ApproxStringMatcher";

  // Initalize list of suggestions, ordered by number of errors
  std::multimap<int,Suggestion> result;
  
  AnalysisGraph* tokenList=static_cast<AnalysisGraph*>(analysis.getData("AnalysisGraph"));

  LinguisticGraph* g=tokenList->getGraph();
  LinguisticGraphVertexIt it,itEnd;
  boost::tie(it,itEnd)=vertices(*g);
  for (;it!=itEnd;it++)
  {
    //matchExactTokenAndFollowers(*g, it, itEnd, result);
    Suggestion suggestion;
    matchApproxTokenAndFollowers(*g, it, itEnd, suggestion);
    result.insert(std::pair<int,Suggestion>(suggestion.nb_error,suggestion));
  }
  
  AnnotationData* annotationData = static_cast< AnnotationData* >(analysis.getData("AnnotationData"));
  if (annotationData==0)
  {
    LINFO << "ApproxStringMatcher::process no annotation data, creating and populating it";
    annotationData=new AnnotationData();
    analysis.setData("AnnotationData",annotationData);
  }
  tokenList->populateAnnotationGraph(annotationData, "AnalysisGraph");
  
#ifdef DEBUG_LP
  LDEBUG << "ending process ApproxStringMatcher";
#endif
  return SUCCESS_ID;
}

LimaStatusCode ApproxStringMatcher::matchExactTokenAndFollowers(
    LinguisticGraph& g, 
    LinguisticGraphVertexIt vStartIt,
    LinguisticGraphVertexIt vEndIt,
    std::multimap<int,Suggestion>& result) const
{
  MORPHOLOGINIT;
  typedef Lima::Common::AccessSuperWordIterator WIt;
  VertexTokenPropertyMap tokenMap=get(vertex_token,g);
  // VertexDataPropertyMap dataMap=get(vertex_data,g);
    Token* currentToken=tokenMap[*vStartIt];
    LimaString form;
    int max_length_element_in_lexicon = 49;
    // TODO: check if it is the case of start or end node in graph...
    if (currentToken==0)
      return SUCCESS_ID;
    int position = currentToken->position();
#ifdef DEBUG_LP
    LDEBUG << "ApproxStringMatcher::matchTokenAndFollowers() from token" << Lima::Common::Misc::limastring2utf8stdstring(currentToken->stringForm());
#endif
    // TODO: vérifier que vEndIt est le noeud 1 (sans token)
    for( ; vStartIt != vEndIt ; vStartIt++ ) {
      currentToken=tokenMap[*vStartIt];
      if (currentToken!=0)
      {
        // currentToken->status()?
#ifdef DEBUG_LP
        LDEBUG << "ApproxStringMatcher::matchTokenAndFollowers() append token" << Lima::Common::Misc::limastring2utf8stdstring(currentToken->stringForm());
#endif
        form.append(currentToken->stringForm());
#ifdef DEBUG_LP
        LDEBUG << "ApproxStringMatcher::matchTokenAndFollowers() form= "
               << Lima::Common::Misc::limastring2utf8stdstring(form);
#endif
      // get words in Lexicon with form as prefix
        std::pair<WIt,WIt>  wordsIt = m_lexicon->getSuperWords(form);
        for( ; wordsIt.first != wordsIt.second ; (wordsIt.first)++ ) 
        {
          Lima::LimaString word = *(wordsIt.first);
#ifdef DEBUG_LP
          LDEBUG << "ApproxStringMatcher::matchTokenAndFollowers() first superWords = "
                 << Lima::Common::Misc::limastring2utf8stdstring(word);
          LDEBUG << "ApproxStringMatcher::matchTokenAndFollowers(): compare to "
                 << Lima::Common::Misc::limastring2utf8stdstring(form) << "=" << word.compare(form);
#endif
          if( word.compare(form) == 0 ) {
            Suggestion suggestion={
              position,                     // startPsition
              position+form.length(),       // endPosition
              0,                            // nb_error
              m_lexicon->getIndex(form)};   // id of term in Lexicon
#ifdef DEBUG_LP
            LDEBUG << "ApproxStringMatcher::matchTokenAndFollowers() success: suggestion= \n"
                  << "{ position=" << suggestion.startPosition
                  << ", length=" << suggestion.endPosition
                  << ", nb_error=" << suggestion.nb_error
                  << ", match_id=" << suggestion.match_id << "}";
#endif
            result.insert(std::pair<int,Suggestion>(suggestion.nb_error,suggestion));
            break;
          }
        }
        if( form.length() > max_length_element_in_lexicon + m_nbMaxError ) {
          break;
        }
      }
    }
  return SUCCESS_ID;
}

LimaStatusCode ApproxStringMatcher::matchApproxTokenAndFollowers(
    LinguisticGraph& g, 
    LinguisticGraphVertexIt vStartIt,
    LinguisticGraphVertexIt vEndIt,
    Suggestion& result) const
{
  typedef Lima::Common::AccessSuperWordIterator WIt;
  MORPHOLOGINIT;
  VertexTokenPropertyMap tokenMap=get(vertex_token,g);
  LimaStatusCode returnValue = SUCCESS_ID;
  result.nb_error = 1000;
  // VertexDataPropertyMap dataMap=get(vertex_data,g);

  // Build string where search will be done
  // Build also a map of position of token in this string
  // Because the string formed is not identical to the original text!
  LimaString form;
  // Position of tokens in form
  std::deque<int> tokenStartPos;
  std::deque<int> tokenEndPos;
  int max_length_element_in_lexicon = 49;
  Token* currentToken=tokenMap[*vStartIt];
  // position of form in original text
  int textStart, textEnd;
  // TODO: vérifier que vEndIt est le noeud 1 (sans token)
  LinguisticGraphVertexIt vIt= vStartIt;
  for( ; vIt != vEndIt ; vIt++ ) {
    currentToken=tokenMap[*vIt];
#ifdef DEBUG_LP
    LDEBUG << "ApproxStringMatcher::matchTokenAndFollowers() from " << *vIt;
#endif
    if (currentToken!=0)
    {
      if( tokenStartPos.empty() ) {
        tokenStartPos.push_back(0);
        tokenEndPos.push_back(currentToken->length());
        textStart = currentToken->position();
        // textEnd = textStart+currentToken->stringForm().length(); ???
        textEnd = currentToken->position() + currentToken->length();
      }
      else {
        int currentStartPos = tokenEndPos.back();
        // TODO: add space when previous and current tokens are not contiguous
        if( currentToken->position() > textEnd) {
          currentStartPos++;
          form.append(" ");
        }
        tokenStartPos.push_back(currentStartPos);
        tokenEndPos.push_back(currentStartPos+currentToken->length());
        textEnd = currentToken->position() + currentToken->length();
      }
#ifdef DEBUG_LP
      LDEBUG << "ApproxStringMatcher::matchTokenAndFollowers() posInform= "
            << tokenStartPos.back() << "," << tokenEndPos.back();
#endif
      assert( currentToken->length() == currentToken->stringForm().length());
      form.append(currentToken->stringForm());
#ifdef DEBUG_LP
      LDEBUG << "ApproxStringMatcher::matchTokenAndFollowers() form= "
            << Lima::Common::Misc::limastring2utf8stdstring(form);
#endif
    }
    if( form.length() > max_length_element_in_lexicon + m_nbMaxError ) {
      break;
    }
  }
  // build pattern from word in lexicon
  std::pair<WIt,WIt>  wordsIt = m_lexicon->getSuperWords("");
  for( ; wordsIt.first != wordsIt.second ; (wordsIt.first)++ ) {
    LimaString pattern = *(wordsIt.first);
    // Search for pattern in form
    Suggestion suggestion;
    int ret = findApproxPattern( pattern, form, suggestion);
    // keep suggestion if best
    if( (ret == 0) && (suggestion.nb_error < result.nb_error) ) {
#ifdef DEBUG_LP
      LDEBUG << "ApproxStringMatcher::matchTokenAndFollowers() suggestion={"
            << "start:" << suggestion.startPosition << ","
            << "end:" << suggestion.endPosition << ","
            << "error:" << suggestion.nb_error << "}";
#endif
      result = suggestion;
      result.startPosition = -1;
      result.endPosition = -1;
      // exact position in text
      int startInForm(0);
      int endInForm(0);
      for( vIt = vStartIt ; vIt != vEndIt ; vIt++ ) {
        currentToken=tokenMap[*vIt];
        startInForm = tokenStartPos.front();
        endInForm = tokenEndPos.front();
        if (currentToken!=0)
        {
#ifdef DEBUG_LP
          LDEBUG << "ApproxStringMatcher::matchTokenAndFollowers() compare with (start,end)="
                 << "(" << startInForm
                 << "," << endInForm << "}";
#endif
          if( result.startPosition == -1 ) {
            if( ( suggestion.startPosition >= startInForm ) && ( suggestion.startPosition < endInForm ) ){
              result.startPosition = currentToken->position();
            }
          }
          if( result.endPosition == -1 ) {
            if( ( suggestion.endPosition >= startInForm ) && ( suggestion.endPosition <= endInForm ) ){
              result.endPosition = currentToken->position()+currentToken->length();
            }
          }
          if( (result.startPosition != -1) && (result.endPosition != -1) ) {
            break;
          }
          tokenStartPos.pop_front();
          tokenEndPos.pop_front();
        }
      }
      #ifdef DEBUG_LP
      LDEBUG << "ApproxStringMatcher::matchTokenAndFollowers() result={"
            << "start:" << result.startPosition << ","
            << "end:" << result.endPosition
            << "error:" << result.nb_error << "}";
#endif

    }
  }
  return returnValue;
}

int ApproxStringMatcher::findApproxPattern(
    LimaString pattern, LimaString text,
    Suggestion& suggestion) const {
  MORPHOLOGINIT;
#ifdef DEBUG_LP
      LDEBUG << "ApproxStringMatcher::findApproxPattern("
             << Lima::Common::Misc::limastring2utf8stdstring(pattern) << ","
             << Lima::Common::Misc::limastring2utf8stdstring(text) << ")";
      #endif

    // pattern buffer structure (result of compilation)
    regex_t preg;
    int cflags = REG_EXTENDED|REG_ICASE|REG_NEWLINE;
    // cflags |= REG_NOSUB;
    // cflags |= REG_LITERAL;
    // cflags |= REG_RIGHT_ASSOC;
    // cflags |= REG_UNGREEDY;
    
    // Compile pattern
    wchar_t parray[pattern.length()];
    int plength = pattern.toWCharArray(parray);
    /*
#ifdef DEBUG_LP
      LDEBUG << "ApproxStringMatcher::findApproxPattern: compilation...";
#endif
      */
    int agrepStatus = regwncomp(&preg, parray, plength, cflags);
    // TODO: agrepStatus???
#ifdef DEBUG_LP
    LDEBUG << "ApproxStringMatcher::findApproxPattern: agrepStatus=" << agrepStatus;
#endif

    regaparams_t params = {
      1,    // int cost_ins;
      1,    // int cost_del;
      1,    // int cost_subst;
      m_nbMaxError,    // int max_cost;
      m_nbMaxError,    // int max_ins;
      m_nbMaxError,    // int max_del;
      m_nbMaxError,    // int max_subst;
      m_nbMaxError,    // int max_err;
    };
    int eflags = REG_NOTBOL;
    // eflags |= REGNOTEOL;
    // pmatch has more than 1 emplacement to be filled with match of subexpression (parenthese expr. in pattern)
    const size_t MAX_MATCH=10;
    regmatch_t pmatch[MAX_MATCH];
    regamatch_t amatch = {
      MAX_MATCH,    // size_t nmatch;
      pmatch,  // regmatch_t *pmatch
      0,    // int cost;
      0,    // int num_ins;
      0,    // int num_del;
      0,    //int num_subst;
    };
    wchar_t tarray[text.length()];
    int tlength = text.toWCharArray(tarray);
    /*
#ifdef DEBUG_LP
    LDEBUG << "ApproxStringMatcher::findApproxPattern: execution...";
#endif
    */
    int execStatus = regawnexec(&preg, tarray, tlength,
                &amatch, params, eflags);
#ifdef DEBUG_LP
      LDEBUG << "ApproxStringMatcher::findApproxPattern: execStatus=" << execStatus;
#endif
    if( execStatus == 0 ) {
      regmatch_t* current_match=&(pmatch[0]);
      suggestion.startPosition = current_match->rm_so;
      suggestion.endPosition = current_match->rm_eo;
      suggestion.nb_error = amatch.num_del+amatch.num_ins;
      // suggestion.match_id???
    }
    return execStatus;
}


} // MorphologicAnalysis
} // LinguisticProcessing
} // Lima
