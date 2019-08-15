/*
 File: ItemsTree.h
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

#ifndef ITEMSTREE_H_
#define ITEMSTREE_H_

// C++
#include <atomic>
#include <vector>

// Qt
#include <QString>
#include <QList>
#include <QObject>

enum class Type: char { Directory = 0, File = 1 };

class Item;
using Items = std::vector<Item *>;

class SplashScreen;
class QApplication;

/** \class ItemFactory
 * \brief Factory for items.
 *
 */
class ItemFactory
{
  public:
    /** \brief ItemFactory class constructor.
     *
     */
    explicit ItemFactory();

    /** \brief ItemFactory class destructor.
     *
     */
    virtual ~ItemFactory();

    /** \brief Returns an item with the given parameters.
     * \param[in] name Item name.
     * \param[in] parent Item parent pointer.
     * \param[in] size Item size.
     * \param[in] type Item type.
     *
     */
    Item *createItem(const QString &name, Item *parent, const unsigned long long size, const Type type);

    /** \brief Writes the created objects to the given stream.
     * \param[inout] stream Output stream.
     *
     */
    void serializeItems(std::ofstream &stream, SplashScreen *splash, QApplication *app);

    /** \brief Creates items from the information in the given stream.
     * \param[inout] stream Input stream.
     * \param[in] splash SplashScreen pointer to sign progress.
     * \param[in] app QApplication needed to process events.
     *
     */
    void deserializeItems(std::ifstream &stream, SplashScreen *splash, QApplication *app);

    /** \brief Returns the number of created items.
     *
     */
    unsigned long long int count() const;

    /** \brief Returns true if the list of items has changed from a certain point in time.
     *
     */
    bool hasBeenModified() const
    { return m_modified; }

    /** \brief Returns a reference to the items vector.
     *
     */
    Items &items()
    { return m_items; }

    /** \brief Deletes the given item.
     * \param[in] item Item pointer.
     *
     */
    void deleteItem(Item *item);

  private:
    /** \brief Returns the list of items contained in the given one.
     * \param[in] item Item object pointer.
     *
     */
    Items traverseItem(Item *item);

    std::atomic<unsigned long long int> m_counter;  /** object counter.                                                  */
    std::vector<Item *>                 m_items;    /** list of items.                                                   */
    bool                                m_modified; /** true if items have been deleted or created from a certain point. */
};

class Item
: public QObject
{
    Q_OBJECT
  public:
    /** \brief Returns the item name
     *
     */
    QString name() const;

    /** \brief Returns the item full name (path + name).
     *
     */
    QString fullName() const;

    /** \brief Returns the item size.
     *
     */
    unsigned long long size() const;

    /** \brief Returns the item parent or null if root item.
     *
     */
    Item * parent() const;

    /** \brief Returns the item type.
     *
     */
    Type type() const;

    /** \brief Returns the items inside this item. Or empty if it's a file.
     *
     */
    Items children();

    /** \brief Adds an item to the children list.
     * \param[in] child Item to add.
     *
     */
    void addChild(Item *child);

    /** \brief Removes the given item from the children list.
     * \param[in] child Item to remove.
     *
     */
    void removeChild(Item *child);

    /** \brief Returns the item id.
     *
     */
    long long int id() const;

    /** \brief Returns the number of files in the item and subitems. 1 if a file.
     *
     */
    unsigned long long filesNumber() const;

    /** \brief Returns the number of directories in the item and subitems. 0 if a file.
     *
     */
    unsigned long long directoriesNumber() const;

    /** \brief Returns true if the item is visible and false otherwise.
     *
     */
    bool isVisible() const
    { return m_visible; }

    /** \brief Sets the item visibility.
     * \param[in] value True to set the item visible, false otherwise.
     *
     */
    void setVisible(const bool value);

    /** \brief Returns the count of visible children.
     *
     */
    unsigned int childrenCount() const;

  private:
    /** \brief Item class constructor.
     * \param[in] name Item name.
     * \param[in] parent Pointer to parent item.
     * \param[in] size Item size.
     * \param[in] type Item type.
     * \param[in] id Item id.
     *
     */
    explicit Item(const QString &name, Item *parent, const unsigned long long size, const Type type, unsigned long long id);

    /** \brief Item class destructor.
     *
     */
    virtual ~Item()
    {};

    /** \brief Serializes the object state to the given stream.
     * \param[in] stream Output file stream.
     *
     */
    void serializeState(std::ofstream &stream) const;

    /** \brief Serializes the object relations to the given stream.
     * \param[in] stream Output file stream.
     *
     */
    void serializeRelations(std::ofstream &stream) const;

    friend class ItemFactory;

    QString             m_name;     /** item name.                        */
    Item               *m_parent;   /** pointer to item parent.           */
    unsigned long long  m_size;     /** item size.                        */
    Type                m_type;     /** item type.                        */
    std::vector<Item *> m_childs;   /** list of children items.           */
    unsigned long long  m_id;       /** item id.                          */
    bool                m_visible;  /** true if visible, false otherwise. */
};

/** \brief Less than method for sorting. Returns true if lhs < rhs.
 * \param[in] lhs Item pointer.
 * \param[in] rhs Item pointer.
 *
 */
bool lessThan(const Item *lhs, const Item *rhs);

/** \brief Finds the directory 'name' item in the given base.
 *
 */
Item * find(const QString &name, Item *base);

/** \brief Returns true if the item is a directory.
 * \param[in] item Item pointer.
 *
 */
bool isDirectory(const Item *item);

#endif // ITEMSTREE_H_
