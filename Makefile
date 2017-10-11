SDL = `sdl2-config --cflags --libs`
GLAD = glad.c -ldl

clang:
	clang++ compute.cpp $(SDL) $(GLAD) -std=c++14 -O2 -o compute

gcc:
	g++ compute.cpp $(SDL) $(GLAD) -std=c++14 -O2 -o compute

clean:
	rm -f compute
