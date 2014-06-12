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

#ifndef _ADDONDIALOG_HPP_
#define _ADDONDIALOG_HPP_

#include "StelDialog.hpp"

#include <QListWidgetItem>
#include <QObject>

class Ui_addonDialogForm;

class AddOnDialog : public StelDialog
{
Q_OBJECT
public:
    AddOnDialog(QObject* parent);
    virtual ~AddOnDialog();
	//! Notify that the application style changed
	void styleChanged();

public slots:
	void retranslate();

protected:
	Ui_addonDialogForm* ui;
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();

private slots:
	void changePage(QListWidgetItem *current, QListWidgetItem *previous);

private:
	//! Defines the columns that will be displayed in the table view.
	//! @enum ModelColumns
	enum ModelColumns {
		ColumnTitle,
		ColumnInstalledVersion,
		ColumnLastVersion,
		ColumnCount
	};
};

#endif // _ADDONDIALOG_HPP_
