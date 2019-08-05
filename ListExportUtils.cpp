/*
 File: ListExportUtils.cpp
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
#include <ListExportUtils.h>

// C++
#include <fstream>
#include <algorithm>

// xlslib
#include <common/xlconfig.h>
#include <xlslib.h>

// Qt
#include <QString>

using namespace xlslib_core;

//-----------------------------------------------------------------------------
bool ListExportUtils::saveToCSV(const std::string& filename, const std::vector<std::pair<std::string, unsigned long long> >& contents)
{
  std::ofstream file(filename, std::ios_base::out|std::ios_base::trunc);

  if(file.is_open())
  {
    file << "Name, Size" << std::endl;

    auto writeLine = [&file](const std::pair<std::string, unsigned long long> &p)
    {
      file << "\"" << p.first << "\", " << p.second << std::endl;
    };
    std::for_each(contents.cbegin(), contents.cend(), writeLine);

    file.close();
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool ListExportUtils::saveToXLS(const std::string& filename, const std::vector<std::pair<std::string, unsigned long long> >& contents)
{
  workbook wb;

  auto excelSheet = wb.sheet("sheet 1");

  excelSheet->label(0, 0, "Name");
  excelSheet->label(0, 1, "Size");

  for (size_t c = 0; c < contents.size(); ++c)
  {
    excelSheet->label(static_cast<int>(c+1), 0, QString::fromLatin1(contents.at(c).first.c_str()).toStdWString());
    excelSheet->label(static_cast<int>(c+1), 1, std::to_string(contents.at(c).second));
  }

  auto result = wb.Dump(filename);

  return (result == NO_ERRORS);
}
