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

#include "qmidinetOptionsForm.h"

#include "qmidinetOptions.h"


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
	}

	// Try to fix window geometry.
	adjustSize();

	// UI signal/slot connections...
	QObject::connect(m_ui.AcceptButton,
		SIGNAL(clicked()),
		SLOT(accept()));
	QObject::connect(m_ui.RejectButton,
		SIGNAL(clicked()),
		SLOT(reject()));
}


// Accept settings (OK button slot).
void qmidinetOptionsForm::accept (void)
{
	// Save options...
	qmidinetOptions *pOptions = qmidinetOptions::getInstance();
	if (pOptions) {
		// Display options...
		pOptions->sInterface = m_ui.InterfaceComboBox->currentText();
		pOptions->iUdpPort   = m_ui.UdpPortSpinBox->value();
		pOptions->iNumPorts  = m_ui.NumPortsSpinBox->value();
		// Take care of some translatable adjustments...
		if (pOptions->sInterface == m_sDefInterface)
			pOptions->sInterface.clear();
		// Save/commit to disk.
		pOptions->saveOptions();
	}

	// Just go with dialog acceptance
	QDialog::accept();
}


// end of qmidinetOptionsForm.cpp

