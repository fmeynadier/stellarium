/*
 * Angle Measure plug-in for Stellarium
 *
 * Copyright (C) 2014 Alexander Wolf, Georg Zotti
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <QHostInfo>
#include "RemoteControl.hpp"
#include "RemoteControlDialog.hpp"
#include "ui_remoteControlDialog.h"

#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"

RemoteControlDialog::RemoteControlDialog()
	: StelDialog("RemoteControl")
	, rc(Q_NULLPTR)
{
	ui = new Ui_remoteControlDialog();
}

RemoteControlDialog::~RemoteControlDialog()
{
	delete ui;
}

void RemoteControlDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutHtml();
	}
}

void RemoteControlDialog::createDialogContent()
{
	rc = GETSTELMODULE(RemoteControl);
	ui->setupUi(dialog);

#ifdef Q_OS_WIN
	//Kinetic scrolling for tablet pc and pc
	QList<QWidget *> addscroll;
	addscroll << ui->aboutTextBrowser;
	installKineticScrolling(addscroll);
#endif

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	// TODO Fill other buttons

	connectCheckBox(ui->enabledCheckbox,"actionShow_Remote_Control");
	connect(ui->enabledCheckbox, SIGNAL(clicked(bool)), this, SLOT(updateIPlabel(bool)));
	updateIPlabel(ui->enabledCheckbox->isChecked());

	ui->activateOnStartCheckBox->setChecked(rc->getFlagAutoStart());
	connect(ui->activateOnStartCheckBox, SIGNAL(toggled(bool)), rc, SLOT(setFlagAutoStart(bool)));
	connect(rc, SIGNAL(flagAutoStartChanged(bool)), ui->activateOnStartCheckBox, SLOT(setChecked(bool)));

	ui->passwordCheckBox->setChecked(rc->getFlagUsePassword());
	connect(ui->passwordCheckBox, SIGNAL(toggled(bool)), rc, SLOT(setFlagUsePassword(bool)));
	connect(rc, SIGNAL(flagUsePasswordChanged(bool)), ui->passwordCheckBox, SLOT(setChecked(bool)));

	ui->passwordEdit->setEnabled(rc->getFlagUsePassword());
	ui->passwordEdit->setText(rc->getPassword());

	connect(rc,SIGNAL(flagUsePasswordChanged(bool)),ui->passwordEdit,SLOT(setEnabled(bool)));
	connect(ui->passwordEdit, SIGNAL(textChanged(QString)), rc, SLOT(setPassword(QString)));

	ui->portNumberSpinBox->setValue(rc->getPort());
	connect(ui->portNumberSpinBox, SIGNAL(valueChanged(int)), rc, SLOT(setPort(int)));

	ui->restartPanel->setVisible(false);
	connect(rc, SIGNAL(flagUsePasswordChanged(bool)), this, SLOT(requiresRestart()));
	connect(rc, SIGNAL(passwordChanged(QString)), this, SLOT(requiresRestart()));
	connect(rc, SIGNAL(portChanged(int)), this, SLOT(requiresRestart()));

	connect(ui->resetButton, SIGNAL(clicked(bool)),this,SLOT(restart()));

	connect(ui->saveSettingsButton, SIGNAL(clicked()), rc, SLOT(saveSettings()));
	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), rc, SLOT(restoreDefaultSettings()));

	setAboutHtml();
}

void RemoteControlDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Remote Control Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + REMOTECONTROL_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + REMOTECONTROL_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td rowspan=2><strong>" + q_("Authors") + ":</strong></td><td>Florian Schaukowitsch</td></tr>";
	html += "<tr><td>Georg Zotti</td></tr>";
	html += "<tr><td><strong>" + q_("Contributors") + ":</strong></td><td>Alexander Wolf</td></tr>";
	html += "</table>";

	html += "<p>" + q_("The Remote Control plugin provides a web interface to allow state changes and triggering scripts using a connected webbrowser.") + "</p>";
	// TODO Add longer instructions?

	html += "<p>" + q_("It is also possible to send commands via command line, e.g..");
	html += "<pre>\n"
		"wget -q --post-data 'id=myScript.ssc' http://localhost:8090/api/scripts/run >/dev/null 2>&amp;1\n"
		"curl --data 'id=myScript.ssc' http://localhost:8090/api/scripts/run >/dev/null 2>&amp;1\n"
		"curl -d     'id=myScript.ssc' http://localhost:8090/api/scripts/run >/dev/null 2>&amp;1\n"
		"</pre>";
	html += q_("This allows triggering automatic show setups for museums etc.") + "</p>";
	html += "<p>" + q_("This plugin was developed during ESA SoCiS 2015.") + "</p>";
	html += "<p>" + q_("This plugin uses the %1QtWebApp HTTP server%2 by Stefan Frings.").arg("<a href=\"http://stefanfrings.de/qtwebapp/index-en.html\">").arg("</a>") + "</p>";

	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Further information can be found in the %1developer documentation%2.")).arg("<a href=\"http://stellarium.org/doc-plugins/head/\">").arg("</a>") + "</p>";
	html += "<p>" + QString(q_("Support is provided via the Launchpad website.  Be sure to put \"%1\" in the subject when posting.")).arg("Remote Control plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("If you have a question, you can %1get an answer here%2").arg("<a href=\"https://answers.launchpad.net/stellarium\">")).arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("Bug reports can be made %1here%2.")).arg("<a href=\"https://bugs.launchpad.net/stellarium\">").arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you would like to make a feature request, you can create a bug report, and set the severity to \"wishlist\".") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you want to read full information about this plugin and its history, you can %1get info here%2.").arg("<a href=\"http://stellarium.org/wiki/index.php/RemoteControl_plugin\">").arg("</a>") + "</li>";
	html += "</ul></p></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}
	ui->aboutTextBrowser->setHtml(html);
}

void RemoteControlDialog::requiresRestart()
{
	ui->restartPanel->setVisible(rc->getFlagEnabled());
}

void RemoteControlDialog::restart()
{
	rc->stopServer();
	rc->startServer();
	ui->restartPanel->setVisible(false);
}

void RemoteControlDialog::updateIPlabel(bool running)
{
	if (running)
	{
		QString localHostName=QHostInfo::localHostName();
		QHostInfo hostInfo = QHostInfo::fromName(localHostName);
		QString ipString("");
		foreach (QHostAddress a, hostInfo.addresses())
		{
			if ((a.protocol() == QAbstractSocket::IPv4Protocol) && a != QHostAddress(QHostAddress::LocalHost))
			{
				ipString += a.toString() + " ";
				continue;
			}
		}
		ui->label_RemoteRunningState->setText(q_("Listening on %1, IP: ").arg(localHostName) + ipString);
		ui->label_RemoteRunningState->show();
	}
	else
	{
		ui->label_RemoteRunningState->setText(q_("Not active."));
		// Maybe even hide the label?
		ui->label_RemoteRunningState->hide();
	}
}
