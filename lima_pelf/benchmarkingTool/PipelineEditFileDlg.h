/*
 *    Copyright 2002-2013 CEA LIST
 * 
 *    This file is part of LIMA.
 * 
 *    LIMA is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 * 
 *    LIMA is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 * 
 *    You should have received a copy of the GNU Affero General Public License
 *    along with LIMA.  If not, see <http://www.gnu.org/licenses/>
 */
#ifndef LIMA_PELF_PIPELINEEDITFILEDLG_H
#define LIMA_PELF_PIPELINEEDITFILEDLG_H

#include <QtCore/QtDebug>
#include <QtCore/QFileInfo>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFileDialog>

#include "ui_pelf-bt-editfiledialog.h"
#include "Pipeline.h"
#include "PipelineUnit.h"

namespace Lima {
namespace Pelf {

class PipelineEditFileDlg : public QDialog, public Ui::PipelineEditFileDialog
{

Q_OBJECT

public:

    int unitIndex;

    PipelineEditFileDlg (QWidget* parent = 0);
    void init (Pipeline* bp);
    void setUnit (PipelineUnit* unit, int index);

public Q_SLOTS:

    void textFileBrowse ();
    void referenceFileBrowse ();
    void checkFilePaths ();
    void submit ();

private:

    PipelineEditFileDialog ui;
    Pipeline* pipeline;

};

} // End namespace Lima
} // End namespace Pelf

#endif // LIMA_PELF_PIPELINEEDITFILEDLG_H
