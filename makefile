

default: fill_test git_test

blk_interpreter: blk_interpreter.c
	gcc -g blk_interpreter.c -o blk_interpreter

fill_test: blk_interpreter fill_test.c
	gcc -g fill_test.c -o fill_test

git_test: git_test.c
	gcc -g git_test.c -o git_test

clean:
	rm blk_trace fill_test git_test
