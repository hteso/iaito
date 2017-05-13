
cmake_minimum_required(VERSION 3.1)

project(iaito VERSION 1.0.0)

set(QT_MIN_VERSION "5.6.0")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(FeatureSummary)
include(CheckCXXCompilerFlag)
include(GenerateExportHeader)

find_package(Qt5 ${QT_MIN_VERSION} NO_MODULE REQUIRED
	WebEngine
	WebEngineWidgets
)

find_package(Radare2 1.4.0 REQUIRED)
include_directories(${RADARE2_INCLUDE_DIRS})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INSTALL_PREFIX ${BUILD_DIR})
set(BIN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/bin)

#####
##### Need to fix / remove AUTOMOC and AUTORCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
include(QMakeProParse)
parse_qmake_pro("${CMAKE_CURRENT_SOURCE_DIR}/iaito.pro" IAITO_PRO)
set(QRC_FILES ${IAITO_PRO_RESOURCES})
######

#
# Win32 settings
#
if(WIN32)
    # use radare2 libraries from submodule on windows
    set(IAITO_WIN32_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../iaito_win32")
    list(APPEND CMAKE_PREFIX_PATH "${IAITO_WIN32_DIR}")

    if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bit
        list(APPEND CMAKE_LIBRARY_PATH "${IAITO_WIN32_DIR}/radare2/lib64")
    else()
        list(APPEND CMAKE_LIBRARY_PATH "${IAITO_WIN32_DIR}/radare2/lib32")
    endif()

    set(RADARE2_INCLUDE_DIRS "${IAITO_WIN32_DIR}/radare2/include/libr" "${IAITO_WIN32_DIR}/include")
endif()
#
# Win32 setting ends

# Compiler settings
#
if(CMAKE_CXX_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_IS_CLANGXX)
	set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra")
endif(CMAKE_CXX_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_IS_CLANGXX)
# Compiler setting ends

##### Need to Fix ######
set(IAITO_VERSION_SUFFIX "-dev")
set(IAITO_VERSION_FULL "${PROJECT_VERSION}${IAITO_VERSION_SUFFIX}")
message(STATUS "Building Iaito version ${IAITO_VERSION_FULL}")
add_definitions("-DAPP_VERSION=\"${IAITO_VERSION_FULL}\"")

set(iaito_SRCS
	${CMAKE_SOURCE_DIR}/analthread.cpp
	${CMAKE_SOURCE_DIR}/createnewdialog.cpp
	${CMAKE_SOURCE_DIR}/helpers.cpp
	${CMAKE_SOURCE_DIR}/hexascii_highlighter.cpp
	${CMAKE_SOURCE_DIR}/hexhighlighter.cpp
	${CMAKE_SOURCE_DIR}/highlighter.cpp
	${CMAKE_SOURCE_DIR}/main.cpp
	${CMAKE_SOURCE_DIR}/mainwindow.cpp
	${CMAKE_SOURCE_DIR}/mdhighlighter.cpp
	${CMAKE_SOURCE_DIR}/newfiledialog.cpp
	${CMAKE_SOURCE_DIR}/optionsdialog.cpp
	${CMAKE_SOURCE_DIR}/qrcore.cpp
	${CMAKE_SOURCE_DIR}/qrdisasm.cpp
	${CMAKE_SOURCE_DIR}/radarewebserver.cpp
)

set(iaitoWidgets_SRCS
	${CMAKE_SOURCE_DIR}/widgets/codegraphic.cpp
	${CMAKE_SOURCE_DIR}/widgets/commentswidget.cpp
	${CMAKE_SOURCE_DIR}/widgets/consolewidget.cpp
	${CMAKE_SOURCE_DIR}/widgets/dashboard.cpp
	${CMAKE_SOURCE_DIR}/widgets/flagswidget.cpp
	${CMAKE_SOURCE_DIR}/widgets/functionswidget.cpp
	${CMAKE_SOURCE_DIR}/widgets/importswidget.cpp
	${CMAKE_SOURCE_DIR}/widgets/memorywidget.cpp
	${CMAKE_SOURCE_DIR}/widgets/notepad.cpp
	${CMAKE_SOURCE_DIR}/widgets/omnibar.cpp
	${CMAKE_SOURCE_DIR}/widgets/pieview.cpp
	${CMAKE_SOURCE_DIR}/widgets/relocswidget.cpp
	${CMAKE_SOURCE_DIR}/widgets/sdbdock.cpp
	${CMAKE_SOURCE_DIR}/widgets/sectionsdock.cpp
	${CMAKE_SOURCE_DIR}/widgets/sectionswidget.cpp
	${CMAKE_SOURCE_DIR}/widgets/sidebar.cpp
	${CMAKE_SOURCE_DIR}/widgets/stringswidget.cpp
	${CMAKE_SOURCE_DIR}/widgets/symbolswidget.cpp
)

set(iaitoDialogs_SRCS
	${CMAKE_SOURCE_DIR}/dialogs/aboutdialog.cpp
	${CMAKE_SOURCE_DIR}/dialogs/commentsdialog.cpp
	${CMAKE_SOURCE_DIR}/dialogs/renamedialog.cpp
	${CMAKE_SOURCE_DIR}/dialogs/xrefsdialog.cpp
)

qt5_wrap_ui(iaito_UI
	${CMAKE_SOURCE_DIR}/createnewdialog.ui
	${CMAKE_SOURCE_DIR}/mainwindow.ui
	${CMAKE_SOURCE_DIR}/newfiledialog.ui
	${CMAKE_SOURCE_DIR}/optionsdialog.ui
	${CMAKE_SOURCE_DIR}/dialogs/aboutdialog.ui
	${CMAKE_SOURCE_DIR}/dialogs/commentsdialog.ui
	${CMAKE_SOURCE_DIR}/dialogs/renamedialog.ui
	${CMAKE_SOURCE_DIR}/dialogs/xrefsdialog.ui
	${CMAKE_SOURCE_DIR}/widgets/commentswidget.ui
	${CMAKE_SOURCE_DIR}/widgets/consolewidget.ui
	${CMAKE_SOURCE_DIR}/widgets/dashboard.ui
	${CMAKE_SOURCE_DIR}/widgets/flagswidget.ui
	${CMAKE_SOURCE_DIR}/widgets/functionswidget.ui
	${CMAKE_SOURCE_DIR}/widgets/importswidget.ui
	${CMAKE_SOURCE_DIR}/widgets/memorywidget.ui
	${CMAKE_SOURCE_DIR}/widgets/notepad.ui
	${CMAKE_SOURCE_DIR}/widgets/relocswidget.ui
	${CMAKE_SOURCE_DIR}/widgets/sdbdock.ui
	${CMAKE_SOURCE_DIR}/widgets/sectionsdock.ui
	${CMAKE_SOURCE_DIR}/widgets/sidebar.ui
	${CMAKE_SOURCE_DIR}/widgets/stringswidget.ui
	${CMAKE_SOURCE_DIR}/widgets/symbolswidget.ui
)

add_executable(iaito ${iaito_SRCS} ${iaitoDialogs_SRCS} ${iaitoWidgets_SRCS} ${iaito_UI} ${QRC_FILES})

target_link_libraries(iaito
	Qt5::WebEngine
	Qt5::WebEngineWidgets
	${RADARE2_LIBRARIES}
)

install(TARGETS iaito RUNTIME DESTINATION ${BIN_INSTALL_DIR})
