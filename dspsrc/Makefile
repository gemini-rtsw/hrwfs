# $Id: Makefile,v 1.3 1999-05-18 21:11:10 cboyer Exp $
#

APPLIC_DIR_TYPE = src

# Where to install the output files:
INSTALL_DSP = .applTop/bin/asm56000

# The following gives the location of the asm56000 utilities
# (Edit for your current location)
ASM56000_DIR = /usr/local/asm56000
ASM56K = $(ASM56000_DIR)/asm56000
DSPLNK = $(ASM56000_DIR)/dsplnk
CLDLOD = $(ASM56000_DIR)/cldlod
SREC   = $(ASM56000_DIR)/srec

# Some internal makefile stuff
INSTALL_PROD = $(PROD:%=$(INSTALL_DSP)/%)

# What to build:
PROD = tim-39.lod tim-39-full.lod tim-47.lod tim-47-full.lod util.lod vme-39.lod vme-47.lod vmerom.lod


install: $(INSTALL_PROD)

tim-39.lod: tim-39.asm timboot.asm timhead.asm
	$(ASM56K) -b -ltimboot.ls timboot.asm
	$(ASM56K) -b -ltim-39.ls -d DOWNLOAD 1 -d CCDTOOL 0 -d IMOCLK 1 tim-39.asm
	$(DSPLNK) -btim-39.cld -v timboot.cln tim-39.cln
	@rm -f $@
	$(CLDLOD) tim-39.cld >$@

tim-39-full.lod: tim-39-full.asm timboot.asm timhead.asm
	$(ASM56K) -b -ltimboot.ls timboot.asm
	$(ASM56K) -b -ltim-39-full.ls -d DOWNLOAD 1 -d CCDTOOL 0  -d IMOCLK 1 tim-39-full.asm
	$(DSPLNK) -btim-39-full.cld -v timboot.cln tim-39-full.cln
	@rm -f $@
	$(CLDLOD) tim-39-full.cld >$@

tim-47.lod: tim-47.asm timboot.asm timhead.asm
	$(ASM56K) -b -ltimboot.ls timboot.asm
	$(ASM56K) -b -ltim-47.ls -d DOWNLOAD 1 -d CCDTOOL 0  -d IMOCLK 1 tim-47.asm
	$(DSPLNK) -btim-47.cld -v timboot.cln tim-47.cln
	@rm -f $@
	$(CLDLOD) tim-47.cld >$@

tim-47-full.lod: tim-47-full.asm timboot.asm timhead.asm
	$(ASM56K) -b -ltimboot.ls timboot.asm
	$(ASM56K) -b -ltim-47-full.ls -d DOWNLOAD 1 -d CCDTOOL 0  -d IMOCLK 1 tim-47-full.asm
	$(DSPLNK) -btim-47-full.cld -v timboot.cln tim-47-full.cln
	@rm -f $@
	$(CLDLOD) tim-47-full.cld >$@

util.lod: utilappl.asm utilboot.asm utilhead.asm
	$(ASM56K) -b -lutilboot.ls -d DOWNLOAD 1 utilboot.asm
	$(ASM56K) -b -lutilappl.ls -d DOWNLOAD 1 -d CCDTOOL 0 utilappl.asm
	$(DSPLNK) -butil.cld -v utilboot.cln utilappl.cln
	@rm -f $@
	$(CLDLOD) util.cld >$@

vme-39.lod: vme-39.asm vmeboot.asm
	$(ASM56K) -b -lvmeboot.ls vmeboot.asm
	$(ASM56K) -b -lvme-39.ls -d DOWNLOAD 1 vme-39.asm
	$(DSPLNK) -bvme-39.cld -v vmeboot.cln vme-39.cln
	@rm -f $@
	$(CLDLOD) vme-39.cld >$@

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

