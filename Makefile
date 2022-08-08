FILES = js_value.o js_primitives.o js_value_binding.o
CXX = clang++
LD = $(CXX)
AR = llvm-ar
CFLAGS = \
	-std=c++17 \
	-g
LDFLAGS = ""
OUTPUT = runtime.a
TESTPROG = testprog.cpp

.PHONEY: all

all: $(OUTPUT)

testprog: $(OUTPUT)
	$(CXX) $(CFLAGS) -o $(basename $(TESTPROG)) $(TESTPROG) $(OUTPUT)


$(OUTPUT): $(FILES)
	$(AR) rc $@ $^

%.o: %.cpp
	$(CXX) $(CFLAGS) -o $@ -c $<

clean:
	@rm -rf $(FILES) $(OUTPUT) $(basename $(TESTPROG))
