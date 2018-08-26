all: fw utils

define RULES
fw-$(1)-$(2):
	make -C firmware BOARD=$(1) APP=$(2)
fw-$(1)-$(2)-clean:
	make -C firmware BOARD=$(1) APP=$(2) clean
endef

$(eval $(call RULES,simtrace,dfu))
$(eval $(call RULES,simtrace,trace))
$(eval $(call RULES,simtrace,cardem))
$(eval $(call RULES,qmod,dfu))
$(eval $(call RULES,qmod,cardem))

fw-clean: fw-simtrace-dfu-clean fw-simtrace-trace-clean fw-simtrace-cardem-clean fw-qmod-dfu-clean fw-qmod-cardem-clean
fw: fw-simtrace-dfu fw-simtrace-trace fw-simtrace-cardem fw-qmod-dfu fw-qmod-cardem

utils:
	make -C host

clean: fw-clean
	make -C host clean

install:
	make -C firmware install
	make -C host install
