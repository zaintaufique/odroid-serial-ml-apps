# MAKEFILE

CXX = g++
CFLAGS=-c -O3 -std=c++11
LDFLAGS=-lm

SRC=knn.cpp ../../controller/ApplicationMonitor_Appl.cpp  ../../controller/ApplicationMonitor_Common.cpp
OBJ=$(SRC:.cpp=.o)
EXE=knn

DEPS=$(SRC:%.cpp=%.d)

all: $(EXE)

$(EXE): $(OBJ)
	$(CXX) $(OBJ) $(LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CFLAGS) -MMD -MP $< -o $@

clean:
	rm -rf *.o *.d $(EXE)

-include $(DEPS)
