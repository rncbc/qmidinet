// qmidinetOptionsForm.cpp
//
/****************************************************************************
   Copyright (C) 2010-2019, rncbc aka Rui Nuno Capela. All rights reserved.

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

#if defined(CONFIG_IPV6)
#include <QNetworkInterface>
#endif


//----------------------------------------------------------------------------
// qmidinetOptionsForm -- UI wrapper form.

// Constructor.
qmidinetOptionsForm::qmidinetOptionsForm (
	QWidget *pParent, Qt::WindowFlags wflags )
	: QDialog(pParent, wflags)
{
	// Setup UI struct...
	m_ui.setupUi(this);

	// Initialize the dialog widgets with default settings...
	m_sDefInterface = tr("(Any)");
	m_ui.InterfaceComboBox->clear();
	m_ui.InterfaceComboBox->addItem(m_sDefInterface);
#if defined(CONFIG_IPV6)
	foreach (const QNetworkInterface& iface, QNetworkInterface::allInterfaces()) {
		if (iface.isValid() &&
			iface.flags().testFlag(QNetworkInterface::CanMulticast) &&
			iface.flags().testFlag(QNetworkInterface::IsUp) &&
			iface.flags().testFlag(QNetworkInterface::IsRunning) &&
			!iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
			m_ui.InterfaceComboBox->addItem(iface.name());
		}
	}
#else
	m_ui.InterfaceComboBox->addItem("wlan0");
	m_ui.InterfaceComboBox->addItem("eth0");
#endif

	m_ui.UdpAddrComboBox->clear();
	m_ui.UdpAddrComboBox->addItem(QMIDINET_UDP_IPV4_ADDR);
#if defined(CONFIG_IPV6)
	m_ui.UdpAddrComboBox->addItem(QMIDINET_UDP_IPV6_ADDR);
#endif
	m_ui.UdpPortSpinBox->setValue(QMIDINET_UDP_PORT);

	// Populate dialog widgets with current settings...
	qmidinetOptions *pOptions = qmidinetOptions::getInstance();
	if (pOptions) {
		if (pOptions->sInterface.isEmpty())
			m_ui.InterfaceComboBox->setCurrentIndex(0);
		else
			m_ui.InterfaceComboBox->setEditText(pOptions->sInterface);
		if (pOptions->sUdpAddr.isEmpty())
			m_ui.UdpAddrComboBox->setCurrentIndex(0);
		else
			m_ui.UdpAddrComboBox->setEditText(pOptions->sUdpAddr);
		comboAddressTextChanged(m_ui.UdpAddrComboBox->currentText());
		m_ui.UdpPortSpinBox->setValue(pOptions->iUdpPort);
		m_ui.NumPortsSpinBox->setValue(pOptions->iNumPorts);
		m_ui.AlsaMidiCheckBox->setChecked(pOptions->bAlsaMidi);
		m_ui.JackMidiCheckBox->setChecked(pOptions->bJackMidi);
	}

#ifndef CONFIG_ALSA_MIDI
	m_ui.AlsaMidiCheckBox->setEnabled(false);
#endif
#ifndef CONFIG_JACK_MIDI
	m_ui.JackMidiCheckBox->setEnabled(false);
#endif

	// Start clean.
	m_iDirtyCount = 0;

	// Try to fix window geometry.
	adjustSize();

	// UI signal/slot connections...
	QObject::connect(m_ui.InterfaceComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(change()));
	QObject::connect(m_ui.UdpAddrComboBox,
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
	QObject::connect(m_ui.DialogButtonBox,
		SIGNAL(clicked(QAbstractButton *)),
		SLOT(buttonClick(QAbstractButton *)));
	QObject::connect(m_ui.UdpAddrComboBox,
		SIGNAL(editTextChanged(const QString&)),
		SLOT(comboAddressTextChanged(const QString&)));
}


// Change settings (anything else slot).
void qmidinetOptionsForm::change (void)
{
	++m_iDirtyCount;
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
			pOptions->sUdpAddr   = m_ui.UdpAddrComboBox->currentText();
			pOptions->iUdpPort   = m_ui.UdpPortSpinBox->value();
			pOptions->iNumPorts  = m_ui.NumPortsSpinBox->value();
			pOptions->bAlsaMidi  = m_ui.AlsaMidiCheckBox->isChecked();
			pOptions->bJackMidi  = m_ui.JackMidiCheckBox->isChecked();
			// Take care of some translatable adjustments...
			if (pOptions->sInterface == m_sDefInterface)
				pOptions->sInterface.clear();
			// Save/commit to disk.
			pOptions->saveOptions();
			// Clean all dirt...
			m_iDirtyCount = 0;
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


// Reset options (generic button slot).
void qmidinetOptionsForm::buttonClick ( QAbstractButton *pButton )
{
	const QDialogButtonBox::ButtonRole buttonRole
		= m_ui.DialogButtonBox->buttonRole(pButton);
	if (buttonRole == QDialogButtonBox::ResetRole) {
		m_ui.InterfaceComboBox->setCurrentIndex(0);
		m_ui.UdpAddrComboBox->setCurrentIndex(0);
		m_ui.UdpPortSpinBox->setValue(QMIDINET_UDP_PORT);
	}
}

void qmidinetOptionsForm::comboAddressTextChanged(const QString &text)
{
#if defined(CONFIG_IPV6)
	QHostAddress addr;
	if(addr.setAddress(text) && addr.isMulticast()) {
		switch(addr.protocol()) {
		case QAbstractSocket::IPv4Protocol:
			m_ui.ProtocolLineEdit->setText("IPv4");
			break;
		case QAbstractSocket::IPv6Protocol:
			m_ui.ProtocolLineEdit->setText("IPv6");
			break;
		default:
			m_ui.ProtocolLineEdit->setText(tr("Invalid address"));
			break;
		}
	} else {
		m_ui.ProtocolLineEdit->setText(tr("Invalid address"));
	}
#else
	m_ui.ProtocolLineEdit->setText("IPv4");
#endif
}

// end of qmidinetOptionsForm.cpp
