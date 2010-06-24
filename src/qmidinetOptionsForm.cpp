// qmidinetOptionsForm.cpp
//
/****************************************************************************
   Copyright (C) 2010, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#include "qmidinetAbout.h"
#include "qmidinetOptionsForm.h"

#include "qmidinetOptions.h"

#include <QMessageBox>


//----------------------------------------------------------------------------
// qmidinetOptionsForm -- UI wrapper form.

// Constructor.
qmidinetOptionsForm::qmidinetOptionsForm (
	QWidget *pParent, Qt::WindowFlags wflags )
	: QDialog(pParent, wflags)
{
	// Setup UI struct...
	m_ui.setupUi(this);

	// Initialize the dialog widgets with deafult settings...
	m_sDefInterface = tr("(Any)");
	m_ui.InterfaceComboBox->clear();
	m_ui.InterfaceComboBox->addItem(m_sDefInterface);
	m_ui.InterfaceComboBox->addItem("wlan0");
	m_ui.InterfaceComboBox->addItem("eth0");

	// Populate dialog widgets with current settings...
	qmidinetOptions *pOptions = qmidinetOptions::getInstance();
	if (pOptions) {
		if (pOptions->sInterface.isEmpty())
			m_ui.InterfaceComboBox->setCurrentIndex(0);
		else
			m_ui.InterfaceComboBox->setEditText(pOptions->sInterface);
		m_ui.UdpPortSpinBox->setValue(pOptions->iUdpPort);
		m_ui.NumPortsSpinBox->setValue(pOptions->iNumPorts);
		m_ui.AlsaMidiCheckBox->setChecked(pOptions->bAlsaMidi);
		m_ui.JackMidiCheckBox->setChecked(pOptions->bJackMidi);
	}


	// Start clean.
	m_iDirtyCount = 0;

	// Try to fix window geometry.
	adjustSize();

	// UI signal/slot connections...
	QObject::connect(m_ui.InterfaceComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(change()));
	QObject::connect(m_ui.UdpPortSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(change()));
	QObject::connect(m_ui.NumPortsSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(change()));
	QObject::connect(m_ui.AlsaMidiCheckBox,
		SIGNAL(toggled(bool)),
		SLOT(change()));
	QObject::connect(m_ui.JackMidiCheckBox,
		SIGNAL(toggled(bool)),
		SLOT(change()));
	QObject::connect(m_ui.DialogButtonBox,
		SIGNAL(accepted()),
		SLOT(accept()));
	QObject::connect(m_ui.DialogButtonBox,
		SIGNAL(rejected()),
		SLOT(reject()));
}


// Change settings (anything else slot).
void qmidinetOptionsForm::change (void)
{
	m_iDirtyCount++;
}


// Accept settings (OK button slot).
void qmidinetOptionsForm::accept (void)
{
	// Save options...
	if (m_iDirtyCount > 0) {
		qmidinetOptions *pOptions = qmidinetOptions::getInstance();
		if (pOptions) {
			// Display options...
			pOptions->sInterface = m_ui.InterfaceComboBox->currentText();
			pOptions->iUdpPort   = m_ui.UdpPortSpinBox->value();
			pOptions->iNumPorts  = m_ui.NumPortsSpinBox->value();
			pOptions->bAlsaMidi  = m_ui.AlsaMidiCheckBox->isChecked();
			pOptions->bJackMidi  = m_ui.JackMidiCheckBox->isChecked();
			// Take care of some translatable adjustments...
			if (pOptions->sInterface == m_sDefInterface)
				pOptions->sInterface.clear();
			// Save/commit to disk.
			pOptions->saveOptions();
		}
	}

	// Just go with dialog acceptance
	QDialog::accept();
}


// Reject options (Cancel button slot).
void qmidinetOptionsForm::reject (void)
{
	// Check if there's any pending changes...
	if (m_iDirtyCount > 0) {
		switch (QMessageBox::warning(this,
			QDialog::windowTitle(),
			tr("Some settings have been changed.\n\n"
			"Do you want to apply the changes?"),
			QMessageBox::Apply |
			QMessageBox::Discard |
			QMessageBox::Cancel)) {
		case QMessageBox::Discard:
			break;
		case QMessageBox::Apply:
			accept();
		default:
			return;
		}
	}

	// Just go with dialog rejection...
	QDialog::reject();
}


// end of qmidinetOptionsForm.cpp

