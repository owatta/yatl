yatl:
	mkdir -p ./bin
	cc -o ./bin/yatl ./src/yatl.c ./src/yatl.h \
	-lreadline -Wall -Wextra
