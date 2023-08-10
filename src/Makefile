# Makefile for vips plugin 

# Object files we build
OBJS = fltload7.o fltload.o module.o

# Source files
SRC = $(BASEOBJS:.o=.cpp) 

# Headers
HEADERS = fltload.h

# All
ALL = $(SRC) $(HEADERS)

#linux
CFLAGS += -shared -fPIC 
CPPFLAGS += `pkg-config vips --cflags`
LDFLAGS += `pkg-config vips --libs`
OUT = vips-fltload.so

release: $(OUT)
debug: $(OUT)

.PHONY: debug  
debug: CFLAGS += -g -Wall

.PHONY: release  
release: CFLAGS += -O3

$(OUT): $(OBJS)
#linux ... must link C++ plugins with the C++ compiler
	$(CC) -o $(OUT) -shared $(OBJS)

clean: 
	$(RM) $(OBJS) 

tags: $(ALL)
	ctags $(ALL)

# version as MAJOR.MINOR
VIPS_VERSION = $(shell pkg-config vips --modversion | \
	         awk '{split($$1,a,"."); print a[1]"."a[2]}')
PLUGIN_DIR = $(VIPSHOME)/lib/vips-modules-$(VIPS_VERSION)

install: $(OUT)
	-mkdir -p $(PLUGIN_DIR)
	-cp $(OUT) $(PLUGIN_DIR)

