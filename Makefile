CXX = g++

TARGET = ./dist/program

SRC = src/main.cpp

CXXFLAGS = $(shell pkg-config --cflags ncurses opencv5)

LDLIBS = -lopencv_core \
         -lopencv_imgproc \
         -lopencv_highgui \
         -lopencv_imgcodecs \
		 -lopencv_videoio \
		 $(shell pkg-config --libs ncurses)


$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDLIBS)

run: $(TARGET)
	./dist/program

clean:
	rm -f $(TARGET)