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
#include <Utils/ListExportUtils.h>
#include <Dialogs/SettingsDialog.h>
#include <Dialogs/ProgressDialog.h>

// C++
#include <fstream>
#include <cassert>

// Qt
#include <QSettings>
#include <QMessageBox>
#include <QDateTime>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTimer>
#include <QMenu>
#include <QInputDialog>

// AWS
#include <aws/core/Aws.h>

const QString STATE    = "State";
const QString GEOMETRY = "Geometry";

//-----------------------------------------------------------------------------
MainWindow::MainWindow(Utils::Configuration &configuration, ItemFactory* factory, QWidget* parent, Qt::WindowFlags flags)
: QMainWindow(parent, flags)
, m_factory{factory}
, m_configuration(configuration)
{
  setupUi(this);

  restoreConfiguration();

  configureTreeView();

  connectSignals();

  m_statusLabel = new QLabel();
  statusBar()->addWidget(m_statusLabel);

  updateStatusLabel();
}

//-----------------------------------------------------------------------------
MainWindow::~MainWindow()
{
  saveConfiguration();

  if(m_factory->hasBeenModified())
  {
    std::ofstream outFile("dbData2.txt");
    m_factory->serializeItems(outFile);
    outFile.close();
  }
}

//-----------------------------------------------------------------------------
void MainWindow::restoreConfiguration()
{
  QSettings settings(Utils::dataPath() + QDir::separator() + "SuperPato.ini", QSettings::IniFormat);

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
void MainWindow::saveConfiguration()
{
  QSettings settings(Utils::dataPath() + QDir::separator() + "SuperPato.ini", QSettings::IniFormat);

  settings.setValue(STATE, saveState());
  settings.setValue(GEOMETRY, saveGeometry());
}

//-----------------------------------------------------------------------------
void MainWindow::connectSignals()
{
  connect(actionSettings, SIGNAL(triggered(bool)), this, SLOT(onSettingsButtonTriggered()));
  connect(m_searchLine, SIGNAL(textChanged(const QString &)), this, SLOT(onSearchTextChanged(const QString &)));
  connect(m_searchButton, SIGNAL(clicked(bool)), this, SLOT(onSearchButtonClicked()));
}

//-----------------------------------------------------------------------------
void MainWindow::onExportActionTriggered()
{
  auto selected = getSelectedFileList(m_configuration.Export_Full_Paths);

  if(selected.empty())
  {
    QMessageBox::information(this, tr("Export list"), tr("No objects selected!"));
    return;
  }

  auto dateTimeString = QDateTime::currentDateTime().toString("dd.mm.yyyy-hh.mm");
  auto suggestion = tr("Pato selected objects %1.xls").arg(dateTimeString);
  auto path = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();
  auto filename = QFileDialog::getSaveFileName(this, tr("Save file list"), QDir(m_configuration.DownloadPath).absoluteFilePath(suggestion), tr("Excel files (*.xls);;CSV files (*.csv)"));

  bool success = false;
  if (!filename.isEmpty())
  {
    if (filename.endsWith(".csv", Qt::CaseInsensitive))
    {
      success = ListExportUtils::saveToCSV(filename.toStdString(), selected);
    }
    else
    {
      if (filename.endsWith(".xls", Qt::CaseInsensitive))
      {
        success = ListExportUtils::saveToXLS(filename.toStdString(), selected);
      }
      else
      {
        QMessageBox::information(this, tr("Export list"), tr("Unknown format '%1'").arg(filename.split('.').last()));
        return;
      }
    }

    if (!success)
    {
      auto message = tr("File '%1' couldn't be saved.").arg(filename);
      QMessageBox::critical(this, tr("Export list"), message);
    }
  }
}

//-----------------------------------------------------------------------------
void MainWindow::onDownloadActionTriggered()
{
  auto selected = getSelectedFileList();

  if(selected.empty())
  {
    QMessageBox::information(this, tr("Download objects"), tr("No objects selected!"));
    return;
  }

  AWSUtils::Operation op;
  op.bucket = AWSUtils::toAwsString(m_configuration.AWS_Bucket);
  op.region = AWSUtils::toAwsString(m_configuration.AWS_Region);
  op.type   = AWSUtils::OperationType::download;
  op.credentials = Aws::Auth::AWSCredentials(AWSUtils::toAwsString(Utils::rot13(m_configuration.AWS_Access_key_id)),
                                             AWSUtils::toAwsString(Utils::rot13(m_configuration.AWS_Secret_access_key)));
  op.keys = std::move(selected);
  op.useLogging = true;

  auto thread = new AWSUtils::S3Thread(op);
  m_threads << thread;

  connect(thread, SIGNAL(finished()), this, SLOT(onOperationFinished()));

  ProgressDialog dialog(thread, this);
  dialog.exec();
}

//-----------------------------------------------------------------------------
void MainWindow::onSettingsButtonTriggered()
{
  SettingsDialog dialog(m_configuration, this);

  if(dialog.exec() == QDialog::Accepted)
  {
    const auto config = dialog.configuration();
    if(config.isValid())
    {
      m_configuration = config;
    }
  }
}

//-----------------------------------------------------------------------------
void MainWindow::onUploadActionTriggered()
{
  auto items = getSelectedItems();

  if(items.size() > 1)
  {
    QMessageBox::information(this, tr("Upload to bucket"), tr("Invalid selection!"));
    return;
  }

  auto files = QFileDialog::getOpenFileNames(this, tr("Upload files"), QDir::homePath());

  if(!files.empty())
  {
    std::vector<std::pair<std::string, unsigned long long>> selected;

    auto gatherFileInformation = [&selected](const QString &filename)
    {
      QFileInfo info(filename);
      if(info.exists() && info.isReadable())
      {
        selected.emplace_back(info.absoluteFilePath().toStdString(), info.size());
      }
    };
    std::for_each(files.cbegin(), files.cend(), gatherFileInformation);

    if(selected.empty())
    {
      QMessageBox::information(this, tr("Upload to bucket"), tr("Cannot read the selected files!"));
      return;
    }

    AWSUtils::Operation op;
    op.bucket = AWSUtils::toAwsString(m_configuration.AWS_Bucket);
    op.region = AWSUtils::toAwsString(m_configuration.AWS_Region);
    op.type   = AWSUtils::OperationType::upload;
    op.credentials = Aws::Auth::AWSCredentials(AWSUtils::toAwsString(Utils::rot13(m_configuration.AWS_Access_key_id)),
                                               AWSUtils::toAwsString(Utils::rot13(m_configuration.AWS_Secret_access_key)));
    op.keys = std::move(selected);
    Aws::String destination;
    if(!items.empty())
    {
      auto item = items.at(0);
      const auto itemName = item->fullName();
      destination = Aws::String(itemName.toStdString().c_str(), itemName.length());
    }
    op.useLogging = true;

    auto thread = new AWSUtils::S3Thread(op);
    m_threads << thread;

    connect(thread, SIGNAL(finished()), this, SLOT(onOperationFinished()));

    ProgressDialog dialog(thread, this);
    dialog.exec();
  }
}

//-----------------------------------------------------------------------------
void MainWindow::onDeleteActionTriggered()
{
  auto selected = getSelectedFileList();

  if(selected.empty())
  {
    QMessageBox::information(this, tr("Delete objects"), tr("No objects selected!"));
    return;
  }

  AWSUtils::Operation op;
  op.bucket = AWSUtils::toAwsString(m_configuration.AWS_Bucket);
  op.region = AWSUtils::toAwsString(m_configuration.AWS_Region);
  op.type   = AWSUtils::OperationType::remove;
  op.credentials = Aws::Auth::AWSCredentials(AWSUtils::toAwsString(Utils::rot13(m_configuration.AWS_Access_key_id)),
                                             AWSUtils::toAwsString(Utils::rot13(m_configuration.AWS_Secret_access_key)));
  op.keys = std::move(selected);
  op.useLogging = true;

  auto thread = new AWSUtils::S3Thread(op);
  m_threads << thread;

  connect(thread, SIGNAL(finished()), this, SLOT(onOperationFinished()));

  ProgressDialog dialog(thread, this);
  dialog.exec();
}

//-----------------------------------------------------------------------------
void MainWindow::onCreateActionTriggered()
{
  auto items = getSelectedItems();

  if(items.size() > 1)
  {
    QMessageBox::information(this, tr("Create directory"), tr("Invalid selection!"));
    return;
  }

  bool ok;
  auto directory = QInputDialog::getText(this, tr("Enter directory name"), tr("Directory:"), QLineEdit::Normal, QDir::home().dirName(), &ok);
  if (!ok || directory.isEmpty() || directory.contains(Utils::DELIMITER))
  {
    QMessageBox::information(this, tr("Create directory"), tr("The name '%1' is invalid!").arg(directory));
    return;
  }

  std::vector<std::pair<std::string, unsigned long long>> selected;

  if(!items.empty())
  {
    auto name = items.at(0)->fullName();
    selected.emplace_back(name.toStdString(), 0);
  }

  AWSUtils::Operation op;
  op.bucket = AWSUtils::toAwsString(m_configuration.AWS_Bucket);
  op.region = AWSUtils::toAwsString(m_configuration.AWS_Region);
  op.type   = AWSUtils::OperationType::create;
  op.credentials = Aws::Auth::AWSCredentials(AWSUtils::toAwsString(Utils::rot13(m_configuration.AWS_Access_key_id)),
                                             AWSUtils::toAwsString(Utils::rot13(m_configuration.AWS_Secret_access_key)));
  op.keys = selected;
  op.parameters = Aws::String(directory.toStdString().c_str(), directory.length());
  op.useLogging = true;

  auto thread = new AWSUtils::S3Thread(op);
  m_threads << thread;

  connect(thread, SIGNAL(finished()), this, SLOT(onOperationFinished()));

  ProgressDialog dialog(thread, this);
  dialog.exec();
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

//-----------------------------------------------------------------------------
void MainWindow::refreshView()
{
  m_treeView->update();
  m_treeView->repaint();
}

//-----------------------------------------------------------------------------
void MainWindow::showEvent(QShowEvent* e)
{
  QMainWindow::showEvent(e);

  if(!m_configuration.isValid())
  {
    QTimer::singleShot(0, this, SLOT(onInvalidConfiguration()));
  }
}

//-----------------------------------------------------------------------------
void MainWindow::onInvalidConfiguration()
{
  while(!m_configuration.isValid())
  {
    QMessageBox::information(this, QApplication::applicationName(), tr("AWS configuration is not valid!"));

    onSettingsButtonTriggered();
  }
}

//-----------------------------------------------------------------------------
void MainWindow::resizeEvent(QResizeEvent* e)
{
  QMainWindow::resizeEvent(e);

  updateGeometry();
  auto width = rect().width();

  m_treeView->setColumnWidth(0, width*0.75);
}

//-----------------------------------------------------------------------------
void MainWindow::onOperationFinished()
{
  auto thread = qobject_cast<AWSUtils::S3Thread *>(sender());
  if(thread)
  {
    auto operation = thread->operation();

    if(!thread->errors().isEmpty())
    {
      auto errorList = thread->errors();

      QMessageBox msgBox(this);
      msgBox.setWindowTitle(tr("%1 operation").arg(AWSUtils::operationTypeToText(operation.type)));
      msgBox.setWindowIcon(QIcon(":/Pato/rubber_duck.svg"));
      msgBox.setText(tr("The operation finished with errors."));
      msgBox.setDetailedText(errorList.join('\n'));
      msgBox.setIcon(QMessageBox::Icon::Critical);
      msgBox.setStandardButtons(QMessageBox::Ok);

      msgBox.exec();
    }
    else
    {
      switch(operation.type)
      {
        case AWSUtils::OperationType::create:
          // TODO
          updateStatusLabel();
          break;
        case AWSUtils::OperationType::remove:
          // TODO
          updateStatusLabel();
          break;
        case AWSUtils::OperationType::upload:
          // TODO
          updateStatusLabel();
          break;
        case AWSUtils::OperationType::download:
        default:
          break;
      }
    }

    delete thread;
  }
  else
  {
    std::cout << "Error: onOperationFinished() -> Unable to identify sender.";
  }
}

//-----------------------------------------------------------------------------
void MainWindow::updateStatusLabel()
{
  auto rootItem = m_factory->items().at(0);
  m_statusLabel->setText(tr("%1 objects in %2 directories totaling %3 bytes.").arg(rootItem->filesNumber()).arg(rootItem->directoriesNumber()).arg(rootItem->size()));
}

//-----------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent* e)
{
  if(m_threads.empty())
  {
    auto stopThread = [](AWSUtils::S3Thread *t)
    {
      if(!t->isFinished())
      {
        t->thread()->terminate();
      }

      delete t;
    };
    std::for_each(m_threads.begin(), m_threads.end(), stopThread);
    m_threads.clear();
  }
}

//-----------------------------------------------------------------------------
void MainWindow::configureTreeView()
{
  auto model = new TreeModel(m_factory->items());
  connect(model, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)), this, SLOT(refreshView()));

  auto filter = new FilterTreeModelProxy();
  filter->setSourceModel(model);
  filter->setFilterCaseSensitivity(Qt::CaseInsensitive);
  filter->setFilterKeyColumn(0);
  filter->setFilterRole(Qt::DisplayRole);

  m_treeView->setModel(filter);
  m_treeView->setAlternatingRowColors(true);
  m_treeView->setAnimated(true);
  m_treeView->setExpandsOnDoubleClick(true);
  m_treeView->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  m_treeView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  m_treeView->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

  connect(m_treeView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onContextMenuRequested(const QPoint &)));
}

//-----------------------------------------------------------------------------
void MainWindow::onContextMenuRequested(const QPoint &pos)
{
  auto index = m_treeView->indexAt(pos);
  auto items = getSelectedItems();

  QMenu contextMenu;

  QAction downloadAction(QIcon(":/Pato/cloud-download.svg"), "Download selected objects...");
  QAction uploadAction(QIcon(":/Pato/cloud-upload.svg"), "Upload files...");
  QAction createAction(QIcon(":/Pato/cloud-create.svg"), "Create subdirectory...");
  QAction deleteAction(QIcon(":/Pato/cloud-delete.svg"), "Delete selected objects...");
  QAction exportAction(QIcon(":/Pato/excel.svg"), "Export object list...");

  connect(&downloadAction, SIGNAL(triggered()), this, SLOT(onDownloadActionTriggered()));
  connect(&uploadAction,   SIGNAL(triggered()), this, SLOT(onUploadActionTriggered()));
  connect(&createAction,   SIGNAL(triggered()), this, SLOT(onCreateActionTriggered()));
  connect(&deleteAction,   SIGNAL(triggered()), this, SLOT(onDeleteActionTriggered()));
  connect(&exportAction,   SIGNAL(triggered()), this, SLOT(onExportActionTriggered()));

  contextMenu.addAction(&downloadAction);
  contextMenu.addAction(&uploadAction);
  contextMenu.addAction(&createAction);
  contextMenu.addAction(&deleteAction);
  contextMenu.addAction(&exportAction);

  if(!index.isValid())
  {
    downloadAction.setEnabled(false);
    uploadAction.setText("Upload files to 'root'");
    createAction.setText("Create subdirectory in 'root'");
    deleteAction.setEnabled(false);
  }
  else
  {
    if(items.size() == 1)
    {
      auto item = items.at(0);
      const auto itemName = item->name();
      if(isDirectory(item))
      {
        contextMenu.setTitle(itemName);
        downloadAction.setText(tr("Download objects in '%1'").arg(itemName));
        uploadAction.setText(tr("Upload files to '%1'").arg(itemName));
        createAction.setText(tr("Create subdirectory in '%1'").arg(itemName));
        deleteAction.setText(tr("Delete '%1' and its contents").arg(itemName));
      }
      else
      {
        downloadAction.setText(tr("Donwload '%1'").arg(itemName));
        uploadAction.setEnabled(false);
        createAction.setEnabled(false);
        deleteAction.setText(tr("Delete '%1'").arg(itemName));
      }
    }
    else
    {
      uploadAction.setEnabled(false);
      createAction.setEnabled(false);
    }

    deleteAction.setEnabled(!m_configuration.DisableDelete);
  }

  contextMenu.exec(QCursor::pos());
}

//-----------------------------------------------------------------------------
Items MainWindow::getSelectedItems() const
{
  Items items;
  QModelIndexList validIndexes, validFilterIndexes;

  auto selectedFilterIndexes = m_treeView->selectionModel()->selectedIndexes();
  std::for_each(selectedFilterIndexes.cbegin(), selectedFilterIndexes.cend(), [&validFilterIndexes](const QModelIndex &i) { if(i.isValid() && i.column() == 0) validFilterIndexes << i; });

  if(!validFilterIndexes.isEmpty())
  {
    auto filterModel = qobject_cast<FilterTreeModelProxy *>(m_treeView->model());
    if(filterModel)
    {
      auto mapIndexesToSource = [&filterModel, &validIndexes](const QModelIndex &i) { auto si = filterModel->mapToSource(i); if(si.isValid()) validIndexes << si; };
      std::for_each(validFilterIndexes.cbegin(), validFilterIndexes.cend(), mapIndexesToSource);

      if(!validIndexes.isEmpty())
      {
        auto indexesToItems = [&items](const QModelIndex &i){ auto item = static_cast<Item *>(i.internalPointer()); if(item) items.push_back(item); };
        std::for_each(validIndexes.cbegin(), validIndexes.cend(), indexesToItems);
      }
    }
  }

  return items;
}

//-----------------------------------------------------------------------------
std::vector<std::pair<std::string, unsigned long long>> MainWindow::getSelectedFileList(bool useFullNames) const
{
  auto items = getSelectedItems();

  std::vector<std::pair<std::string, unsigned long long>> selected;

  std::function<void(Item *)> searchSelectedFiles = [&selected, &searchSelectedFiles, this, useFullNames](Item *i)
  {
    if(i->type() == Type::File)
    {
      const auto name = useFullNames ? i->fullName() : i->name();
      selected.emplace_back(name.toStdString(), i->size());
    }
    else
    {
      auto children = i->children();
      std::for_each(begin(children), end(children), searchSelectedFiles);
    }
  };
  std::for_each(begin(items), end(items), searchSelectedFiles);

  return selected;
}
