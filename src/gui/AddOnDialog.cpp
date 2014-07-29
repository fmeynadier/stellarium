/*
 * Stellarium
 * Copyright (C) 2014 Marcos Cardinot
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include <QDateTime>
#include <QSqlQueryModel>
#include <QStringBuilder>

#include "AddOnDialog.hpp"
#include "ui_addonDialog.h"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"
#include "addons/AddOnTableModel.hpp"

AddOnDialog::AddOnDialog(QObject* parent) : StelDialog(parent)
{
    ui = new Ui_addonDialogForm;
}

AddOnDialog::~AddOnDialog()
{
	delete ui;
	ui = NULL;
}

void AddOnDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateTabBarListWidgetWidth();
	}
}

void AddOnDialog::styleChanged()
{
}

void AddOnDialog::createDialogContent()
{
	connect(&StelApp::getInstance().getStelAddOnMgr(), SIGNAL(updateTableViews()),
		this, SLOT(populateTables()));

	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	// build and populate all tableviews
	populateTables();

	// catalog updates
	ui->txtLastUpdate->setText(StelApp::getInstance().getStelAddOnMgr().getLastUpdateString());
	connect(ui->btnUpdate, SIGNAL(clicked()), this, SLOT(updateCatalog()));

	// setting up tabs
	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
		this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));
	ui->stackedWidget->setCurrentIndex(CATALOG);
	ui->stackListWidget->setCurrentRow(CATALOG);
	m_currentTableView = ui->catalogsTableView;

	// Install and Remove
	connect(ui->btnInstall, SIGNAL(clicked()), this, SLOT(installSelectedRows()));
	connect(ui->btnRemove, SIGNAL(clicked()), this, SLOT(removeSelectedRows()));

	updateTabBarListWidgetWidth();
}

void AddOnDialog::updateTabBarListWidgetWidth()
{
	ui->stackListWidget->setWrapping(false);

	// Update list item sizes after translation
	ui->stackListWidget->adjustSize();

	QAbstractItemModel* model = ui->stackListWidget->model();
	if (!model)
		return;

	int width = 0;
	for (int row = 0; row < model->rowCount(); row++)
	{
		width += ui->stackListWidget->sizeHintForRow(row);
		width += ui->stackListWidget->iconSize().width();
	}

	// Hack to force the window to be resized...
	ui->stackListWidget->setMinimumWidth(width);
}

void AddOnDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
	if (!current)
	{
		current = previous;
	}
	ui->stackedWidget->setCurrentIndex(ui->stackListWidget->row(current));

	switch (ui->stackedWidget->currentIndex())
	{
		case CATALOG:
			m_currentTableView = ui->catalogsTableView;
			break;
		case LANDSCAPE:
			m_currentTableView = ui->landscapeTableView;
			break;
		case LANGUAGEPACK:
			m_currentTableView = ui->languageTableView;
			break;
		case SCRIPT:
			m_currentTableView = ui->scriptsTableView;
			break;
		case STARLORE:
			m_currentTableView = ui->starloreTbleView;
			break;
		case TEXTURE:
			m_currentTableView = ui->texturesTableView;
			break;
	}

}

void AddOnDialog::setUpTableView(QTableView* tableView, QString tableName)
{
	AddOnTableModel* model = new AddOnTableModel(tableName);
	tableView->setModel(model);

	tableView->setColumnHidden(0, true); // hide id
	tableView->setColumnHidden(1, true); // hide addonid
	tableView->setColumnHidden(2, true); // hide firststel
	tableView->setColumnHidden(3, true); // hide laststel

	// hide imcompatible add-ons
	for (int row = 0; row < model->rowCount(); row++)
	{
		tableView->setRowHidden(row,
					!isCompatible(model->index(row, 2).data().toString(),
						      model->index(row, 3).data().toString()));
	}

	tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	int lastColumn = tableView->horizontalHeader()->count() - 1;
	tableView->horizontalHeader()->setSectionResizeMode(lastColumn, QHeaderView::ResizeToContents);
	tableView->verticalHeader()->setVisible(false);
	tableView->setAlternatingRowColors(false);
	tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	tableView->setEditTriggers(false);
}

bool AddOnDialog::isCompatible(QString first, QString last)
{
	QStringList c = StelUtils::getApplicationVersion().split(".");
	QStringList f = first.split(".");
	QStringList l = last.split(".");

	if (c.size() < 3 || f.size() < 3 || l.size() < 3) {
		return false; // invalid version
	}

	int currentVersion = QString(c.at(0) % "00" % c.at(1) % "0" % c.at(2)).toInt();
	int firstVersion = QString(f.at(0) % "00" % f.at(1) % "0" % f.at(2)).toInt();
	int lastVersion = QString(l.at(0) % "00" % l.at(1) % "0" % l.at(2)).toInt();

	if (currentVersion < firstVersion || currentVersion > lastVersion)
	{
		return false; // out of bounds
	}

	return true;
}

void AddOnDialog::insertCheckBoxes(QTableView* tableview, int tab) {
	// Insert clean checkboxes to the current tableview
	QHash<int, QCheckBox*> checkboxes;
	int lastColumn = tableview->horizontalHeader()->count() - 1;
	for(int row=0; row<tableview->model()->rowCount(); ++row)
	{
		QCheckBox* cbox = new QCheckBox();
		tableview->setIndexWidget(tableview->model()->index(row, lastColumn), cbox);
		cbox->setStyleSheet("QCheckBox { padding-left: 8px; }");
		checkboxes.insert(row, cbox);
	}
	m_checkBoxes.insert(tab, checkboxes);
}

void AddOnDialog::populateTables()
{
	// destroying all checkboxes
	while (!m_checkBoxes.isEmpty())
	{
		QHash<int, QCheckBox*> cbs;
		cbs = m_checkBoxes.takeFirst();
		int i = 0;
		while (!cbs.isEmpty())
		{
			delete cbs.take(i);
			i++;
		}
	}

	// CATALOGS
	setUpTableView(ui->catalogsTableView, TABLE_CATALOG);
	insertCheckBoxes(ui->catalogsTableView, CATALOG);

	// LANDSCAPES
	setUpTableView(ui->landscapeTableView,TABLE_LANDSCAPE);
	insertCheckBoxes(ui->landscapeTableView, LANDSCAPE);

	// LANGUAGE PACK
	setUpTableView(ui->languageTableView, TABLE_LANGUAGE_PACK);
	insertCheckBoxes(ui->languageTableView, LANGUAGEPACK);

	// SCRIPTS
	setUpTableView(ui->scriptsTableView, TABLE_SCRIPT);
	insertCheckBoxes(ui->scriptsTableView, SCRIPT);

	// STARLORE (SKY CULTURE)
	setUpTableView(ui->starloreTbleView, TABLE_SKY_CULTURE);
	insertCheckBoxes(ui->starloreTbleView, STARLORE);

	// TEXTURES
	setUpTableView(ui->texturesTableView, TABLE_TEXTURE);
	insertCheckBoxes(ui->texturesTableView, TEXTURE);
}

void AddOnDialog::updateCatalog()
{
	ui->btnUpdate->setEnabled(false);
	ui->txtLastUpdate->setText(q_("Updating catalog..."));

	QNetworkRequest req(QUrl("http://cardinot.sourceforge.net/getUpdates.php?time="
				 % QString::number(StelApp::getInstance().getStelAddOnMgr().getLastUpdate())));
	req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
	req.setAttribute(QNetworkRequest::RedirectionTargetAttribute, false);
	req.setRawHeader("User-Agent", StelUtils::getApplicationName().toLatin1());
	m_pUpdateCatalogReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	m_pUpdateCatalogReply->setReadBufferSize(1024*1024*2);
	connect(m_pUpdateCatalogReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
	connect(m_pUpdateCatalogReply, SIGNAL(error(QNetworkReply::NetworkError)),
		this, SLOT(downloadError(QNetworkReply::NetworkError)));
}

void AddOnDialog::downloadError(QNetworkReply::NetworkError)
{
	Q_ASSERT(m_pUpdateCatalogReply);
	qWarning() << "Error updating database catalog!" << m_pUpdateCatalogReply->errorString();
	ui->btnUpdate->setEnabled(true);
	ui->txtLastUpdate->setText(q_("Database update failed!"));
}

void AddOnDialog::downloadFinished()
{
	Q_ASSERT(m_pUpdateCatalogReply);
	if (m_pUpdateCatalogReply->error() != QNetworkReply::NoError)
	{
		m_pUpdateCatalogReply->deleteLater();
		m_pUpdateCatalogReply = NULL;
		return;
	}

	QByteArray data = m_pUpdateCatalogReply->readAll();
	QString result(data);

	if (!result.isEmpty())
	{
		if(!StelApp::getInstance().getStelAddOnMgr().updateCatalog(result))
		{
			ui->btnUpdate->setEnabled(true);
			ui->txtLastUpdate->setText(q_("Database update failed!"));
			return;
		}
	}

	qint64 currentTime = QDateTime::currentMSecsSinceEpoch()/1000;
	ui->btnUpdate->setEnabled(true);
	StelApp::getInstance().getStelAddOnMgr().setLastUpdate(currentTime);
	ui->txtLastUpdate->setText(StelApp::getInstance().getStelAddOnMgr().getLastUpdateString());
	populateTables();
}

void AddOnDialog::installSelectedRows() {
	int currentTab = ui->stackedWidget->currentIndex();
	Q_ASSERT(m_checkBoxes.at(currentTab).count() == m_currentTableView->model()->rowCount());
	QList<int> addonIdList;
	for (int i=0;i<m_checkBoxes.at(currentTab).count(); ++i)
	{
		if (!m_checkBoxes.at(currentTab).value(i)->checkState())
		{
			continue;
		}
		addonIdList.append(m_currentTableView->model()->index(i, 1).data().toInt());
	}

	foreach (int addonId , addonIdList) {
		StelApp::getInstance().getStelAddOnMgr().installAddOn(addonId);
	}

	// TODO: fix download result
	/*
	typedef QPair<QString, bool> Result;
	QList<Result> resultList; // <title, result>
	// Display results
	foreach (Result r, resultList) {
		// TODO: display it in a nice message box.
		QString installed = r.second ? " installed!" : " not installed!";
		qDebug() << "Add-on: " % r.first % installed;
	}
	*/
}

void AddOnDialog::removeSelectedRows() {
	int currentTab = ui->stackedWidget->currentIndex();
	Q_ASSERT(m_checkBoxes.at(currentTab).count() == m_currentTableView->model()->rowCount());
	QList<int> addonIdList;
	for (int i=0;i<m_checkBoxes.at(currentTab).count(); ++i)
	{
		if (!m_checkBoxes.at(currentTab).value(i)->checkState())
		{
			continue;
		}
		addonIdList.append(m_currentTableView->model()->index(i, 1).data().toInt());
	}

	foreach (int addonId , addonIdList) {
		StelApp::getInstance().getStelAddOnMgr().removeAddOn(addonId);
	}
}
