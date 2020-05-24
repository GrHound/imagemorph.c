FLAGS =  -pedantic -Wunused -Wunused-function -Wunused-parameter -Wunused-variable -Wall 
#        -pg
imagemorph:	imagemorph.o Makefile
	gcc $(FLAGS) imagemorph.c -lm  -o imagemorph
	
clean:
	\rm imagemorph imagemorph.o

