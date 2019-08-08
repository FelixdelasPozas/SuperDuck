/*
 File: Utils.h
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

#ifndef UTILS_H_
#define UTILS_H_

// Project
#include <Model/ItemsTree.h>

// Qt
#include <QString>

// C++
#include <map>

namespace Utils
{
  /** \brief Returns the path where the application data is.
   *
   */
  QString dataPath();

  /** \brief Returns the database file with full path.
   *
   */
  QString databaseFile();

  /** \brief Returns true if the given file is a valid database
   * \param[in] filename Filename with full path.
   *
   */
  bool isDatabaseFile(const QString &filename);

  static const QString DATABASE_NAME = "dbData.txt";
  static const QString DELIMITER =  "/";

  /** \brief Simple text obfuscation.
   * \param[in] text Text string.
   */
  QString rot13(const QString &text);

  /** \brief Builds the map of files to transfer.
   * \param[in] items List of items.
   *
   */
  std::map<std::string, unsigned long long> processItems(const Items items);

  /** \struct Configuration
   * \brief Application configuration.
   *
   */
  struct Configuration
  {
    QString AWS_Access_key_id;     /** AWS key id.                                                   */
    QString AWS_Secret_access_key; /** AWS secret key.                                               */
    QString AWS_Bucket;            /** AWS bucket.                                                   */
    QString AWS_Region;            /** AWS region of the bucket.                                     */
    QString Database_file;         /** database file location on disk.                               */
    bool    Export_Full_Paths;     /** true to export files with full path, false otherwise.         */
    bool    Download_Full_Paths;   /** true to create paths when downloading files, false otherwise. */
    bool    DisableDelete;         /** true to disable delete objects actions, false otherwise.      */
    QString DownloadPath;          /** Path in which to save the files and folders.                  */

    /** \brief Returns true if its a valid configuration.
     *
     */
    bool isValid() const;

    /** \brief Loads the configuration from the ini settings file.
     *
     */
    void load();

    /** \brief Saves the configuration to the ini settings file.
     *
     */
    void save();
  };
};

#endif // UTILS_H_
