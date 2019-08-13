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
#include <Dialogs/SettingsDialog.h>
#include <Utils/Utils.h>
#include <Utils/AWSUtils.h>

// Qt
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QDebug>

// AWS
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetBucketAclRequest.h>

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
  m_downloadLineEdit->setText(QDir::toNativeSeparators(config.DownloadPath));
  m_disableDelete->setChecked(config.DisableDelete);

  connectSignals();

  checkCredentialsFile();
}

//-----------------------------------------------------------------------------
void SettingsDialog::onFolderButtonClicked()
{
  auto dbFile = QFileDialog::getOpenFileName(this, tr("Select database file"), Utils::databaseFile(), tr("Text files (*.txt)"));

  if(!dbFile.isEmpty())
  {
    QFileInfo info(dbFile);
    if(info.exists() && info.isReadable() && info.isWritable())
    {
      m_dbLine->setText(QDir::toNativeSeparators(dbFile));
    }
    else
    {
      QMessageBox::critical(this, tr("Select database file"), tr("'%1' does not appear to be a valid database file.").arg(dbFile));
    }
  }
}

//-----------------------------------------------------------------------------
void SettingsDialog::connectSignals()
{
  connect(m_dirButton, SIGNAL(clicked(bool)), this, SLOT(onFolderButtonClicked()));
  connect(m_downloadButton, SIGNAL(clicked(bool)), this, SLOT(onDownloadPathButtonClicked()));
  connect(m_permissionsButton, SIGNAL(clicked(bool)), this, SLOT(onPermissionsButtonClicked()));
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

  QFileInfo info(m_downloadLineEdit->text());
  if(!info.exists() || !info.isWritable())
  {
    message = tr("'%1' does not appear to be a valid directory or can't write in it.").arg(m_downloadLineEdit->text());
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
  config.DownloadPath = QDir::fromNativeSeparators(m_downloadLineEdit->text());
  config.DisableDelete = m_disableDelete->isChecked();

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

//-----------------------------------------------------------------------------
void SettingsDialog::onDownloadPathButtonClicked()
{
  const auto title = tr("Select download directory");
  auto path = QFileDialog::getExistingDirectory(this, title, m_downloadLineEdit->text(), QFileDialog::ShowDirsOnly);

  if(!path.isEmpty())
  {
    QFileInfo info(path);
    if(info.exists() && info.isDir() && info.isWritable())
    {
      m_downloadLineEdit->setText(QDir::toNativeSeparators(path));
    }
    else
    {
      QMessageBox::critical(this, title, tr("'%1' does not appear to be a valid directory or can't write in it.").arg(path));
    }
  }
}

//-----------------------------------------------------------------------------
void SettingsDialog::onPermissionsButtonClicked()
{
  const auto key = m_keyId->text();
  const auto access_key = m_accessKey->text();

  if(key.isEmpty() || key.length() != 20 || access_key.isEmpty() || access_key.length() != 40)
  {
    QMessageBox::warning(this, tr("Check AWS permissions"), tr("Cannot check without valid AWS credentials"));
    return;
  }

  Aws::SDKOptions options;
  Aws::InitAPI(options);
  {
    auto credentials = Aws::Auth::AWSCredentials(AWSUtils::toAwsString(m_keyId->text()), AWSUtils::toAwsString(m_accessKey->text()));

    Aws::Client::ClientConfiguration clientConfig;
    clientConfig.region = AWSUtils::toAwsString(REGIONS.at(m_regionCombo->currentIndex()));
    clientConfig.connectTimeoutMs = 30000;
    clientConfig.requestTimeoutMs = 30000;

    // Set up the get request
    Aws::S3::S3Client s3_client(credentials, clientConfig);

    Aws::S3::Model::GetBucketAclRequest get_request;
    auto bucket = AWSUtils::toAwsString(m_bucket->text());
    get_request.SetBucket(bucket);

    // Get the current access control policy
    auto result = s3_client.GetBucketAcl(get_request);
    if (!result.IsSuccess())
    {
      auto error = result.GetError();
      auto message = tr("Error: %1. %2.").arg(AWSUtils::toQString(error.GetExceptionName())).arg(AWSUtils::toQString(error.GetMessage()));
      m_permissionsLineEdit->setText(message);
    }
    else
    {
      QStringList permissions;
      auto grants = result.GetResult().GetGrants();
      for (auto & grant : grants)
      {
        permissions << AWSUtils::permissionToText(grant.GetPermission());
      }

      auto text = permissions.join(" + ");
      m_permissionsLineEdit->setText(text);
    }
  }
  Aws::ShutdownAPI(options);
}
