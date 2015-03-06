# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp \
    matrices.cpp \
    sbaFunctions.cpp \
    points3D.cpp \
    coef.cpp \
    widget.cpp

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../usr/local/lib/release/ -lopencv_highgui
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../usr/local/lib/debug/ -lopencv_highgui
else:unix: LIBS += -L$$PWD/../../../usr/local/lib/ -lopencv_highgui

INCLUDEPATH += $$PWD/../../../usr/local/include
DEPENDPATH += $$PWD/../../../usr/local/include

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../../usr/lib/release/ -lopencv_core
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../../usr/lib/debug/ -lopencv_core
else:symbian: LIBS += -lopencv_core
else:unix: LIBS += -L$$PWD/../../../../../usr/local/lib/ -lopencv_core

INCLUDEPATH += $$PWD/../../../../../usr/include
DEPENDPATH += $$PWD/../../../../../usr/include

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../../usr/lib/release/ -lopencv_highgui
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../../usr/lib/debug/ -lopencv_highgui
else:symbian: LIBS += -lopencv_highgui
else:unix: LIBS += -L$$PWD/../../../../../usr/local/lib/ -lopencv_highgui /home/konstantin/Qt5.1.1/5.1.1/gcc/lib/libicui18n.so.51 /home/konstantin/Qt5.1.1/5.1.1/gcc/lib/libicuuc.so.51 /home/konstantin/Qt5.1.1/5.1.1/gcc/lib/libicudata.so.51

LIBS += -lsba \
        -llapack \
        -lf2c \
        -lblas

HEADERS += \
    matrices.h \
    points3D.h \
    coef.h \
    sbaFunctions.h \
    widget.h \
    main.h



win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../usr/lib/release/ -lglut
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../usr/lib/debug/ -lglut
else:unix: LIBS += -L$$PWD/../../../usr/lib/ -lglut

INCLUDEPATH += $$PWD/../../../usr/include
DEPENDPATH += $$PWD/../../../usr/include

QT += opengl
QT += webkit webkitwidgets

RESOURCES += \
    web.qrc
