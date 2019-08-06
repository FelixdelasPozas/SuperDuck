/*
 File: SettingsDialog.cpp
 Created on: 6/08/2019
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
#include <SettingsDialog.h>
#include <Utils.h>

// Qt
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>

const QStringList REGIONS = { "us-east-1", "us-east-2", "us-west-1", "us-west-2", "ca-central-1",
                              "eu-central-1", "eu-west-1", "eu-west-2", "eu-west-3", "eu-north-1",
                              "ap-east-1", "ap-northeast-1", "ap-northeast-2", "ap-northeast-3",
                              "ap-southeast-1", "ap-southeast-2", "ap-south-1", "sa-east-1"};

//-----------------------------------------------------------------------------
SettingsDialog::SettingsDialog(Utils::Configuration &config, QWidget* parent, Qt::WindowFlags f)
: QDialog(parent, f)
{
  setupUi(this);

  m_keyId->setText(Utils::rot13(config.AWS_Access_key_id));
  m_accessKey->setText(Utils::rot13(config.AWS_Secret_access_key));
  m_bucket->setText(config.AWS_Bucket);
  m_regionCombo->insertItems(0, REGIONS);
  if(!config.AWS_Region.isEmpty()) m_regionCombo->setCurrentIndex(REGIONS.indexOf(config.AWS_Region));

  m_dbLine->setText(QDir::toNativeSeparators(config.Database_file));
  m_downloadPaths->setChecked(config.Download_Full_Paths);
  m_exportPaths->setChecked(config.Export_Full_Paths);

  connectSignals();

  checkCredentialsFile();
}

//-----------------------------------------------------------------------------
void SettingsDialog::onFolderButtonClicked()
{
  auto dbFile = QFileDialog::getOpenFileName(this, tr("Select database file"), Utils::databaseFile(), tr("Text files (*.txt)"));

  if(!dbFile.isEmpty() && !Utils::isDatabaseFile(dbFile))
  {
    QMessageBox::critical(this, tr("Select database file"), tr("'%1' does not appear to be a valid database file.").arg(dbFile));
  }
  else
  {
    m_dbLine->setText(QDir::toNativeSeparators(dbFile));
  }
}

//-----------------------------------------------------------------------------
void SettingsDialog::connectSignals()
{
  connect(m_dirButton, SIGNAL(clicked(bool)), this, SLOT(onFolderButtonClicked()));
}

//-----------------------------------------------------------------------------
void SettingsDialog::accept()
{
  const auto title = tr("Settings");
  QString message;

  if(m_keyId->text().isEmpty() || m_keyId->text().length() != 20)
  {
    message = tr("AWS access key is not valid.");
  }

  if(m_accessKey->text().isEmpty() || m_accessKey->text().length() != 40)
  {
    message = tr("AWS secret access key is not valid.");
  }

  if(m_bucket->text().isEmpty())
  {
    message = tr("Invalid bucket.");
  }

  if(!Utils::isDatabaseFile(m_dbLine->text()))
  {
    message = tr("'%1' does not appear to be a valid database file.").arg(m_dbLine->text());
  }

  if(!message.isEmpty())
  {
    QMessageBox::critical(this, title, message);
    return;
  }

  QDialog::accept();
}

//-----------------------------------------------------------------------------
Utils::Configuration SettingsDialog::configuration() const
{
  Utils::Configuration config;
  config.AWS_Access_key_id = Utils::rot13(m_keyId->text());
  config.AWS_Secret_access_key = Utils::rot13(m_accessKey->text());
  config.AWS_Bucket = m_bucket->text();
  config.AWS_Region = REGIONS.at(m_regionCombo->currentIndex());
  config.Database_file = QDir::fromNativeSeparators(m_dbLine->text());
  config.Export_Full_Paths = m_exportPaths->isChecked();
  config.Download_Full_Paths = m_downloadPaths->isChecked();

  return config;
}

//-----------------------------------------------------------------------------
void SettingsDialog::checkCredentialsFile()
{
  auto homeDir = QDir(QDir::homePath());
  auto credentialsExists = homeDir.exists(".aws/credentials");

  m_credentialsFileGroup->setEnabled(!credentialsExists);

  auto message = (credentialsExists ? tr("AWS credentials file exists.") : tr("AWS credentials file doesn't exists"));
  m_credentialsFileLabel->setText(message);

  if(!credentialsExists)
  {
    connect(m_creadentialsFileButton, SIGNAL(clicked(bool)), this, SLOT(createCredentialsFile()));
  }
}

//-----------------------------------------------------------------------------
void SettingsDialog::createCredentialsFile()
{
  auto key = m_keyId->text();
  auto access_key = m_accessKey->text();

  if(key.isEmpty() || key.length() < 20 || access_key.isEmpty() || access_key.length() < 40)
  {
    auto message = tr("The credentials fields are invalid.");
    QMessageBox::critical(this, tr("Create AWS credentials file"), message);
    return;
  }

  auto homeDir = QDir(QDir::homePath());
  if(!homeDir.exists(".aws") && !homeDir.mkdir(".aws"))
  {
    auto message = tr("Unable to create AWS home directory.");
    QMessageBox::critical(this, tr("Create AWS credentials file"), message);
    return;
  }

  QFile awsFile(homeDir.absoluteFilePath(".aws/credentials"));

  if(!awsFile.open(QIODevice::WriteOnly|QIODevice::Truncate))
  {
    auto message = tr("Unable to create AWS credentials file.");
    QMessageBox::critical(this, tr("Create AWS credentials file"), message);
    return;
  }

  awsFile.write("[default]\n");
  awsFile.write(tr("aws_access_key_id = %1\n").arg(m_keyId->text()).toLatin1());
  awsFile.write(tr("aws_secret_access_key = %1\n").arg(m_accessKey->text()).toLatin1());
  awsFile.close();

  checkCredentialsFile();
}
