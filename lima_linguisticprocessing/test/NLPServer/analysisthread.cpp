/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

#include "analysisthread.h"

//#include "linguisticProcessing/core/AnalysisDumper/JsonResults.h"

#include "common/MediaticData/mediaticData.h"
#include "common/Data/strwstrtools.h"
#include "common/Handler/AbstractXmlDocumentHandler.h"
#include "linguisticProcessing/client/AnalysisHandlers/SimpleStreamHandler.h"
 #include "linguisticProcessing/client/AnalysisHandlers/BowTextWriter.h"
#include "linguisticProcessing/client/AnalysisHandlers/BowTextHandler.h"

// factories
#include "linguisticProcessing/client/LinguisticProcessingClientFactory.h"

#include <qhttprequest.h>
#include <qhttpresponse.h>

#include <QStringList>
#include <QPair>
#include <boost/graph/buffer_concepts.hpp>

#include <fstream>
#include <sstream>      // std::stringstream
#include <QtCore/QString>
#include <QtCore/QTemporaryFile>

using namespace Lima;
using namespace Lima::Common;
using namespace Lima::Common::XMLConfigurationFiles;


class AnalysisThreadPrivate
{
friend class AnalysisThread;
public:
  AnalysisThreadPrivate (Lima::LinguisticProcessing::AbstractLinguisticProcessingClient* m_analyzer, 
                        QHttpRequest *req, QHttpResponse *resp,
                        const std::set<std::string>& langs);
  ~AnalysisThreadPrivate() {}
  
  Lima::LinguisticProcessing::AbstractLinguisticProcessingClient* m_analyzer;
  QHttpRequest* m_request;
  QHttpResponse* m_response;
  const std::set<std::string>& m_langs;
};

AnalysisThreadPrivate::AnalysisThreadPrivate(Lima::LinguisticProcessing::AbstractLinguisticProcessingClient* analyzer, 
                    QHttpRequest *req, QHttpResponse *resp,
                    const std::set<std::string>& langs) :
    m_analyzer(analyzer),
    m_request(req), m_response(resp),
    m_langs(langs)
{
}

AnalysisThread::AnalysisThread (Lima::LinguisticProcessing::AbstractLinguisticProcessingClient* analyzer, 
                  QHttpRequest *req, QHttpResponse *resp, 
                  const std::set<std::string>& langs, QObject* parent ):
    QThread(parent),
    m_d(new AnalysisThreadPrivate(analyzer,req,resp,langs))
{
  CORECLIENTLOGINIT;
  LDEBUG << "AnalysisThread::AnalysisThread()...";
  connect(this,SIGNAL(started()),this,SLOT(slotStarted()),Qt::QueuedConnection);
}

AnalysisThread::~AnalysisThread()
{
  delete m_d;
}

void AnalysisThread::slotStarted()
{
  CORECLIENTLOGINIT;
  LDEBUG << "AnalysisThread::slotStarted";

}

void AnalysisThread::startAnalysis()
{
  CORECLIENTLOGINIT;
  LDEBUG << "AnalysisThread::startAnalysis" << m_d->m_request->methodString() << m_d->m_request->url().path();
  // TODO: Could we factoriez code between GET and POST method?
  // First case = GET
  if (m_d->m_request->methodString() == "HTTP_GET" && m_d->m_request->url().path() == "/annot")
  {
    // Arguments to be read from the request
    QString language, pipeline, text;
    QPair<QString, QString> item;
    std::map<std::string,std::string> metaData;
    std::set<std::string> inactiveUnits;
    
    LDEBUG << "AnalysisThread::startAnalysis: process annot request (mode HTTP_GET)";
    
    std::map<std::string, AbstractAnalysisHandler*> handlers;
    LinguisticProcessing::SimpleStreamHandler* seLogWriter = new LinguisticProcessing::SimpleStreamHandler();
    handlers.insert(std::make_pair("se", seLogWriter));

    // OME?? Dont forget to delete it!!!
    std::ostringstream* oss = new std::ostringstream();
    seLogWriter->setOut(oss);
   
    // read arguments from the request
    Q_FOREACH( item, m_d->m_request->url().queryItems())
    {
      QTemporaryFile tempFile;
      metaData["FileName"]=tempFile.fileName().toUtf8().constData();
      if (item.first == "lang")
      {
        language = item.second;
        metaData["Lang"]=language.toUtf8().data();
        LDEBUG << "AnalysisThread::startAnalysis: " << "language=" << language;
      }
      if (item.first == "pipeline")
      {
        pipeline = item.second;
        LDEBUG << "AnalysisThread::startAnalysis: " << "pipeline=" << pipeline;
      }
      if (item.first == "text")
      {
        text = item.second;
        LDEBUG << "AnalysisThread::startAnalysis: " << "text='" << text << "'";
      }
    }
    if( m_d->m_langs.find(metaData["Lang"]) == m_d->m_langs.end() )
    {
        m_d->m_response->writeHead(400);
        QString errorMessage = QString(tr("Language %1 is no initialized")).arg(language);
        m_d->m_response->end(errorMessage.toUtf8());
    }
    else if( !language.isEmpty() && !text.isEmpty() )
    {
      LDEBUG << "Analyzing" << language << text;
      std::ostringstream ots;
      std::string pipe = pipeline.toUtf8().data();
      m_d->m_analyzer->analyze(text,metaData, pipe, handlers, inactiveUnits);

      std::string resultString("<?xml version='1.0' encoding='UTF-8'?>");
      resultString.append(oss->str());
      LDEBUG << "AnalysisThread::startAnalysis: seLogger output is " << QString::fromUtf8(resultString.c_str());

      m_d->m_response->setHeader("Content-Type", "text/xml; charset=utf-8");
      m_d->m_response->writeHead(200);

//      QString body = QString::fromUtf8(resultString.c_str());
//      m_d->m_response->end(body.toUtf8());
      m_d->m_response->end(QByteArray(resultString.c_str()));
      m_d->m_request->deleteLater();
      deleteLater();
      quit();
    }
    else
    {
        m_d->m_response->writeHead(400);
        m_d->m_response->end(QByteArray("Empty language or text"));
    }
    LDEBUG << "AnalysisThread::startAnalysis: delete oss... ";
    delete seLogWriter;
    if( oss != 0 ) {
      delete oss;
      oss = 0;
    }
  }
  // commande HTTP_POST
  else if (m_d->m_request->methodString() == "HTTP_POST" && m_d->m_request->url().path() == "/annot")
  {
    LDEBUG << "AnalysisThread::startAnalysis: process extractEN request (mode HTTP_POST)";

    QString language, pipeline, text_QS;
    std::string text_s;
    QPair<QString, QString> item;
    std::map<std::string,std::string> metaData;
    Q_FOREACH( item, m_d->m_request->url().queryItems())
    {
      if (item.first == "lang")
      {
        language = item.second;
        metaData["Lang"]=language.toUtf8().data();
        LDEBUG << "AnalysisThread::startAnalysis: " << "language=" << language;
      }
      if (item.first == "pipeline")
      {
        pipeline = item.second;
        LDEBUG << "AnalysisThread::startAnalysis: " << "pipeline=" << pipeline;
      }
    }

    std::set<std::string> inactiveUnits;
    const QByteArray& body = m_d->m_request->body();
    text_s = std::string(body.data());

    std::map<std::string, AbstractAnalysisHandler*> handlers;
    LinguisticProcessing::SimpleStreamHandler* seLogWriter = new LinguisticProcessing::SimpleStreamHandler();
    handlers.insert(std::make_pair("se", seLogWriter));
    std::ostringstream* oss = new std::ostringstream();
    seLogWriter->setOut(oss);
   
    if( language.isEmpty() )
    {
      m_d->m_response->writeHead(400);
      m_d->m_response->end(QByteArray("Empty language"));
      return;
    }
    else if( m_d->m_langs.find(metaData["Lang"]) == m_d->m_langs.end() )
    {
      m_d->m_response->writeHead(400);
      QString errorMessage = QString("Language %1 no initialized").arg(language);
//  errorMessage << "language " << language " is no initialized"));
      m_d->m_response->end(errorMessage.toUtf8());
    }
    else if( !language.isEmpty() && !text_s.empty() )
    {
      metaData["Lang"]=language.toUtf8().data();
      QTemporaryFile tempFile;
      metaData["FileName"]=tempFile.fileName().toUtf8().constData();
      std::string pipe = pipeline.toUtf8().data();
      text_QS = Misc::utf8stdstring2limastring(text_s);

      m_d->m_analyzer->analyze(text_QS,metaData, pipe, handlers, inactiveUnits);

      std::string resultString("<?xml version='1.0' encoding='UTF-8'?>");
      resultString.append(oss->str());
      LDEBUG << "AnalysisThread::startAnalysis: seLogger output is " << QString::fromUtf8(resultString.c_str());

      m_d->m_response->setHeader("Content-Type", "text/xml; charset=utf-8");
      m_d->m_response->writeHead(200);
//      QString body = QString::fromUtf8(resultString.c_str());
//      m_d->m_response->end(body.toUtf8());
      m_d->m_response->end(QByteArray(resultString.c_str()));
    }
    else
    {
      m_d->m_response->writeHead(400);
      m_d->m_response->end(QByteArray("Empty language or text"));
      return;
    }
    delete seLogWriter;
    if( oss != 0 ) {
      delete oss;
      oss = 0;
    }
  }
  else
  {
    m_d->m_response->writeHead(405);
    m_d->m_response->setHeader("Allow","GET,POST");
    m_d->m_response->end(QByteArray("Only GET and POST search queries are currently allowed."));
  }
}

// #include "searchthread.moc"
