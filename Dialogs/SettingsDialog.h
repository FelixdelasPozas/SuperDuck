/*
 File: SettingsDialog.h
 Created on: 6/08/2019
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

#ifndef SETTINGSDIALOG_H_
#define SETTINGSDIALOG_H_

// C+
#include <winsock2.h>

// Project
#include <Utils/Utils.h>
#include "ui_SettingsDialog.h"

// Qt
#include <QWidget>
#include <QDialog>

// AWS
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/s3/model/AccessControlPolicy.h>
#include <aws/s3/model/Permission.h>

/** \class SettingsDialog
 * \brief Implements a dialog to manipulate application settings.
 *
 */
class SettingsDialog
: public QDialog
, private Ui::SettingsDialog
{
    Q_OBJECT
  public:
    /** \brief Settings dialog class constructor.
     * \param[in] parent Raw pointer of the widget parent of this one.
     * \param[in] flags Window flags.
     *
     */
    explicit SettingsDialog(Utils::Configuration &config, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    /** \brief Settings dialog class virtual destructor.
     *
     */
    virtual ~SettingsDialog()
    {}

    /** \brief Returns the dialog configuration.
     *
     */
    Utils::Configuration configuration() const;

  protected:
    virtual void accept() override;

  private slots:
    /** \brief Opens a file dialog to locate database file.
     *
     */
    void onFolderButtonClicked();

    /** \brief Creates the AWS credentials file if the configuration is valid.
     *
     */
    void createCredentialsFile();

    /** \brief Opens a folder selection dialog to locate default download path.
     *
     */
    void onDownloadPathButtonClicked();

    /** \brief Checks the user permissions on the bucket and shows in the UI.
     *
     */
    void onPermissionsButtonClicked();

  private:
    /** \brief Helper method to connect Ui signals to slots.
     *
     */
    void connectSignals();

    /** \brief Modifies the UI based on the existence of AWS credentials file.
     *
     */
    void checkCredentialsFile();
};

#endif // SETTINGSDIALOG_H_
