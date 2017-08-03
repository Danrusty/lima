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

// NAUTITIA
//
// jys 24-JUL-2002
//
// Tokenizer is the main program of Tokenizer stuff.

#include "Tokenizer.h"

#include "Automaton.h"
#include "State.h"
// #include "linguisticProcessing/core/Tokenizer/CharChart.h"
#include "common/misc/Exceptions.h"
#include "common/Data/strwstrtools.h"
#include "common/AbstractFactoryPattern/SimpleFactory.h"
#include "linguisticProcessing/core/LinguisticResources/LinguisticResources.h"
#include "linguisticProcessing/common/linguisticData/LimaStringText.h"
#include "linguisticProcessing/core/LinguisticAnalysisStructure/AnalysisGraph.h"
#include "common/XMLConfigurationFiles/xmlConfigurationFileExceptions.h"
#include "common/tools/FileUtils.h"
#include "common/MediaticData/mediaticData.h"
#include "common/time/timeUtilsController.h"
#include <string>

using namespace std;
using namespace Lima::LinguisticProcessing::LinguisticAnalysisStructure;
using namespace Lima::Common::XMLConfigurationFiles;
using namespace Lima::Common::Misc;

namespace Lima
{
namespace LinguisticProcessing
{
namespace FlatTokenizer
{

typedef SimpleFactory<MediaProcessUnit,Tokenizer> TokenizerFactory;
Q_GLOBAL_STATIC_WITH_ARGS(TokenizerFactory,
                          tokenizerFactory,
                          (FLATTOKENIZER_CLASSID));

class TokenizerPrivate
{
public:
  TokenizerPrivate();
  virtual ~TokenizerPrivate();

  Automaton _automaton;
  CharChart* _charChart;
  MediaId _language;

};

TokenizerPrivate::TokenizerPrivate() : _automaton()
{
  LIMA_UNUSED(tokenizerFactory);
}

TokenizerPrivate::~TokenizerPrivate()
{
}

Tokenizer::Tokenizer() : m_d(new TokenizerPrivate())
{}

Tokenizer::~Tokenizer()
{
  delete m_d;
}

const CharChart* Tokenizer::charChart() const {return m_d->_charChart;}
CharChart* Tokenizer::charChart() {return m_d->_charChart;}
void Tokenizer::setCharChart(CharChart* charChart) {m_d->_charChart = charChart;}

void Tokenizer::init(
  Common::XMLConfigurationFiles::GroupConfigurationStructure& unitConfiguration,
  Manager* manager)

{
#ifdef DEBUG_LP
  TOKENIZERLOGINIT;
  LDEBUG << "Tokenizer::init";
#endif
  m_d->_language=manager->getInitializationParameters().media;
  try
  {
    string charchartId=unitConfiguration.getParamsValueAtKey("charChart");
    AbstractResource* res=LinguisticResources::single().getResource(m_d->_language,charchartId);
    m_d->_charChart=static_cast<CharChart*>(res);
  }
  catch (NoSuchParam& )
  {
    TOKENIZERLOGINIT;
    LERROR << "no param 'charChart' in Tokenizer group configuration (language="
    << (int) m_d->_language << ")";
    throw InvalidConfiguration();
  }

  try
  {
    QString fileName=Common::Misc::findFileInPaths(Common::MediaticData::MediaticData::single().getResourcesPath().c_str(),unitConfiguration.getParamsValueAtKey("automatonFile").c_str());
    m_d->_automaton.setCharChart(m_d->_charChart);
    m_d->_automaton.loadFromFile(fileName.toUtf8().constData());
  }
  catch (NoSuchParam& )
  {
    TOKENIZERLOGINIT;
    LERROR << "no param 'automatonFile' in Tokenizer group configuration (language="
    << (int) m_d->_language << ")";
    throw InvalidConfiguration();
  }
  // when input XML file is syntactically wrong
  catch (XmlSyntaxException exc)
  {
    std::ostringstream mess;
    mess << "XmlSyntaxException at line "<<exc._lineNumber<<" cause: ";
    switch (exc._why)
    {
        case XmlSyntaxException::SYNTAX_EXC : mess << "SYNTAX_EXC"; break;
        case XmlSyntaxException::NO_DATA_EXC : mess << "NO_DATA_EXC"; break;
        case XmlSyntaxException::DOUBLE_EXC : mess << "DOUBLE_EXC"; break;
        case XmlSyntaxException::FWD_CLASS_EXC : mess << "FWD_CLASS_EXC"; break;
        case XmlSyntaxException::MULT_CLASS_EXC : mess << "MULT_CLASS_EXC"; break;
        case XmlSyntaxException::EOF_EXC : mess << "EOF_EXC"; break;
        case XmlSyntaxException::NO_CODE_EXC : mess << "NO_CODE_EXC"; break;
        case XmlSyntaxException::BAD_CODE_EXC : mess << "BAD_CODE_EXC"; break;
        case XmlSyntaxException::NO_CLASS_EXC : mess << "NO_CLASS_EXC"; break;
        case XmlSyntaxException::UNK_CLASS_EXC : mess << "UNK_CLASS_EXC"; break;
        case XmlSyntaxException::INT_ERROR_EXC : mess << "INT_ERROR_EXC"; break;
        case XmlSyntaxException::INV_CLASS_EXC : mess << "INV_CLASS_EXC"; break;
        default: mess << "??";
    }
    TOKENIZERLOGINIT;
    LERROR << mess.str();
    throw InvalidConfiguration();
  }
  catch (std::exception &exc)
  {
    // @todo remove all causes of InfiniteLoopException
    TOKENIZERLOGINIT;
    LERROR << exc.what();
    throw InvalidConfiguration();
  }

}

LimaStatusCode Tokenizer::process(
  AnalysisContent& analysis) const
{
  TimeUtilsController flatTokenizerProcessTime("FlatTokenizer");
  TOKENIZERLOGINIT;
  LINFO << "start tokenizer process";
  LimaStringText* originalText=static_cast<LimaStringText*>(analysis.getData("Text"));
  AnalysisGraph* anagraph(0);

  anagraph=new AnalysisGraph("AnalysisGraph",m_d->_language,true,true);
  analysis.setData("AnalysisGraph",anagraph);
  LinguisticGraph* graph=anagraph->getGraph();
  // Gets transformed file into characters class string
  Text* text=new Text(m_d->_language, m_d->_charChart);
  text->setText(*originalText);
  text->setGraph(anagraph->firstVertex(),graph);

  LINFO << "Running automaton on" << *originalText;
  const State* newState = m_d->_automaton.run(*text);
  while (newState)
  {
    LTRACE << "Running automaton";
    newState = m_d->_automaton.run(*text, newState);
  }
  if (text->position() < text->size()-1)
  {
    LERROR << "Tokenized up to " << text->position()
        << " for a text of size " << text->size() ;
  }
  text->finalizeAndUnsetGraph();
  remove_edge(anagraph->firstVertex(),anagraph->lastVertex(),*(anagraph->getGraph()));
  delete text;
  return SUCCESS_ID;
}

} //namespace FlatTokenizer
} // namespace LinguisticProcessing
} // namespace Lima
