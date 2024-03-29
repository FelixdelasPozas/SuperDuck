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
#include <Model/TreeModel.h>

// Qt
#include <QApplication>

// C++
#include <cassert>

//-----------------------------------------------------------------------------
TreeModel::TreeModel(ItemFactory *factory, QObject* parent)
: QAbstractItemModel(parent)
, m_factory{factory}
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

  Item *parentItem = parent.isValid() ? getItem(parent) : m_factory->items().at(0);

  if (!parentItem) return QModelIndex();

  auto children = parentItem->children();

  if(urow < parentItem->childrenCount())
  {
    if(m_filter.isEmpty())
    {
      return createIndex(row, column, children.at(row));
    }
    else
    {
      auto child = findVisibleItem(parentItem, row);
      if(child)
      {
        return createIndex(row, column, child);
      }
    }
  }

  return QModelIndex();
}

//-----------------------------------------------------------------------------
QModelIndex TreeModel::parent(const QModelIndex& index) const
{
  if (!index.isValid()) return QModelIndex();

  auto childItem = getItem(index);
  auto parentItem = childItem ? childItem->parent() : nullptr;

  return indexOf(parentItem);
}

//-----------------------------------------------------------------------------
int TreeModel::rowCount(const QModelIndex& parent) const
{
  if(!parent.isValid()) return m_factory->items().at(0)->childrenCount();

  return getItem(parent)->childrenCount();
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
    return Qt::ItemIsEnabled|Qt::ItemIsSelectable | QAbstractItemModel::flags(index);
  }

  return Qt::NoItemFlags;
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
void TreeModel::createSubdirectory(Item* parent, const QString &name)
{
  auto idx = indexOf(parent);
  auto children = parent->children();
  const auto childrenSize = parent->childrenCount();
  int row = 0;
  std::for_each(children.cbegin(), children.cend(), [&row, name](const Item *i) { if(i->isVisible() && i->name() < name) ++row; });

  beginInsertRows(idx, row, row);

  m_factory->createItem(name, parent, 0, Type::Directory);

  endInsertRows();

  emit dataChanged(index(0,0, idx), index(childrenSize, 0, idx));
  emit dataChanged(idx, idx);
}

//-----------------------------------------------------------------------------
void TreeModel::removeItem(Item* item)
{
  // NOTE: doesn't need to be recursive, Qt will remove the children too.
  assert(item != m_factory->items().at(0));

  const auto itemIndex = indexOf(item);
  const auto parentIndex = indexOf(item->parent());

  beginRemoveRows(parentIndex, itemIndex.row(), itemIndex.row());

  m_factory->deleteItem(item);

  endRemoveRows();

  emit dataChanged(parentIndex, parentIndex);
}

//-----------------------------------------------------------------------------
void TreeModel::removeItems(Items items)
{
  std::for_each(items.begin(), items.end(), [this](Item *i) { removeItem(i); });
}

//-----------------------------------------------------------------------------
void TreeModel::setFilter(const QString& text)
{
  if(m_filter != text)
  {
    m_filter = text;

    auto items = m_factory->items();

    beginResetModel();

    std::for_each(items.begin(), items.end(), [](Item *i) {if(i) i->setVisible(false); });

    std::for_each(items.begin(), items.end(), [text](Item *i) { if(i) i->setVisible(text.isEmpty() || i->name().contains(text, Qt::CaseInsensitive)); });

    endResetModel();
  }
}

//-----------------------------------------------------------------------------
void TreeModel::addItem(Item* item)
{
  assert(item != m_factory->items().at(0));

  const auto itemIndex = indexOf(item);
  const auto parentIndex = indexOf(item->parent());
  beginInsertRows(parentIndex, itemIndex.row(), itemIndex.row());

  // nothing to be done.

  endInsertRows();

  emit dataChanged(parentIndex, parentIndex);
}

//-----------------------------------------------------------------------------
void TreeModel::addItems(Items items)
{
  std::for_each(items.begin(), items.end(), [this](Item *i) { addItem(i); });
}

//-----------------------------------------------------------------------------
Item* TreeModel::findVisibleItem(Item *parent, int row) const
{
  unsigned int urow = row;
  if(parent && urow < parent->childrenCount())
  {
    unsigned int count = 0;

    auto countRows = [&count, urow](const Item *i)
    {
      if(i)
      {
        if(i->isVisible())
        {
          if(count == urow) return true;
          ++count;
        }
      }

      return false;
    };
    auto children = parent->children();
    auto it = std::find_if(children.cbegin(), children.cend(), countRows);
    if(it != children.cend()) return *it;
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
QModelIndex TreeModel::indexOf(Item* item, int column) const
{
  if(item && item->id() != 0)
  {
    auto parent = item->parent();
    int row = 0;

    auto computeRow = [&row, item](const Item *i)
    {
      if(i->isVisible())
      {
        if(i == item) return true;
        ++row;
      }
      return false;
    };
    auto children = parent->children();
    auto it = std::find_if(children.cbegin(), children.cend(), computeRow);
    if(it != children.cend())
    {
      return createIndex(row, column, item);
    }
  }

  return QModelIndex();
}

