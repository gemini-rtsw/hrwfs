# $Id: Makefile,v 1.7 2003-10-27 19:53:05 cboyer Exp $
#

include .applTop
-include $(APPLIC_TOP)/config/CONFIG

APPLIC_DIR_TYPE = src

# Define cp if we don't have it yet
ifndef CP
CP = cp
endif

.applTop : ../.applTop
	$(CP) $< $@

# Where to install the output files:
INSTALL_DSP = $(APPLIC_TOP)/bin/asm56000

# The following gives the location of the asm56000 utilities
# (Edit for your current location)
ASM56000_DIR = /usr/software/dev/packages/asm56000
ASM56K = $(ASM56000_DIR)/asm56000
DSPLNK = $(ASM56000_DIR)/dsplnk
CLDLOD = $(ASM56000_DIR)/cldlod
SREC   = $(ASM56000_DIR)/srec

# Some internal makefile stuff
INSTALL_PROD = $(PROD:%=$(INSTALL_DSP)/%)

# What to build:
PROD = timrom.lod timappl0.lod timappl1.lod tim-47.lod tim-47-full.lod utilrom.lod utilpatch.lod util-47.lod vme-47.lod vmerom.lod


buildInstall: $(INSTALL_PROD)

install: $(INSTALL_PROD)

timrom.lod: timappl0.asm timappl1.asm timboot.asm timhead.asm
	$(ASM56K) -b -ltimboot.ls timboot.asm
	$(ASM56K) -b -ltimappl0.ls -d DOWNLOAD 0 -d CCDTOOL 0 -d IMOCLK 1 timappl0.asm
	$(ASM56K) -b -ltimappl1.ls -d DOWNLOAD 0 -d CCDTOOL 0 -d IMOCLK 1 timappl1.asm
	$(DSPLNK) -btim.cld -v timboot.cln timappl0.cln timappl1.cln
	@rm -f $@
	$(CLDLOD) tim.cld >$@

timromold.lod: tim-47.asm tim-47-full.asm timboot.asm timhead.asm
	$(ASM56K) -b -ltimboot.ls timboot.asm
	$(ASM56K) -b -ltim-47.ls -d DOWNLOAD 0 -d CCDTOOL 0 -d IMOCLK 1 tim-47.asm
	$(ASM56K) -b -ltim-47-full.ls -d DOWNLOAD 0 -d CCDTOOL 0 -d IMOCLK 1 tim-47-full.asm
	$(DSPLNK) -btim.cld -v timboot.cln tim-47.cln tim-47-full.cln
	@rm -f $@
	$(CLDLOD) tim.cld >$@

timappl0.lod: timappl0.asm timboot.asm timhead.asm
	$(ASM56K) -b -ltimboot.ls timboot.asm
	$(ASM56K) -b -ltimappl0.ls -d DOWNLOAD 1 -d CCDTOOL 0 -d IMOCLK 1 timappl0.asm
	$(DSPLNK) -btimappl0.cld -v timboot.cln timappl0.cln
	@rm -f $@
	$(CLDLOD) timappl0.cld >$@

timappl1.lod: timappl1.asm timboot.asm timhead.asm
	$(ASM56K) -b -ltimboot.ls timboot.asm
	$(ASM56K) -b -ltimappl1.ls -d DOWNLOAD 1 -d CCDTOOL 0 -d IMOCLK 1 timappl1.asm
	$(DSPLNK) -btimappl1.cld -v timboot.cln timappl1.cln
	@rm -f $@
	$(CLDLOD) timappl1.cld >$@

tim-47.lod: tim-47.asm timboot.asm timhead.asm
	$(ASM56K) -b -ltimboot.ls timboot.asm
	$(ASM56K) -b -ltim-47.ls -d DOWNLOAD 1 -d CCDTOOL 0 -d IMOCLK 1 tim-47.asm
	$(DSPLNK) -btim-47.cld -v timboot.cln tim-47.cln
	@rm -f $@
	$(CLDLOD) tim-47.cld >$@

tim-47-full.lod: tim-47-full.asm timboot.asm timhead.asm
	$(ASM56K) -b -ltimboot.ls timboot.asm
	$(ASM56K) -b -ltim-47-full.ls -d DOWNLOAD 1 -d CCDTOOL 0 -d IMOCLK 1 tim-47-full.asm
	$(DSPLNK) -btim-47-full.cld -v timboot.cln tim-47-full.cln
	@rm -f $@
	$(CLDLOD) tim-47-full.cld >$@

utilrom.lod: utilappl1.asm utilboot.asm utilhead.asm
	$(ASM56K) -b -lutilboot.ls utilboot.asm
	$(ASM56K) -b -lutilappl1.ls -d DOWNLOAD 0 -d CCDTOOL 0 utilappl1.asm
	$(DSPLNK) -butil.cld -v utilboot.cln utilappl1.cln
	@rm -f $@
	$(CLDLOD) util.cld >$@
	$(SREC) -mw utilrom.lod

utilpatch.lod: utilpatch.asm utilboot.asm utilhead.asm
	$(ASM56K) -b -lutilboot.ls utilboot.asm
	$(ASM56K) -b -lutilpatch.ls -d DOWNLOAD 0 -d CCDTOOL 0 utilpatch.asm
	$(DSPLNK) -butilpatch.cld -v utilboot.cln utilpatch.cln
	@rm -f $@
	$(CLDLOD) utilpatch.cld >$@

util-47.lod: util-47.asm utilboot.asm utilhead.asm
	$(ASM56K) -b -lutilboot.ls -d DOWNLOAD 1 utilboot.asm
	$(ASM56K) -b -lutil-47.ls -d DOWNLOAD 1 -d CCDTOOL 0 util-47.asm
	$(DSPLNK) -butil-47.cld -v utilboot.cln util-47.cln
	@rm -f $@
	$(CLDLOD) util-47.cld >$@

vme-47.lod: vme-47.asm vmeboot.asm
	$(ASM56K) -b -lvmeboot.ls vmeboot.asm
	$(ASM56K) -b -lvme-47.ls -d DOWNLOAD 1 vme-47.asm
	$(DSPLNK) -bvme-47.cld -v vmeboot.cln vme-47.cln
	@rm -f $@
	$(CLDLOD) vme-47.cld >$@

vmerom.lod: vmeboot.asm
	$(ASM56K) -b -lvmeboot.ls vmeboot.asm
	$(DSPLNK) -bvmeboot.cld -v vmeboot.cln
	@rm -f $@
	$(CLDLOD) vmeboot.cld >$@
	$(SREC) -bs $@

$(INSTALL_DSP)/%: %
	@echo "Installing $@"
	@test -d $(INSTALL_DSP) || mkdir $(INSTALL_DSP)
	@/usr/ucb/install -m 555 $< $(INSTALL_DSP)

clean::
	@echo "Cleaning"
	@rm -f *.lod *.cld *.ls *.cln

