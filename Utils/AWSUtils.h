/*
 File: AWSUtils.h
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

#ifndef AWSUTILS_H_
#define AWSUTILS_H_

// C++
#include <vector>
#include <winsock2.h>

// AWS
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/s3/model/Permission.h>
#include <aws/transfer/TransferManager.h>

// Qt
#include <QThread>
#include <QMap>

namespace AWSUtils
{
  /** \brief Helper method to convert a QString into an Aws::String.
   * \param[in] text QString object.
   *
   */
  Aws::String toAwsString(const QString &text);

  /** \brief Helper method to convert a Aws::String into a QString.
   * \param[in] text Aws::String object.
   *
   */
  QString toQString(const Aws::String &text);

  enum class OperationType: char { download = 0, upload, remove };

  /** \brief Returns the text of the given operation.
   * \param[in] type Operation type.
   *
   */
  QString operationTypeToText(const OperationType &type);

  /** \brief Helper method that returns the text equivalent of the permission.
   * \param[in] permission AWS model permission enum.
   *
   */
  QString permissionToText(Aws::S3::Model::Permission permission);

  static const QString DELIMITER =  "/";

  /** \struct Operation
   * \brief Defines an operation over a bucket.
   *
   */
  struct Operation
  {
    Aws::Auth::AWSCredentials                                credentials; /** S3 credentials.                             */
    Aws::String                                              bucket;      /** S3 bucket.                                  */
    Aws::String                                              region;      /** S3 region.                                  */
    OperationType                                            type;        /** type of operation.                          */
    std::vector<std::pair<std::string, unsigned long long>>  keys;        /** operation elements.                         */
    Aws::String                                              parameters;  /** additional operation parameters.            */
    bool                                                     useLogging;  /** true to log the operation, false otherwise. */
  };

  /** \class S3Thread
   * \brief Base class for operations on a S3 bucket.
   *
   */
  class S3Thread
  : public QThread
  {
      Q_OBJECT
    public:
      /** \brief S3Thread class constructor.
       * \param[in] operation Operation struct.
       * \param[in] parent Raw pointer of the QObject parent of this one.
       *
       */
      explicit S3Thread(Operation operation, QObject *parent = nullptr);

      /** \brief S3Thread class virtual destructor.
       *
       */
      virtual ~S3Thread()
      {};

      virtual void run();

      /** \brief Returns the list of errors of the operation, if any.
       *
       */
      QMap<QString, QStringList> errors() const
      { return m_errors; }

      /** \brief Returns the operation data.
       *
       */
      const Operation &operation() const
      { return m_operation; }

      /** \brief Aborts the operation.
       *
       */
      void abort();

      /** \brief Returns true if the task has been aborted during execution and false otherwise.
       *
       */
      bool isAborted() const;

    signals:
      void progress(int);
      void globalProgress(int);
      void message(const QString &);

    private:
      /** \brief Returns the index of the given key in the operation keys.
       * \param[in] key Text string.
       *
       */
      int findCurrentFileIndex(const QString &key);

      const Operation            m_operation; /** operation structure.                                 */
      QMap<QString, QStringList> m_errors;    /** maps objects with its errors, empty if successful.   */
      bool                       m_abort;     /** true if the task needs to abort or has been aborted. */
      unsigned int               m_fileCount; /** transfer files count.                                */

      std::shared_ptr<Aws::Transfer::TransferManager> m_manager; /** AWS transfer manager shared pointer. */
  };
};

#endif // AWSUTILS_H_
