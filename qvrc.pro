VERSION = 0.1

MOC_DIR = build
OBJECTS_DIR = build
UI_DIR  = build

VPATH += src

HEADERS       = glwidget.h \
		util.h \
                window.h \
		colorbutton.h \
		transfuncwidget.h \
		transfuncarea.h \
		transfunclutarea.h \
		transfuncalphaarea.h \
		presetmanager.h


SOURCES       = glwidget.cpp \
		util.cpp \ 
                main.cpp \
                window.cpp \
		transfuncwidget.cpp \
		transfuncarea.cpp \
		transfunclutarea.cpp \
		transfuncalphaarea.cpp \
		presetmanager.cpp


QT           += widgets

DISTFILES += \
AUTHORS \
COPYING \
tools/dicom2raw.py \
shaders/firstpass.vert \
shaders/firstpass.frag \
shaders/raycast.vert \
shaders/raycast.frag \
presets/mip.json \
presets/invmip.json \
presets/stent_ossa_vasi.json \
presets/stag.json \
presets/bones.json \
datasets/head256.raw 

# install
target.path = .
INSTALLS += target

CONFIG += silent console
