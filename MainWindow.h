/*
 File: MainWindow.h
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

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

// Project
#include <ItemsTree.h>
#include <TreeModel.h>
#include <Utils.h>
#include "ui_MainWindow.h"

// Qt
#include <QMainWindow>

/** \class MainWindow
 * \brief Implements the main window of the application.
 *
 */
class MainWindow
: public QMainWindow
, private Ui::ElPato
{
    Q_OBJECT
  public:
    /** \brief MainWindow class constructor.
     * \param[in] configuration Application configuration struct.
     * \param[in] factory Item factory.
     * \param[in] parent Raw pointer of the widget parent of this one.
     * \param[in] flags Qt window flags.
     *
     */
    explicit MainWindow(Utils::Configuration &configuration, ItemFactory *factory, QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    /** \brief MainWindow class virtual destructor.
     *
     */
    virtual ~MainWindow();

  protected:
    virtual void showEvent(QShowEvent *e) override;

  private slots:
    /** \brief Looks for selected items and writes an excel file to disk.
     *
     */
    void onExcelButtonTriggered();

    /** \brief Looks for selected items to download from S3 bucket.
     *
     */
    void onDownloadButtonTriggered();

    /** \brief Shows the settings dialog.
     *
     */
    void onSettingsButtonTriggered();

    /** \brief Shows a file selection dialog and uploads the files to the S3 bucket.
     *
     */
    void onUploadButtonTriggered();

    /** \brief Deletes selected items from the S3 bucket.
     *
     */
    void onDeleteButtonTriggered();

    /** \brief Creates a new directory on the S3 bucket.
     *
     */
    void onCreateButtonTriggered();

    /** \brief Updates the UI when the search text changes.
     * \param[in] text Filter text.
     *
     */
    void onSearchTextChanged(const QString &text);

    /** \brief Updates the filter of the model with the text from the search field.
     *
     */
    void onSearchButtonClicked();

    /** \brief Refreshes the tree view when an item changes state.
     *
     */
    void refreshView();

    /** \brief Forces the user to add a valid configuration.
     *
     */
    void onInvalidConfiguration();

  private:
    /** \brief Returns the list of selected files as pairs of full_name-size.
     *
     */
    std::vector<std::pair<std::string, unsigned long long>> selectedFiles() const;

    /** \brief Helper method to restore application position and size.
     *
     */
    void restoreConfiguration();

    /** \brief Helper method to save application position and size.
     *
     */
    void saveConfiguration();

    /** \brief Helper method to connect UI signals to their slots.
     *
     */
    void connectSignals();

    ItemFactory          *m_factory;       /** item factory pointer.      */
    FilterTreeModelProxy *m_filter;        /** tree model filter.         */
    Utils::Configuration &m_configuration; /** application configuration. */
};

#endif // MAINWINDOW_H_
