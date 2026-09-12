IRIX_ROOT := ../irix/root

# Vendored from https://github.com/Decompollaborate/rabbitizer (pinned at
# decompals/ido-static-recomp @ d5aec59's subrepo commit
# 72bf240f468d30286888212b5fb773fae94340f6) - replaces the old libcapstone
# dependency as of the decompals rewrite (2022-10-16).
RABBITIZER := rabbitizer
RABBITIZER_LIB := $(RABBITIZER)/build/librabbitizerpp.a

# Source pinned to decompals/ido-static-recomp @ d5aec59
RECOMP_PACKAGE_VERSION := decompals/ido-static-recomp @ d5aec59

cc: OPT_CFLAGS := -O2
cfe: OPT_CFLAGS := -O2
uopt: OPT_CFLAGS := -O2
ugen: OPT_CFLAGS := -O2
as1: OPT_CFLAGS := -O2
acpp: OPT_CFLAGS := -O2

RECOMP := recomp

ugen_c.c: RECOMP_FLAGS := --conservative

all: cc cfe uopt ugen as1 acpp copt ujoin uld umerge usplit err.english.cc

clean:
	$(RM) cc* cfe* uopt* ugen* as1* acpp* copt* ujoin* uld* umerge* usplit* err.english.cc $(RECOMP) libc_impl.o version_info.o
	$(MAKE) -C $(RABBITIZER) distclean

$(RABBITIZER_LIB):
	$(MAKE) -C $(RABBITIZER) static CC=$(CC) CXX=$(CXX) DEBUG=0

$(RECOMP): recomp.cpp $(RABBITIZER_LIB)
	$(CXX) recomp.cpp -o $@ -std=c++17 -O2 -Wno-switch \
		-I $(RABBITIZER)/include -I $(RABBITIZER)/cplusplus/include \
		-L $(RABBITIZER)/build -lrabbitizerpp -ldl -Wl,-export-dynamic

libc_impl.o: libc_impl.c libc_impl.h
	$(CC) $< -c -fno-strict-aliasing -O2 -DIDO53

version_info.o: version_info.c
	$(CC) $< -c -O2 -DIDO53 \
		-DPACKAGE_VERSION='"$(RECOMP_PACKAGE_VERSION)"' -DDATETIME='"n/a"'

err.english.cc: $(IRIX_ROOT)/usr/lib/err.english.cc
	cp $^ $@

cc_c.c: $(IRIX_ROOT)/usr/bin/cc $(RECOMP)
	./$(RECOMP) $(RECOMP_FLAGS) $< > $@

%_c.c: $(IRIX_ROOT)/usr/lib/% $(RECOMP)
	./$(RECOMP) $(RECOMP_FLAGS) $< > $@

%: %_c.c libc_impl.o version_info.o
	$(CC) libc_impl.o version_info.o $< -o $@ $(OPT_CFLAGS) -fno-strict-aliasing -lm -no-pie

.PHONY: all clean
