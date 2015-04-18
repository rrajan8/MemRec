all:
	g++ -std=c++11 -c -o rvm.o rvm.cpp
	ar rc rvm.a rvm.o
	ranlib rvm.a
	
clean:
	rm *.o *.a
