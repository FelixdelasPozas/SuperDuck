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
#include <Dialogs/AboutDialog.h>

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
}

//-----------------------------------------------------------------------------
void MainWindow::restoreConfiguration()
{
  QSettings settings(Utils::dataPath() + QDir::separator() + "SuperDuck.ini", QSettings::IniFormat);

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
  QSettings settings(Utils::dataPath() + QDir::separator() + "SuperDuck.ini", QSettings::IniFormat);

  settings.setValue(STATE, saveState());
  settings.setValue(GEOMETRY, saveGeometry());

  settings.sync();
}

//-----------------------------------------------------------------------------
void MainWindow::connectSignals()
{
  connect(actionSettings, SIGNAL(triggered(bool)), this, SLOT(onSettingsButtonTriggered()));
  connect(actionAbout, SIGNAL(triggered(bool)), this, SLOT(onAboutButtonTriggered()));
  connect(m_searchLine, SIGNAL(textChanged(const QString &)), this, SLOT(onSearchTextChanged(const QString &)));
  connect(m_searchLine, SIGNAL(returnPressed()), this, SLOT(onSearchButtonClicked()));
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
  auto suggestion = tr("SuperDuck selected objects %1.xls").arg(dateTimeString);
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
  op.parameters = Aws::String(m_configuration.DownloadPath.toStdString().c_str(), m_configuration.DownloadPath.length());
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

  if(!m_configuration.isValid())
  {
    QMessageBox::warning(this, tr("SuperDuck"), tr("Without valid AWS credentials file uploads, downloads or removal are not possible."));
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

  auto path  = items.front()->fullName();
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
    op.parameters = Aws::String(path.toStdString().c_str(), path.length());
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
  auto items = getSelectedItems();
  Item *parent;
  auto checkParent = [&parent](const Item *i)
  {
    if(!parent) parent = i->parent();
    if(i->parent() != parent) return true;

    return false;
  };
  auto it = std::find_if(items.cbegin(), items.cend(), checkParent);

  const auto title = tr("Delete objects");

  if(it != items.cend())
  {
    QMessageBox::information(this, title, tr("Selection musn't have multiple parents."));
    return;
  }

  if(items.size() == 1)
  {
    auto item = items.at(0);
    if(isDirectory(item) && item->children().size() == 0)
    {
      QMessageBox msgBox(this);
      msgBox.setWindowTitle(title);
      msgBox.setWindowIcon(QIcon(":/Pato/rubber-duck.svg"));
      msgBox.setStandardButtons(QMessageBox::Cancel|QMessageBox::Ok);
      msgBox.setText(tr("Do you really want to delete the directory '%1'?").arg(item->name()));
      msgBox.setIcon(QMessageBox::Icon::Question);

      if(msgBox.exec() == QMessageBox::Ok)
      {
        m_model->removeItem(item);

        updateStatusLabel();
      }
      return;
    }
  }

  auto selected = getSelectedFileList();

  if(selected.empty())
  {
    QMessageBox::information(this, title, tr("No objects selected!"));
    return;
  }

  int dirNum = 0, fileNum = 0;
  std::for_each(items.cbegin(), items.cend(), [&dirNum, &fileNum](const Item *i){ if(i) { dirNum += i->directoriesNumber(); fileNum += i->filesNumber(); }});

  QString message = tr("Do you really want to delete ");
  if(fileNum > 0)
  {
    message += tr("%1 file%2%3").arg(fileNum).arg(fileNum > 1 ? "s":"").arg(dirNum > 0 ? " and ":"?");
  }
  if(dirNum > 0)
  {
    message += tr("%1 director%2?").arg(dirNum).arg(dirNum > 1 ? "ies":"y");
  }

  QString details = tr("Objects to be deleted from the bucket:");
  auto addSelectedFiles = [&details](const std::pair<std::string, unsigned long long> &p)
  {
    details += tr("\n%1").arg(QString::fromStdString(p.first));
  };
  std::for_each(selected.cbegin(), selected.cend(), addSelectedFiles);

  QMessageBox msgBox(this);
  msgBox.setWindowTitle(title);
  msgBox.setWindowIcon(QIcon(":/Pato/rubber-duck.svg"));
  msgBox.setStandardButtons(QMessageBox::Cancel|QMessageBox::Ok);
  msgBox.setText(message);
  msgBox.setDetailedText(details);
  msgBox.setIcon(QMessageBox::Icon::Question);

  if(msgBox.exec() == QMessageBox::Ok)
  {
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
}

//-----------------------------------------------------------------------------
void MainWindow::onCreateActionTriggered()
{
  auto items = getSelectedItems();

  const auto title = tr("Create directory");

  if(items.size() > 1)
  {
    QMessageBox::information(this, title, tr("Invalid selection!"));
    return;
  }

  bool ok;
  auto directory = QInputDialog::getText(this, tr("Enter directory name"), tr("Directory:"), QLineEdit::Normal, "New_Directory", &ok);
  if(!ok || directory.isEmpty()) return;

  if (directory.contains(AWSUtils::DELIMITER))
  {
    QMessageBox::information(this, title, tr("The name '%1' is invalid!\nMust not contain the '/' character.").arg(directory));
    return;
  }

  auto parent = (items.empty() ? m_factory->items().at(0) : items.front());
  auto children = parent->children();
  auto it = std::find_if(children.cbegin(), children.cend(), [&directory](const Item *i){ if(i) return (i->name().compare(directory, Qt::CaseInsensitive) == 0); return false; });
  if(it != children.cend())
  {
    QMessageBox::information(this, title, tr("The name '%1' is invalid!\nThe parent has already a directory with that name.").arg(directory));
    return;
  }

  m_model->createSubdirectory(parent, directory);

  updateStatusLabel();
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
  QApplication::setOverrideCursor(Qt::WaitCursor);

  auto selectedIndexes = m_treeView->selectionModel()->selectedIndexes();

  m_model->setFilter(m_searchLine->text());

  restoreExpandedIndexes();

  QModelIndex lastIndex;
  auto selectIndex = [this, &lastIndex](const QModelIndex &i)
  {
    auto item = static_cast<Item *>(i.internalPointer());
    auto index = m_model->indexOf(item);
    if(index.isValid())
    {
      m_treeView->selectionModel()->select(index, QItemSelectionModel::SelectionFlag::Select);
      lastIndex = index;
    }
  };
  std::for_each(selectedIndexes.cbegin(), selectedIndexes.cend(), selectIndex);
  if(lastIndex.isValid()) m_treeView->scrollTo(lastIndex, QAbstractItemView::ScrollHint::EnsureVisible);

  updateStatusLabel();

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void MainWindow::showEvent(QShowEvent* e)
{
  QMainWindow::showEvent(e);

  if(!m_configuration.isValid()) QTimer::singleShot(0, this, SLOT(onInvalidConfiguration()));
}

//-----------------------------------------------------------------------------
void MainWindow::onInvalidConfiguration()
{
  QMessageBox::information(this, QApplication::applicationName(), tr("AWS configuration is not valid!"));
  onSettingsButtonTriggered();
}

//-----------------------------------------------------------------------------
void MainWindow::resizeEvent(QResizeEvent* e)
{
  QMainWindow::resizeEvent(e);

  updateGeometry();
  const auto width = rect().width();
  m_treeView->setColumnWidth(0, width*0.75);
}

//-----------------------------------------------------------------------------
void MainWindow::onOperationFinished()
{
  auto thread = qobject_cast<AWSUtils::S3Thread *>(sender());
  if(thread)
  {
    const auto items = getSelectedItems();
    const auto operation = thread->operation();
    const auto &errors = thread->errors();

    if(!errors.isEmpty())
    {
      QString details = tr("There has been errors in the following objects:");
      for(auto i = errors.cbegin(); i != errors.cend(); ++i)
      {
        details += tr("\n%1: %2").arg(i.key()).arg(i.value().join('\n'));
      }

      QMessageBox msgBox(this);
      msgBox.setWindowTitle(tr("%1 operation").arg(AWSUtils::operationTypeToText(operation.type)));
      msgBox.setWindowIcon(QIcon(":/Pato/rubber_duck.svg"));
      msgBox.setText(tr("The operation finished with errors."));
      msgBox.setDetailedText(details);
      msgBox.setIcon(QMessageBox::Icon::Critical);
      msgBox.setStandardButtons(QMessageBox::Ok);

      msgBox.exec();
    }

    auto itemsWithErrors = errors.keys();

    switch(operation.type)
    {
      case AWSUtils::OperationType::remove:
        for(auto it = items.begin(); it != items.end(); ++it)
        {
          if(!itemsWithErrors.contains((*it)->fullName())) m_factory->deleteItem(*it);
        }
        updateStatusLabel();
        break;
      case AWSUtils::OperationType::upload:
        {
          auto parentItem = items.at(0);
          for(auto it = operation.keys.cbegin(); it != operation.keys.cend(); ++it)
          {
            const auto pair = (*it);
            const auto filename = QString::fromStdString(pair.first);
            if(!itemsWithErrors.contains(filename))
            {
              QFileInfo info(filename);
              m_factory->createItem(info.fileName(), parentItem, pair.second, Type::File);
            }
          }
        }
        updateStatusLabel();
        break;
      case AWSUtils::OperationType::download:
      default:
        break;
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
  m_model = new TreeModel(m_factory);

  m_treeView->setModel(m_model);
  m_treeView->setAlternatingRowColors(true);
  m_treeView->setAnimated(true);
  m_treeView->setExpandsOnDoubleClick(true);
  m_treeView->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  m_treeView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  m_treeView->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

  connect(m_treeView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onContextMenuRequested(const QPoint &)));
  connect(m_treeView, SIGNAL(collapsed(const QModelIndex &)), this, SLOT(onIndexCollapsed(const QModelIndex &)));
  connect(m_treeView, SIGNAL(expanded(const QModelIndex &)), this, SLOT(onIndexExpanded(const QModelIndex &)));
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
    deleteAction.setEnabled(!m_configuration.DisableDelete);

    if(items.size() == 1)
    {
      const auto item = items.at(0);
      const auto itemName = item->name();
      if(isDirectory(item))
      {
        contextMenu.setTitle(itemName);
        downloadAction.setText(tr("Download objects in '%1'").arg(itemName));
        uploadAction.setText(tr("Upload files to '%1'").arg(itemName));
        createAction.setText(tr("Create subdirectory in '%1'").arg(itemName));
        deleteAction.setText(tr("Delete '%1' and its contents").arg(itemName));

        downloadAction.setEnabled(item->childrenCount() > 0);
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
      Item *parent = nullptr;
      auto checkParent = [&parent](const Item *i)
      {
        if(!parent) parent = i->parent();
        if(i->parent() != parent) return true;

        return false;
      };
      auto it = std::find_if(items.cbegin(), items.cend(), checkParent);
      bool multipleParents = (it != items.cend());

      deleteAction.setEnabled(!m_configuration.DisableDelete && !multipleParents);
      uploadAction.setEnabled(false);
      createAction.setEnabled(false);
    }
  }

  contextMenu.exec(QCursor::pos());
}

//-----------------------------------------------------------------------------
Items MainWindow::getSelectedItems() const
{
  Items items;
  QModelIndexList validIndexes;

  auto selectedIndexes = m_treeView->selectionModel()->selectedIndexes();
  std::for_each(selectedIndexes.cbegin(), selectedIndexes.cend(), [&validIndexes](const QModelIndex &i) { if(i.isValid() && i.column() == 0) validIndexes << i; });

  if(!validIndexes.isEmpty())
  {
    auto indexesToItems = [&items](const QModelIndex &i){ auto item = static_cast<Item *>(i.internalPointer()); if(item) items.push_back(item); };
    std::for_each(validIndexes.cbegin(), validIndexes.cend(), indexesToItems);
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

//-----------------------------------------------------------------------------
void MainWindow::onIndexExpanded(const QModelIndex& index)
{
  if(!m_expanded.contains(index)) m_expanded << index;
}

//-----------------------------------------------------------------------------
void MainWindow::onIndexCollapsed(const QModelIndex& index)
{
  if(m_expanded.contains(index)) m_expanded.removeOne(index);
}

//-----------------------------------------------------------------------------
void MainWindow::restoreExpandedIndexes()
{
  QModelIndexList newList;
  auto expandIndex = [this, &newList](const QModelIndex &i)
  {
    auto item = static_cast<Item *>(i.internalPointer());
    auto index = m_model->indexOf(item);
    if(index.isValid())
    {
      newList << index;
      m_treeView->expand(index);
    }
  };
  std::for_each(m_expanded.cbegin(), m_expanded.cend(), expandIndex);

  m_expanded = newList;
}

//-----------------------------------------------------------------------------
void MainWindow::onAboutButtonTriggered()
{
  AboutDialog dialog(this);
  dialog.exec();
}
