# ===================================================================================================
# Makefile for the ROOT programs - derived from $ROOTSYS/test/Makefile
# using ROOT v5.34.18
# Orig author: Fons Rademakers, 29/2/2000 - Copyright (c) 2000 Rene Brun and Fons Rademakers
# ===================================================================================================

# ---------------------------------------------------------------------------------------------------
# general ROOT flags etc.
# ---------------------------------------------------------------------------------------------------
RC     := root-config
ifeq ($(shell which $(RC) 2>&1 | sed -ne "s@.*/$(RC)@$(RC)@p"),$(RC))
MKARCH := $(wildcard $(shell $(RC) --etcdir)/Makefile.arch)
RCONFIG := $(wildcard $(shell $(RC) --incdir)/RConfigure.h)
endif
ifneq ($(MKARCH),)
include $(MKARCH)
else
ifeq ($(ROOTSYS),)
ROOTSYS = ..
endif
include $(ROOTSYS)/etc/Makefile.arch
endif
# Tutorials dir needed by stressProof
ifneq ($(RCONFIG),)
TUTDIR := $(wildcard $(shell grep ROOTDOCDIR $(RCONFIG) | sed "s|.*\"\(.*\)\"|\1|")/tutorials)
endif
ifeq ($(TUTDIR),)
ifeq ($(ROOTSYS),)
ROOTSYS = ..
endif
TUTDIR := $(ROOTSYS)/tutorials
endif
-include ../MyConfig.mk

# ---------------------------------------------------------------------------------------------------
# search path for included files and files-to-make; included ROOT libraries
# ---------------------------------------------------------------------------------------------------
CXXFLAGS += -g -I../ -I./ -I../src/ -I../include/
VPATH    += ./:../:../src:../include
LD       += $(shell $(RC) --libs) -lMinuit -lTMVA -lXMLIO -lTreePlayer

# ---------------------------------------------------------------------------------------------------
# Added -std=c++0x to enable c++11 support for e.g., stoi()
# Tested on clang (MAC) and on gcc 4.6.3 (LINUX). See: https://gcc.gnu.org/projects/cxx0x.html
# ---------------------------------------------------------------------------------------------------
CXXFLAGS += -std=c++0x

# ---------------------------------------------------------------------------------------------------
# objects, libraries and shared libraries
# ---------------------------------------------------------------------------------------------------
OptMapsO        = OptMaps.$(ObjSuf) 			OptMapsDict.$(ObjSuf)
UtilsO        	= Utils.$(ObjSuf) 				UtilsDict.$(ObjSuf)
VarMapsO        = VarMaps.$(ObjSuf) 			VarMapsDict.$(ObjSuf)
OutMngrO        = OutMngr.$(ObjSuf)       OutMngrDict.$(ObjSuf)
BaseClassO      = BaseClass.$(ObjSuf) 		BaseClassDict.$(ObjSuf)
CatFormatO      = CatFormat.$(ObjSuf) 		CatFormatDict.$(ObjSuf)
ANNZO        		= ANNZ.$(ObjSuf) 					ANNZDict.$(ObjSuf)

OptMapsS        = OptMaps.$(SrcSuf) 			OptMapsDict.$(SrcSuf)
UtilsS          = Utils.$(SrcSuf) 				UtilsDict.$(SrcSuf)
VarMapsS        = VarMaps.$(SrcSuf) 			VarMapsDict.$(SrcSuf)
OutMngrS        = OutMngr.$(SrcSuf)       OutMngrDict.$(SrcSuf)
BaseClassS      = BaseClass.$(SrcSuf) 		BaseClassDict.$(SrcSuf)
CatFormatS      = CatFormat.$(SrcSuf) 		CatFormatDict.$(SrcSuf)
ANNZS        		= ANNZ.$(SrcSuf) 					ANNZDict.$(SrcSuf)

OptMapsSO       = libOptMaps.$(DllSuf)
UtilsSO       	= libUtils.$(DllSuf)
VarMapsSO       = libVarMaps.$(DllSuf)
OutMngrSO	      = libOutMngr.$(DllSuf)
BaseClassSO			= libBaseClass.$(DllSuf)
CatFormatSO			= libCatFormat.$(DllSuf)
ANNZSO					= libANNZ.$(DllSuf)

ifeq ($(PLATFORM),win32)
	OptMapsLIB      	= libOptMaps.lib
	UtilsLIB      		= libUtils.lib
	VarMapsLIB      	= libVarMaps.lib
	OutMngrLIB        = libOutMngr.lib
	BaseClassLIB      = libBaseClass.lib
	CatFormatLIB      = libCatFormat.lib
	ANNZLIB      			= libANNZ.lib
else
	OptMapsLIB        = $(shell pwd)/$(OptMapsSO)
	UtilsLIB          = $(shell pwd)/$(UtilsSO)
	VarMapsLIB        = $(shell pwd)/$(VarMapsSO)
	OutMngrLIB      	= $(shell pwd)/$(OutMngrSO)
	BaseClassLIB 			= $(shell pwd)/$(BaseClassSO)
	CatFormatLIB 			= $(shell pwd)/$(CatFormatSO)
	ANNZLIB 					= $(shell pwd)/$(ANNZSO)
endif

MYMAINO    = myANNZ.$(ObjSuf)
MYMAINS    = myANNZ.$(SrcSuf)
myANNZ     = myANNZ$(ExeSuf)

OBJS       = $(OptMapsO) $(UtilsO) $(VarMapsO) $(OutMngrO) $(BaseClassO) $(CatFormatO) $(ANNZO)
PROGRAMS   = $(myANNZ) 

ifeq ($(ARCH),aix5)
  MAKESHARED = /usr/vacpp/bin/makeC++SharedLib
endif

all:	$(PROGRAMS)

# ===================================================================================================
# classes
# ---------------------------------------------------------------------------------------------------
# OptMaps:
# ---------------------------------------------------------------------------------------------------
$(OptMapsSO): $(OptMapsO)
ifeq ($(ARCH),aix5)
		$(MAKESHARED) $(OutPutOpt) $@ $(LIBS) -p 0 $^
else
ifeq ($(PLATFORM),macosx)
# We need to make both the .dylib and the .so
		$(LD) $(SOFLAGS)$@ $(LDFLAGS) $^ $(OutPutOpt) $@ $(EXPLLINKLIBS)
ifneq ($(subst $(MACOSX_MINOR),,1234),1234)
ifeq ($(MACOSX_MINOR),4)
		ln -sf $@ $(subst .$(DllSuf),.so,$@)
endif
endif
else
ifeq ($(PLATFORM),win32)
		bindexplib $* $^ > $*.def
		lib -nologo -MACHINE:IX86 $^ -def:$*.def \
		   $(OutPutOpt)$(OptMapsLIB)
		$(LD) $(SOFLAGS) $(LDFLAGS) $^ $*.exp $(LIBS) \
		   $(OutPutOpt)$@
		$(MT_DLL)
else
		$(LD) $(SOFLAGS) $(LDFLAGS) $^ $(OutPutOpt) $@ $(EXPLLINKLIBS)
endif
endif
endif
		@echo "$@ done"

# ---------------------------------------------------------------------------------------------------
# Utils
# ---------------------------------------------------------------------------------------------------
$(UtilsSO): $(UtilsO) $(OptMapsO)
ifeq ($(ARCH),aix5)
		$(MAKESHARED) $(OutPutOpt) $@ $(LIBS) -p 0 $^
else
ifeq ($(PLATFORM),macosx)
# We need to make both the .dylib and the .so
		$(LD) $(SOFLAGS)$@ $(LDFLAGS) $^ $(OutPutOpt) $@ $(EXPLLINKLIBS)
ifneq ($(subst $(MACOSX_MINOR),,1234),1234)
ifeq ($(MACOSX_MINOR),4)
		ln -sf $@ $(subst .$(DllSuf),.so,$@)
endif
endif
else
ifeq ($(PLATFORM),win32)
		bindexplib $* $^ > $*.def
		lib -nologo -MACHINE:IX86 $^ -def:$*.def \
		   $(OutPutOpt)$(UtilsLIB)
		$(LD) $(SOFLAGS) $(LDFLAGS) $^ $*.exp $(LIBS) \
		   $(OutPutOpt)$@
		$(MT_DLL)
else
		$(LD) $(SOFLAGS) $(LDFLAGS) $^ $(OutPutOpt) $@ $(EXPLLINKLIBS)
endif
endif
endif
		@echo "$@ done"

# ---------------------------------------------------------------------------------------------------
# VarMaps:
# ---------------------------------------------------------------------------------------------------
$(VarMapsSO): $(VarMapsO) $(OptMapsO) $(UtilsO)
ifeq ($(ARCH),aix5)
		$(MAKESHARED) $(OutPutOpt) $@ $(LIBS) -p 0 $^
else
ifeq ($(PLATFORM),macosx)
# We need to make both the .dylib and the .so
		$(LD) $(SOFLAGS)$@ $(LDFLAGS) $^ $(OutPutOpt) $@ $(EXPLLINKLIBS)
ifneq ($(subst $(MACOSX_MINOR),,1234),1234)
ifeq ($(MACOSX_MINOR),4)
		ln -sf $@ $(subst .$(DllSuf),.so,$@)
endif
endif
else
ifeq ($(PLATFORM),win32)
		bindexplib $* $^ > $*.def
		lib -nologo -MACHINE:IX86 $^ -def:$*.def \
		   $(OutPutOpt)$(VarMapsLIB)
		$(LD) $(SOFLAGS) $(LDFLAGS) $^ $*.exp $(LIBS) \
		   $(OutPutOpt)$@
		$(MT_DLL)
else
		$(LD) $(SOFLAGS) $(LDFLAGS) $^ $(OutPutOpt) $@ $(EXPLLINKLIBS)
endif
endif
endif
		@echo "$@ done"


# ---------------------------------------------------------------------------------------------------
# OutMngr
# ---------------------------------------------------------------------------------------------------
$(OutMngrSO): $(OutMngrO) $(OptMapsO) $(UtilsO) $(VarMapsO)
ifeq ($(ARCH),aix5)
		$(MAKESHARED) $(OutPutOpt) $@ $(LIBS) -p 0 $^
else
ifeq ($(PLATFORM),macosx)
# We need to make both the .dylib and the .so
		$(LD) $(SOFLAGS)$@ $(LDFLAGS) $^ $(OutPutOpt) $@ $(EXPLLINKLIBS)
ifneq ($(subst $(MACOSX_MINOR),,1234),1234)
ifeq ($(MACOSX_MINOR),4)
		ln -sf $@ $(subst .$(DllSuf),.so,$@)
endif
endif
else
ifeq ($(PLATFORM),win32)
		bindexplib $* $^ > $*.def
		lib -nologo -MACHINE:IX86 $^ -def:$*.def \
		   $(OutPutOpt)$(OutMngrLIB)
		$(LD) $(SOFLAGS) $(LDFLAGS) $^ $*.exp $(LIBS) \
		   $(OutPutOpt)$@
		$(MT_DLL)
else
		$(LD) $(SOFLAGS) $(LDFLAGS) $^ $(OutPutOpt) $@ $(EXPLLINKLIBS)
endif
endif
endif
		@echo "$@ done"

# ---------------------------------------------------------------------------------------------------
# BaseClass
# ---------------------------------------------------------------------------------------------------
$(BaseClassSO): $(BaseClassO) $(OptMapsO) $(UtilsO) $(VarMapsO) $(OutMngrO)
ifeq ($(ARCH),aix5)
		$(MAKESHARED) $(OutPutOpt) $@ $(LIBS) -p 0 $^
else
ifeq ($(PLATFORM),macosx)
# We need to make both the .dylib and the .so
		$(LD) $(SOFLAGS)$@ $(LDFLAGS) $^ $(OutPutOpt) $@ $(EXPLLINKLIBS)
ifneq ($(subst $(MACOSX_MINOR),,1234),1234)
ifeq ($(MACOSX_MINOR),4)
		ln -sf $@ $(subst .$(DllSuf),.so,$@)
endif
endif
else
ifeq ($(PLATFORM),win32)
		bindexplib $* $^ > $*.def
		lib -nologo -MACHINE:IX86 $^ -def:$*.def \
		   $(OutPutOpt)$(BaseClassLIB)
		$(LD) $(SOFLAGS) $(LDFLAGS) $^ $*.exp $(LIBS) \
		   $(OutPutOpt)$@
		$(MT_DLL)
else
		$(LD) $(SOFLAGS) $(LDFLAGS) $^ $(OutPutOpt) $@ $(EXPLLINKLIBS)
endif
endif
endif
		@echo "$@ done"

# ---------------------------------------------------------------------------------------------------
# CatFormat
# ---------------------------------------------------------------------------------------------------
$(CatFormatSO): $(CatFormatO) $(OptMapsO) $(UtilsO) $(VarMapsO) $(OutMngrO) $(BaseClassO)
ifeq ($(ARCH),aix5)
		$(MAKESHARED) $(OutPutOpt) $@ $(LIBS) -p 0 $^
else
ifeq ($(PLATFORM),macosx)
# We need to make both the .dylib and the .so
		$(LD) $(SOFLAGS)$@ $(LDFLAGS) $^ $(OutPutOpt) $@ $(EXPLLINKLIBS)
ifneq ($(subst $(MACOSX_MINOR),,1234),1234)
ifeq ($(MACOSX_MINOR),4)
		ln -sf $@ $(subst .$(DllSuf),.so,$@)
endif
endif
else
ifeq ($(PLATFORM),win32)
		bindexplib $* $^ > $*.def
		lib -nologo -MACHINE:IX86 $^ -def:$*.def \
		   $(OutPutOpt)$(CatFormatLIB)
		$(LD) $(SOFLAGS) $(LDFLAGS) $^ $*.exp $(LIBS) \
		   $(OutPutOpt)$@
		$(MT_DLL)
else
		$(LD) $(SOFLAGS) $(LDFLAGS) $^ $(OutPutOpt) $@ $(EXPLLINKLIBS)
endif
endif
endif
		@echo "$@ done"


# ---------------------------------------------------------------------------------------------------
# ANNZ
# ---------------------------------------------------------------------------------------------------
$(ANNZSO): $(ANNZO) $(OptMapsO) $(UtilsO) $(VarMapsO) $(OutMngrO) $(BaseClassO)
ifeq ($(ARCH),aix5)
		$(MAKESHARED) $(OutPutOpt) $@ $(LIBS) -p 0 $^
else
ifeq ($(PLATFORM),macosx)
# We need to make both the .dylib and the .so
		$(LD) $(SOFLAGS)$@ $(LDFLAGS) $^ $(OutPutOpt) $@ $(EXPLLINKLIBS)
ifneq ($(subst $(MACOSX_MINOR),,1234),1234)
ifeq ($(MACOSX_MINOR),4)
		ln -sf $@ $(subst .$(DllSuf),.so,$@)
endif
endif
else
ifeq ($(PLATFORM),win32)
		bindexplib $* $^ > $*.def
		lib -nologo -MACHINE:IX86 $^ -def:$*.def \
		   $(OutPutOpt)$(ANNZLIB)
		$(LD) $(SOFLAGS) $(LDFLAGS) $^ $*.exp $(LIBS) \
		   $(OutPutOpt)$@
		$(MT_DLL)
else
		$(LD) $(SOFLAGS) $(LDFLAGS) $^ $(OutPutOpt) $@ $(EXPLLINKLIBS)
endif
endif
endif
		@echo "$@ done"

# ---------------------------------------------------------------------------------------------------
# myANNZ
# ---------------------------------------------------------------------------------------------------
$(myANNZ):  $(OptMapsSO) $(UtilsSO) $(VarMapsSO) $(OutMngrSO) $(BaseClassSO) $(CatFormatSO) $(ANNZSO) $(MYMAINO)
		$(LD) $(LDFLAGS) $(MYMAINO) *.so* $(LIBS) $(OutPutOpt)$@
		$(MT_EXE)
		@echo "$@ done"
# ===================================================================================================


# ---------------------------------------------------------------------------------------------------
# recompile the objects if there is any change to the respective *.hpp *.cpp files
# ---------------------------------------------------------------------------------------------------
OptMaps.$(ObjSuf): 				OptMaps.hpp                                                 ../src/OptMaps*.cpp 	 ../include/OptMaps*.hpp
Utils.$(ObjSuf):   				OptMaps.hpp Utils.hpp 					          									../src/Utils*.cpp 		 ../include/Utils*.hpp
VarMaps.$(ObjSuf):        OptMaps.hpp Utils.hpp VarMaps.hpp                           ../src/VarMaps*.cpp    ../include/VarMaps*.hpp   CntrMap.hpp
OutMngr.$(ObjSuf):        OptMaps.hpp Utils.hpp VarMaps.hpp OutMngr.hpp               ../src/OutMngr*.cpp    ../include/OutMngr*.hpp
BaseClass.$(ObjSuf):  		OptMaps.hpp Utils.hpp VarMaps.hpp OutMngr.hpp               ../src/BaseClass*.cpp  ../include/BaseClass*.hpp
CatFormat.$(ObjSuf):  		OptMaps.hpp Utils.hpp VarMaps.hpp OutMngr.hpp BaseClass.hpp ../src/CatFormat*.cpp  ../include/CatFormat*.hpp
ANNZ.$(ObjSuf):   				OptMaps.hpp Utils.hpp VarMaps.hpp OutMngr.hpp BaseClass.hpp ../src/ANNZ*.cpp 			 ../include/ANNZ*.hpp

myANNZ.$(ObjSuf):  				../include/*.hpp ../src/*.cpp 

# ---------------------------------------------------------------------------------------------------
# dictionary generation rules
# ---------------------------------------------------------------------------------------------------
OptMapsDict.$(SrcSuf): OptMaps.hpp OptMapsLinkDef.hpp
	@echo "Generating dictionary $@..."
	$(ROOTCINT) -f $@ -c $^

UtilsDict.$(SrcSuf): Utils.hpp UtilsLinkDef.hpp
	@echo "Generating dictionary $@..."
	$(ROOTCINT) -f $@ -c $^

VarMapsDict.$(SrcSuf): VarMaps.hpp VarMapsLinkDef.hpp
	@echo "Generating dictionary $@..."
	$(ROOTCINT) -f $@ -c $^

OutMngrDict.$(SrcSuf): OutMngr.hpp OutMngrLinkDef.hpp
	@echo "Generating dictionary $@..."
	$(ROOTCINT) -f $@ -c $^

BaseClassDict.$(SrcSuf): BaseClass.hpp BaseClassLinkDef.hpp
	@echo "Generating dictionary $@..."
	$(ROOTCINT) -f $@ -c $^

CatFormatDict.$(SrcSuf): CatFormat.hpp CatFormatLinkDef.hpp
	@echo "Generating dictionary $@..."
	$(ROOTCINT) -f $@ -c $^

ANNZDict.$(SrcSuf): ANNZ.hpp ANNZLinkDef.hpp
	@echo "Generating dictionary $@..."
	$(ROOTCINT) -f $@ -c $^


# ---------------------------------------------------------------------------------------------------
# general stuff
# ---------------------------------------------------------------------------------------------------
.$(SrcSuf).$(ObjSuf):
	$(CXX)  $(CXXFLAGS) -c $<

objclean:
		@rm -f $(OBJS) $(TRACKMATHSRC) core *Dict.*
clean:      objclean
		@rm -f $(PROGRAMS) $(OptMapsSO) $(OptMapsLIB) $(UtilsSO) $(UtilsLIB) $(VarMapsSO) $(VarMapsLIB) *Dict.* *.def *.exp \
		   *.root *.ps *.o *.so *.lib *.dll *.d *.log .def so_locations \
		   files/* testdb.sqlite

.SUFFIXES: .$(SrcSuf) .$(ObjSuf) .$(DllSuf)
#.PHONY:    Aclock Hello Tetris

