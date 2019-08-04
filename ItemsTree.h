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
using Items = QList<Item *>;

class SplashScreen;
class QApplication;

/** \class ItemFactory
 * \brief Factory for items.
 *
 */
class ItemFactory
: public QObject
{
    Q_OBJECT
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
    void serializeItems(std::ofstream &stream) const;

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

  private slots:
    /** \brief Marks the model as modified when an item is destroyed.
     *
     */
    void onItemDestroyed(QObject *obj);

  private:
    std::atomic<unsigned long long int> m_counter;  /** object counter.                                                  */
    std::vector<Item *>                 m_items;    /** list of items.                                                   */
    bool                                m_modified; /** true if items have been deleted or created from a certain point. */
};

class Item
: public QObject
{
    Q_OBJECT
  public:
    /** \brief Item class destructor.
     *
     */
    virtual ~Item()
    {};

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

    /** \brief Returns true if the item is selected and false otherwise.
     *
     */
    bool isSelected() const;

    /** \brief Returns the item type.
     *
     */
    Type type() const;

    /** \brief Returns the items inside this item. Or empty if it's a file.
     *
     */
    Items children() const;

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

    /** \brief Selects/Unselects the item.
     * \param[in] value True to select and false otherwise.
     *
     */
    void setSelected(bool value);

    /** \brief Returns the item id.
     *
     */
    long long int id() const;

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

    QString            m_name;     /** item name.                         */
    Item              *m_parent;   /** pointer to item parent.            */
    bool               m_selected; /** true if selected, false otherwise. */
    unsigned long long m_size;     /** item size.                         */
    Type               m_type;     /** item type.                         */
    QList<Item *>      m_childs;   /** list of children items.            */
    unsigned long long m_id;       /** item id.                           */
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

#endif // ITEMSTREE_H_
