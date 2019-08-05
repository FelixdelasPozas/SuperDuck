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
#include <ListExportUtils.h>

// C++
#include <fstream>
#include <cassert>

// Qt
#include <QSettings>
#include <QMessageBox>
#include <QDateTime>
#include <QFileDialog>
#include <QStandardPaths>

const QString STATE    = "State";
const QString GEOMETRY = "Geometry";

//-----------------------------------------------------------------------------
MainWindow::MainWindow(ItemFactory* factory, QWidget* parent, Qt::WindowFlags flags)
: QMainWindow(parent, flags)
, m_factory{factory}
{
  setupUi(this);

  restoreSettings();

  auto model = new TreeModel(factory->items());

  auto filter = new FilterTreeModelProxy();
  filter->setSourceModel(model);
  filter->setFilterCaseSensitivity(Qt::CaseInsensitive);
  filter->setFilterKeyColumn(0);

  m_treeView->setModel(filter);
  m_treeView->setAlternatingRowColors(true);
  m_treeView->setAnimated(true);
  m_treeView->setExpandsOnDoubleClick(true);
  m_treeView->header()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);

  connectSignals();
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
  connect(actionCreate_list, SIGNAL(triggered(bool)), this, SLOT(onExcelButtonTriggered()));
  connect(actionDownload, SIGNAL(triggered(bool)), this, SLOT(onDownloadButtonTriggered()));
  connect(actionSettings, SIGNAL(triggered(bool)), this, SLOT(onSettingsButtonTriggered()));
  connect(actionUpload, SIGNAL(triggered(bool)), this, SLOT(onUploadButtonTriggered()));
  connect(actionDelete, SIGNAL(triggered(bool)), this, SLOT(onDeleteButtonTriggered()));
  connect(actionCreate_directory, SIGNAL(triggered(bool)), this, SLOT(onCreateButtonTriggered()));
  connect(m_searchLine, SIGNAL(textChanged(const QString &)), this, SLOT(onSearchTextChanged(const QString &)));

  connect(m_searchButton, SIGNAL(clicked(bool)), this, SLOT(onSearchButtonClicked()));
}

//-----------------------------------------------------------------------------
void MainWindow::onExcelButtonTriggered()
{
  auto contents = selectedFiles();

  if(contents.empty())
  {
    QMessageBox::information(this, tr("Export list"), tr("No files selected!"));
    return;
  }

  auto dateTimeString = QDateTime::currentDateTime().toString("dd.mm.yyyy-hh.mm");
  auto suggestion = tr("Pato selected files %1.xls").arg(dateTimeString);
  auto path = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();
  auto filename = QFileDialog::getSaveFileName(this, tr("Save Excel file"), path + QDir::separator() + suggestion, tr("Excel files (*.xls);;CSV files (*.csv)"));

  bool success = false;
  if (!filename.isEmpty())
  {
    if (filename.endsWith(".csv", Qt::CaseInsensitive))
    {
      success = ListExportUtils::saveToCSV(filename.toStdString(), contents);
    }
    else
    {
      assert(filename.endsWith(".xls", Qt::CaseInsensitive));
      success = ListExportUtils::saveToXLS(filename.toStdString(), contents);
    }

    if (!success)
    {
      auto message = tr("File '%1' couldn't be saved.").arg(filename);
      QMessageBox::critical(this, tr("Export list"), message);
    }
  }
}

//-----------------------------------------------------------------------------
std::vector<std::pair<std::string, unsigned long long> > MainWindow::selectedFiles() const
{
  std::vector<std::pair<std::string, unsigned long long>> contents;
  auto &items = m_factory->items();

  auto searchSelectedFiles = [&contents](const Item *i)
  {
    if(i && i->isSelected() && i->type() == Type::File)
    {
      contents.emplace_back(i->fullName().toLatin1(), i->size());
    }
  };
  std::for_each(items.cbegin(), items.cend(), searchSelectedFiles);

  return contents;
}

//-----------------------------------------------------------------------------
void MainWindow::onDownloadButtonTriggered()
{
  auto contents = selectedFiles();

  if(contents.empty())
  {
    QMessageBox::information(this, tr("Download files"), tr("No files selected!"));
    return;
  }

  auto defaultDir = QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first();
  auto dir = QFileDialog::getExistingDirectory(this, tr("Select download directory"), defaultDir, QFileDialog::ShowDirsOnly);

  if(!dir.isEmpty())
  {
    // TODO
  }
}

//-----------------------------------------------------------------------------
void MainWindow::onSettingsButtonTriggered()
{
  // TODO
}

//-----------------------------------------------------------------------------
void MainWindow::onUploadButtonTriggered()
{
  // TODO
}

//-----------------------------------------------------------------------------
void MainWindow::onDeleteButtonTriggered()
{
  // TODO
}

//-----------------------------------------------------------------------------
void MainWindow::onCreateButtonTriggered()
{
  // TODO
}

//-----------------------------------------------------------------------------
void MainWindow::onSearchTextChanged(const QString& text)
{
  m_searchButton->setEnabled(!text.isEmpty());

  if(text.isEmpty()) onSearchButtonClicked();
}

//-----------------------------------------------------------------------------
void MainWindow::onSearchButtonClicked()
{
  auto filter = qobject_cast<FilterTreeModelProxy *>(m_treeView->model());
  if(filter)
  {
    QApplication::setOverrideCursor(Qt::WaitCursor);

    filter->setFilterFixedString(m_searchLine->text());

    QApplication::restoreOverrideCursor();
  }
}
