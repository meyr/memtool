#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>


int main(int argc, const char *argv[])
{
	int mem_dev, request_mem_size;
	uint32_t page_size, alloc_mem_size, page_mask, mem_address;
	uint8_t *mem_pointer,*virt_addr;

	page_size = sysconf(_SC_PAGESIZE);
	page_mask = page_size - 1;
	printf("page size : %d\n", page_size);

	if(argv[1] != NULL)
		request_mem_size = strtol(argv[1], NULL, 10);
	else
		request_mem_size = 1;
	alloc_mem_size = (((request_mem_size / page_size) + 1) * page_size);
	printf("alloc mem size : %d\n", alloc_mem_size);


	mem_dev = open("/dev/mem", O_RDWR | O_SYNC);
	if(mem_dev == -1) {
		perror("open /dev/mem fail ");
		exit(0);
	}
	
	mem_address = 0x10000000;
	mem_pointer = mmap(NULL,
			alloc_mem_size,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			mem_dev,
			(mem_address & ~page_mask));

	if(mem_pointer == MAP_FAILED) {
		perror("mmap fail");
		goto done;
	}

	virt_addr = mem_pointer + (mem_address & page_mask);

	munmap(mem_pointer, alloc_mem_size);
done:
	close(mem_dev);
	return 0;
}
