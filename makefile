memtool: memtool.o
	gcc $^ -o $@
clean:
	rm -rf *.o memtool
