
CXX 	= icpx
CXXFLAGS = -O3 -lkokkos -std=c++17 -fiopenmp -fopenmp-targets=spir64="-fno-exceptions" -D__STRICT_ANSI__
LDFLAGS = 
KOKKOS_DEVICES	= "OpenMPTarget"
KOKKOS_ARCH 	= "BDW"

#KOKKOS_DEVICES	= "OpenMP"
#KOKKOS_ARCH	= "SKX"	# skylake

SRC = $(wildcard *.cpp)

default: build
	echo "Start Build"


EXE_NAME = "ksz"
EXE = ${EXE_NAME}

LINK = ${CXX}
LINKFLAGS = -lkokkoscore -lkokkoscontainers

DEPFLAGS = -M

OBJ = $(SRC:.cpp=.o)
LIB =

build: $(EXE)

$(EXE): $(OBJ) $(KOKKOS_LINK_DEPENDS)
	$(LINK) $(KOKKOS_LDFLAGS) $(LINKFLAGS) $(EXTRA_PATH) $(OBJ) $(KOKKOS_LIBS) $(LIB) $(CXXFLAGS) -o $(EXE)


# Compilation rules

%.o:%.cpp $(KOKKOS_CPP_DEPENDS)
	$(CXX) $(KOKKOS_CPPFLAGS) $(KOKKOS_CXXFLAGS) $(CXXFLAGS) $(EXTRA_INC) -c $<

clean: 
	rm -f *.o ksz

test: $(EXE)
	./$(EXE)
