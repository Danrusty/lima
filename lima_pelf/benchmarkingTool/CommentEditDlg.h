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
#ifndef LIMA_PELF_COMMENTEDITDLG_H
#define LIMA_PELF_COMMENTEDITDLG_H

#include <QtCore/QtDebug>
#include <QtWidgets/QDialog>

#include "BenchmarkingResult.h"
#include "Pipeline.h"

#include "ui_pelf-bt-commenteditdialog.h"

namespace Lima {
namespace Pelf {

class CommentEditDlg : public QDialog, public Ui::CommentEditDialog
{

Q_OBJECT

public:

    CommentEditDlg (QWidget* parent = 0);
    void init (BenchmarkingResult* br, Pipeline* p = 0);

public Q_SLOTS:

    void submit ();

private:

    CommentEditDialog ui;
    BenchmarkingResult* benchmarkingResult;
    Pipeline* pipeline;

};

} // End namespace Lima
} // End namespace Pelf

#endif // LIMA_PELF_COMMENTEDITDLG_H
