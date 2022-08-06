FILES = js_value.o js_primitives.o runtime.o
CXX = clang++
LD = $(CXX)
CFLAGS = \
	-std=c++17
LDFLAGS = ""
OUTPUT = output

.PHONEY: all

all: $(OUTPUT)

$(OUTPUT): $(FILES)
	$(LD) -o $@ $^

%.o: %.cpp
	$(CXX) $(CFLAGS) -o $@ -c $<

