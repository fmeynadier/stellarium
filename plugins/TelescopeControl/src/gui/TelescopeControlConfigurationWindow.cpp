/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2009-2010 Bogdan Marinov (this file)
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "Dialog.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelStyle.hpp"
#include "TelescopeControl.hpp"
#include "TelescopePropertiesWindow.hpp"
#include "TelescopeControlConfigurationWindow.hpp"
#include "ui_telescopeControlConfigurationWindow.h"
#include "StelGui.hpp"
#include <QDebug>
#include <QFrame>
#include <QTimer>
#include <QFile>
#include <QFileDialog>
#include <QHash>
#include <QHeaderView>
#include <QSettings>
#include <QStandardItem>

using namespace TelescopeControlGlobals;


TelescopeControlConfigurationWindow::TelescopeControlConfigurationWindow()
{
	ui = new Ui_widgetTelescopeControlConfiguration;
	
	//TODO: Fix this - it's in the same plugin
	telescopeManager = GETSTELMODULE(TelescopeControl);
	connectionListModel = new QStandardItemModel(0, ColumnCount);
	
	telescopeCount = 0;
}

TelescopeControlConfigurationWindow::~TelescopeControlConfigurationWindow()
{	
	delete ui;
	
	delete connectionListModel;
}

void TelescopeControlConfigurationWindow::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

// Initialize the dialog widgets and connect the signals/slots
void TelescopeControlConfigurationWindow::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Inherited connect
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	
	//Connect: sender, signal, receiver, method
	//Page: Connection
	connect(ui->pushButtonChangeStatus, SIGNAL(clicked()),
	        this, SLOT(toggleSelectedConnection()));
	connect(ui->pushButtonConfigure, SIGNAL(clicked()),
	        this, SLOT(configureSelectedConnection()));
	connect(ui->pushButtonRemove, SIGNAL(clicked()),
	        this, SLOT(removeSelectedConnection()));

	connect(ui->pushButtonNewStellarium, SIGNAL(clicked()),
	        this, SLOT(createNewStellariumConnection()));
	connect(ui->pushButtonNewVirtual, SIGNAL(clicked()),
	        this, SLOT(createNewVirtualConnection()));
#ifdef Q_OS_WIN32
	connect(ui->pushButtonNewAscom, SIGNAL(clicked()),
	        this, SLOT(createNewAscomConnection()));
#endif
	
	connect(ui->telescopeTreeView, SIGNAL(clicked (const QModelIndex &)),
	        this, SLOT(selectConnection(const QModelIndex &)));
	//connect(ui->telescopeTreeView, SIGNAL(activated (const QModelIndex &)),
	//        this, SLOT(configureTelescope(const QModelIndex &)));
	
	//Page: Options:
	connect(ui->checkBoxReticles, SIGNAL(toggled(bool)),
	        telescopeManager, SLOT(setFlagTelescopeReticles(bool)));
	connect(ui->checkBoxLabels, SIGNAL(toggled(bool)),
	        telescopeManager, SLOT(setFlagTelescopeLabels(bool)));
	connect(ui->checkBoxCircles, SIGNAL(toggled(bool)),
	        telescopeManager, SLOT(setFlagTelescopeCircles(bool)));
	
	connect(ui->checkBoxEnableLogs, SIGNAL(toggled(bool)),
	        telescopeManager, SLOT(setFlagUseTelescopeServerLogs(bool)));
	
	//In other dialogs:
	connect(&propertiesWindow, SIGNAL(changesDiscarded()),
	        this, SLOT(discardChanges()));
	connect(&propertiesWindow, SIGNAL(changesSaved(QString)),
	        this, SLOT(saveChanges(QString)));
	
	//Initialize the style
	updateStyle();

	//Deal with the ASCOM-specific fields
#ifdef Q_OS_WIN32
	if (telescopeManager->canUseAscom())
		ui->labelAscomNotice->setVisible(false);
	else
		ui->pushButtonNewAscom->setEnabled(false);
#else
	ui->pushButtonNewAscom->setVisible(false);
	ui->labelAscomNotice->setVisible(false);
	ui->groupBoxAscom->setVisible(false);
#endif
	
	populateConnectionList();
	
	//Checkboxes
	ui->checkBoxReticles->setChecked(telescopeManager->getFlagTelescopeReticles());
	ui->checkBoxLabels->setChecked(telescopeManager->getFlagTelescopeLabels());
	ui->checkBoxCircles->setChecked(telescopeManager->getFlagTelescopeCircles());
	ui->checkBoxEnableLogs->setChecked(telescopeManager->getFlagUseTelescopeServerLogs());
	
	//About page
	//TODO: Expand
	QString htmlPage = "<html><head></head><body>";
	htmlPage += "<h2>Stellarium Telescope Control Plug-in</h2>";
	htmlPage += "<h3>Version " + QString(TELESCOPE_CONTROL_VERSION) + "</h3>";
	htmlPage += "<p>Copyright &copy; 2006 Johannes Gajdosik, Michael Heinz</p>";
	htmlPage += "<p>Copyright &copy; 2009-2010 Bogdan Marinov</p>"
				"<p>This plug-in is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.</p>"
				"<p>This plug-in is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.</p>"
				"<p>You should have received a copy of the GNU General Public License along with this program; if not, write to:</p>"
				"<address>Free Software Foundation, Inc.<br/>"
				"51 Franklin Street, Fifth Floor<br/>"
				"Boston, MA  02110-1301, USA</address>"
				"<p><a href=\"http://www.fsf.org\">http://www.fsf.org/</a></p>";
#ifdef Q_OS_WIN32
	htmlPage += "<h3>QAxContainer Module</h3>";
	htmlPage += "This plug-in is statically linked to Nokia's QAxContainer library, which is distributed under the following license:";
	htmlPage += "<p>Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).<br/>"
	            "All rights reserved.</p>"
	            "<p>Contact: Nokia Corporation (qt-info@nokia.com)</p>"
	            "<p>You may use this file under the terms of the BSD license as follows:</p>"
	            "<blockquote><p>\"Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:</p>"
	            "<p>* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.</p>"
	            "<p>* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.</p>"
	            "<p>* Neither the name of Nokia Corporation and its Subsidiary(-ies) nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.</p>"
	            "<p>THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\"</p></blockquote>";
#endif
	htmlPage += "</body></html>";
	ui->textBrowserAbout->setHtml(htmlPage);

	htmlPage = "<html><head></head><body>";
	QFile helpFile(":/telescopeControl/help.utf8");
	helpFile.open(QFile::ReadOnly | QFile::Text);
	htmlPage += helpFile.readAll();
	helpFile.close();
	htmlPage += "</body></html>";
	ui->textBrowserHelp->setHtml(htmlPage);

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	ui->textBrowserAbout->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	ui->textBrowserHelp->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	
	//Everything must be initialized by now, start the updateTimer
	//TODO: Find if it's possible to run it only when the dialog is visible
	QTimer* updateTimer = new QTimer(this);
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateConnectionStates()));
	updateTimer->start(200);
}

void TelescopeControlConfigurationWindow::selectConnection(const QModelIndex & index)
{
	//Extract selected item index
	int selectedSlot = connectionListModel->data( connectionListModel->index(index.row(),0) ).toInt();
	updateStatusButtonForSlot(selectedSlot);

	//In all cases
	ui->pushButtonRemove->setEnabled(true);
}

void TelescopeControlConfigurationWindow::configureConnection(const QModelIndex & currentIndex)
{
	configuredTelescopeIsNew = false;
	configuredSlot = connectionListModel->data( connectionListModel->index(currentIndex.row(), ColumnSlot) ).toInt();
	
	//Stop the telescope first if necessary
	if(!telescopeManager->stopTelescopeAtSlot(configuredSlot))
		return;

	//Update the status in the list
	updateConnectionStates();
	
	setVisible(false);
	propertiesWindow.setVisible(true); //This should be called first to actually create the dialog content
	
	propertiesWindow.prepareForExistingConfiguration(configuredSlot);
}

void TelescopeControlConfigurationWindow::toggleSelectedConnection()
{
	if(!ui->telescopeTreeView->currentIndex().isValid())
		return;
	
	//Extract selected slot
	int selectedSlot = connectionListModel->data( connectionListModel->index(ui->telescopeTreeView->currentIndex().row(), ColumnSlot) ).toInt();
	if(telescopeManager->isConnectedClientAtSlot(selectedSlot))
		telescopeManager->stopTelescopeAtSlot(selectedSlot);
	else
		telescopeManager->startTelescopeAtSlot(selectedSlot);
	updateConnectionStates();
}

void TelescopeControlConfigurationWindow::configureSelectedConnection()
{
	if(ui->telescopeTreeView->currentIndex().isValid())
		configureConnection(ui->telescopeTreeView->currentIndex());
}

void TelescopeControlConfigurationWindow::createNewStellariumConnection()
{
	if(telescopeCount >= SLOT_COUNT)
		return;
	
	configuredTelescopeIsNew = true;
	configuredSlot = findFirstUnoccupiedSlot();
	
	setVisible(false);
	propertiesWindow.setVisible(true); //This should be called first to actually create the dialog content
	propertiesWindow.prepareNewStellariumConfiguration(configuredSlot);
}

void TelescopeControlConfigurationWindow::createNewVirtualConnection()
{
	if(telescopeCount >= SLOT_COUNT)
		return;

	configuredTelescopeIsNew = true;
	configuredSlot = findFirstUnoccupiedSlot();

	setVisible(false);
	propertiesWindow.setVisible(true); //This should be called first to actually create the dialog content
	propertiesWindow.prepareNewVirtualConfiguration(configuredSlot);
}

#ifdef Q_OS_WIN32
void TelescopeControlConfigurationWindow::createNewAscomConnection()
{
	if (telescopeCount >= SLOT_COUNT)
		return;

	configuredTelescopeIsNew = true;
	configuredSlot = findFirstUnoccupiedSlot();

	setVisible(false);
	propertiesWindow.setVisible(true); //This should be called first to actually create the dialog content
	propertiesWindow.prepareNewAscomConfiguration(configuredSlot);
}
#endif

void TelescopeControlConfigurationWindow::removeSelectedConnection()
{
	if(!ui->telescopeTreeView->currentIndex().isValid())
		return;
	
	//Extract selected slot
	int selectedSlot = connectionListModel->data( connectionListModel->index(ui->telescopeTreeView->currentIndex().row(),0) ).toInt();
	
	//Stop the telescope if necessary and remove it
	if(telescopeManager->stopTelescopeAtSlot(selectedSlot))
	{
		//TODO: Update status?
		if(!telescopeManager->removeTelescopeAtSlot(selectedSlot))
		{
			//TODO: Add debug
			return;
		}
	}
	else
	{
		//TODO: Add debug
		return;
	}
	
	//Save the changes to file
	telescopeManager->saveTelescopes();
	configuredTelescopeIsNew = true;//HACK!
	populateConnectionList();
}

void TelescopeControlConfigurationWindow::saveChanges(QString name)
{
	//Save the changes to file
	telescopeManager->saveTelescopes();

	populateConnectionList();
	
	configuredTelescopeIsNew = false;
	propertiesWindow.setVisible(false);
	setVisible(true);//Brings the current window to the foreground
}

void TelescopeControlConfigurationWindow::discardChanges()
{
	propertiesWindow.setVisible(false);
	setVisible(true);//Brings the current window to the foreground
	
	if (telescopeCount >= SLOT_COUNT)
		ui->groupBoxNewButtons->setEnabled(false);
	if (telescopeCount == 0)
		ui->pushButtonRemove->setEnabled(false);
	
	configuredTelescopeIsNew = false;
}

void TelescopeControlConfigurationWindow::updateConnectionStates()
{
	if (telescopeCount == 0)
		return;

	//If the dialog is not visible, don't bother updating the status
	if (!visible())
		return;
	
	int slotNumber = -1;
	for (int i=0; i<(connectionListModel->rowCount()); i++)
	{
		slotNumber = connectionListModel->data( connectionListModel->index(i, ColumnSlot) ).toInt();
		QString newStatusString = getStatusStringForSlot(slotNumber);
		connectionListModel->setData(connectionListModel->index(i, ColumnStatus), newStatusString, Qt::DisplayRole);
	}
	
	if(ui->telescopeTreeView->currentIndex().isValid())
	{
		int selectedSlot = connectionListModel->data( connectionListModel->index(ui->telescopeTreeView->currentIndex().row(), ColumnSlot) ).toInt();
		//Update the ChangeStatus button
		updateStatusButtonForSlot(selectedSlot);
	}
}

void TelescopeControlConfigurationWindow::updateStatusButtonForSlot(int slot)
{
	if(telescopeManager->isConnectedClientAtSlot(slot))
	{
		ui->pushButtonChangeStatus->setText("Disconnect");
		ui->pushButtonChangeStatus->setToolTip("Disconnect from the selected telescope");
		ui->pushButtonChangeStatus->setEnabled(true);
	}
	else
	{
		ui->pushButtonChangeStatus->setText("Connect");
		ui->pushButtonChangeStatus->setToolTip("Connect to the selected telescope");
		ui->pushButtonChangeStatus->setEnabled(true);
	}
}

int TelescopeControlConfigurationWindow::findFirstUnoccupiedSlot()
{
	//Find the first unoccupied slot (there is at least one)
	for (int slot = MIN_SLOT_NUMBER; slot < SLOT_NUMBER_LIMIT; slot++)
	{
		//configuredSlot = (i+1)%SLOT_COUNT;
		if(!telescopeManager->isExistingClientAtSlot(slot))
			return slot;
	}
	return -1;
}

void TelescopeControlConfigurationWindow::updateStyle()
{
	if (dialog)
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		Q_ASSERT(gui);
		QString style(gui->getStelStyle().htmlStyleSheet);
		ui->textBrowserAbout->document()->setDefaultStyleSheet(style);
		ui->textBrowserHelp->document()->setDefaultStyleSheet(style);
	}
}

void TelescopeControlConfigurationWindow::populateConnectionList()
{
	//Remember the previously selected connection, if any
	int selectedRow = 0;
	if (!configuredTelescopeIsNew)
		selectedRow = ui->telescopeTreeView->currentIndex().row();

	//Initializing the list of telescopes
	connectionListModel->clear();
	telescopeCount = 0;

	connectionListModel->setColumnCount(ColumnCount);
	QStringList headerStrings;
	headerStrings << "#";
	//headerStrings << "Start";
	headerStrings <<  "Status";
	headerStrings << "Connection";
	headerStrings << "Interface";
	headerStrings << "Name";
	connectionListModel->setHorizontalHeaderLabels(headerStrings);

	ui->telescopeTreeView->setModel(connectionListModel);
	ui->telescopeTreeView->header()->setMovable(false);
	ui->telescopeTreeView->header()->setResizeMode(ColumnSlot, QHeaderView::ResizeToContents);
	ui->telescopeTreeView->header()->setStretchLastSection(true);

	//Populating the list
	QStandardItem * tempItem;

	//Cycle the slots
	for (int slot = MIN_SLOT_NUMBER; slot < SLOT_NUMBER_LIMIT; slot++)
	{
		//Slot #
		//int slotNumber = (i+1)%SLOT_COUNT;//Making sure slot 0 is last

		//Read the telescope properties
		const QVariantMap& properties = telescopeManager->getTelescopeAtSlot(slot);
		if(properties.isEmpty())
			continue;

		QString name = properties.value("name").toString();
		if (name.isEmpty())
			continue;
		QString connection = properties.value("connection").toString();
		if (connection.isEmpty())
			continue;
		QString type = properties.value("type").toString();

		QString connectionTypeLabel;
		QString interfaceTypeLabel;
		if (connection == "internal")
		{
			if (type.isEmpty())
				continue;

			connectionTypeLabel = "direct";
#ifdef Q_OS_WIN32
			if (type == "Ascom")
				interfaceTypeLabel = "ASCOM";
			else
#endif
				connectionTypeLabel = "Stellarium";
			connectionType[slot] = ConnectionInternal;
		}
		else if (connection == "local")
		{
			connectionTypeLabel = "local";
			interfaceTypeLabel = "Stellarium";
			connectionType[slot] = ConnectionLocal;
		}
		else if (connection == "remote")
		{
			connectionTypeLabel = "remote";
			interfaceTypeLabel = "Stellarium";
			connectionType[slot] = ConnectionRemote;
		}
		else
		{
			connectionTypeLabel = "direct";
			interfaceTypeLabel = "virtual";
			connectionType[slot] = ConnectionVirtual;
		}

		//New column on a new row in the list: Slot number
		int lastRow = connectionListModel->rowCount();
		tempItem = new QStandardItem(QString::number(slot));
		tempItem->setEditable(false);
		connectionListModel->setItem(lastRow, ColumnSlot, tempItem);

		//TODO: This is not updated, because it was commented out
		//tempItem = new QStandardItem;
		//tempItem->setEditable(false);
		//tempItem->setCheckable(true);
		//tempItem->setCheckState(Qt::Checked);
		//tempItem->setData("If checked, this telescope will start when Stellarium is started", Qt::ToolTipRole);
		//telescopeListModel->setItem(lastRow, ColumnStartup, tempItem);//Start-up checkbox

		//New column on a new row in the list: Connection status
		QString newStatusString = getStatusStringForSlot(slot);
		tempItem = new QStandardItem(newStatusString);
		tempItem->setEditable(false);
		connectionListModel->setItem(lastRow, ColumnStatus, tempItem);

		//New column on a new row in the list: Connection type
		tempItem = new QStandardItem(connectionTypeLabel);
		tempItem->setEditable(false);
		connectionListModel->setItem(lastRow, ColumnType, tempItem);

		//New column on a new row in the list: Interface type
		tempItem = new QStandardItem(interfaceTypeLabel);
		tempItem->setEditable(false);
		connectionListModel->setItem(lastRow, ColumnInterface, tempItem);

		//New column on a new row in the list: Telescope name
		tempItem = new QStandardItem(name);
		tempItem->setEditable(false);
		connectionListModel->setItem(lastRow, ColumnName, tempItem);

		//After everything is done, count this as loaded
		telescopeCount++;
	}

	//Finished populating the table, let's sort it by slot number
	//ui->telescopeTreeView->setSortingEnabled(true);//Set in the .ui file
	ui->telescopeTreeView->sortByColumn(ColumnSlot, Qt::AscendingOrder);
	//(Works even when the table is empty)
	//(Makes redundant the delay of 0 above)

	if(telescopeCount > 0)
	{
		ui->telescopeTreeView->setFocus();
		ui->telescopeTreeView->setCurrentIndex(connectionListModel->index(selectedRow,0));
		ui->pushButtonChangeStatus->setEnabled(true);
		ui->pushButtonConfigure->setEnabled(true);
		ui->pushButtonRemove->setEnabled(true);
		ui->telescopeTreeView->header()->setResizeMode(ColumnType, QHeaderView::ResizeToContents);
		ui->labelWarning->setText(LABEL_TEXT_CONTROL_TIP);
	}
	else
	{
		ui->pushButtonChangeStatus->setEnabled(false);
		ui->pushButtonConfigure->setEnabled(false);
		ui->pushButtonRemove->setEnabled(false);
		ui->telescopeTreeView->header()->setResizeMode(ColumnType, QHeaderView::Interactive);
		ui->pushButtonNewStellarium->setFocus();
		if(telescopeManager->getDeviceModels().isEmpty())
			ui->labelWarning->setText(LABEL_TEXT_NO_DEVICE_MODELS);
		else
			ui->labelWarning->setText(LABEL_TEXT_ADD_TIP);
	}

	//If there are less than the maximal number of telescopes,
	//new ones can be added
	if(telescopeCount < SLOT_COUNT)
		ui->groupBoxNewButtons->setEnabled(true);
	else
		ui->groupBoxNewButtons->setEnabled(false);
}

QString TelescopeControlConfigurationWindow::getStatusStringForSlot(int slot)
{
	if (telescopeManager->isConnectedClientAtSlot(slot))
	{
		return "Connected";
	}
	else if(telescopeManager->isExistingClientAtSlot(slot))
	{
		return "Connecting";
	}
	else
	{
		return "Disconnected";
	}
}
