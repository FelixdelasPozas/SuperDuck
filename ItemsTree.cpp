/*
 File: ItemsTree.cpp
 Created on: 4 ago. 2019
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
#include <ItemsTree.h>
#include <SplashScreen.h>

// Qt
#include <QDir>
#include <QMessageBox>

// C++
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cassert>
#include <algorithm>

//-----------------------------------------------------------------------------
ItemFactory::ItemFactory()
: m_counter{0}
, m_modified{false}
{
}

//-----------------------------------------------------------------------------
Item* ItemFactory::createItem(const QString& name, Item* parent, const unsigned long long size, const Type type)
{
  const auto item = new Item(name, parent, size, type, m_counter++);

  connect(item, SIGNAL(destroyed(QObject *)), this, SLOT(onItemDestroyed(QObject *)));

  m_items.push_back(item);
  m_modified = true;

  return item;
}

//-----------------------------------------------------------------------------
unsigned long long int ItemFactory::count() const
{
  return m_items.size();
}

//-----------------------------------------------------------------------------
ItemFactory::~ItemFactory()
{
  std::for_each(m_items.begin(), m_items.end(), [](Item *i) { if(i) delete i; });
}

//-----------------------------------------------------------------------------
void ItemFactory::serializeItems(std::ofstream& stream) const
{
  std::for_each(m_items.cbegin(), m_items.cend(), [&stream](const Item *i){ if(i) i->serializeState(stream); });
  stream << "---" << std::endl;
  std::for_each(m_items.cbegin(), m_items.cend(), [&stream](const Item *i){ if(i) i->serializeRelations(stream); });
}

//-----------------------------------------------------------------------------
void ItemFactory::onItemDestroyed(QObject* obj)
{
  auto item = qobject_cast<Item *>(obj);
  if(item)
  {
    if(item)
    {
      item->parent()->removeChild(item);
    }

    m_items.erase(std::remove(m_items.begin(), m_items.end(), item), m_items.end());
    m_modified = true;
  }
}

//-----------------------------------------------------------------------------
Item::Item(const QString& name, Item* parent, const unsigned long long size, const Type type, unsigned long long id)
: m_name    (name)
, m_parent  {parent}
, m_selected{false}
, m_size    {size}
, m_type    {type}
, m_id      {id}
{
}

//-----------------------------------------------------------------------------
QString Item::name() const
{
  return m_name;
}

//-----------------------------------------------------------------------------
QString Item::fullName() const
{
  QString completeName;
  if(m_name.isEmpty()) return completeName;

  if(m_parent) completeName = m_parent->fullName();

  completeName += "/" + m_name;

  return completeName;
}

//-----------------------------------------------------------------------------
unsigned long long Item::size() const
{
  return m_size;
}

//-----------------------------------------------------------------------------
Item* Item::parent() const
{
  return m_parent;
}

//-----------------------------------------------------------------------------
bool Item::isSelected() const
{
  return m_selected;
}

//-----------------------------------------------------------------------------
Type Item::type() const
{
  return m_type;
}

//-----------------------------------------------------------------------------
Items Item::children() const
{
  return m_childs;
}

//-----------------------------------------------------------------------------
void Item::addChild(Item* child)
{
  if(child)
  {
    m_childs << child;
    std::sort(m_childs.begin(), m_childs.end(), lessThan);
  }
}

//-----------------------------------------------------------------------------
void Item::setSelected(bool value)
{
  if(m_selected != value)
  {
    m_selected = value;
    std::for_each(m_childs.begin(), m_childs.end(), [value](Item *i) { if(i) i->setSelected(value); });
  }
}

//-----------------------------------------------------------------------------
long long int Item::id() const
{
  return m_id;
}

//-----------------------------------------------------------------------------
bool lessThan(const Item* lhs, const Item* rhs)
{
  if(!lhs) return true;
  if(!rhs) return false;
  if(lhs->type() == Type::Directory && rhs->type() != Type::Directory) return false;
  if(lhs->type() != Type::Directory && rhs->type() == Type::Directory) return true;
  return (lhs->name() < rhs->name());
}

//-----------------------------------------------------------------------------
Item* find(const QString& name, Item* base)
{
  if(base)
  {
    auto children = base->children();

    auto it = std::find_if(children.begin(), children.end(), [&name](Item *c) { if(c) return c->fullName() == name; return false; });
    if(it == children.end())
    {
      return find(name, base->parent());
    }
    else
    {
      return *it;
    }
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
void Item::serializeState(std::ofstream& stream) const
{
  stream << std::to_string(m_id);                           // id
  stream << " " << (m_type == Type::Directory ? "d" : "f"); // type
  stream << " \"" << m_name.toStdString() << "\" ";         // name
  stream << std::to_string(m_size) << std::endl;            // size
}

//-----------------------------------------------------------------------------
void Item::serializeRelations(std::ofstream& stream) const
{
  if(m_type == Type::Directory && !m_childs.isEmpty())
  {
    QStringList childIds; // children_ids
    std::for_each(m_childs.cbegin(), m_childs.cend(), [&childIds](const Item *c) {if(c) childIds << QString::fromStdString(std::to_string(c->id())); });
    auto compound = childIds.join(":").toStdString();
    if(compound != "0" && !compound.empty())
    {
      stream << std::to_string(m_id) << " " << compound << std::endl;
    }
    // children's parent is implicit.
  }
}

//-----------------------------------------------------------------------------
void Item::removeChild(Item* child)
{
  if(child && m_childs.contains(child))
  {
    m_childs.removeAll(child);
  }
}

//-----------------------------------------------------------------------------
void ItemFactory::deserializeItems(std::ifstream& stream, SplashScreen *splash, QApplication *app)
{
  stream.seekg(0, std::ios_base::end);
  auto streamSize = stream.tellg();
  stream.seekg(0, std::ios_base::beg);
  int progress = 0;

  if(stream.is_open())
  {
    std::string line;
    bool finishedStates = false;
    while(!finishedStates && !stream.eof())
    {
      int cProgress = (stream.tellg() * 100) / streamSize;
      if(cProgress != progress)
      {
        progress = cProgress;
        splash->setProgress(cProgress);
        app->processEvents();
      }

      std::getline(stream, line);

      if(line == "---")
      {
        finishedStates = true;
        continue;
      }

      std::string token;
      std::string name;
      unsigned long long id = 0;
      Type type;

      auto pos = line.find(' ');
      if(pos != std::string::npos)
      {
        token = line.substr(0, pos);
        id = std::strtoull(token.c_str(), nullptr, 10);
        line.erase(0, pos +1);
      }
      else
      {
        QMessageBox::critical(nullptr, "Database", "Error loading the database");
        exit(0);
      }

      type = (line.front() == 'd' ? Type::Directory : Type::File);

      line.erase(0, 3);
      pos = line.find('\"');
      if(pos != std::string::npos)
      {
        name = line.substr(0, pos);
        line.erase(0, pos+1);
      }
      else
      {
        QMessageBox::critical(nullptr, "Database", "Error loading the database");
        exit(0);
      }

      unsigned long long size = std::strtoull(line.c_str(), nullptr, 10);

      auto item = new Item(QString::fromStdString(name), nullptr, size, type, id);
      m_items.push_back(item);
    }

    m_counter = m_items.size();

    if(line != "---" || stream.eof())
    {
      QMessageBox::critical(nullptr, "Database", "Error loading the database");
      exit(0);
    }

    while(!stream.eof())
    {
      int cProgress = (stream.tellg() * 100) / streamSize;
      if(cProgress != progress)
      {
        progress = cProgress;
        splash->setProgress(cProgress);
        app->processEvents();
      }

      std::getline(stream, line);

      auto qLine = QString::fromStdString(line);
      auto parts = qLine.split(' ');

      if(parts.size() != 2) continue;

      unsigned long long id = std::strtoull(parts[0].toStdString().c_str(), nullptr, 10);
      auto children = parts[1].split(':');

      for(auto child: children)
      {
        auto clean = child.trimmed();
        unsigned long long cid =  std::strtoull(clean.toStdString().c_str(), nullptr, 10);
        if(cid == 0ULL) continue;

        assert(cid < m_counter && id < m_counter && m_items[cid] != nullptr && m_items[id] != nullptr);

        m_items[cid]->m_parent = m_items[id];
        m_items[id]->m_childs << m_items[cid];
      }
    }
  }

  stream.close();
  m_modified = false;

  auto it = std::find_if(m_items.cbegin() + 1, m_items.cend(), [](Item *i){ return i && !i->parent() && i->id() != 0; });
  if(it != m_items.cend())
  {
    QMessageBox::critical(nullptr, "Database", "Error loading the database");
    exit(0);
  }
}
