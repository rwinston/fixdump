LDFLAGS = -lquickfix
BOOSTDIR = /Users/rorywinston/sandbox/boost_1_53_0

all: fixdump fix2json

fix2json:
	g++ -I /usr/local/include -I ${BOOSTDIR} fix2json.cpp -o fix2json -lquickfix

fixdump:
	g++ -I /usr/local/include -I ${BOOSTDIR} fixdump.cpp libboost_program_options.a -o fixdump -lquickfix 

clean:
	rm -f *.o fixdump fix2json
