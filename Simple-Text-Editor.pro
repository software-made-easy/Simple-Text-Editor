QT       += core gui printsupport svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
greaterThan(QT_MAJOR_VERSION, 5): QT += core5compat

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x051208    # disables all the APIs deprecated before Qt 5.12.8

SOURCES += \
    main.cpp \
    textedit.cpp \
    about.cpp

HEADERS += \
    textedit.h \
    about.h

RESOURCES += \
    textedit.qrc

TRANSLATIONS += \
    STE_de.ts

CONFIG += lrelease
CONFIG += embed_translations

VERSION = 0.1.0
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle.properties \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/values/libs.xml

ANDROID_PACKAGE_SOURCE_DIR = \
        $$PWD/android
