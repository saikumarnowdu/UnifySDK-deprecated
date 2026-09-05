CXX ?= g++
ROOT := $(abspath ../../../../)
HTML := $(abspath dashboard.html)
INC := \
  -I$(ROOT)/include \
  -I$(ROOT)/applications/zpc/components/zwave/zwave_tx/include \
  -I$(ROOT)/applications/zpc/components/zwave/zwave_tx/src \
  -I$(ROOT)/components/include \
  -I$(ROOT)/components/uic_log/include \
  -I$(ROOT)/components/uic_contiki/core \
  -I$(ROOT)/components/uic_contiki/core/sys \
  -I$(ROOT)/components/uic_contiki/core/lib \
  -I$(ROOT)/components/uic_contiki/core/dev \
  -I$(ROOT)/components/uic_contiki/cpu/native \
  -I$(ROOT)/components/uic_contiki/include \
  -I$(ROOT)/components/uic_contiki/platform/posix \
  -I$(ROOT)/applications/zpc/components/zwave/zwave_definitions/include \
  -I$(ROOT)/applications/zpc/components/zwave_api/include \
  -I$(ROOT)/applications/zpc/components/zwave/zwave_controller/include \
  -I$(ROOT)/applications/zpc/components/zwave/zwave_rx/include \
  -I$(ROOT)/applications/zpc/components/zwave/zwave_network_management/include \
  -I$(ROOT)/applications/zpc/components/zwave/zwave_tx_groups/include

CXXFLAGS += -std=gnu++17 -pipe -Wall -Werror -O2 \
  -DDASHBOARD_HTML_PATH=\"$(HTML)\"

.PHONY: all clean

all: zwave_tx_obs_sim

zwave_tx_obs_sim: zwave_tx_obs_sim.cpp \
    $(ROOT)/applications/zpc/components/zwave/zwave_tx/src/zwave_tx_route_cache.cpp
	$(CXX) $(CXXFLAGS) $(INC) $^ -o $@ -lpthread

clean:
	rm -f zwave_tx_obs_sim
