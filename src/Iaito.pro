# DELETE_COMMENTS:
# No idea what this does exactly
# ballessay: this tells qmake that this project will be an application ;)
TEMPLATE = app

TARGET = iaito

# The application version
win32 {
  VERSION = 1.0
} else {
  VERSION = 1.0-dev
}

ICON = img/Enso.icns

QT       += core gui webkit webkitwidgets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QT_CONFIG -= no-pkg-config

CONFIG += c++11

# Define the preprocessor macro to get the application version in our application.
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

# why do we need to know the lib extension?
macx {
    QMAKE_CXXFLAGS = -mmacosx-version-min=10.7 -std=gnu0x -stdlib=libc++
    EXTSO=dylib
} else {
    win32 {
        EXTSO=dll
    } else {
        EXTSO=so
    }
}

INCLUDEPATH *= .

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    newfiledialog.cpp \
    optionsdialog.cpp \
    highlighter.cpp \
    qrcore.cpp \
    createnewdialog.cpp \
    hexascii_highlighter.cpp \
    webserverthread.cpp \
    widgets/pieview.cpp \
    widgets/sectionswidget.cpp \
    widgets/codegraphic.cpp \
    widgets/notepad.cpp \
    mdhighlighter.cpp \
    widgets/functionswidget.cpp \
    dialogs/renamedialog.cpp \
    dialogs/aboutdialog.cpp \
    widgets/importswidget.cpp \
    widgets/symbolswidget.cpp \
    widgets/relocswidget.cpp \
    widgets/commentswidget.cpp \
    widgets/stringswidget.cpp \
    widgets/flagswidget.cpp \
    widgets/memwidget/memorywidget.cpp \
    qrdisasm.cpp \
    widgets/sdbdock.cpp \
    analthread.cpp \
    dialogs/commentsdialog.cpp \
    widgets/sidebar.cpp \
    helpers.cpp \
    widgets/omnibar.cpp \
    widgets/dashboard.cpp \
    dialogs/xrefsdialog.cpp \
    hexhighlighter.cpp

HEADERS  += \
    mainwindow.h \
    newfiledialog.h \
    optionsdialog.h \
    highlighter.h \
    qrcore.h \
    createnewdialog.h \
    hexascii_highlighter.h \
    webserverthread.h \
    widgets/pieview.h \
    widgets/sectionswidget.h \
    widgets/codegraphic.h \
    widgets/notepad.h \
    mdhighlighter.h \
    widgets/functionswidget.h \
    dialogs/renamedialog.h \
    dialogs/aboutdialog.h \
    widgets/importswidget.h \
    widgets/symbolswidget.h \
    widgets/relocswidget.h \
    widgets/commentswidget.h \
    widgets/stringswidget.h \
    widgets/flagswidget.h \
    widgets/memwidget/memorywidget.h \
    qrdisasm.h \
    widgets/sdbdock.h \
    analthread.h \
    dialogs/commentsdialog.h \
    widgets/sidebar.h \
    helpers.h \
    widgets/omnibar.h \
    widgets/dashboard.h \
    dialogs/xrefsdialog.h \
    widgets/banned.h \
    hexhighlighter.h

FORMS    += \
    mainwindow.ui \
    newfiledialog.ui \
    optionsdialog.ui \
    createnewdialog.ui \
    widgets/notepad.ui \
    widgets/functionswidget.ui \
    dialogs/aboutdialog.ui \
    dialogs/renamedialog.ui \
    widgets/importswidget.ui \
    widgets/symbolswidget.ui \
    widgets/relocswidget.ui \
    widgets/commentswidget.ui \
    widgets/stringswidget.ui \
    widgets/flagswidget.ui \
    widgets/memwidget/memorywidget.ui \
    widgets/sdbdock.ui \
    dialogs/commentsdialog.ui \
    widgets/sidebar.ui \
    widgets/dashboard.ui \
    dialogs/xrefsdialog.ui

RESOURCES += \
    resources.qrc

include(lib_radare2.pri)
