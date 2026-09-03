include .applTop
-include $(APPLIC_TOP)/config/CONFIG

APPLIC_DIR_TYPE = db

-include $(APPLIC_TOP)/config/RULES.Dirs

# Define cp if we don't have it yet
ifndef CP
CP = cp
endif

.applTop : ../.applTop
	$(CP) $< $@

clean ::
	$(RM) .applTop
	$(RM) ../dbd/gemini$(APPLIC_DBNAME).dbd

