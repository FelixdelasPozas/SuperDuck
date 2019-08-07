/*
 File: TreeModel.cpp
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

// Project
#include <TreeModel.h>

// Qt
#include <QApplication>

//-----------------------------------------------------------------------------
TreeModel::TreeModel(Items &items, QObject* parent)
: QAbstractItemModel(parent)
, m_items(items)
{
}

//-----------------------------------------------------------------------------
QVariant TreeModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid()) return QVariant();

  auto item = getItem(index);

  auto toAppropiateUnits = [](const unsigned long long size)
  {
    double dSize = size;
    double value = dSize/(1024.*1024.);
    if(value < 1) return tr("%1 bytes").arg(size);
    value = dSize/(1024.*1024.*1024.);
    if(value < 1) return tr("%1 Mb").arg(QString::number(dSize/(1024.*1024.), 'f', 2));
    value = dSize/(1024.*1024.*1024.*1024.);
    if(value < 1) return tr("%1 Gb").arg(QString::number(dSize/(1024.*1024.*1024.), 'f', 2));
    return tr("%1 Tb").arg(QString::number(value, 'f', 2));
  };

  switch(role)
  {
    case Qt::DisplayRole:
      if(index.column() == 0) return item->name();
      if(index.column() == 1) return toAppropiateUnits(item->size());
      break;
    case Qt::DecorationRole:
      if(index.column() == 0)
      {
        if(item->type() == Type::Directory) return m_iconProvider.icon(QFileIconProvider::Folder);
        return m_iconProvider.icon(QFileIconProvider::File);
      }
      break;
    case Qt::CheckStateRole:
      if(index.column() == 0)
      {
        if(item->isSelected()) return Qt::Checked;
        return Qt::Unchecked;
      }
      break;
    default:
      break;
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    switch(section)
    {
      case 0: return tr("Name");
        break;
      case 1: return tr("Size");
        break;
      default:
        break;
    }
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
QModelIndex TreeModel::index(int row, int column, const QModelIndex& parent) const
{
  auto urow = static_cast<unsigned int>(row);
  if (!parent.isValid() && urow < m_items[0]->children().size())
  {
    return createIndex(row, column, m_items[0]->children().at(row));
  }

  auto parentItem = getItem(parent);

  if (!parentItem) return QModelIndex();

  if(urow < parentItem->children().size())
  {
    return createIndex(row, column, parentItem->children().at(row));
  }

  return QModelIndex();
}

//-----------------------------------------------------------------------------
QModelIndex TreeModel::parent(const QModelIndex& index) const
{
  if (!index.isValid()) return QModelIndex();

  auto childItem = getItem(index);
  auto parentItem = childItem ? childItem->parent() : nullptr;

  if (!parentItem) return QModelIndex();

  if(parentItem->id() == 0) return QModelIndex();

  auto children = parentItem->children();
  auto it = std::find(children.cbegin(), children.cend(), childItem);

  if(it == children.cend()) return QModelIndex();

  return createIndex(std::distance(children.cbegin(), it), 0, parentItem);
}

//-----------------------------------------------------------------------------
int TreeModel::rowCount(const QModelIndex& parent) const
{
  if(!parent.isValid()) return m_items[0]->children().size();

  auto item = getItem(parent);

  return item ? item->children().size() : 0;
}

//-----------------------------------------------------------------------------
int TreeModel::columnCount(const QModelIndex& parent) const
{
  return 2;
}

//-----------------------------------------------------------------------------
Qt::ItemFlags TreeModel::flags(const QModelIndex& index) const
{
  if (index.isValid())
  {
    return Qt::ItemIsEnabled|Qt::ItemIsUserCheckable | QAbstractItemModel::flags(index);
  }

  return Qt::NoItemFlags;
}

//-----------------------------------------------------------------------------
bool TreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if(index.isValid() && role == Qt::CheckStateRole)
  {
    auto item = getItem(index);
    item->setSelected(value.toBool());

    emit dataChanged(index, index, QVector<int>(Qt::CheckStateRole));

    auto rowsNum = rowCount(index);
    if(rowsNum > 0) emitDataChanged(index, rowsNum);

    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
Item* TreeModel::getItem(const QModelIndex& index) const
{
  Item *item = nullptr;

  if(index.isValid())
  {
    item = static_cast<Item *>(index.internalPointer());
  }

  return item;
}

//-----------------------------------------------------------------------------
void TreeModel::emitDataChanged(const QModelIndex& index, const int rowsCount)
{
  if(rowsCount > 0)
  {
    emit dataChanged(index.child(0, 0), index.child(rowsCount-1, 0), QVector<int>(Qt::CheckStateRole));

    for (int i = 0; i < rowsCount; ++i)
    {
      auto child = index.child(i, 0);
      auto count = rowCount(child);
      if(count > 0) emitDataChanged(child, count);
    }
  }
}

//-----------------------------------------------------------------------------
FilterTreeModelProxy::FilterTreeModelProxy(QObject* parent)
: QSortFilterProxyModel(parent)
{
}

//-----------------------------------------------------------------------------
bool FilterTreeModelProxy::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
  if (QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent))
      return true;

  const QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
  const int count = sourceModel()->rowCount(index);

  for (int i = 0; i < count; ++i)
  {
    if (filterAcceptsRow(i, index)) return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool FilterTreeModelProxy::setData(const QModelIndex& index, const QVariant& value, int role)
{
  auto result = QSortFilterProxyModel::setData(index, value, role);

  QApplication::processEvents();

  if(result)
  {
    emit dataChanged(index, index, QVector<int>(role));

    auto rowsNum = rowCount(index);
    if(rowsNum > 0) emitDataChanged(index, rowsNum);
  }

  return result;
}

//-----------------------------------------------------------------------------
void FilterTreeModelProxy::setFilterFixedString(const QString& pattern)
{
  beginResetModel();

  QSortFilterProxyModel::setFilterFixedString(pattern);

  endResetModel();
}

//-----------------------------------------------------------------------------
void FilterTreeModelProxy::emitDataChanged(const QModelIndex& index, const int rowsCount)
{
  if(rowsCount > 0)
  {
    emit dataChanged(index.child(0, 0), index.child(rowsCount-1, 0), QVector<int>(Qt::CheckStateRole));

    for (int i = 0; i < rowsCount; ++i)
    {
      auto child = index.child(i, 0);
      auto count = rowCount(child);
      if(count > 0) emitDataChanged(child, count);
    }
  }
}
