FILES = js_value.o js_primitives.o js_value_binding.o runtime.o
CXX = clang++
LD = $(CXX)
CFLAGS = \
	-std=c++17 \
	-g
LDFLAGS = ""
OUTPUT = output

.PHONEY: all

all: $(OUTPUT)

$(OUTPUT): $(FILES)
	$(LD) -o $@ $^

%.o: %.cpp
	$(CXX) $(CFLAGS) -o $@ -c $<

clean:
	@rm -rf $(FILES) $(OUTPUT)
