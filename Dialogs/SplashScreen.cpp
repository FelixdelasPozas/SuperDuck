/*
 File: SplashScreen.cpp
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

// Project
#include <Dialogs/SplashScreen.h>

// Qt
#include <QStyle>
#include <QStyleOptionProgressBar>

//-----------------------------------------------------------------------------
SplashScreen::SplashScreen(QApplication* app, QWidget* parent)
: QSplashScreen(QPixmap(":/Pato/splash_frame_00.png"), Qt::WindowStaysOnTopHint)
, m_progress{0}
, m_app{app}
, m_frame{1}
{
  installEventFilter(this);
}

//-----------------------------------------------------------------------------
void SplashScreen::setProgress(int value)
{
  if(m_progress != value)
  {
    m_progress = std::max(0, std::min(100, value));

    auto numberString = QString::number(m_frame % 16);
    numberString = (m_frame % 16 < 10) ? "0" + numberString : numberString;
    auto image = tr(":/Pato/splash_frame_%1.png").arg(numberString);

    setPixmap(QPixmap(image));
    m_frame = (m_frame + 1) % 16;
  }
}

//-----------------------------------------------------------------------------
bool SplashScreen::eventFilter(QObject* target, QEvent* event)
{
  if((event->type() == QEvent::MouseButtonPress) ||
     (event->type() == QEvent::MouseButtonDblClick) ||
     (event->type() == QEvent::MouseButtonRelease) ||
     (event->type() == QEvent::KeyPress) ||
     (event->type() == QEvent::KeyRelease))
  {
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
void SplashScreen::drawContents(QPainter* painter)
{
  QSplashScreen::drawContents(painter);

  // Set style for progressbar...
  QStyleOptionProgressBar pbstyle;
  pbstyle.initFrom(this);
  pbstyle.state = QStyle::State_Enabled;
  pbstyle.textVisible = true;
  pbstyle.textAlignment = Qt::AlignCenter;
  pbstyle.text = tr("%1 ... %2%").arg(m_message).arg(m_progress);
  pbstyle.minimum = 0;
  pbstyle.maximum = 100;
  pbstyle.progress = m_progress;
  pbstyle.invertedAppearance = false;
  pbstyle.rect = QRect(0, rect().height()-20, rect().width(), 20);

  // Draw it...
  style()->drawControl(QStyle::CE_ProgressBar, &pbstyle, painter, this);
}

//-----------------------------------------------------------------------------
void SplashScreen::setMessage(const QString& message)
{
  m_message = message;
}
