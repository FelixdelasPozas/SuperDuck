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
#include <AWSUtils.h>

// AWS
#include <winsock2.h>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/Object.h>

const Aws::String BUCKET_NAME = "lccc-pato";

//-----------------------------------------------------------------------------
bool AWSUtils::downloadFiles(const std::string& path, const std::vector<std::pair<std::string, unsigned long long> > contents, bool fullPaths)
{
  Aws::SDKOptions options;
  Aws::InitAPI(options);
  {
    const Aws::String bucket_name = BUCKET_NAME;
    std::cout << "Objects in S3 bucket: " << bucket_name << std::endl;

    Aws::S3::S3Client s3_client;

    Aws::S3::Model::ListObjectsRequest objects_request;
    objects_request.WithBucket(bucket_name);

    auto list_objects_outcome = s3_client.ListObjects(objects_request);

    if (list_objects_outcome.IsSuccess())
    {
      Aws::Vector<Aws::S3::Model::Object> object_list = list_objects_outcome.GetResult().GetContents();

      for (auto const &s3_object : object_list)
      {
        std::cout << "* " << s3_object.GetKey() << std::endl;
      }
    }
    else
    {
      std::cout << "ListObjects error: " << list_objects_outcome.GetError().GetExceptionName() << " " << list_objects_outcome.GetError().GetMessage() << std::endl;
    }
  }

  Aws::ShutdownAPI(options);
  return false;
}
