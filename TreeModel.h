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
#include <ItemsTree.h>

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
     *
     */
    explicit TreeModel(ItemsVector &items, QObject *parent = nullptr);

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
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    /** \brief Returns the item associated with the given QModelIndex.
     * \param[in] index QModelIndex struct.
     *
     */
    Item *getItem(const QModelIndex &index) const;

  private:
    /** \brief Emits the data changed signal for all children of the given index.
     * \param[in] index QModelIndex struct.
     *
     */
    void emitDataChanged(const QModelIndex &index);

    ItemsVector      &m_items;        /** reference to items list.  */
    QFileIconProvider m_iconProvider; /** icons provider.           */
};

/** \class FilterTreeModelProxy
 * \brief Implements a proxy filter for the tree model.
 *
 */
class FilterTreeModelProxy
: public QSortFilterProxyModel
{
    Q_OBJECT
  public:
    /** \brief FilterTreeModelProxy class constructor.
     * \param[in] parent Raw pointer of the object parent of this one.
     *
     */
    explicit FilterTreeModelProxy(QObject *parent = nullptr);

    /** \brief FilterTreeModelProxy class virtual destructor.
     *
     */
    virtual ~FilterTreeModelProxy()
    {};
  protected:
      virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
};

#endif // TREEMODEL_H_
