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

// Project
#include <Model/ItemsTree.h>

// Qt
#include <QAbstractItemModel>
#include <QFileIconProvider>
#include <QSortFilterProxyModel>

/** \class TreeModel
 * \brief Implements a Qt model for the tree structure.
 *
 */
class TreeModel
: public QAbstractItemModel
{
    Q_OBJECT
  public:
    /** \brief TreeModel class constructor.
     * \param[in] factory ItemFactory object pointer.
     * \param[in] parent Raw pointer of the object parent of this one.
     *
     */
    explicit TreeModel(ItemFactory *factory, QObject *parent = nullptr);

    /** \brief TreeModel class virtual destructor.
     *
     */
    virtual ~TreeModel()
    {}

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    virtual bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex())
    { emit layoutChanged(); return true; }

    /** \brief Returns the item associated with the given QModelIndex.
     * \param[in] index QModelIndex struct.
     *
     */
    Item *getItem(const QModelIndex &index) const;

    /** \brief Creates a subdirectory with the given name under the given parent.
     * \param[in] parent Parent node.
     * \param[in] directoryName Directory name.
     *
     */
    void createSubdirectory(Item *parent, const QString &directoryName);

    /** \brief Removes the item from the model.
     *
     */
    void removeItem(Item *item);

    /** \brief Removes the items from the model one by one.
     *
     */
    void removeItems(Items items);

    /** \brief Set the text to filter by name.
     * \param[in] text Text string.
     */
    void setFilter(const QString &text);

  private:
    /** \brief Counts and returns the visible child and the given row.
     *
     */
    Item *findVisibleItem(Item *parent, int row) const;

    ItemFactory      *m_factory;      /** Item factory object. */
    QFileIconProvider m_iconProvider; /** icons provider.      */
    QString           m_filter;       /** text to filter by.   */
};

#endif // TREEMODEL_H_
