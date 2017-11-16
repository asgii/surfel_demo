SDL = `sdl2-config --cflags --libs`
GLAD = lib/glad/src/glad.c -ldl

LIBS = $(SDL) $(GLAD)

SRC = $(addprefix src/, main.cpp compute.cpp projection.cpp pcdReader.cpp sdl_utils.cpp)

DST = build/demo

clang:
	clang++ $(SRC) $(LIBS) -std=c++14 -O2 -o $(DST)

gcc:
	g++ $(SRC) $(LIBS) -std=c++14 -O2 -o $(DST)

clean:
	rm -f $(DST)
