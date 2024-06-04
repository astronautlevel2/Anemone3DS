#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
# GRAPHICS is a list of directories containing graphics files
# GFXBUILD is the directory where converted graphics files will be placed
#   If set to $(BUILD), it will statically link in the converted
#   files as if they were data files.
#
# NO_SMDH: if set to anything, no SMDH file is generated.
# ROMFS is the directory which contains the RomFS, relative to the Makefile (Optional)
# APP_TITLE is the name of the app stored in the SMDH file (Optional)
# APP_DESCRIPTION is the description of the app stored in the SMDH file (Optional)
# APP_AUTHOR is the author of the app stored in the SMDH file (Optional)
# ICON is the filename of the icon (.png), relative to the project folder.
#   If not set, it attempts to use one of the following (in this order):
#     - <Project name>.png
#     - icon.png
#     - <libctru folder>/default_icon.png
#---------------------------------------------------------------------------------

# Your values.
APP_TITLE           :=	Anemone3DS
APP_DESCRIPTION     :=	A complete theming tool for the 3DS
APP_AUTHOR          :=	Anemone3DS Team


TARGET              :=	$(subst $e ,_,$(notdir $(APP_TITLE)))
OUTDIR              :=	out
BUILD               :=	build
SOURCES             :=	source source/quirc
INCLUDES            :=	include
ROMFS               :=	romfs
GRAPHICS            :=	assets
GFXBUILD            :=	$(ROMFS)/gfx


# Path to the files
# If left blank, will try to use "icon.png", "$(TARGET).png", or the default ctrulib icon, in that order
ICON                :=	meta/icon.png

BANNER_AUDIO        :=	meta/audio.wav
BANNER_IMAGE        :=	meta/banner.png

RSF_PATH            :=	meta/app.rsf

# If left blank, makerom will use the default Homebrew logo
LOGO                :=	meta/logo.bin


# If left blank, makerom will use default values (0xff3ff and CTR-P-CTAP, respectively)
# Be careful if UNIQUE_ID is the same as other apps: it will overwrite the previously installed one
UNIQUE_ID           :=	0xBAFE0
PRODUCT_CODE        :=	CTR-P-ANEM

# Don't really need to change this
ICON_FLAGS          :=	nosavebackups,visible

ifeq ($(strip $(NOGIT)),)
    VERSION           :=  $(shell git describe --tags --match v[0-9]* --abbrev=7 | sed 's/-[0-9]*-g/-/')
    VERSION_MAJOR     :=  $(shell echo $(VERSION) | cut -c2- | cut -f1 -d- | cut -f1 -d.)
    VERSION_MINOR     :=  $(shell echo $(VERSION) | cut -c2- | cut -f1 -d- | cut -f2 -d.)
    VERSION_BUILD     :=  $(shell echo $(VERSION) | cut -c2- | cut -f1 -d- | cut -f3 -d.)

    ifeq ($(strip $(VERSION_BUILD)),)
        VERSION_BUILD := 0
    endif
endif

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS	:=	-g -Wall -Wextra -O2 -mword-relocations \
			-ffunction-sections \
			$(ARCH)

CFLAGS	+=	$(INCLUDE) -D__3DS__ -D_GNU_SOURCE -DVERSION="\"$(VERSION)\"" -DUSER_AGENT="\"$(APP_TITLE)/$(VERSION)\"" -DAPP_TITLE="\"$(APP_TITLE)\""
CFLAGS	+=	`arm-none-eabi-pkg-config --cflags-only-other libcurl vorbisidec libarchive jansson libpng`
ifneq ($(strip $(CITRA_MODE)),)
	CFLAGS += -DCITRA_MODE
endif

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:= `arm-none-eabi-pkg-config --libs libcurl vorbisidec libarchive jansson libpng` -lcitro2d -lcitro3d -lctru -lm

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(CTRULIB) $(PORTLIBS)


#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(OUTDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PICAFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.v.pica)))
SHLISTFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.shlist)))
GFXFILES	:=	$(foreach dir,$(GRAPHICS),$(notdir $(wildcard $(dir)/*.t3s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

#---------------------------------------------------------------------------------
ifeq ($(GFXBUILD),$(BUILD))
#---------------------------------------------------------------------------------
export T3XFILES :=  $(GFXFILES:.t3s=.t3x)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
export ROMFS_T3XFILES	:=	$(patsubst %.t3s, $(GFXBUILD)/%.t3x, $(GFXFILES))
export T3XHFILES		:=	$(patsubst %.t3s, $(BUILD)/%.h, $(GFXFILES))
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES_SOURCES 	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES)) \
			$(PICAFILES:.v.pica=.shbin.o) $(SHLISTFILES:.shlist=.shbin.o) \
			$(addsuffix .o,$(T3XFILES))

export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)

export HFILES	:=	$(PICAFILES:.v.pica=_shbin.h) $(SHLISTFILES:.shlist=_shbin.h) \
			$(addsuffix .h,$(subst .,_,$(BINFILES))) \
			$(GFXFILES:.t3s=.h)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export _3DSXDEPS	:=	$(if $(NO_SMDH),,$(OUTPUT).smdh)

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.png)
	ifneq (,$(findstring $(TARGET).png,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).png
	else
		ifneq (,$(findstring icon.png,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.png
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_SMDH)),)
	export _3DSXFLAGS += --smdh=$(OUTPUT).smdh
endif

ifneq ($(ROMFS),)
	export _3DSXFLAGS += --romfs=$(CURDIR)/$(ROMFS)
endif

.PHONY: all clean

#---------------------------------------------------------------------------------
MAKEROM		?=	makerom

MAKEROM_ARGS		:=	-elf "$(OUTPUT).elf" -rsf "$(RSF_PATH)" -banner "$(BUILD)/banner.bnr" -icon "$(BUILD)/icon.icn" -DAPP_TITLE="$(APP_TITLE)" -DAPP_PRODUCT_CODE="$(PRODUCT_CODE)" -DAPP_UNIQUE_ID="$(UNIQUE_ID)"

ifeq ($(strip $(NOGIT)),)
    MAKEROM_ARGS    +=  -major $(VERSION_MAJOR) -minor $(VERSION_MINOR) -micro $(VERSION_BUILD)
endif

ifneq ($(strip $(LOGO)),)
	MAKEROM_ARGS	+=	 -logo "$(LOGO)"
endif
ifneq ($(strip $(ROMFS)),)
	MAKEROM_ARGS	+=	 -DAPP_ROMFS="$(ROMFS)"
endif

BANNERTOOL	?=	bannertool

ifeq ($(suffix $(BANNER_IMAGE)),.cgfx)
	BANNER_IMAGE_ARG := -ci
else
	BANNER_IMAGE_ARG := -i
endif

ifeq ($(suffix $(BANNER_AUDIO)),.cwav)
	BANNER_AUDIO_ARG := -ca
else
	BANNER_AUDIO_ARG := -a
endif

#---------------------------------------------------------------------------------
all: $(BUILD) $(GFXBUILD) $(DEPSDIR) $(ROMFS_T3XFILES) $(T3XHFILES) $(OUTDIR)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
	@$(BANNERTOOL) makebanner $(BANNER_IMAGE_ARG) "$(BANNER_IMAGE)" $(BANNER_AUDIO_ARG) "$(BANNER_AUDIO)" -o "$(BUILD)/banner.bnr"
	@$(BANNERTOOL) makesmdh -s "$(APP_TITLE)" -l "$(APP_DESCRIPTION)" -p "$(APP_AUTHOR)" -i "$(APP_ICON)" -f "$(ICON_FLAGS)" -o "$(BUILD)/icon.icn"
	$(MAKEROM) -f cia -o "$(OUTPUT).cia" -target t -exefslogo $(MAKEROM_ARGS)

$(BUILD):
	@mkdir -p $@

ifneq ($(GFXBUILD),$(BUILD))
$(GFXBUILD):
	@mkdir -p $@
endif

ifneq ($(DEPSDIR),$(BUILD))
$(DEPSDIR):
	@mkdir -p $@
endif

$(OUTDIR):
	@mkdir -p $@

#---------------------------------------------------------------------------------
clean:
	$(SILENTMSG) clean ...
	@rm -fr $(BUILD) $(GFXBUILD) $(DEPSDIR) $(OUTDIR)

#---------------------------------------------------------------------------------
$(GFXBUILD)/%.t3x	$(BUILD)/%.h	:	%.t3s
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@tex3ds -i $< -H $(BUILD)/$*.h -d $(DEPSDIR)/$*.d -o $(GFXBUILD)/$*.t3x

#---------------------------------------------------------------------------------
else

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).3dsx	:	$(OUTPUT).elf $(_3DSXDEPS)

$(OFILES_SOURCES) : $(HFILES)

$(OUTPUT).elf	:	$(OFILES)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
.PRECIOUS	:	%.t3x %.shbin
#---------------------------------------------------------------------------------
%.t3x.o	%_t3x.h :	%.t3x
#---------------------------------------------------------------------------------
	$(SILENTMSG) $(notdir $<)
	$(bin2o)

#---------------------------------------------------------------------------------
%.shbin.o %_shbin.h : %.shbin
#---------------------------------------------------------------------------------
	$(SILENTMSG) $(notdir $<)
	$(bin2o)

-include $(DEPSDIR)/*.d

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
