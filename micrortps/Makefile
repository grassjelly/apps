-include $(TOPDIR)/.config
-include $(TOPDIR)/Make.defs
include $(APPDIR)/Make.defs

MICRO_RTPS_DIR = $(APPDIR)/micrortps/micro-RTPS-client
MICRO_RTPS_INSTALL_DIR = $(MICRO_RTPS_DIR)/build/install

ifeq ($(CONFIG_WINDOWS_NATIVE),y)
  BIN = ..\libapps$(LIBEXT)
else
ifeq ($(WINTOOL),y)
  BIN = ..\\libapps$(LIBEXT)
else
  BIN = ../libapps$(LIBEXT)
endif
endif

# Build type flag
ifeq ($(CONFIG_DEBUG_SYMBOLS),y)
	BUILD_TYPE = Debug
else
	BUILD_TYPE = Release
endif

# Verbose flag
ifeq ($(CONFIG_MICRORTPS_VERBOSE_OUTPUT),y)
	VERBOSE_OPT = ON
else
	VERBOSE_OPT = OFF
endif

all: .built
  .PHONY: clean depend distclean preconfig
  .PRECIOUS: ../../libapps$(LIBEXT)

.built: cmakecompile
	$(call DELDIR, $(MICRO_RTPS_DIR)/build/extracted)
	$(Q) mkdir $(MICRO_RTPS_DIR)/build/extracted
	$(Q) cd $(MICRO_RTPS_DIR)/build/extracted; \
		arm-none-eabi-ar -x $(MICRO_RTPS_INSTALL_DIR)/lib/libmicrortps_client.a; \
		arm-none-eabi-ar -x $(MICRO_RTPS_INSTALL_DIR)/lib/libmicrocdr.a; \
		arm-none-eabi-ar -x $(MICRO_RTPS_INSTALL_DIR)/lib/libmicrortps_transport.a
	$(Q) arm-none-eabi-ar -qc $(BIN) $(MICRO_RTPS_DIR)/build/extracted/*.o
	$(Q) touch .built

cmakecompile: $(MICRO_RTPS_DIR) $(MICRO_RTPS_INSTALL_DIR)/lib/libmicrortps_client.a

$(MICRO_RTPS_DIR):
	@echo "Cloning eProsima/Micro-RTPS-Client"	
	$(Q) git clone --recursive https://github.com/eProsima/micro-RTPS-client.git

$(MICRO_RTPS_INSTALL_DIR)/lib/libmicrortps_client.a:
	$(Q) mkdir micro-RTPS-client/build
	$(Q) cd micro-RTPS-client/build; \
		cmake -DCMAKE_TOOLCHAIN_FILE=$(MICRO_RTPS_DIR)/../toolchain.cmake \
			-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
			-DVERBOSE=$(VERBOSE_OPT) \
			-DCHECK_ENDIANNESS=OFF \
			-DTHIRDPARTY=ON \
			-DARCH_CPU_FLAGS="$(ARCHCPUFLAGS)" \
			-DARCH_OPT_FLAGS="$(ARCHOPTIMIZATION)" \
			-DCMAKE_INSTALL_PREFIX=$(MICRO_RTPS_INSTALL_DIR) ..; \
		make install

context: cmakecompile

.depend:

depend: .depend

clean:
	$(call DELDIR, $(MICRO_RTPS_DIR)/build)
	$(call DELFILE, $(MICRO_RTPS_DIR)/.built)

distclean: clean
	$(call DELDIR, $(MICRO_RTPS_DIR))

preconfig:

-include Make.dep
