/*
 File: ProgressDialog.h
 Created on: 8/08/2019
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

#ifndef DIALOGS_PROGRESSDIALOG_H_
#define DIALOGS_PROGRESSDIALOG_H_

// Project
#include <Utils/AWSUtils.h>
#include "ui_ProgressDialog.h"

// Qt
#include <QDialog>

/** \class ProgressDialog
 * \brief Implements a dialog to show the progress of the operations.
 *
 */
class ProgressDialog
: public QDialog
, private Ui::ProgressDialog
{
    Q_OBJECT
  public:
    /** \brief ProgressDialog class constructor.
     * \param[in] parent Raw pointer of the QWidget parent of this one.
     * \param[in] flags Qt window flags.
     *
     */
    explicit ProgressDialog(AWSUtils::S3Thread *thread, QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    /** \brief ProgressDialog class virtual destructor.
     *
     */
    virtual ~ProgressDialog()
    {}

  protected:
    void showEvent(QShowEvent *e) override;

  private slots:
    /** \brief Updates the global progress bar.
     * \param[in] progress Progress value in [0, 100]
     *
     */
    void setGlobalProgress(int progress);

    /** \brief Updates the individual operation progress bar.
     * \param[in] progress Progress value in [0, 100]
     *
     */
    void setProgress(int progress);

    /** \brief Updates the operation message.
     * \param[in] message Text.
     *
     */
    void setMessage(const QString &message);

    /** \brief Cancels the operation.
     *
     */
    void onCancelButtonPressed();

  private:
    AWSUtils::S3Thread *m_thread; /** operation thread. */
};

#endif // DIALOGS_PROGRESSDIALOG_H_
