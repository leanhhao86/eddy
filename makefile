eddy: main.o gapbuffer.o appendbuffer.o GBLL.o utils.o everything.h
	cc -g -o eddy utils.o gapbuffer.o appendbuffer.o GBLL.o main.o
main.o: eddy.c everything.h
	cc -g -o main.o -c eddy.c 
gapbuffer.o: gapbuffer.c everything.h
	cc -g -o gapbuffer.o -c gapbuffer.c 
appendbuffer.o: appendbuffer.c everything.h
	cc -g -o appendbuffer.o -c appendbuffer.c 
GBLL.o: GBLL.c everything.h
	cc -g -o GBLL.o -c GBLL.c 
utils.o: utils.c everything.h
	cc -g -o utils.o -c utils.c 
