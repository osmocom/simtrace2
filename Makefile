all: fw utils

define RULES
fw-$(1)-$(2):
	make -C firmware BOARD=$(1) APP=$(2)
fw-$(1)-$(2)-clean:
	make -C firmware BOARD=$(1) APP=$(2) clean
endef

$(eval $(call RULES,simtrace,dfu))
$(eval $(call RULES,simtrace,blupdate))
$(eval $(call RULES,simtrace,trace))
$(eval $(call RULES,simtrace,cardem))
$(eval $(call RULES,qmod,dfu))
$(eval $(call RULES,qmod,blupdate))
$(eval $(call RULES,qmod,cardem))
$(eval $(call RULES,ngff_cardem,dfu))
$(eval $(call RULES,ngff_cardem,blupdate))
$(eval $(call RULES,ngff_cardem,trace))
$(eval $(call RULES,ngff_cardem,cardem))

fw-clean: fw-simtrace-dfu-clean fw-simtrace-blupdate-clean fw-simtrace-trace-clean fw-simtrace-cardem-clean \
	fw-qmod-dfu-clean fw-qmod-blupdate-clean fw-qmod-cardem-clean \
	fw-ngff_cardem-dfu-clean fw-ngff_cardem-blupdate-clean fw-ngff_cardem-trace-clean fw-ngff_cardem-cardem-clean
fw: fw-simtrace-dfu fw-simtrace-blupdate fw-simtrace-trace fw-simtrace-cardem \
	fw-qmod-dfu fw-qmod-blupdate fw-qmod-cardem \
	fw-ngff_cardem-dfu fw-ngff_cardem-blupdate fw-ngff_cardem-trace fw-ngff_cardem-cardem

utils:
	(cd host && \
	 autoreconf -fi && \
	 ./configure --prefix=/usr --disable-werror && \
	 make)

clean: fw-clean
	if [ -e host/Makefile ]; then \
		make -C host clean; \
	fi

install:
	make -C firmware install
	make -C host install
