OUT := drv
OBJ := ps2.o
CXXFLAGS := -Wall -g -std=c++11

all: $(OUT)

$(OUT): $(OBJ)
	$(CXX) -o $(OUT) $(OBJ) $(LIBS)

clean:
	rm -f $(OUT) $(OBJ)
