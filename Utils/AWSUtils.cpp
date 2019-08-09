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
#include <aws/s3/S3Client.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/Object.h>

using namespace Aws;

//-----------------------------------------------------------------------------
AWSUtils::S3Thread::S3Thread(Operation operation, QObject* parent)
: QThread(parent)
, m_operation(operation)
, m_abort{false}
{
}

//-----------------------------------------------------------------------------
void AWSUtils::S3Thread::run()
{
  if(m_operation.useLogging)
  {
    Utils::Logging::InitializeAWSLogging(MakeShared<Utils::Logging::DefaultLogSystem>("RunUnitTests", Utils::Logging::LogLevel::Trace, "aws_sdk_"));
  }

  SDKOptions options;
  InitAPI(options);

  switch(m_operation.type)
  {
    case OperationType::download:
      downloadOperation();
      break;
    case OperationType::upload:
      uploadOperation();
      break;
    case OperationType::remove:
      removeOperation();
      break;
    default:
      m_errors << "Undefined operation.";
      break;
  }

  ShutdownAPI(options);

  if(m_operation.useLogging)
  {
    Utils::Logging::ShutdownAWSLogging();
  }
}

//-----------------------------------------------------------------------------
void AWSUtils::S3Thread::downloadOperation()
{
  emit globalProgress(10);
  emit message("Configuring operation");

  Aws::Client::ClientConfiguration clientConfig;
  clientConfig.region = m_operation.region;

  S3::S3Client s3_client(m_operation.credentials, clientConfig);

  S3::Model::ListObjectsRequest objects_request;
  objects_request.WithBucket(m_operation.bucket);

  emit globalProgress(30);
  emit message("Starting operation");

  auto list_objects_outcome = s3_client.ListObjects(objects_request);

  bool retry = true;
  int count = 0;
  while (count < 3 && retry)
  {
    if (list_objects_outcome.IsSuccess())
    {
      auto object_list = list_objects_outcome.GetResult().GetContents();

      for (auto const &s3_object : object_list)
      {
        std::cout << "* " << s3_object.GetKey() << std::endl;
      }

      retry = false;
    }
    else
    {
      const auto error = list_objects_outcome.GetError();

      const auto exceptionName = error.GetExceptionName();
      const auto message = error.GetMessage();
      m_errors << QString::fromLocal8Bit(exceptionName.c_str(), exceptionName.length());
      m_errors << QString::fromLocal8Bit(message.c_str(), message.length());

      retry = error.ShouldRetry();
    }
  }
  emit message("Ending operation");
  emit globalProgress(100);
}

//-----------------------------------------------------------------------------
void AWSUtils::S3Thread::uploadOperation()
{
  // TODO
}

//-----------------------------------------------------------------------------
void AWSUtils::S3Thread::removeOperation()
{
  // TODO
}

//-----------------------------------------------------------------------------
void AWSUtils::S3Thread::abort()
{
  m_abort = true;

  // TODO: how to stop?
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
