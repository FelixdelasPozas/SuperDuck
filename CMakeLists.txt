project (SuperDuck)

cmake_minimum_required (VERSION 2.8.6)

# Version Number
set (SUPER_DUCK_VERSION_MAJOR 1)
set (SUPER_DUCK_VERSION_MINOR 0)
set (SUPER_DUCK_VERSION_PATCH 0)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)

add_definitions(
  -std=c++11
  -fpermissive
  -mwindows)

include_directories (${CMAKE_SOURCE_DIR})
include_directories (${PROJECT_BINARY_DIR})

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Find the QtWidgets library
find_package(Qt5 COMPONENTS Core Widgets)
include_directories ( ${Qt5Widgets_INCLUDE_DIRS}
                      ${Qt5Core_INCLUDE_DIRS})
					  
# Locate the AWS SDK for C++ package.
# Requires that you build with:
#   -DCMAKE_PREFIX_PATH=/path/to/sdk_install
find_package(AWSSDK REQUIRED COMPONENTS s3)
set(S3_INCLUDE_DIRS "D:/Desarrollo/Bibliotecas/aws-sdk-cpp-1.7.154/aws-cpp-sdk-s3/include"
                    "D:/Desarrollo/Bibliotecas/aws-sdk-cpp-1.7.154/aws-cpp-sdk-transfer/include"
                    "D:/Desarrollo/Bibliotecas/aws-sdk-cpp-1.7.154/aws-cpp-sdk-core/include")
set(S3_LIBRARIES 
                 "D:/Desarrollo/Bibliotecas/aws-sdk-build/aws-cpp-sdk-core/libaws-cpp-sdk-core.dll"
                 "D:/Desarrollo/Bibliotecas/aws-sdk-build/aws-cpp-sdk-s3/libaws-cpp-sdk-s3.dll"
                 "D:/Desarrollo/Bibliotecas/aws-sdk-build/aws-cpp-sdk-transfer/libaws-cpp-sdk-transfer.dll"
                 )
include_directories(${S3_INCLUDE_DIRS})

set(XLSLIB_INCLUDE_DIRS "D:/Desarrollo/Bibliotecas/xlslib/xlslib/src")
set(XLSLIB_LIBRARIES "D:/Desarrollo/Bibliotecas/xlslib-build/libxlslib.a")
include_directories (${XLSLIB_INCLUDE_DIRS})
					  
if (CMAKE_BUILD_TYPE MATCHES Debug)
  set(CORE_EXTERNAL_LIBS ${CORE_EXTERNAL_LIBS} ${QT_QTTEST_LIBRARY})
endif (CMAKE_BUILD_TYPE MATCHES Debug)

set(CMAKE_CXX_FLAGS " -Wall -Wno-deprecated -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

configure_file("${PROJECT_SOURCE_DIR}/SuperDuck.rc.in" "${PROJECT_BINARY_DIR}/SuperDuck.rc")
configure_file("${PROJECT_SOURCE_DIR}/installer/script.iss.in" "${PROJECT_BINARY_DIR}/script.iss")

set (CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -o <OBJECT> -i <SOURCE>")
ENABLE_LANGUAGE(RC)

# Add Qt Resource files
qt5_add_resources(RESOURCES
	resources/resources.qrc
	)

set (SOURCES 
	${RESOURCES}
	${PROJECT_BINARY_DIR}/SuperDuck.rc
	Dialogs/SplashScreen.cpp
	Dialogs/SettingsDialog.cpp
	Dialogs/ProgressDialog.cpp
	Dialogs/AboutDialog.cpp
	Model/ItemsTree.cpp
	Model/TreeModel.cpp
	MainWindow.cpp
	Utils/ListExportUtils.cpp
	Utils/AWSUtils.cpp
	Utils/Utils.cpp
	main.cpp
	)
  
set (LIBRARIES 
    Qt5::Core
 	Qt5::Widgets
	${AWSSDK_LINK_LIBRARIES}
	${XLSLIB_LIBRARIES}
	${S3_LIBRARIES}
	ws2_32
	wsock32
	userenv
	version
	)
	
add_executable(SuperDuck ${SOURCES})
target_link_libraries (SuperDuck ${LIBRARIES})
	