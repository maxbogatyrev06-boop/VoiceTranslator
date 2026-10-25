QT       += core gui widgets
QT       += multimedia multimediawidgets
QT       += texttospeech
QT       += network
QT       += websockets
QT       += sql

CONFIG   += c++17

TARGET   = VoiceTranslator
TEMPLATE = app

SOURCES += \
    audio/audiorecorder.cpp \
    auth/logindialog.cpp \
    auth/registerdialog.cpp \
    auth/verifydialog.cpp \
    communication/communicationdialog.cpp \
    main.cpp \
    mainwindow.cpp \
    nearby/nearbydialog.cpp \
    network/networkmanager.cpp \
    network/websocketclient.cpp \
    profile/profiledialog.cpp \
    stt/sttmanager.cpp \
    translator/translator.cpp \
    tts/ttsmanager.cpp

HEADERS += \
    audio/audiorecorder.h \
    auth/logindialog.h \
    auth/registerdialog.h \
    auth/verifydialog.h \
    communication/communicationdialog.h \
    mainwindow.h \
    nearby/nearbydialog.h \
    network/networkmanager.h \
    network/websocketclient.h \
    profile/profiledialog.h \
    stt/sttmanager.h \
    translator/translator.h \
    tts/ttsmanager.h

FORMS += \
    auth/logindialog.ui \
    auth/registerdialog.ui \
    auth/verifydialog.ui \
    communication/communicationdialog.ui \
    mainwindow.ui \
    nearby/nearbydialog.ui \
    profile/profiledialog.ui