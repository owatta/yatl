yatl:
	mkdir -p ./bin
	cc -o ./bin/yatl ./src/yatl.c ./src/yatl.h \
	-g -O0 -lreadline -Wall -Wextra
