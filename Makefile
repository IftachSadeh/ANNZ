# ===================================================================================================
# Makefile for the ROOT programs - derived from $ROOTSYS/test/Makefile
# using ROOT v5.34.18
# Orig author: Fons Rademakers, 29/2/2000 - Copyright (c) 2000 Rene Brun and Fons Rademakers
# ===================================================================================================

# ---------------------------------------------------------------------------------------------------
# general ROOT flags etc.
# ---------------------------------------------------------------------------------------------------
RC := root-config
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
# -include ../MyConfig.mk

# ---------------------------------------------------------------------------------------------------
# add rpath to LDFLAGS, which may be needed for precompiled root versions
# ---------------------------------------------------------------------------------------------------
CURRENT_DIR = $(shell pwd)
ROOTLIB := $(wildcard $(shell $(RC) --libdir))
LDFLAGS += -Wl,-rpath,$(ROOTLIB),-rpath,$(CURRENT_DIR)

# ---------------------------------------------------------------------------------------------------
# search path for included files and files-to-make; included ROOT libraries
# ---------------------------------------------------------------------------------------------------
CXXFLAGS += -g -I../ -I./ -I../src/ -I../include/
VPATH    += ./:../:../src:../include
MORELIBS  = -lMinuit -lTMVA -lXMLIO -lTreePlayer
LD       += $(shell $(RC) --libs) $(MORELIBS)
LIBS     += $(MORELIBS)

# ---------------------------------------------------------------------------------------------------
# Added -std=c++0x to enable c++11 support for e.g., stoi()
# Tested on clang (MAC) and on gcc 4.6.3 (LINUX). See: https://gcc.gnu.org/projects/cxx0x.html
# ---------------------------------------------------------------------------------------------------
CXXFLAGS += -std=c++0x

# # ---------------------------------------------------------------------------------------------------
# # for degudding only - cancel optimization (remove -O2 flag) to speed up compilation
# # ---------------------------------------------------------------------------------------------------
# OPT    = 
# OPT2   = 
# CXXOPT = 
# # ---------------------------------------------------------------------------------------------------

# ---------------------------------------------------------------------------------------------------
# objects and shared libraries
# ---------------------------------------------------------------------------------------------------
OptMaps_O    = OptMaps.$(ObjSuf)
OptMaps_S    = OptMaps.$(SrcSuf)
OptMaps_SO   = libOptMaps.$(DllSuf)

Utils_O      = Utils.$(ObjSuf)
Utils_S      = Utils.$(SrcSuf)
Utils_SO     = libUtils.$(DllSuf)

VarMaps_O    = VarMaps.$(ObjSuf)
VarMaps_S    = VarMaps.$(SrcSuf)
VarMaps_SO   = libVarMaps.$(DllSuf)

OutMngr_O    = OutMngr.$(ObjSuf)
OutMngr_S    = OutMngr.$(SrcSuf)
OutMngr_SO   = libOutMngr.$(DllSuf)

BaseClass_O  = BaseClass.$(ObjSuf)
BaseClass_S  = BaseClass.$(SrcSuf)
BaseClass_SO = libBaseClass.$(DllSuf)

CatFormat_O  = CatFormat.$(ObjSuf)
CatFormat_S  = CatFormat.$(SrcSuf)
CatFormat_SO = libCatFormat.$(DllSuf)

ANNZ_O       = ANNZ.$(ObjSuf)
ANNZ_S       = ANNZ.$(SrcSuf)
ANNZ_SO      = libANNZ.$(DllSuf)

myANNZ_O     = myANNZ.$(ObjSuf)
myANNZ_SO    = myANNZ.$(DllSuf)
myANNZ_E     = myANNZ

Wrapper_O    = Wrapper.$(ObjSuf)
Wrapper_S    = Wrapper.$(SrcSuf)
Wrapper_SO   = Wrapper.$(DllSuf)

# ---------------------------------------------------------------------------------------------------
# main rule
# ---------------------------------------------------------------------------------------------------
all: $(myANNZ_E) $(Wrapper_SO)

# ---------------------------------------------------------------------------------------------------
# messaging
# ---------------------------------------------------------------------------------------------------
txtred = $(shell tput setaf 1) # Red
txtgrn = $(shell tput setaf 2) # Green
txtylw = $(shell tput setaf 3) # Yellow
txtblu = $(shell tput setaf 4) # Blue
txtrst = $(shell tput sgr0)    # Reset

msg0 = "$(txtblu)---------------------- DONE:"$(txtgrn)
msg1 = "$(txtblu)---------------------- DONE:"$(txtylw)
msg2 = "$(txtblu)----------------------$(txtrst)"


# ---------------------------------------------------------------------------------------------------
# pre-compile the header file
# 	see e.g.,: http://itscompiling.eu/2017/01/12/precompiled-headers-cpp-compilation/
# ---------------------------------------------------------------------------------------------------
Common_H        = commonInclude.hpp
Common_HGC      = $(Common_H).gch
Common_HGC_FLAG = -include $(Common_H)

$(Common_HGC): $(Common_H)
	c++ $(CXXFLAGS)  -o $(Common_HGC) ../include/$(Common_H)
	c++ $(CXXFLAGS) ../include/$(Common_H)
	@echo $(msg1) $@ $(msg2)


# ---------------------------------------------------------------------------------------------------
# objects
# ---------------------------------------------------------------------------------------------------
$(OptMaps_O): OptMaps.hpp ../src/OptMaps*.cpp $(Common_HGC)
	c++ $(CXXFLAGS) $(Common_HGC_FLAG) -c -o $(OptMaps_O) ../src/OptMaps.cpp
	@echo $(msg1) $@ $(msg2)

$(Utils_O): Utils.hpp ../src/Utils*.cpp OptMaps.hpp
	c++ $(CXXFLAGS) $(Common_HGC_FLAG) -c -o $(Utils_O) ../src/Utils.cpp
	@echo $(msg1) $@ $(msg2)

$(VarMaps_O): VarMaps.hpp ../src/VarMaps*.cpp CntrMap.hpp OptMaps.hpp Utils.hpp 
	c++ $(CXXFLAGS) $(Common_HGC_FLAG) -c -o $(VarMaps_O) ../src/VarMaps.cpp
	@echo $(msg1) $@ $(msg2)

$(OutMngr_O): OutMngr.hpp ../src/OutMngr*.cpp OptMaps.hpp Utils.hpp
	c++ $(CXXFLAGS) $(Common_HGC_FLAG) -c -o $(OutMngr_O) ../src/OutMngr.cpp
	@echo $(msg1) $@ $(msg2)

$(BaseClass_O): BaseClass.hpp ../src/BaseClass*.cpp OptMaps.hpp Utils.hpp VarMaps.hpp OutMngr.hpp
	c++ $(CXXFLAGS) $(Common_HGC_FLAG) -c -o $(BaseClass_O) ../src/BaseClass.cpp
	@echo $(msg1) $@ $(msg2)

$(CatFormat_O): CatFormat.hpp ../src/CatFormat*.cpp OptMaps.hpp Utils.hpp VarMaps.hpp OutMngr.hpp BaseClass.hpp
	c++ $(CXXFLAGS) $(Common_HGC_FLAG) -c -o $(CatFormat_O) ../src/CatFormat.cpp
	@echo $(msg1) $@ $(msg2)

$(ANNZ_O): ANNZ.hpp ../src/ANNZ*.cpp OptMaps.hpp Utils.hpp VarMaps.hpp OutMngr.hpp BaseClass.hpp
	c++ $(CXXFLAGS) $(Common_HGC_FLAG) -c -o $(ANNZ_O) ../src/ANNZ.cpp
	@echo $(msg1) $@ $(msg2)

$(myANNZ_O): myANNZ.hpp ../src/myANNZ*.cpp OptMaps.hpp Utils.hpp VarMaps.hpp OutMngr.hpp BaseClass.hpp CatFormat.hpp ANNZ.hpp
	c++ $(CXXFLAGS) $(Common_HGC_FLAG) -c -o $(myANNZ_O) ../src/myANNZ.cpp
	@echo $(msg1) $@ $(msg2)

$(Wrapper_O): Wrapper.hpp ../src/Wrapper*.cpp OptMaps.hpp Utils.hpp VarMaps.hpp OutMngr.hpp BaseClass.hpp CatFormat.hpp ANNZ.hpp myANNZ.hpp
	c++ $(CXXFLAGS) $(Common_HGC_FLAG) -c -o $(Wrapper_O) ../src/Wrapper.cpp
	@echo $(msg1) $@ $(msg2)


# ---------------------------------------------------------------------------------------------------
# shared libraries
# ---------------------------------------------------------------------------------------------------
$(OptMaps_SO): $(OptMaps_O)
ifeq ($(PLATFORM),macosx)
	$(LD) $(SOFLAGS)$@ $(LDFLAGS) $^ $(OutPutOpt) $@
else
	$(LD) $(SOFLAGS)   $(LDFLAGS) $^ $(OutPutOpt) $@
endif
	@echo $(msg0) $@ $(msg2)

$(Utils_SO): $(Utils_O) $(OptMaps_SO)
ifeq ($(PLATFORM),macosx)
	$(LD) $(SOFLAGS)$@ $(LDFLAGS) $^ $(OutPutOpt) $@
else
	$(LD) $(SOFLAGS)   $(LDFLAGS) $^ $(OutPutOpt) $@
endif
	@echo $(msg0) $@ $(msg2)

$(VarMaps_SO): $(VarMaps_O) $(OptMaps_SO) $(Utils_SO)
ifeq ($(PLATFORM),macosx)
	$(LD) $(SOFLAGS)$@ $(LDFLAGS) $^ $(OutPutOpt) $@
else
	$(LD) $(SOFLAGS)   $(LDFLAGS) $^ $(OutPutOpt) $@
endif
	@echo $(msg0) $@ $(msg2)

$(OutMngr_SO): $(OutMngr_O) $(OptMaps_SO) $(Utils_SO) $(VarMaps_SO)
ifeq ($(PLATFORM),macosx)
	$(LD) $(SOFLAGS)$@ $(LDFLAGS) $^ $(OutPutOpt) $@
else
	$(LD) $(SOFLAGS)   $(LDFLAGS) $^ $(OutPutOpt) $@
endif
	@echo $(msg0) $@ $(msg2)

$(BaseClass_SO): $(BaseClass_O) $(OptMaps_SO) $(Utils_SO) $(VarMaps_SO) $(OutMngr_SO)
ifeq ($(PLATFORM),macosx)
	$(LD) $(SOFLAGS)$@ $(LDFLAGS) $^ $(OutPutOpt) $@
else
	$(LD) $(SOFLAGS)   $(LDFLAGS) $^ $(OutPutOpt) $@
endif
	@echo $(msg0) $@ $(msg2)

$(CatFormat_SO): $(CatFormat_O) $(OptMaps_SO) $(Utils_SO) $(VarMaps_SO) $(OutMngr_SO) $(BaseClass_SO)
ifeq ($(PLATFORM),macosx)
	$(LD) $(SOFLAGS)$@ $(LDFLAGS) $^ $(OutPutOpt) $@
else
	$(LD) $(SOFLAGS)   $(LDFLAGS) $^ $(OutPutOpt) $@
endif
	@echo $(msg0) $@ $(msg2)

$(ANNZ_SO): $(ANNZ_O) $(OptMaps_SO) $(Utils_SO) $(VarMaps_SO) $(OutMngr_SO) $(BaseClass_SO)
ifeq ($(PLATFORM),macosx)
	$(LD) $(SOFLAGS)$@ $(LDFLAGS) $^ $(OutPutOpt) $@
else
	$(LD) $(SOFLAGS)   $(LDFLAGS) $^ $(OutPutOpt) $@
endif
	@echo $(msg0) $@ $(msg2)

$(myANNZ_SO): $(myANNZ_O) $(OptMaps_SO) $(Utils_SO) $(VarMaps_SO) $(OutMngr_SO) $(BaseClass_SO) $(CatFormat_SO) $(ANNZ_SO)
ifeq ($(PLATFORM),macosx)
	$(LD) $(SOFLAGS)$@ $(LDFLAGS) $^ $(OutPutOpt) $@ 
else
	$(LD) $(SOFLAGS)   $(LDFLAGS) $^ $(OutPutOpt) $@
endif
	@echo $(msg0) $@ $(msg2)

# ---------------------------------------------------------------------------------------------------
# myANNZ_E
# ---------------------------------------------------------------------------------------------------
$(myANNZ_E): $(myANNZ_SO) $(OptMaps_SO) $(Utils_SO) $(VarMaps_SO) $(OutMngr_SO) $(BaseClass_SO) $(CatFormat_SO) $(ANNZ_SO)
	$(LD) $(LDFLAGS) $^ $(LIBS) $(OutPutOpt) $@ $(MT_EXE)
	@echo $(msg0) $@ $(msg2)
# ===================================================================================================

# ---------------------------------------------------------------------------------------------------
# Wrapper_SO
# ---------------------------------------------------------------------------------------------------
$(Wrapper_SO): $(Wrapper_O) $(OptMaps_SO) $(Utils_SO) $(VarMaps_SO) $(OutMngr_SO) $(BaseClass_SO) $(CatFormat_SO) $(ANNZ_SO) $(myANNZ_SO)
ifeq ($(PLATFORM),macosx)
	$(LD) $(SOFLAGS)$@ $(LDFLAGS) $^ $(OutPutOpt) $@ 
else
	$(LD) $(SOFLAGS)   $(LDFLAGS) $^ $(OutPutOpt) $@
endif
	@echo $(msg0) $@ $(msg2)
# ---------------------------------------------------------------------------------------------------
