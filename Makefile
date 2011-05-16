## Take a look at PPMdType.h for additional compiler & environment options

#DEBUG = 1
DEBUG = 0

OBJS = pzip.o Model.o pz_interface.o

CXX = $(CC)
LINK = $(CC)

PZEXE = pzip
PZLIB = pzlib.a


#CODE_FLAGS = -fno-exceptions -fno-rtti -pedantic -Wall \
#		-Wno-unknown-pragmas -Wno-sign-compare -Wno-conversion
CODE_FLAGS = -static -Os -fno-rtti  -Wall \
                -Wno-unknown-pragmas -Wno-sign-compare -Wno-conversion
OPT_FLAGS = -static -Os -O1 -fomit-frame-pointer -fstrict-aliasing \
		-fno-schedule-insns -fno-schedule-insns2 \
		-finline-functions  -finline-limit=6000
DEBUG_FLAGS =
LD_FLAGS = -s -static -Os

# DEBUG
#       CODE_FLAGS =
#	OPT_FLAGS = 
#       DEBUG_FLAGS = -g -pg
#       LD_FLAGS = -s

CXXFLAGS = -static -Os $(CODE_FLAGS) $(OPT_FLAGS) $(DEBUG_FLAGS)

#PPMd.exe: $(OBJS)
#	$(LINK) -o PPMd.exe $(OBJS) -lstdc++ -Xlinker $(LD_FLAGS)

all: $(PZEXE)_lib

$(PZLIB): $(PZLIB)(Model.o) $(PZLIB)(pz_interface.o)
$(PZEXE)_nolib: $(OBJS) $(PZLIB)
	$(LINK) -static -Os $(DEBUG_FLAGS) -o $(PZEXE) $(OBJS) -lstdc++
$(PZEXE)_lib: $(OBJS) $(PZLIB)
	$(LINK) -static -Os $(DEBUG_FLAGS) -o $(PZEXE) pzip.o $(PZLIB) -lstdc++

Model.o: Model.cpp pz_type.hpp Model.hpp Exception.hpp Coder.hpp SubAlloc.hpp SubAlloc_impl.hpp pz_io.hpp
#	$(CXX) $(CODE_FLAGS) $(OPT_FLAGS) $(DEBUG_FLAGS) -c Model.cpp
pz_interface.o: pz_interface.cpp pz_type.hpp pz_interface.hpp Model.hpp 
#	$(CXX) $(CODE_FLAGS) $(OPT_FLAGS) $(DEBUG_FLAGS) -c PZInterface.cpp
pzip.o: pzip.cpp pz_type.hpp pz_interface.hpp pz_io.hpp 
#	$(CXX) $(CODE_FLAGS) $(OPT_FLAGS) $(DEBUG_FLAGS) -c PPMd.cpp

clean: 
	-rm $(OBJS) $(PZLIB) $(PZEXE)
