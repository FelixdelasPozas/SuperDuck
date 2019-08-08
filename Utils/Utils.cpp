/*
 File: Utils.cpp
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
#include <Utils/Utils.h>

// Qt
#include <QStandardPaths>
#include <QDir>
#include <QString>
#include <QChar>
#include <QSettings>

const QString ROOT_NODE_LINE = "0 d \"\" ";
const QString SEPARATOR = "/";

const QString AWS_KEY_ID     = "AWS key id";
const QString AWS_SECRET_KEY = "AWS secret key";
const QString AWS_BUCKET     = "AWS bucket";
const QString AWS_REGION     = "AWS region";
const QString EXPORT_PATHS   = "Export full paths";
const QString DOWNLOAD_PATHS = "Download with full paths";
const QString DATABASE_FILE  = "Database file";
const QString DISABLE_DELETE = "Disable delete actions";
const QString DOWNLOAD_PATH  = "Download path";

//-----------------------------------------------------------------------------
QString Utils::dataPath()
{
  return QStandardPaths::standardLocations(QStandardPaths::DataLocation).first();
}

//-----------------------------------------------------------------------------
QString Utils::databaseFile()
{
  return dataPath() + SEPARATOR + DATABASE_NAME;
}

//-----------------------------------------------------------------------------
bool Utils::isDatabaseFile(const QString& filename)
{
  QFileInfo info(filename);

  if(info.exists())
  {
    QFile file(filename);
    if(file.open(QIODevice::ReadOnly))
    {
      QString line = file.readLine();

      if(!line.isEmpty())
      {
        return line.startsWith(ROOT_NODE_LINE, Qt::CaseSensitive);
      }
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
QString Utils::rot13(const QString& text)
{
  QString r = text;
  int i = r.length();
  while (i--)
  {
    if ((r[i] >= QChar('A') && r[i] <= QChar('M')) || (r[i] >= QChar('a') && r[i] <= QChar('m')))
    {
      r[i] = static_cast<QChar>(QChar(r[i]).unicode() + 13);
    }
    else
    {
      if ((r[i] >= QChar('N') && r[i] <= QChar('Z')) || (r[i] >= QChar('n') && r[i] <= QChar('z')))
      {
        r[i] = static_cast<QChar>(QChar(r[i]).unicode() - 13);
      }
    }
  }
  return r;
}

//-----------------------------------------------------------------------------
bool Utils::Configuration::isValid() const
{
  return !AWS_Access_key_id.isEmpty() && (AWS_Access_key_id.length() == 20) &&
         !AWS_Secret_access_key.isEmpty() && (AWS_Secret_access_key.length() == 40) &&
         !AWS_Bucket.isEmpty() && !AWS_Region.isEmpty();
}

//-----------------------------------------------------------------------------
void Utils::Configuration::load()
{
  QSettings settings(Utils::dataPath() + SEPARATOR + "SuperPato.ini", QSettings::IniFormat);

  AWS_Access_key_id     = settings.value(AWS_KEY_ID,     QString()).toString();
  AWS_Secret_access_key = settings.value(AWS_SECRET_KEY, QString()).toString();
  AWS_Bucket            = settings.value(AWS_BUCKET,     QString()).toString();
  AWS_Region            = settings.value(AWS_REGION,     QString()).toString();
  Database_file         = settings.value(DATABASE_FILE,  Utils::databaseFile()).toString();
  Download_Full_Paths   = settings.value(DOWNLOAD_PATHS, false).toBool();
  Export_Full_Paths     = settings.value(EXPORT_PATHS,   true).toBool();
  DisableDelete         = settings.value(DISABLE_DELETE, true).toBool();
  DownloadPath          = settings.value(DOWNLOAD_PATH,  QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString();
}

//-----------------------------------------------------------------------------
void Utils::Configuration::save()
{
  QSettings settings(Utils::dataPath() + SEPARATOR + "SuperPato.ini", QSettings::IniFormat);

  settings.setValue(AWS_KEY_ID,     AWS_Access_key_id);
  settings.setValue(AWS_SECRET_KEY, AWS_Secret_access_key);
  settings.setValue(AWS_BUCKET,     AWS_Bucket);
  settings.setValue(AWS_REGION,     AWS_Region);
  settings.setValue(DATABASE_FILE,  Database_file);
  settings.setValue(DOWNLOAD_PATHS, Download_Full_Paths);
  settings.setValue(EXPORT_PATHS,   Export_Full_Paths);
  settings.setValue(DISABLE_DELETE, DisableDelete);
  settings.setValue(DOWNLOAD_PATH,  DownloadPath);
}

//-----------------------------------------------------------------------------
std::map<std::string, unsigned long long> Utils::processItems(const Items items)
{
  std::map<std::string, unsigned long long> result;

  auto processSelection = [&result](const Item *i)
  {
    QString fullName = i->fullName() + (isDirectory(i) ? DELIMITER:"");

    result.emplace(fullName.toStdString(), i->size());
  };
  std::for_each(items.cbegin(), items.cend(), processSelection);

  return result;
}
