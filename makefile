

default: fill_test

blk_interpreter:
	gcc -g blk_interpreter.c -o blk_interpreter

fill_test: blk_interpreter
	gcc -g fill_test.c -o fill_test

clean:
	rm blk_trace fill_test
