
#include "LimaGuiApplication.h"
#include "ConllParser.h"
#include "Threads.h"

#include "common/MediaticData/mediaticData.h"
#include "linguisticProcessing/client/AnalysisHandlers/SimpleStreamHandler.h"

#include <deque>
#include <iostream>
#include <string>

#include <QFile>

using namespace Lima;
using namespace Lima::Common;
using namespace Lima::Common::MediaticData;
using namespace Lima::Common::XMLConfigurationFiles;
using namespace Lima::LinguisticProcessing;

LimaGuiApplication::LimaGuiApplication(QObject* parent) : QObject(parent) {
  //initializeLimaAnalyzer();
   auto ith = new InitializeThread(this);
   ith->start();
}

/// PUBLIC METHODS

QString qstr_parseFile(std::string path) {
  QString tump;
  QFile file(QString(path.c_str()));
  if (file.open(QFile::ReadOnly)) {
    QTextStream qts(&file);
    tump = qts.readAll();
    file.close();
  }
  else {
    std::cout << "didn't open : " << path << std::endl;
    std::cout << "Error opening file: " << strerror(errno) << std::endl;
  }
  return tump;
}

/// expecting the format : "file:<actual_url>"
std::string cleanUrl(std::string url) {
  std::string str;
  std::vector<std::string> tmpStrList = split(url,':');
  if (!tmpStrList.size()) {
    LTELL("FILE DIALOG URL FORMAT ERROR : " << url);
  }
  else {
    str = tmpStrList[1];
  }
  
  return str;
}

QString cleanUrl(QString url) {
  return QString(cleanUrl(url.toStdString()).c_str());
}

//////////////////////////////
//////////////////////////////
//////////////////////////////
//////////////////////////////

bool LimaGuiApplication::openMultipleFiles(QStringList urls) {
  bool result = false;
  for (const auto& url : urls) {
    result |= openFile(url);
  }
  return result;
}

bool LimaGuiApplication::openFile(QString filepath) {
  LTELL("OPENING FILE");
  /// FileDialog returns something like : "file:///C:/..."
  /// so we need to remove the unnecessary content
  std::vector<std::string> tmpStrList = split(filepath.toStdString(),':');
  if (!tmpStrList.size()) {
    LTELL("FILE DIALOG URL FORMAT ERROR : " << filepath.toStdString());
    return false;
  }
  
  std::string path = tmpStrList[1];
  
  for (auto& file : m_openFiles) {
    if (file.url == path) {
      LTELL("This file is already open.");
      return false;
    }
  }

  tmpStrList = split(path, '/');
  if (!tmpStrList.size()) {
    LTELL("FILE NAME FORMAT ERROR : " << path);
    return false;
  }
  
  std::string filename = tmpStrList[tmpStrList.size() - 1];


  
  m_fileContent = qstr_parseFile(path);
  
  LimaGuiFile lgf;
  lgf.name = filename;
  lgf.url = path;
  
  m_openFiles.push_back(lgf);
  
  m_fileUrl = QString(path.c_str());
  m_fileName = QString(filename.c_str());
  
  /// qml part : open a new tab {title= m_fileName; content=m_fileContent}
  return true;
}

bool LimaGuiApplication::saveFile(QString filename) {
  
  LimaGuiFile* lgfile = getFile(filename.toStdString());
  
  if (!lgfile) {
    LTELL("This file doesn't exist : " << filename.toStdString());
    return false;
  }
  
  QFile file(QString(lgfile->url.c_str()));
  if (file.open(QFile::WriteOnly | QFile::Text)) {
    QTextStream qts(&file);
    qts << m_fileContent;
    file.close();
  }
  else {
    std::cout << "didn't open : " << lgfile->url << std::endl;
    std::cout << "Error opening file: " << strerror(errno) << std::endl;
    return false;
  }
  
  return true;
}

bool LimaGuiApplication::saveFileAs(QString filename, QString newUrl) {
  /// check if file is open
  LimaGuiFile* lgfile = getFile(filename.toStdString());
  
  if (!lgfile) {
    LTELL("This file doesn't exist : " << filename.toStdString());
    return false;
  }
  
  /// check new url
  std::vector<std::string> tmpStrList = split(newUrl.toStdString(),':');
  if (!tmpStrList.size()) {
    LTELL("FILE DIALOG URL FORMAT ERROR : " << newUrl.toStdString());
  }
  
  std::string path = tmpStrList[1];
  
  QFile file(newUrl);
  if (file.open(QFile::WriteOnly | QFile::Text)) {
    QTextStream qts(&file);
    qts << m_fileContent;
    file.close();
  }
  else {
    std::cout << "didn't open : " << newUrl << std::endl;
    std::cout << "Error opening file: " << strerror(errno) << std::endl;
    return false;
  }
  
  return true;
}

void LimaGuiApplication::closeFile(QString filename, bool save) {
  
  if (save) {
    saveFile(filename);
  }  
  
  for (std::vector<LimaGuiFile>::const_iterator it = m_openFiles.begin(); it != m_openFiles.end(); ++it) {
    if (it->name == filename.toStdString()) {
      m_openFiles.erase(it);
      return;
    }
  }
  
  LTELL("This file doesn't exist : " << filename.toStdString());
  /// qml part : close tab
}

LimaGuiFile* LimaGuiApplication::getFile(std::string filename) {
  for (auto& file : m_openFiles) {
    if (file.name == filename) {
      return &file;
    }
  }
  return nullptr;
}


/// ANALYZER METHODS
///

void LimaGuiApplication::analyzeText(QString content, QObject* target) {
  beginNewAnalysis(content, target);
}

void LimaGuiApplication::beginNewAnalysis(QString content, QObject* target) {
  LTELL("ANALYYZING :");
  LTELL(content.toStdString());
  auto at = new AnalysisThread(this, content);
  if (target) at->setResultView(target);
  at->start();
}

void LimaGuiApplication::analyze(QString content) {
  
  // PARAMETERS :
  // Text
 
  // Metadata
  std::map<std::string, std::string> metaData;
  metaData["FileName"] = "";
  metaData["Lang"] = "fre";
  
  // Pipeline
  std::string pipeline = "main";
  
  // Handlers 
  std::set<std::string> dumpers;
  
  dumpers.insert("text");
  
  std::map<std::string, AbstractAnalysisHandler*> handlers;
  
  SimpleStreamHandler* simpleStreamHandler = 0;
  
  if (dumpers.find("text") != dumpers.end())
  {
    simpleStreamHandler = new SimpleStreamHandler();
    simpleStreamHandler->setOut(out);
    handlers.insert(std::make_pair("simpleStreamHandler", simpleStreamHandler));
  }
  
  std::set<std::string> inactiveUnits;
  // QString::fromUtf8(contentText.c_str())
  m_analyzer->analyze(content, metaData, pipeline, handlers, inactiveUnits);
  
  if (simpleStreamHandler)
    delete simpleStreamHandler;
}

void LimaGuiApplication::analyzeFile(QString filename, QObject* target) {
  if (filename != m_fileName) {
    if (!selectFile(filename)) {
      return;
    }
  }
  analyzeText(m_fileContent, target);
}

void LimaGuiApplication::analyzeFileFromUrl(QString url, QObject* target) {
  if (openFile(url)) {
    analyzeText(m_fileContent, target);
    closeFile(m_fileName);
  }
  else {
    LTELL("There was a problem somewhere");
  }
}

bool LimaGuiApplication::selectFile(QString filename) {
  LimaGuiFile* lgf = getFile(filename.toStdString());
  if (lgf) {
    m_fileContent = qstr_parseFile(lgf->url);
    m_fileName = QString(lgf->name.c_str());
    m_fileUrl = QString(lgf->url.c_str());
    return true;
  }
  else {
    LTELL("This file doesn't exist : " << filename.toStdString());
  }
  return false;
}


/// INITIALIZATION METHODS

void LimaGuiApplication::initializeLimaAnalyzer() {
  
  std::string configDir = qgetenv("LIMA_CONF").constData();
  LTELL("Config Dir is " << configDir << "|" << configDir << "|" << configDir);
  if (configDir == "") {
    configDir = "/home/jocelyn/Lima/lima/../Dist/lima-gui/debug/share/config/lima";
  }
  
  std::deque<std::string> langs = {"eng","fre"};
  std::deque<std::string> pipelines = {"main"};
  
  // initialize common
  std::string resourcesPath = qgetenv("LIMA_RESOURCES").constData();
  if( resourcesPath.empty() )
    resourcesPath = "/usr/share/apps/lima/resources/";
  std::string commonConfigFile("lima-common.xml");
  
  std::ostringstream oss;
  std::ostream_iterator<std::string> out_it (oss,", ");
  std::copy ( langs.begin(), langs.end(), out_it );
  Common::MediaticData::MediaticData::changeable().init(
    resourcesPath,
    configDir,
    commonConfigFile,
    langs);
  
  
  std::cout << "Langs:" << std::endl;
  for (unsigned int i = 0;i < langs.size(); i++) {
    std::cout << langs[i] << std::endl;
  }
  
  // initialize linguistic processing
  std::string clientId("lima-coreclient");
  std::string lpConfigFile("lima-analysis.xml");
  Lima::Common::XMLConfigurationFiles::XMLConfigurationFileParser lpconfig(configDir + "/" + lpConfigFile);
  
  LinguisticProcessingClientFactory::changeable().configureClientFactory(
    clientId,
    lpconfig,
    langs,
    pipelines);
  
  m_analyzer = std::dynamic_pointer_cast<AbstractLinguisticProcessingClient>(LinguisticProcessingClientFactory::single().createClient(clientId)); 
  
  std::cout << "Pipelines:" << std::endl;
  for (unsigned int i=0; i < pipelines.size(); i++) {
    std::cout << pipelines[i] << std::endl;
  }
}

void LimaGuiApplication::resetLimaAnalyzer() {
  // delete m_analzer;
  initializeLimaAnalyzer();
}

//LimaGuiThread* LimaGuiApplication::getThread(std::string name) {
//  if (threads.find(name) != threads.end()) {
//    return threads[name];
//  }
//  else {
//    return nullptr;
//  }
//}

//void LimaGuiApplication::newThread(std::string name, LimaGuiThread * lgt) {
//  if (threads.find(name) != threads.end()) {
//    // kill current thread
//  }
//  threads[name] = lgt;

//  connect(lgt, SIGNAL(finished()), lgt, SLOT(deleteLater()));
//}

//void LimaGuiApplication::destroyThread(std::string name) {
//  std::map<std::string, LimaGuiThread*>::const_iterator it = threads.find(name);
//  if (it != threads.end()) {
//    // kill thread

//    delete threads[name];
//    threads.erase(it);
//  }
//}

//void LimaGuiApplication::destroyThread(LimaGuiThread * lgt) {
//  for (auto& kv : threads) {
//    if (kv.second == lgt) {
//      destroyThread(kv.first);
//      return;
//    }
//  }

//  // kill thread
//  delete lgt;
//}

void LimaGuiApplication::setTextBuffer(std::string str) {
  m_text = QString::fromUtf8(str.c_str());
  textChanged();
}

void LimaGuiApplication::writeInConsole(std::string str) {
  m_consoleOutput += QString(str.c_str());
}

void LimaGuiApplication::test() {
  LTELL("This is a warning");
  std::cout << "All your bases are belong to us" << std::endl;
}

/// BUFFERS ACCESSERS

QString LimaGuiApplication::fileContent() const { return m_fileContent; }
QString LimaGuiApplication::fileName() const { return m_fileName; }
QString LimaGuiApplication::fileUrl() const { return m_fileUrl; }
QString LimaGuiApplication::text() const { return m_text; }
QString LimaGuiApplication::consoleOutput() const { return m_consoleOutput; }

void LimaGuiApplication::setFileContent(const QString& s) { m_fileContent = s; }
void LimaGuiApplication::setFileName(const QString& s) { m_fileName = s; }
void LimaGuiApplication::setFileUrl(const QString& s) { m_fileUrl = s; }
void LimaGuiApplication::setText(const QString& s) {m_text = s; textChanged();}
void LimaGuiApplication::setConsoleOuput(const QString& s) { m_consoleOutput = s;}

void LimaGuiApplication::toggleAnalyzerState() {
  m_analyzerAvailable = !m_analyzerAvailable;
  readyChanged();
}

void LimaGuiApplication::setAnalyzerState(bool bo) {
  m_analyzerAvailable = bo;
  readyChanged();
}

bool LimaGuiApplication::available() {
  return m_analyzerAvailable;
}

void LimaGuiApplication::registerQmlObject(QString s, QObject* qo) {
  if (qo) {
    qml_objects[s] = qo;
  }
}

QObject* LimaGuiApplication::getQmlObject(const QString& name) {
  if (qml_objects.find(name) != qml_objects.end()) {
    return qml_objects[name];
  }
  else {
    return nullptr;
  }
}
