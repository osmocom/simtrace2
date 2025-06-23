osmocom/simtrace2/version.h: osmocom/simtrace2/version.h.tpl
	$(AM_V_GEN)$(MKDIR_P) $(dir $@)
	$(AM_V_GEN)sed \
		-e "s/{{VERSION}}/$$(echo '@VERSION@' | cut -d. -f1-3)/g" \
		-e "s/{{VERSION_MAJOR}}/$$(echo '@VERSION@' | cut -d. -f1)/g" \
		-e "s/{{VERSION_MINOR}}/$$(echo '@VERSION@' | cut -d. -f2)/g" \
		-e "s/{{VERSION_PATCH}}/$$(echo '@VERSION@' | cut -d. -f3)/g" \
		$< > $@

nobase_include_HEADERS = \
		osmocom/simtrace2/apdu_dispatch.h \
		osmocom/simtrace2/simtrace2_api.h \
		osmocom/simtrace2/simtrace_usb.h \
		osmocom/simtrace2/simtrace_prot.h \
		osmocom/simtrace2/usb_util.h \
		osmocom/simtrace2/gsmtap.h \
		osmocom/simtrace2/version.h \
		$(NULL)
