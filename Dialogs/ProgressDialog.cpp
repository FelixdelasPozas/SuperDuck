/*
 File: ProgressDialog.cpp
 Created on: 8/08/2019
 Author: Felix de las Pozas Alvarez

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Project
#include <Dialogs/ProgressDialog.h>

//-----------------------------------------------------------------------------
ProgressDialog::ProgressDialog(AWSUtils::S3Thread* thread, QWidget* parent, Qt::WindowFlags flags)
: QDialog(parent, flags)
, m_thread(thread)
{
  setupUi(this);

  connect(m_thread, SIGNAL(globalProgress(int)), this, SLOT(setGlobalProgress(int)));
  connect(m_thread, SIGNAL(progress(int)), this, SLOT(setProgress(int)));
  connect(m_thread, SIGNAL(message(const QString &)), this, SLOT(setMessage(const QString &)));
  connect(m_thread, SIGNAL(finished()), this, SLOT(close()));

  connect(m_cancelButton, SIGNAL(clicked(bool)), this, SLOT(onCancelButtonPressed()));
}

//-----------------------------------------------------------------------------
void ProgressDialog::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  m_thread->run();
}

//-----------------------------------------------------------------------------
void ProgressDialog::setGlobalProgress(int progress)
{
  m_globalProgress->setValue(progress);
}

//-----------------------------------------------------------------------------
void ProgressDialog::setProgress(int progress)
{
  m_operationProgress->setValue(progress);
}

//-----------------------------------------------------------------------------
void ProgressDialog::setMessage(const QString& message)
{
  m_operationLabel->setText(message);
}

//-----------------------------------------------------------------------------
void ProgressDialog::onCancelButtonPressed()
{
  m_thread->abort();
  close();
}
