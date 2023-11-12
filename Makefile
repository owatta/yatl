yatl:
	mkdir -p ./bin
	cc -o ./bin/yatl ./src/yatl.c ./src/yatl.h \
	-g -O1 -lreadline -Wall -Wextra
