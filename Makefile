FLAGS = -Isrc/include/SDL2 -Lsrc/lib -Wall -std=c++17 -lmingw32 -lSDL2main -lSDL2 -lm

run: clean spirograph
	./spirograph

spirograph:
	g++ spirograph.cpp ${FLAGS} -o spirograph

test:
	g++ test.cpp ${FLAGS} -o test
	./test

clean:
	del spirograph.exe


