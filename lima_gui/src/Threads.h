/*
    Copyright 2017 CEA LIST

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
/**
 * \file    Threads.h
 * \author  Jocelyn Vernay
 * \date    Wed, Sep 06 2017
 * 
 */

#ifndef lima_gui_analysis_thread_h
#define lima_gui_analysis_thread_h

#include "LimaGuiExport.h"

#include <sstream>

#include <QThread>

namespace Lima 
{
namespace Gui 
{

class LimaGuiApplication;

/// \class LimaGuiThread
///
/// \brief A QThread implementation for LIMA Gui.
/// This class is meant to be related with a LimaGuiApplication object.
class LIMA_GUI_EXPORT LimaGuiThread : public QThread 
{
  Q_OBJECT
public:
  LimaGuiThread(LimaGuiApplication* app);

  ///
  /// \brief method inherited from QThread
  virtual void run() override = 0;


protected:
  LimaGuiApplication* m_application = nullptr;
};

/// \class AnalysisThread
/// \brief This thread starts the analysis with preferred parameters.
class LIMA_GUI_EXPORT AnalysisThread : public LimaGuiThread 
{
  
public:

  ///
  /// \brief Bare constructor. The text is empty by default
  AnalysisThread(LimaGuiApplication* app);

  ///
  /// \param text : text to be analyzed
  AnalysisThread(LimaGuiApplication* app, const QString& text);

  ///
  void run() override;
  
  std::ostream& getOut() { return out; }

  /// SETTERS

  void setText(const QString&);
  void setResultView(QObject*);
  void setName(const std::string&);

  /// \brief Send the results to the view.
  void notifyView();

private:

  ///< the ostream of the thread to receive the results
  std::stringstream out;

  // identify the analysis
  // ideally the name of the file
  // or a generated name
  std::string m_name;

  ///< the QML QObject (a ResultView object) to be notified once the analysis is completed
  QObject* m_resultView = nullptr;

  ///< content to analyze
  QString m_text;


};

/// \class InitializeThread
/// \brief Thread to initialize lima while the QML is being loaded.
class LIMA_GUI_EXPORT InitializeThread : public LimaGuiThread {
  
public:
  InitializeThread(LimaGuiApplication* a);

  /// \brief This simply calls LimaGuiApplication::initializeLimaAnalyzer.
  void run() override;
};

class LIMA_GUI_EXPORT TestThread : public LimaGuiThread {
public:
  TestThread(LimaGuiApplication* application);
  void run() override;
};

} // END namespace Gui
} // END namespace Lima

#endif // lima_gui_analysis_thread_h
