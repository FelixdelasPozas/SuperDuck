/*
 File: ListExportUtils.h
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

#ifndef LISTEXPORTUTILS_H_
#define LISTEXPORTUTILS_H_

// C++
#include <string>
#include <vector>

namespace ListExportUtils
{
  /** \brief Saves the given contents to a CSV with the given filename. Returns true
   * of success and false on failure.
   * \param[in] filename Output file name.
   * \param[in] contents File names and sizes.
   *
   */
  bool saveToCSV(const std::string &filename, const std::vector<std::pair<std::string, unsigned long long>> &contents);

  /** \brief Saves the given contents to a XLS with the given filename. Returns true
   * on success and false on failure.
   * \param[in] filename Output file name.
   * \param[in] contents File names and sizes.
   *
   */
  bool saveToXLS(const std::string &filename, const std::vector<std::pair<std::string, unsigned long long>> &contents);
};

#endif // LISTEXPORTUTILS_H_
