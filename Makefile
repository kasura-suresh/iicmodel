PROJECT := i2c
SRCS    := $(wildcard *.cpp)
OBJS    := $(SRCS:.cpp=.o)
CFLAGS  := -g -Wall

all: $(PROJECT)

$(PROJECT): $(OBJS)
	g++ -o $@ $^ -L$(SYSTEMC)/lib-macosx64 -lsystemc

%.o: %.cpp
	g++ $(CFLAGS) -c -o $@ -I$(SYSTEMC)/include $<

clean:
	rm -f $(OBJS) $(PROJECT)
