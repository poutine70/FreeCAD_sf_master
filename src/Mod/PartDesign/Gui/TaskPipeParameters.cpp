/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <sstream>
# include <QRegExp>
# include <QTextStream>
# include <QMessageBox>
# include <Precision.hxx>
#endif

#include "ui_TaskPipeParameters.h"
#include "ui_TaskPipeOrientation.h"
#include "ui_TaskPipeScaling.h"
#include "TaskPipeParameters.h"
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/FeaturePipe.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/Body.h>
#include "TaskSketchBasedParameters.h"
#include "ReferenceSelection.h"
#include "Workbench.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPipeParameters */


//**************************************************************************
//**************************************************************************
// Task Parameter
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskPipeParameters::TaskPipeParameters(ViewProviderPipe *PipeView,bool newObj, QWidget *parent)
    : TaskSketchBasedParameters(PipeView, parent, "PartDesign_Pipe",tr("Pipe parameters"))
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskPipeParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->comboBoxTransition, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onTransitionChanged(int)));
    connect(ui->buttonRefAdd, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefAdd(bool)));
    connect(ui->buttonRefRemove, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefRemove(bool)));
    connect(ui->tangent, SIGNAL(toggled(bool)), 
            this, SLOT(onTangentChanged(bool)));
    connect(ui->buttonProfileBase, SIGNAL(toggled(bool)),
            this, SLOT(onBaseButton(bool)));
    
    this->groupLayout()->addWidget(proxy);
         
    //add initial values
    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(PipeView->getObject());
    std::vector<std::string> strings = pipe->Spine.getSubValues();
    for (std::vector<std::string>::const_iterator i = strings.begin(); i != strings.end(); i++)
        ui->listWidgetReferences->addItem(QString::fromStdString(*i));
        
    ui->comboBoxTransition->setCurrentIndex(pipe->Transition.getValue());
    ui->tangent->setChecked(pipe->SpineTangent.getValue());

    updateUI();
}

void TaskPipeParameters::updateUI()
{
    
}

void TaskPipeParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == none)
        return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (referenceSelected(msg)) {
            if (selectionMode == refAdd) {
                QString sub = QString::fromStdString(msg.pSubName);
                if(!sub.isEmpty())
                    ui->listWidgetReferences->addItem(QString::fromStdString(msg.pSubName));
                
                ui->profileBaseEdit->setText(QString::fromStdString(msg.pObjectName));
            }
            else if (selectionMode == refRemove) {
                QString sub = QString::fromStdString(msg.pSubName);
                if(!sub.isEmpty())
                    removeFromListWidget(ui->listWidgetReferences, QString::fromAscii(msg.pSubName));
                else {
                    ui->profileBaseEdit->clear();
                }                
            } else if(selectionMode == refObjAdd) {
                ui->listWidgetReferences->clear();
                ui->profileBaseEdit->setText(QString::fromAscii(msg.pObjectName));
            }
            //clearButtons(none);
            recomputeFeature();
        } 
        clearButtons();
        exitSelectionMode();
    }
}

TaskPipeParameters::~TaskPipeParameters()
{
    delete ui;
}

void TaskPipeParameters::changeEvent(QEvent *e)
{
}

void TaskPipeParameters::onTransitionChanged(int idx) {
    
    static_cast<PartDesign::Pipe*>(vp->getObject())->Transition.setValue(idx);
    recomputeFeature();
}

void TaskPipeParameters::onButtonRefAdd(bool checked) {
    
    if (checked) {
        //clearButtons(refAdd);
        //hideObject();
        Gui::Selection().clearSelection();
        selectionMode = refAdd;
        //DressUpView->highlightReferences(true);
    }
}

void TaskPipeParameters::onButtonRefRemove(bool checked) {

    if (checked) {
        //clearButtons(refRemove);
        //hideObject();
        Gui::Selection().clearSelection();        
        selectionMode = refRemove;
        //DressUpView->highlightReferences(true);
    }
}

void TaskPipeParameters::onBaseButton(bool checked) {

    if (checked) {
        //clearButtons(refRemove);
        //hideObject();
        Gui::Selection().clearSelection();        
        selectionMode = refObjAdd;
        //DressUpView->highlightReferences(true);
    }
}


void TaskPipeParameters::onTangentChanged(bool checked) {

    static_cast<PartDesign::Pipe*>(vp->getObject())->SpineTangent.setValue(checked);
    recomputeFeature();
}


void TaskPipeParameters::removeFromListWidget(QListWidget* widget, QString itemstr) {

    QList<QListWidgetItem*> items = widget->findItems(itemstr, Qt::MatchExactly);
    if (!items.empty()) {
        for (QList<QListWidgetItem*>::const_iterator i = items.begin(); i != items.end(); i++) {
            QListWidgetItem* it = widget->takeItem(widget->row(*i));
            delete it;
        }
    }
}

bool TaskPipeParameters::referenceSelected(const SelectionChanges& msg) const {
    
    if ((msg.Type == Gui::SelectionChanges::AddSelection) && (
                (selectionMode == refAdd) || (selectionMode == refRemove) 
                || (selectionMode == refObjAdd))) {

        if (strcmp(msg.pDocName, vp->getObject()->getDocument()->getName()) != 0)
            return false;

        // not allowed to reference ourself
        const char* fname = vp->getObject()->getNameInDocument();        
        if (strcmp(msg.pObjectName, fname) == 0)
            return false;
       
        //change the references 
        std::string subName(msg.pSubName);
        std::vector<std::string> refs = static_cast<PartDesign::Pipe*>(vp->getObject())->Spine.getSubValues();
        std::vector<std::string>::iterator f = std::find(refs.begin(), refs.end(), subName);

        if(selectionMode != refObjAdd) {
            if (selectionMode == refAdd) {
                if (f == refs.end())
                    refs.push_back(subName);
                else
                    return false; // duplicate selection
            } else {
                if (f != refs.end())
                    refs.erase(f);
                else
                    return false;
            }        
        }
        else {
            refs.clear();
        }
        static_cast<PartDesign::Pipe*>(vp->getObject())->Spine.setValue(vp->getObject()->getDocument()->getObject(msg.pObjectName),
                                                                        refs);
        return true;
    }

    return false;
}

void TaskPipeParameters::clearButtons() {

    ui->buttonRefAdd->setChecked(false);
    ui->buttonRefRemove->setChecked(false);
    ui->buttonProfileBase->setChecked(false);
}

void TaskPipeParameters::exitSelectionMode() {

    selectionMode = none;
    Gui::Selection().clearSelection();
}


//**************************************************************************
//**************************************************************************
// Tassk Orientation
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskPipeOrientation::TaskPipeOrientation(ViewProviderPipe* PipeView, bool newObj, QWidget* parent)
    : TaskSketchBasedParameters(PipeView, parent, "PartDesign_Pipe", tr("Section orientation")) {

    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskPipeOrientation();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->comboBoxMode, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onOrientationChanged(int)));
    connect(ui->buttonRefAdd, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefAdd(bool)));
    connect(ui->buttonRefRemove, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefRemove(bool)));
    connect(ui->tangent, SIGNAL(toggled(bool)), 
            this, SLOT(onTangentChanged(bool)));
    connect(ui->buttonProfileBase, SIGNAL(toggled(bool)),
            this, SLOT(onBaseButton(bool)));
    connect(ui->stackedWidget, SIGNAL(currentChanged(int)),
            this, SLOT(updateUI(int)));
    connect(ui->curvelinear, SIGNAL(toggled(bool)),
            this, SLOT(onCurvelinearChanged(bool)));
    connect(ui->doubleSpinBoxX, SIGNAL(valueChanged(double)),
            this, SLOT(onBinormalChanged(double)));
    connect(ui->doubleSpinBoxY, SIGNAL(valueChanged(double)),
            this, SLOT(onBinormalChanged(double)));
    connect(ui->doubleSpinBoxZ, SIGNAL(valueChanged(double)),
            this, SLOT(onBinormalChanged(double)));
    
    this->groupLayout()->addWidget(proxy);

    //add initial values
    PartDesign::Pipe* pipe = static_cast<PartDesign::Pipe*>(PipeView->getObject());
    std::vector<std::string> strings = pipe->AuxillerySpine.getSubValues();
    for (std::vector<std::string>::const_iterator i = strings.begin(); i != strings.end(); i++)
        ui->listWidgetReferences->addItem(QString::fromStdString(*i));
        
    ui->comboBoxMode->setCurrentIndex(pipe->Mode.getValue());
    ui->tangent->setChecked(pipe->AuxillerySpineTangent.getValue());
    ui->curvelinear->setChecked(pipe->AuxilleryCurvelinear.getValue());    

    updateUI(0);
}

TaskPipeOrientation::~TaskPipeOrientation() {

}

void TaskPipeOrientation::onOrientationChanged(int idx) {

    static_cast<PartDesign::Pipe*>(vp->getObject())->Mode.setValue(idx);
    recomputeFeature();
}

void TaskPipeOrientation::changeEvent(QEvent* e) {
    QFrame::changeEvent(e);
}

void TaskPipeOrientation::clearButtons() {

}

void TaskPipeOrientation::exitSelectionMode() {

}

void TaskPipeOrientation::onButtonRefAdd(bool checked) {

    if (checked) {
        Gui::Selection().clearSelection();        
        selectionMode = refAdd;
    }
}

void TaskPipeOrientation::onButtonRefRemove(bool checked) {

    if (checked) {
        Gui::Selection().clearSelection();        
        selectionMode = refRemove;
    }
}

void TaskPipeOrientation::onBaseButton(bool checked) {

    if (checked) {
        Gui::Selection().clearSelection();        
        selectionMode = refObjAdd;
    }
}

void TaskPipeOrientation::onTangentChanged(bool checked) {

}

void TaskPipeOrientation::onCurvelinearChanged(bool checked) {

    static_cast<PartDesign::Pipe*>(vp->getObject())->AuxilleryCurvelinear.setValue(checked);
    recomputeFeature();
}

void TaskPipeOrientation::onBinormalChanged(double) {

    Base::Vector3d vec(ui->doubleSpinBoxX->value(),
                       ui->doubleSpinBoxY->value(),
                       ui->doubleSpinBoxZ->value());
    
    static_cast<PartDesign::Pipe*>(vp->getObject())->Binormal.setValue(vec);
    recomputeFeature();
}



void TaskPipeOrientation::onSelectionChanged(const SelectionChanges& msg) {

    if (selectionMode == none)
        return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (referenceSelected(msg)) {
            if (selectionMode == refAdd) {
                QString sub = QString::fromStdString(msg.pSubName);
                if(!sub.isEmpty())
                    ui->listWidgetReferences->addItem(QString::fromStdString(msg.pSubName));
                
                ui->profileBaseEdit->setText(QString::fromStdString(msg.pObjectName));
            }
            else if (selectionMode == refRemove) {
                QString sub = QString::fromStdString(msg.pSubName);
                if(!sub.isEmpty())
                    removeFromListWidget(ui->listWidgetReferences, QString::fromAscii(msg.pSubName));
                else {
                    ui->profileBaseEdit->clear();
                }                
            } else if(selectionMode == refObjAdd) {
                ui->listWidgetReferences->clear();
                ui->profileBaseEdit->setText(QString::fromAscii(msg.pObjectName));
            }
            //clearButtons(none);
            recomputeFeature();
        } 
        clearButtons();
        exitSelectionMode();
    }
}

bool TaskPipeOrientation::referenceSelected(const SelectionChanges& msg) const {

    if ((msg.Type == Gui::SelectionChanges::AddSelection) && (
                (selectionMode == refAdd) || (selectionMode == refRemove) 
                || (selectionMode == refObjAdd))) {

        if (strcmp(msg.pDocName, vp->getObject()->getDocument()->getName()) != 0)
            return false;

        // not allowed to reference ourself
        const char* fname = vp->getObject()->getNameInDocument();        
        if (strcmp(msg.pObjectName, fname) == 0)
            return false;
       
        //change the references 
        std::string subName(msg.pSubName);
        std::vector<std::string> refs = static_cast<PartDesign::Pipe*>(vp->getObject())->AuxillerySpine.getSubValues();
        std::vector<std::string>::iterator f = std::find(refs.begin(), refs.end(), subName);

        if(selectionMode != refObjAdd) {
            if (selectionMode == refAdd) {
                if (f == refs.end())
                    refs.push_back(subName);
                else
                    return false; // duplicate selection
            } else {
                if (f != refs.end())
                    refs.erase(f);
                else
                    return false;
            }        
        }
        else {
            refs.clear();
        }
        static_cast<PartDesign::Pipe*>(vp->getObject())->AuxillerySpine.setValue(vp->getObject()->getDocument()->getObject(msg.pObjectName),
                                                                        refs);
        return true;
    }

    return false;
}

void TaskPipeOrientation::removeFromListWidget(QListWidget* w, QString name) {

}

void TaskPipeOrientation::updateUI(int idx) {

    //make sure we resize to the size of the current page
    for(int i=0; i<ui->stackedWidget->count(); ++i)
        ui->stackedWidget->widget(i)->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    
    ui->stackedWidget->widget(idx)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}


//**************************************************************************
//**************************************************************************
// Task Scaling
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
TaskPipeScaling::TaskPipeScaling(ViewProviderPipe* PipeView, bool newObj, QWidget* parent)
    : TaskSketchBasedParameters(PipeView, parent, "PartDesign_Pipe", tr("Section transformation")) {

            // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskPipeScaling();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->comboBoxScaling, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onScalingChanged(int)));
    connect(ui->buttonRefAdd, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefAdd(bool)));
    connect(ui->buttonRefRemove, SIGNAL(toggled(bool)),
            this, SLOT(onButtonRefRemove(bool)));
    connect(ui->stackedWidget, SIGNAL(currentChanged(int)),
            this, SLOT(updateUI(int)));
    
    this->groupLayout()->addWidget(proxy);
      
//     ui->comboBoxTransition->clear();
//     ui->comboBoxTransition->insertItem(0, tr("Transformed"));
//     ui->comboBoxTransition->insertItem(1, tr("Right Corner"));
//     ui->comboBoxTransition->insertItem(2, tr("Round Corner"));
//     ui->comboBoxTransition->setCurrentIndex(0);
    

    updateUI(0);
}

TaskPipeScaling::~TaskPipeScaling() {

}


void TaskPipeScaling::changeEvent(QEvent* e) {
    QFrame::changeEvent(e);
}

void TaskPipeScaling::clearButtons() {

}

void TaskPipeScaling::exitSelectionMode() {

}

void TaskPipeScaling::onButtonRefAdd(bool checked) {

}

void TaskPipeScaling::onButtonRefRemove(bool checked) {

}

void TaskPipeScaling::onScalingChanged(int idx) {

    static_cast<PartDesign::Pipe*>(vp->getObject())->Transformation.setValue(idx);
    recomputeFeature();
}

void TaskPipeScaling::onSelectionChanged(const SelectionChanges& msg) {

}

bool TaskPipeScaling::referenceSelected(const SelectionChanges& msg) const {

}

void TaskPipeScaling::removeFromListWidget(QListWidget* w, QString name) {

}

void TaskPipeScaling::updateUI(int idx) {

    //make sure we resize to the size of the current page
    for(int i=0; i<ui->stackedWidget->count(); ++i)
        ui->stackedWidget->widget(i)->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    
    ui->stackedWidget->widget(idx)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgPipeParameters::TaskDlgPipeParameters(ViewProviderPipe *PipeView,bool newObj)
   : TaskDlgSketchBasedParameters(PipeView)
{
    assert(PipeView);
    parameter    = new TaskPipeParameters(PipeView,newObj);
    orientation  = new TaskPipeOrientation(PipeView,newObj);
    scaling      = new TaskPipeScaling(PipeView,newObj);

    Content.push_back(parameter);
    Content.push_back(orientation);
    Content.push_back(scaling);
}

TaskDlgPipeParameters::~TaskDlgPipeParameters()
{

}

//==== calls from the TaskView ===============================================================


bool TaskDlgPipeParameters::accept()
{/*
    std::string name = vp->getObject()->getNameInDocument();

    // save the history 
    parameter->saveHistory();

    try {
        //Gui::Command::openCommand("Pipe changed");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Length = %f",name.c_str(),parameter->getLength());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Reversed = %i",name.c_str(),parameter->getReversed()?1:0);
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Midplane = %i",name.c_str(),parameter->getMidplane()?1:0);
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Length2 = %f",name.c_str(),parameter->getLength2());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Offset = %f",name.c_str(),parameter->getOffset());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Type = %u",name.c_str(),parameter->getMode());
        std::string facename = (const char*)parameter->getFaceName();

        if (!facename.empty()) {
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.UpToFace = %s", name.c_str(), facename.c_str());
        } else
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.UpToFace = None", name.c_str());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!vp->getObject()->isValid())
            throw Base::Exception(vp->getObject()->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromAscii(e.what()));
        return false;
    }

    return true;*/
}

//bool TaskDlgPipeParameters::reject()
//{
//    // get the support and Sketch
//    PartDesign::Pipe* pcPipe = static_cast<PartDesign::Pipe*>(PipeView->getObject()); 
//    Sketcher::SketchObject *pcSketch = 0;
//    App::DocumentObject    *pcSupport = 0;
//    if (pcPipe->Sketch.getValue()) {
//        pcSketch = static_cast<Sketcher::SketchObject*>(pcPipe->Sketch.getValue()); 
//        pcSupport = pcSketch->Support.getValue();
//    }
//
//    // roll back the done things
//    Gui::Command::abortCommand();
//    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
//    
//    // if abort command deleted the object the support is visible again
//    if (!Gui::Application::Instance->getViewProvider(pcPipe)) {
//        if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
//            Gui::Application::Instance->getViewProvider(pcSketch)->show();
//        if (pcSupport && Gui::Application::Instance->getViewProvider(pcSupport))
//            Gui::Application::Instance->getViewProvider(pcSupport)->show();
//    }
//
//    //Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
//    //Gui::Command::commitCommand();
//
//    return true;
//}



#include "moc_TaskPipeParameters.cpp"