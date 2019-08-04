/*
 File: TreeModel.h
 Created on: 4/08/2019
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

#ifndef TREEMODEL_H_
#define TREEMODEL_H_

// Qt
#include <QFileSystemModel>

/** \class TreeModel
 * \brief Implements a Qt model for the tree structure.
 *
 */
class TreeModel
: public QFileSystemModel
{
    Q_OBJECT
  public:
    /** \brief TreeModel class constructor.
     *
     */
    explicit TreeModel(QObject *parent = nullptr);

    /** \brief TreeModel class virtual destructor.
     *
     */
    virtual ~TreeModel()
    {}
};

#endif // TREEMODEL_H_
