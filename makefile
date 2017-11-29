

default: fill_test

blk_interpreter: blk_interpreter.c
	gcc -g blk_interpreter.c -o blk_interpreter

fill_test: blk_interpreter fill_test.c
	gcc -g fill_test.c -o fill_test

clean:
	rm blk_trace fill_test
