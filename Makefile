SDL = `sdl2-config --cflags --libs`
GLAD = glad.c -ldl

SRC = compute.cpp projection.cpp pcdReader.cpp sdl_utils.cpp

clang:
	clang++ $(SRC) $(SDL) $(GLAD) -std=c++14 -O2 -o compute

gcc:
	g++ $(SRC) $(SDL) $(GLAD) -std=c++14 -O2 -o compute

clean:
	rm -f compute
