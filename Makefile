ALL = $(basename $(wildcard *.c) $(wildcard *.cc))

all: ${ALL}

%: %.c
	$(CC) -static -o $@ $<
%: %.cc
	$(CXX) -static -o $@ $<

clean:
	rm -rf ${ALL}
