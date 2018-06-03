JOB=MyShell2
FILE=MyShell_2.c

$(JOB):$(FILE)
	gcc -g -o $@ $^

.PHONY:clean

clean:
	rm -f $(JOB)
