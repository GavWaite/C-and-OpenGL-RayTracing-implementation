CC=g++
LIBS= -lGLU -lGL -lglut
# For building on macOS use the LIBS bellow
#LIBS= -framework OpenGL -framework GLUT -framework CoreVideo -framework IOKit -framework Cocoa -lglfw3 -lGLEW -L/usr/local/lib -L /usr/pkg/lib

all:
	g++ -std=c++11 -o demo2 *.cpp $(LIBS)

run: all
	./demo2

clean: 
	rm -f demo2 *.o *~ core
