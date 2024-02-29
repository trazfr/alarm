PROGRAM			:= alarm
UNITTEST		:= $(PROGRAM)_test

INSTALL_FOLDER	?= /opt/local/alarm
USE_FRAMEBUFFER	?= $(shell pkg-config egl --exists && echo 1)
USE_WAYLAND		?= $(shell pkg-config wayland-egl --exists && echo 1)
USE_SDL			?= $(shell pkg-config sdl2 --exists && echo 1)
USE_LIBMODPLUG	?= $(shell pkg-config libmodplug && echo 1)
USE_MPG123		?= $(shell pkg-config libmpg123 && echo 1)
USE_VORBISFILE	?= $(shell pkg-config vorbisfile && echo 1)

MODULES			:= src
INCLUDE_MODULES	:=$(addprefix -I,$(MODULES))
BUILD_BASE 		:= build
SRC_DIR			:= $(MODULES)
BUILD_DIR		:= $(addprefix $(BUILD_BASE)/,$(MODULES))

TEST_DIR		:= test
BUILD_DIR_TEST	:= $(addprefix $(BUILD_BASE)/,$(TEST_DIR))

SRC				:= $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.cpp))
OBJ				:= $(patsubst %.cpp,$(BUILD_BASE)/%.o,$(SRC))
SRC_TEST		:= $(filter-out src/main.cpp,$(SRC))
SRC_TEST		+= $(foreach sdir,$(TEST_DIR),$(wildcard $(sdir)/*.cpp))
OBJ_TEST		:= $(patsubst %.cpp,$(BUILD_BASE)/%.o,$(SRC_TEST))

ASSETS_DIR		:= assets/textures assets/music assets/shader
SVG_ASSETS		:= $(foreach sdir,$(ASSETS_DIR),$(wildcard $(sdir)/*.svg))
TTF_ASSETS		:= $(foreach sdir,$(ASSETS_DIR),$(wildcard $(sdir)/*.ttf))
SHADER_ASSETS	:= $(foreach sdir,$(ASSETS_DIR),$(wildcard $(sdir)/*.frag)) $(foreach sdir,$(ASSETS_DIR),$(wildcard $(sdir)/*.vert))
MESSAGES_ASSETS	:= $(foreach sdir,assets/messages,$(wildcard $(sdir)/*.po))
ASSETS_BUILD_DIR:= $(addprefix $(BUILD_BASE)/,$(ASSETS_DIR))
ASSETS_COMP		:= $(patsubst %.svg,$(BUILD_BASE)/%.dds,$(SVG_ASSETS)) \
				   $(patsubst %.ttf,$(BUILD_BASE)/%.dds,$(TTF_ASSETS)) \
				   $(addprefix $(BUILD_BASE)/,$(SHADER_ASSETS)) \
				   $(patsubst %.po,$(BUILD_BASE)/%/LC_MESSAGES/alarm.mo,$(MESSAGES_ASSETS))

CPPFLAGS		:= -pipe -ffunction-sections \
					-std=c++17 -Wall -Wextra -pedantic -Werror \
					$(shell pkg-config alsa --cflags) \
					$(INCLUDE_MODULES)
LDFLAGS			:= -pipe -Wl,--gc-sections \
					$(shell pkg-config alsa --libs) \
					-lstdc++fs
GCOV_CPPFLAGS	= -fprofile-arcs -ftest-coverage
GCOV_LDFLAGS	= -lgcov
LDFLAGS_TEST	:= -lgtest \
					-lpthread

# read the DEBUG environment variable DEBUG=<undef>, 0, 1, 2
D ?= $(DEBUG)
ifeq ("$(D)","2")
CPPFLAGS 		+= -g -O0 $(GCOV_CPPFLAGS) -DALARM_ASSETS_DIR='"$(PWD)/$(BUILD_BASE)/assets"'
LDFLAGS			+= -g $(GCOV_LDFLAGS)
RELEASE_MODE	= 0
else ifeq ("$(D)","1")
CPPFLAGS 		+= -g -O2 -DNDEBUG -DALARM_ASSETS_DIR='"$(PWD)/$(BUILD_BASE)/assets"'
LDFLAGS			+= -g
RELEASE_MODE	= 0
else
CPPFLAGS 		+= -O2 -DNDEBUG -DALARM_ASSETS_DIR='"$(INSTALL_FOLDER)/assets"' -DRELEASE_MODE
RELEASE_MODE	= 1
endif

# audio formats
ifeq ("$(USE_LIBMODPLUG)","1")
CPPFLAGS		+= $(shell pkg-config libmodplug --cflags)
LDFLAGS			+= $(shell pkg-config libmodplug --libs)
else
CPPFLAGS		+= -DNO_AUDIO_READ_MOD
endif
ifeq ("$(USE_MPG123)","1")
CPPFLAGS		+= $(shell pkg-config libmpg123 --cflags)
LDFLAGS			+= $(shell pkg-config libmpg123 --libs)
else
CPPFLAGS		+= -DNO_AUDIO_READ_MP3
endif
ifeq ("$(USE_VORBISFILE)","1")
CPPFLAGS		+= $(shell pkg-config vorbisfile --cflags)
LDFLAGS			+= $(shell pkg-config vorbisfile --libs)
else
CPPFLAGS		+= -DNO_AUDIO_READ_OGG
endif

# install on Raspberry PI (no choice here as there are incompatibilities with GLESv2 / EGL)
ifneq ("$(wildcard /opt/vc/include/bcm_host.h)","")

	CPPFLAGS	+= -DNO_WINDOW_WAYLAND \
					-DNO_WINDOW_SDL \
					-I/opt/vc/include
	LDFLAGS		+= -L/opt/vc/lib \
				   -lbcm_host \
				   -lbrcmEGL \
				   -lbrcmGLESv2

# others: egl, wayland or sdl2
else

CPPFLAGS		+= $(shell pkg-config glesv2 --cflags) -DNO_WINDOW_RASPBERRYPI
LDFLAGS			+= $(shell pkg-config glesv2 --libs)
ifeq ("$(USE_FRAMEBUFFER)","1")
	CPPFLAGS	+= $(shell pkg-config egl --cflags)
	LDFLAGS		+= $(shell pkg-config egl --libs)
else
	CPPFLAGS	+= -DNO_WINDOW_FRAMEBUFFER
endif
ifeq ("$(USE_WAYLAND)","1")
	CPPFLAGS	+= $(shell pkg-config wayland-egl --cflags)
	LDFLAGS		+= $(shell pkg-config wayland-egl --libs)
else
	CPPFLAGS	+= -DNO_WINDOW_WAYLAND
endif
ifeq ("$(USE_SDL)","1")
	CPPFLAGS	+= $(shell sdl2-config --cflags)
	LDFLAGS			+= $(shell sdl2-config --libs)
else
	CPPFLAGS	+= -DNO_WINDOW_SDL
endif

endif

V ?= $(VERBOSE)
ifeq ("$(V)","1")
Q :=
vecho := @true
else
Q := @
vecho := @echo
endif

.PHONY: all build_all test checkdirs clean install uninstall lcov

all: build_all
build_all: checkdirs $(PROGRAM) $(ASSETS_COMP)
test: checkdirs $(UNITTEST) $(ASSETS_COMP)
	./$(UNITTEST)
ifeq ("$(D)","2")
	$(vecho) "Generate lcov"
	$(Q) rm -rf lcov
	$(Q) mkdir -p lcov
	$(Q) lcov --directory $(BUILD_BASE) -c -o lcov/lcov_full.info
	$(Q) lcov --remove lcov/lcov_full.info -o lcov/lcov.info '/usr/*' '$(PWD)/test/*'
	$(Q) genhtml -o lcov -t "coverage" lcov/lcov.info
	xdg-open "$(PWD)/lcov/src/index.html" || true
endif

ifeq ("$(RELEASE_MODE)","1")
install: build_all
	$(vecho) "Install to $(INSTALL_FOLDER)"
	$(Q) mkdir -p $(INSTALL_FOLDER)
	$(Q) cp alarm $(INSTALL_FOLDER)
	$(Q) cp -a build/assets $(INSTALL_FOLDER)
	@echo "Install done. Please run $(INSTALL_FOLDER)/alarm config.json"

uninstall:
	$(V) "Uninstall from $(INSTALL_FOLDER)"
	$(Q) rm -rf $(INSTALL_FOLDER)
endif


$(PROGRAM): $(OBJ)
	$(vecho) "LINK $@"
	$(Q) $(CXX) -o $@ $^ $(LDFLAGS)

$(UNITTEST): $(OBJ_TEST)
	$(vecho) "LINK $@"
	$(Q) $(CXX) -o $@ $^ $(LDFLAGS) $(LDFLAGS_TEST)

checkdirs: $(BUILD_DIR) $(BUILD_DIR_TEST) $(ASSETS_BUILD_DIR)

$(BUILD_DIR):
	$(Q) mkdir -p $@
$(BUILD_DIR_TEST):
	$(Q) mkdir -p $@
$(ASSETS_BUILD_DIR):
	$(Q) mkdir -p $@

clean:
	$(Q) rm -rf $(BUILD_BASE) lcov
	$(Q) rm -f $(PROGRAM) $(UNITTEST)

vpath %.cpp $(SRC_DIR)
vpath %.cpp $(TEST_DIR)
vpath %.svg $(ASSETS_DIR)
vpath %.ttf $(ASSETS_DIR)
vpath %.frag $(ASSETS_DIR)
vpath %.vert $(ASSETS_DIR)

IM_FILTER=-format dds -background none -channel green -fx '0' -channel blue -fx '0' -define dds:compression=none -gravity northwest

$(BUILD_BASE)/assets/textures/clock.dds: assets/textures/clock.svg Makefile
	$(vecho) "Convert $<"
	$(Q) inkscape $< --export-overwrite --export-filename=$@.png
	$(Q) convert -extent 256x256 $(IM_FILTER) $@.png $@
	$(Q) rm -f $@.png

$(BUILD_BASE)/assets/textures/arrow.dds: assets/textures/arrow.svg Makefile
	$(vecho) "Convert $<"
	$(Q) inkscape $< --export-overwrite --export-filename=$@.png
	$(Q) convert -extent 64x64 $(IM_FILTER) $@.png $@
	$(Q) rm -f $@.png

$(BUILD_BASE)/assets/textures/font.dds: assets/textures/font.ttf Makefile assets/alphabet.txt
	$(vecho) "Convert $<"
	$(Q) convert -extent 128x128 -gravity northwest -background none -fill red -define dds:compression=none -font $< -pointsize 16 \
		label:"@assets/alphabet.txt" $@

$(BUILD_BASE)/assets/messages/%/LC_MESSAGES/alarm.mo: assets/messages/%.po
	$(vecho) "msgfmt $<"
	$(Q) mkdir -p $$(dirname -- $@)
	$(Q) msgfmt --output-file=$@ $<

$(BUILD_BASE)/%.frag: %.frag
	$(vecho) "CP $<"
	$(Q) cp $< $@

$(BUILD_BASE)/%.vert: %.vert
	$(vecho) "CP $<"
	$(Q) cp $< $@

define compile-objects
$1/%.o: %.cpp Makefile
	$(vecho) "CXX $$<"
	$(Q) $(CXX) $(CPPFLAGS) -c -o $$@ $$<
endef

$(foreach bdir,$(BUILD_DIR),$(eval $(call compile-objects,$(bdir))))
$(foreach bdir,$(BUILD_DIR_TEST),$(eval $(call compile-objects,$(bdir))))
