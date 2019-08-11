/*
 File: SplashScreen.h
 Created on: 3/08/2019
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

#ifndef SPLASHSCREEN_H_
#define SPLASHSCREEN_H_

// Qt
#include <QSplashScreen>
#include <QApplication>

/** \class SplashScreen
 * \brief Implements application splash screen.
 *
 */
class SplashScreen
: public QSplashScreen
{
    Q_OBJECT
  public:
    /** \brief SplashScreen class constructor.
     * \param[in] qApp QApplication pointer.
     * \param[in] parent Raw pointer of the widget parent of this one.
     *
     */
    explicit SplashScreen(QApplication *app = nullptr, QWidget *parent = nullptr);

    /** \brief SplashScreen class virtual destructor.
     *
     */
    virtual ~SplashScreen()
    {}

    /** \brief Sets the message to show in the progress bar.
     * \param[in] message Text message.
     *
     */
    virtual void setMessage(const QString &message);

    bool eventFilter(QObject *target, QEvent *event);

  public slots:
    /** \brief Updates the progress bar with the given progress value.
     * \param[in] value Progress value in [0, 100].
     *
     */
    void setProgress(int value);

  protected:
    void drawContents(QPainter *painter);

  private:
    int           m_progress; /** progress bar value.  */
    QApplication *m_app;      /** application pointer. */
    QString       m_message;  /** current message.     */
    int           m_frame;    /** current frame.       */

};

#endif // SPLASHSCREEN_H_
