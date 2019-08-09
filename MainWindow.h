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
#include <Model/ItemsTree.h>
#include <Model/TreeModel.h>
#include <Utils/Utils.h>
#include <Utils/AWSUtils.h>
#include "ui_MainWindow.h"

// Qt
#include <QMainWindow>

// C++
#include <map>

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
    virtual void resizeEvent(QResizeEvent *e) override;
    virtual void closeEvent(QCloseEvent *e) override;

  private slots:
    /** \brief Looks for selected items and writes an excel or csv file to disk.
     *
     */
    void onExportActionTriggered();

    /** \brief Looks for selected items to download from S3 bucket.
     *
     */
    void onDownloadActionTriggered();

    /** \brief Shows the settings dialog.
     *
     */
    void onSettingsButtonTriggered();

    /** \brief Shows a file selection dialog and uploads the files to the S3 bucket.
     *
     */
    void onUploadActionTriggered();

    /** \brief Deletes selected items from the S3 bucket.
     *
     */
    void onDeleteActionTriggered();

    /** \brief Creates a new directory on the S3 bucket.
     *
     */
    void onCreateActionTriggered();

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

    /** \brief Gets the results of a finished operation.
     *
     */
    void onOperationFinished();

    /** \brief Prepares and shows a menu at the given position.
     * \param[in] pos Position in the tree where a menu was requested.
     *
     */
    void onContextMenuRequested(const QPoint &pos);

  private:
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

    /** \brief Updates the statistics in the status bar.
     *
     */
    void updateStatusLabel();

    /** \brief Configures the tree view and the context menu.
     *
     */
    void configureTreeView();

    /** \brief Returns the list of items selected in the tree view.
     *
     */
    Items getSelectedItems() const;

    /** \brief Returns a list of pairs name-size of selected items and it's contents.
     * \param[in] useFullNames True to generate the list using the full names, and false otherwise.
     *
     */
    std::vector<std::pair<std::string, unsigned long long> > getSelectedFileList(bool useFullNames = true) const;

    ItemFactory               *m_factory;       /** item factory pointer.                           */
    TreeModel                 *m_model;         /** tree model for the items.                       */
    Utils::Configuration      &m_configuration; /** application configuration.                      */
    QLabel                    *m_statusLabel;   /** status bar label.                               */
    QList<AWSUtils::S3Thread*> m_threads;        /** list of threads executing or pending execution. */
};

#endif // MAINWINDOW_H_
