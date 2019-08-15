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
#include <Model/ItemsTree.h>
#include <Dialogs/SplashScreen.h>
#include <Utils/AWSUtils.h>

// Qt
#include <QDir>
#include <QMessageBox>
#include <QString>
#include <QIcon>

// C++
#include <cstdlib>
#include <fstream>
#include <cassert>
#include <algorithm>
#include <iterator>

//-----------------------------------------------------------------------------
ItemFactory::ItemFactory()
: m_counter{0}
, m_modified{false}
{
}

//-----------------------------------------------------------------------------
ItemFactory::~ItemFactory()
{
  std::for_each(begin(m_items), end(m_items), [](Item *i) { if(i) delete i; });
}

//-----------------------------------------------------------------------------
Item* ItemFactory::createItem(const QString& name, Item* parent, const unsigned long long size, const Type type)
{
  const auto item = new Item(name, parent, size, type, m_counter++);
  if(parent) parent->addChild(item);

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
void ItemFactory::serializeItems(std::ofstream& stream, SplashScreen *splash, QApplication *app)
{
  splash->setMessage("Saving database");
  auto size = m_items.size();
  int progress = 0;
  unsigned long long count = 0;
  m_counter = 0;

  // needed to restore visibility to report correct size. Also restore ids to consecutive numbers.
  std::for_each(m_items.begin(), m_items.end(), [&](Item *i) { if(i) { i->m_id = m_counter++; i->setVisible(true); } });

  auto serializeItemsState = [&](const Item *i)
  {
    int cProgress = (count * 100) / (2*size);
    if(cProgress != progress)
    {
      progress = cProgress;
      splash->setProgress(cProgress);
      app->processEvents();
    }

    if(i)
    {
      i->serializeState(stream);
    }
    ++count;
  };
  std::for_each(m_items.cbegin(), m_items.cend(), serializeItemsState);

  stream << "---" << std::endl;

  auto serializeItemsRelations = [&](const Item *i)
  {
    int cProgress = (count * 100) / (2*size);
    if(cProgress != progress)
    {
      progress = cProgress;
      splash->setProgress(cProgress);
      app->processEvents();
    }

    if(i)
    {
      i->serializeRelations(stream);
    }
    ++count;
  };
  std::for_each(m_items.cbegin(), m_items.cend(), serializeItemsRelations);
}

//-----------------------------------------------------------------------------
void ItemFactory::deserializeItems(std::ifstream& stream, SplashScreen *splash, QApplication *app)
{
  stream.seekg(0, std::ios_base::end);
  auto streamSize = stream.tellg();
  stream.seekg(0, std::ios_base::beg);
  int progress = 0;
  unsigned long long max = 0;

  const QString title("Database");
  const QString errorMessage("Error loading the database");

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
        QMessageBox msgBox(splash);
        msgBox.setWindowTitle(title);
        msgBox.setText(errorMessage);
        msgBox.setWindowIcon(QIcon(":/Pato/rubber-duck.ico"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();

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
        QMessageBox msgBox(splash);
        msgBox.setWindowTitle(title);
        msgBox.setText(errorMessage);
        msgBox.setWindowIcon(QIcon(":/Pato/rubber-duck.ico"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();

        exit(0);
      }

      unsigned long long size = std::strtoull(line.c_str(), nullptr, 10);

      auto item = new Item(QString::fromStdString(name), nullptr, size, type, id);
      max = std::max(max, id);
      m_items.push_back(item);
    }

    if(line != "---" || stream.eof())
    {
      QMessageBox msgBox(splash);
      msgBox.setWindowTitle(title);
      msgBox.setText(errorMessage);
      msgBox.setWindowIcon(QIcon(":/Pato/rubber-duck.ico"));
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.exec();

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

      if(line.empty())
      {
        continue;
      }

      auto qLine = QString::fromStdString(line);
      auto parts = qLine.split(' ');

      if(parts.size() != 2) continue;

      unsigned long long id = std::strtoull(parts[0].toStdString().c_str(), nullptr, 10);
      auto children = parts[1].split(':');

      // ids need to be consecutive or this won't work!
      for(auto child: children)
      {
        auto clean = child.trimmed();
        const unsigned long long cid =  std::strtoll(clean.toStdString().c_str(), nullptr, 10);
        if(cid == 0ULL) continue;

        m_items[cid]->m_parent = m_items[id];
        m_items[id]->m_childs.push_back(m_items[cid]);
      }
    }
  }

  stream.close();
  m_modified = false;
  m_counter = m_items.size();

  auto sortChildren = [this](Item *i)
  {
    if(isDirectory(i))
    {
      std::sort(begin(i->m_childs), end(i->m_childs), lessThan);
    }
  };
  std::for_each(begin(m_items), end(m_items), sortChildren);

  if(m_items.empty())
  {
    // create root item.
    createItem("", nullptr, 0, Type::Directory);
  }
  else
  {
    auto it = std::find_if(m_items.cbegin() + 1, m_items.cend(), [](Item *i){ return i && !i->parent() && i->id() != 0; });
    if(it != m_items.cend())
    {
      QMessageBox msgBox(splash);
      msgBox.setWindowTitle(title);
      msgBox.setText(errorMessage);
      msgBox.setWindowIcon(QIcon(":/Pato/rubber-duck.ico"));
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.exec();

      exit(0);
    }
  }

  assert(m_counter == m_items.size());
  assert((m_items.at(0)->id() == 0) && (m_items.at(0)->name() == ""));
}

//-----------------------------------------------------------------------------
Items ItemFactory::traverseItem(Item* item)
{
  Items list;

  list.push_back(item);

  if(isDirectory(item))
  {
    auto children = item->children();
    auto appendList = [&list, this](Item *i)
    {
      if(i)
      {
        auto subList = traverseItem(i);
        list.insert(list.end(), subList.begin(), subList.end());
      }
    };
    std::for_each(children.begin(), children.end(), appendList);
  }

  return list;
}

//-----------------------------------------------------------------------------
void ItemFactory::deleteItem(Item* item)
{
  auto toDelete = traverseItem(item);

  if(item->parent()) item->parent()->removeChild(item);

  auto eraseAndDelete = [this](Item *i)
  {
    m_items.erase(std::remove(begin(m_items), end(m_items), i), end(m_items));
    delete i;
  };
  std::for_each(toDelete.begin(), toDelete.end(), eraseAndDelete);

  m_modified = true;
}

//-----------------------------------------------------------------------------
Item::Item(const QString& name, Item* parent, const unsigned long long size, const Type type, unsigned long long id)
: m_name    (name)
, m_parent  {parent}
, m_size    {size}
, m_type    {type}
, m_id      {id}
, m_visible {true}
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
  QStringList paths;

  if(m_parent) paths << m_parent->fullName();
  paths << name();

  paths.removeAll(QString(""));

  return paths.join(AWSUtils::DELIMITER);
}

//-----------------------------------------------------------------------------
unsigned long long Item::size() const
{
  if(m_type == Type::File && isVisible()) return m_size;

  unsigned long long size = 0;
  std::for_each(m_childs.cbegin(), m_childs.cend(), [&size](const Item *i) { if(i && i->isVisible()) size += i->size(); });
  return size;
}

//-----------------------------------------------------------------------------
Item* Item::parent() const
{
  return m_parent;
}

//-----------------------------------------------------------------------------
Type Item::type() const
{
  return m_type;
}

//-----------------------------------------------------------------------------
Items Item::children()
{
  return m_childs;
}

//-----------------------------------------------------------------------------
void Item::addChild(Item* child)
{
  if(child)
  {
    m_childs.push_back(child);
    std::sort(begin(m_childs), end(m_childs), lessThan);
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
  if(!lhs) return false;
  if(!rhs) return true;
  if(lhs->type() == Type::Directory && rhs->type() != Type::Directory) return true;
  if(lhs->type() != Type::Directory && rhs->type() == Type::Directory) return false;
  return (lhs->name() < rhs->name());
}

//-----------------------------------------------------------------------------
Item* find(const QString& name, Item* base)
{
  if(base)
  {
    auto children = base->children();

    auto it = std::find_if(children.cbegin(), children.cend(), [&name](Item *c) { return c && (c->fullName() == name); });
    if(it == children.cend())
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
  stream << std::to_string(size()) << std::endl;            // size
}

//-----------------------------------------------------------------------------
void Item::setVisible(const bool value)
{
  m_visible = value;
  if(value && parent()) parent()->setVisible(value);
}

//-----------------------------------------------------------------------------
void Item::serializeRelations(std::ofstream& stream) const
{
  if(m_type == Type::Directory && !m_childs.empty())
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
  if(child)
  {
    m_childs.erase(std::remove(m_childs.begin(), m_childs.end(), child), m_childs.end());
  }
}

//-----------------------------------------------------------------------------
unsigned long long Item::filesNumber() const
{
  unsigned long long count = 0;

  if(m_type == Type::File)
  {
    return (isVisible() ? 1 : 0);
  }

  std::for_each(m_childs.cbegin(), m_childs.cend(), [&count](const Item *i) { if(i && i->isVisible()) count += i->filesNumber(); });

  return count;
}

//-----------------------------------------------------------------------------
unsigned long long Item::directoriesNumber() const
{
  unsigned long long count = 0;

  if(m_type == Type::Directory && isVisible())
  {
    count = 1;
    std::for_each(m_childs.cbegin(), m_childs.cend(), [&count](const Item *i) { if(i && i->isVisible()) count += i->directoriesNumber(); });
  }

  return count;
}

//-----------------------------------------------------------------------------
bool isDirectory(const Item* item)
{
  return item->type() == Type::Directory;
}

//-----------------------------------------------------------------------------
unsigned int Item::childrenCount() const
{
  if(!isDirectory(this)) return 0;

  return std::count_if(m_childs.cbegin(), m_childs.cend(), [](const Item *i) { if(i) return i->isVisible(); return false; });
}
