/*
 File: MainWindow.cpp
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

// Project
#include <MainWindow.h>

// C++
#include <fstream>

// Qt
#include <QSettings>

const QString STATE    = "State";
const QString GEOMETRY = "Geometry";

//-----------------------------------------------------------------------------
MainWindow::MainWindow(ItemFactory* factory, QWidget* parent, Qt::WindowFlags flags)
: QMainWindow(parent, flags)
, m_factory{factory}
{
  setupUi(this);

  connectSignals();

  restoreSettings();
}

//-----------------------------------------------------------------------------
MainWindow::~MainWindow()
{
  saveSettings();

  if(m_factory->hasBeenModified())
  {
    std::ofstream outFile("dbData2.txt");
    m_factory->serializeItems(outFile);
    outFile.close();
  }
}

//-----------------------------------------------------------------------------
void MainWindow::restoreSettings()
{
  QSettings settings("ElPato.ini", QSettings::IniFormat);

  if(settings.contains(STATE))
  {
    auto state = settings.value(STATE).toByteArray();
    restoreState(state);
  }

  if(settings.contains(GEOMETRY))
  {
    auto geometry = settings.value(GEOMETRY).toByteArray();
    restoreGeometry(geometry);
  }
}

//-----------------------------------------------------------------------------
void MainWindow::saveSettings()
{
  QSettings settings("ElPato.ini", QSettings::IniFormat);

  settings.setValue(STATE, saveState());
  settings.setValue(GEOMETRY, saveGeometry());
}

//-----------------------------------------------------------------------------
void MainWindow::connectSignals()
{
}
