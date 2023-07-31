all:
	g++ ISRC/Include -LSRC/lib -o main.cpp -lmingw32 -lSDL2main -lSDL2