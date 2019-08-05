/*
 File: main.cpp
 Created on: 30/10/2018
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
#include <SplashScreen.h>
#include <ItemsTree.h>
#include <MainWindow.h>

// Qt
#include <QApplication>
#include <QSharedMemory>
#include <QObject>
#include <QMessageBox>
#include <QString>
#include <QIcon>
#include <QSplashScreen>
#include <QObject>
#include <QFile>

// C++
#include <iostream>
#include <unistd.h>
#include <fstream>

//-----------------------------------------------------------------
void deserializeListMethod(SplashScreen *splash, QApplication *app, ItemFactory &factory)
{
  std::ifstream stream;
  stream.open("cloud_tree.txt", std::ios_base::in);

  auto root = factory.createItem("", nullptr, 0, Type::Directory);
  Item *currentRoot = nullptr;

  stream.seekg(0, std::ios_base::end);
  auto streamSize = stream.tellg();
  stream.seekg(0, std::ios_base::beg);

  int progress = 0;

  std::string delimiter(" ");
  std::string directory;

  if (stream.is_open())
  {
    while (!stream.eof())
    {
      int cProgress = (stream.tellg() * 100) / streamSize;
      if(progress != cProgress)
      {
        progress = cProgress;
        splash->setProgress(progress);
        app->processEvents();
      }

      std::string line;
      bool isDirectory = false;
      std::getline(stream, line);

      if (line == "\n") continue;

      if (line.back() == ':')
      {
        directory = line.substr(1, line.length() - 2);
        if (directory.empty())
        {
          //std::cout << "assign root" << std::endl;
          currentRoot = root;
        }
        else
        {
          //std::cout << "directory " << directory << std::endl;

          currentRoot = find(QString::fromStdString(directory), currentRoot);
          if (!currentRoot)
          {
            //std::cout << "couldn't find " << directory << std::endl;
            return;
          }
        }
        continue;
      }

      unsigned int count = 0;
      unsigned long long size = 0;
      size_t pos = 0;
      std::string token;
      std::string name;

      while ((pos = line.find(delimiter)) != std::string::npos)
      {
        token = line.substr(0, pos);

        //std::cout << "token \"" << token << "\"" << std::endl;

        if (count == 0 && token == "total") break; // total line

        if (count == 0 && token.front() == 'd')
        {
          isDirectory = true;
        }

        if (token.size() == 0 && count < 8)
        {
          line.erase(0, delimiter.length());
          continue;
        }

        if (count == 4)
        {
          size = atoll(token.c_str());
        }

        if (count >= 8)
        {
          name += token + " ";
        }

        //std::cout << "token " << count << " " << token << std::endl;

        line.erase(0, pos + delimiter.length());
        ++count;
      }

      if (token == "total")
      {
        //std::cout << "skip" << std::endl;
        continue;
      }

      name += line;

      if (name.empty())
      {
        continue;
      }

      //std::cout << "name " << name << " isDir " << (isDirectory ? "true":"false") << " size " << size << std::endl;

      if (count != 0 && size != 0)
      {
        auto item = factory.createItem(QString::fromStdString(name), currentRoot, (isDirectory ? 0 : size), (isDirectory ? Type::Directory : Type::File));
        if (currentRoot)
        {
          currentRoot->addChild(item);
        }
        else
        {
          std::cout << "couldn't insert " << item->name().toStdString() << " size " << size << (isDirectory ? " dir " : " file ") << std::endl;
          return;
        }
      }
    }
    std::cout << "finished " << factory.count() << std::endl;
  }
}

//-----------------------------------------------------------------
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
  const char symbols[] =
  { 'I', 'E', '!', 'X' };
//  QString output = QString("[%1] %2 (%3:%4 -> %5)").arg( symbols[type] ).arg( msg ).arg(context.file).arg(context.line).arg(context.function);
  QString output = QString("[%1] %2").arg(symbols[type]).arg(msg);
  std::cerr << output.toStdString() << std::endl;
  if (type == QtFatalMsg) abort();
}

//-----------------------------------------------------------------
int main(int argc, char **argv)
{
  qInstallMessageHandler(myMessageOutput);

  QApplication app(argc, argv);

  // allow only one instance running
  QSharedMemory guard;
  guard.setKey("SuperDuck");

  if (!guard.create(1))
  {
    QMessageBox msgbox;
    msgbox.setWindowIcon(QIcon(":/Pato/rubber-duck.ico"));
    msgbox.setWindowTitle(QObject::tr("Super Pato"));
    msgbox.setIcon(QMessageBox::Information);
    msgbox.setText(QObject::tr("An instance is already running!"));
    msgbox.setStandardButtons(QMessageBox::Ok);
    msgbox.exec();

    return 0;
  }

  SplashScreen splash(&app);
  splash.setMessage(QString("Loading database"));
  splash.show();

  std::ifstream stream;
  stream.open("dbData.txt", std::ios_base::in);

  ItemFactory factory;

  if(stream.is_open())
  {
    factory.deserializeItems(stream, &splash, &app);
  }
  else
  {
    QMessageBox msgbox;
    msgbox.setWindowIcon(QIcon(":/Pato/rubber-duck.ico"));
    msgbox.setWindowTitle(QObject::tr("Super Pato"));
    msgbox.setIcon(QMessageBox::Information);
    msgbox.setText(QObject::tr("Unable to find database!"));
    msgbox.setStandardButtons(QMessageBox::Ok);
    msgbox.exec();

    return 0;
  }

  MainWindow application(&factory);

  splash.hide();

  application.show();

  return app.exec();
}
