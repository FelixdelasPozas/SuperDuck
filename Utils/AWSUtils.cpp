/*
 File: AWSUtils.cpp
 Created on: 5/08/2019
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
#include <Utils/AWSUtils.h>

// C++
#include <winsock2.h>

// AWS
#include <aws/core/Aws.h>
#include <aws/core/utils/logging/AWSLogging.h>
#include <aws/core/utils/logging/DefaultLogSystem.h>
#include <aws/core/utils/threading/ThreadTask.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/utils/memory/stl/AWSAllocator.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/Object.h>
#include <aws/transfer/TransferHandle.h>

// Qt
#include <QFileInfo>

using namespace Aws;
using namespace Aws::Transfer;

static const char *ALLOCATION_TAG = "SuperDuckTransfer";

//-----------------------------------------------------------------------------
AWSUtils::S3Thread::S3Thread(Operation operation, QObject* parent)
: QThread(parent)
, m_operation(operation)
, m_abort{false}
, m_fileCount{0}
, m_manager{nullptr}
{
}

//-----------------------------------------------------------------------------
void AWSUtils::S3Thread::run()
{
  if(m_operation.useLogging)
  {
    Utils::Logging::InitializeAWSLogging(MakeShared<Utils::Logging::DefaultLogSystem>(ALLOCATION_TAG, Utils::Logging::LogLevel::Trace, "aws_sdk_"));
  }

  SDKOptions options;
  InitAPI(options);

  Aws::Client::ClientConfiguration clientConfig;
  clientConfig.region = m_operation.region;
  clientConfig.connectTimeoutMs = 30000;
  clientConfig.requestTimeoutMs = 30000;

  auto executor  = Aws::MakeShared<Aws::Utils::Threading::PooledThreadExecutor>(ALLOCATION_TAG, 4);
  auto s3_client = Aws::MakeShared<Aws::S3::S3Client>(ALLOCATION_TAG, m_operation.credentials, clientConfig);

  int progressValue = 0;
  int globalProgressValue = 0;
  QString fileNameKey;

  if(m_operation.type == AWSUtils::OperationType::remove)
  {
    unsigned int count = 0;
    for(auto it = m_operation.keys.cbegin(); it != m_operation.keys.cend() && !m_abort; ++it)
    {
      auto p = *it;
      auto shortName = QFileInfo(QString::fromStdString(p.first)).fileName();
      emit message(operationTypeToText(m_operation.type) + ": " + shortName);

      auto filename = Aws::String(p.first.c_str(), p.first.length());

      Aws::S3::Model::DeleteObjectRequest object_request;
      object_request.WithBucket(m_operation.bucket).WithKey(filename);

      auto result = s3_client->DeleteObject(object_request);

      if(m_abort) break;

      if (!result.IsSuccess())
      {
        const auto error = result.GetError();
        auto exceptionName = AWSUtils::toQString(error.GetExceptionName());
        auto errorMessage = AWSUtils::toQString(error.GetMessage());
        m_errors[QString::fromStdString(p.first)] << exceptionName + " -> " + errorMessage;
      }

      int pValue = (++count * 100)/m_operation.keys.size();
      if(progressValue != pValue)
      {
        progressValue = pValue;
        emit globalProgress(progressValue);
      }
    }
  }
  else
  {
    auto transferCallback = [&](const TransferManager *tm, const std::shared_ptr<const TransferHandle> &th)
    {
      auto fileKey = AWSUtils::toQString(th->GetKey());
      if(fileNameKey != fileKey)
      {
        fileNameKey = fileKey;
        auto shortName = QFileInfo(fileKey).fileName();
        emit message(operationTypeToText(m_operation.type) + ": " + shortName);
      }

      auto total = th->GetBytesTotalSize();
      auto current = th->GetBytesTransferred();
      int pValue = (current * 100)/total;

      if(progressValue != pValue)
      {
        progressValue = pValue;
        emit progress(pValue);
      }

      int gValue = (m_fileCount * 100)/m_operation.keys.size();
      if(globalProgressValue != gValue)
      {
        globalProgressValue = gValue;
        emit globalProgress(gValue);
      }
    };

    TransferManagerConfiguration transferManagerConfig(executor.get());
    transferManagerConfig.s3Client = s3_client;
    transferManagerConfig.downloadProgressCallback = transferCallback;
    transferManagerConfig.uploadProgressCallback = transferCallback;

    m_manager = TransferManager::Create(transferManagerConfig);

    if(m_operation.type == AWSUtils::OperationType::download)
    {
      std::shared_ptr<TransferHandle> downloadHandle;
      for(auto it = m_operation.keys.cbegin(); it != m_operation.keys.cend(); ++it)
      {
        auto p = *it;
        auto fKey = Aws::String(p.first.c_str(), p.first.length());
        auto fNameStr = QFileInfo(QString::fromStdString(p.first)).fileName();
        auto fName = AWSUtils::toAwsString(fNameStr);
        downloadHandle = m_manager->DownloadFile(fKey, m_operation.bucket, fName);
        downloadHandle->WaitUntilFinished();

        ++m_fileCount;

        int retries = 0;
        while(downloadHandle->GetStatus() == TransferStatus::FAILED && retries++ < 5)
        {
          m_manager->RetryDownload(downloadHandle);
          downloadHandle->WaitUntilFinished();
        }

        if(downloadHandle->GetStatus() != TransferStatus::COMPLETED)
        {
          const auto error = downloadHandle->GetLastError();
          auto exceptionName = AWSUtils::toQString(error.GetExceptionName());
          auto errorMessage = AWSUtils::toQString(error.GetMessage());
          m_errors[QString::fromStdString(p.first)] << exceptionName + " -> " + errorMessage;
        }

        if(m_abort) break;
      }
    }
    else
    {
      assert(m_operation.type == AWSUtils::OperationType::upload);

      std::shared_ptr<TransferHandle> uploadHandle;
      for(auto it = m_operation.keys.cbegin(); it != m_operation.keys.cend(); ++it)
      {
        auto p = *it;
        auto fName = Aws::String(p.first.c_str(), p.first.length());
        auto baseFile = QFileInfo(QString::fromStdString(p.first)).fileName();
        auto baseFileAws = AWSUtils::toAwsString(baseFile);
        auto fKeyStr = m_operation.parameters + baseFileAws;
        uploadHandle = m_manager->UploadFile(fName, m_operation.bucket,  baseFileAws, "binary", Aws::Map<Aws::String, Aws::String>());
        uploadHandle->WaitUntilFinished();

        ++m_fileCount;

        int retries = 0;
        while(uploadHandle->GetStatus() == TransferStatus::FAILED && retries++ < 5)
        {
          m_manager->RetryUpload(fName, uploadHandle);
          uploadHandle->WaitUntilFinished();
        }

        if(uploadHandle->GetStatus() != TransferStatus::COMPLETED)
        {
          const auto error = uploadHandle->GetLastError();
          auto exceptionName = AWSUtils::toQString(error.GetExceptionName());
          auto errorMessage = AWSUtils::toQString(error.GetMessage());
          m_errors[QString::fromStdString(p.first)] << exceptionName + " -> " + errorMessage;
        }

        if(m_abort) break;
      }
    }
  }

  emit progress(100);
  emit globalProgress(100);
  emit message("Finished!");

  ShutdownAPI(options);

  if(m_operation.useLogging)
  {
    Utils::Logging::ShutdownAWSLogging();
  }
}

//-----------------------------------------------------------------------------
void AWSUtils::S3Thread::abort()
{
  m_abort = true;
}

//-----------------------------------------------------------------------------
Aws::String AWSUtils::toAwsString(const QString& text)
{
  return Aws::String(text.toStdString().c_str(), text.length());
}

//-----------------------------------------------------------------------------
QString AWSUtils::toQString(const Aws::String& text)
{
  return QString::fromLocal8Bit(text.c_str(), text.length());
}

//-----------------------------------------------------------------------------
QString AWSUtils::operationTypeToText(const OperationType& type)
{
  switch(type)
  {
    case OperationType::download:
      return "Download";
      break;
    case OperationType::remove:
      return "Delete";
      break;
    case OperationType::upload:
      return "Upload";
      break;
    default:
      break;
  }

  return QString();
}

//-----------------------------------------------------------------------------
int AWSUtils::S3Thread::findCurrentFileIndex(const QString& key)
{
  int result = 0;

  std::vector<std::pair<std::string, unsigned long long>>::const_iterator it;

  if(m_operation.type == AWSUtils::OperationType::download)
  {
    auto findKey = [&key](const std::pair<std::string, unsigned long long> &p) { return key == QString::fromStdString(p.first); };
    it = std::find_if(m_operation.keys.cbegin(), m_operation.keys.cend(), findKey);
  }
  else
  {
    auto findKey = [&key](const std::pair<std::string, unsigned long long> &p)
    {
      QFileInfo keyInfo(key);
      QFileInfo other(QString::fromStdString(p.first));
      return keyInfo.fileName() == other.fileName();
    };
    it = std::find_if(m_operation.keys.cbegin(), m_operation.keys.cend(), findKey);
  }

  if(it != m_operation.keys.cend())
  {
    result = std::distance(m_operation.keys.cbegin(), it);
  }

  return result;
}

//-----------------------------------------------------------------------------
QString AWSUtils::permissionToText(Aws::S3::Model::Permission permission)
{
  switch(permission)
  {
    case Aws::S3::Model::Permission::FULL_CONTROL:
      return "FULL CONTROL";
      break;
    case Aws::S3::Model::Permission::WRITE:
      return "WRITE";
      break;
    case Aws::S3::Model::Permission::READ:
      return "READ";
      break;
    case Aws::S3::Model::Permission::WRITE_ACP:
      return "WRITE_ACP";
      break;
    case Aws::S3::Model::Permission::READ_ACP:
      return "READ_ACP";
      break;
    case Aws::S3::Model::Permission::NOT_SET:
    default:
      return "NOT SET";
      break;
  }

  return QString();
}
