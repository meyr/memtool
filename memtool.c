#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>

/*  linux kernel must 
 *  CONFIG_DEVMEM=y : create virtual device  /dev/mvm
 *  CONFIG_STRICT_DEVMEM=n : do not strict access /dev/mem
 */
void show_help(const char *func_name)
{
	printf("usage : %s 0x<address> [<count>]\n", func_name);
}

int main(int argc, const char *argv[])
{
	int mem_dev, request_mem_size, i;
	uint32_t page_size, alloc_mem_size, page_mask, mem_address;
	uint8_t *mem_pointer,*virt_addr;

	if(argc == 1){
		show_help(argv[0]);
		exit(0);
	}

	page_size = sysconf(_SC_PAGESIZE);
	page_mask = page_size - 1;
	//printf("page size : %d\n", page_size);

	if(argv[1] != NULL) {
		mem_address = strtol(argv[1], NULL, 16);
		//printf("access memory 0x%x\n", mem_address);
	}
	if(argv[2] != NULL)
		request_mem_size = strtol(argv[2], NULL, 10);
	else
		request_mem_size = 1;
	alloc_mem_size = (((request_mem_size / page_size) + 1) * page_size);
	//printf("alloc mem size : %d\n", alloc_mem_size);


	mem_dev = open("/dev/mem", O_RDWR | O_SYNC);
	if(mem_dev == -1) {
		perror("open /dev/mem fail ");
		exit(0);
	}else
		printf("open /dev/mem success\n");
	

	printf("mmap physical address 0x%x\n", (mem_address & ~page_mask));
	mem_pointer = mmap(NULL,
			alloc_mem_size,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			mem_dev,
			(mem_address & ~page_mask));
	
	if(mem_pointer == MAP_FAILED) {
		perror("mmap fail");
		goto done;
	}else
		printf("mmap success\n");
	
	virt_addr = mem_pointer + (mem_address & page_mask);

	for (i = 0; i < request_mem_size; i++) {
		printf("0x%x = 0x%x\n", mem_address + i, *((unsigned char *)virt_addr + i));
	}
	
	munmap(mem_pointer, alloc_mem_size);
done:
	close(mem_dev);
	return 0;
}
