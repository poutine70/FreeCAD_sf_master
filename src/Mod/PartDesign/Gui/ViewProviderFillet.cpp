/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include "ViewProviderFillet.h"
#include "TaskFilletParameters.h"
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>


using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderFillet,PartDesignGui::ViewProviderDressUp)

bool ViewProviderFillet::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
		TaskDlgDressUpParameters *dressUpDlg = NULL;

        //// When double-clicking on the item for this fillet the
        //// object unsets and sets its edit mode without closing
        //// the task panel
        //Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        //TaskDlgFilletParameters *padDlg = qobject_cast<TaskDlgFilletParameters *>(dlg);
        //if (padDlg && padDlg->getFilletView() != this)
        //    padDlg = 0; // another pad left open its task panel
        //if (dlg && !padDlg) {
        //    QMessageBox msgBox;
        //    msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
        //    msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
        //    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        //    msgBox.setDefaultButton(QMessageBox::Yes);
        //    int ret = msgBox.exec();
        //    if (ret == QMessageBox::Yes)
        //        Gui::Control().reject();
        //    else
        //        return false;
        //}


        if (checkDlgOpen(dressUpDlg)) {
            // always change to PartDesign WB, remember where we come from
            oldWb = Gui::Command::assureWorkbench("PartDesignWorkbench");

            // start the edit dialog
            if (dressUpDlg)
                Gui::Control().showDialog(dressUpDlg);
            else
                Gui::Control().showDialog(new TaskDlgFilletParameters(this));

            return true;
        } else {
            return false;
        }
    }
    else {
        return ViewProviderDressUp::setEdit(ModNum);
    }
}


