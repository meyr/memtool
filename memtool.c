#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <string.h>

/*  linux kernel must 
 *  CONFIG_DEVMEM=y : create virtual device  /dev/mvm
 *  CONFIG_STRICT_DEVMEM=n : do not strict access /dev/mem
 */
#define BYTETOBINARYPATTERN "%d%d%d%d%d%d%d%d"
#define BYTETOBINARY(byte)  \
  (byte & 0x80 ? 1 : 0), \
  (byte & 0x40 ? 1 : 0), \
  (byte & 0x20 ? 1 : 0), \
  (byte & 0x10 ? 1 : 0), \
  (byte & 0x08 ? 1 : 0), \
  (byte & 0x04 ? 1 : 0), \
  (byte & 0x02 ? 1 : 0), \
  (byte & 0x01 ? 1 : 0) 
#define READ_MODE	(1 << 1)
#define WRITE_MODE	(1 << 2)
#define SHOW_MODE	(1 << 3)
#define MONITOR_MODE	(1 << 4)
#define is_read_mode (op_mode == READ_MODE)
#define is_write_mode (op_mode == WRITE_MODE)
uint8_t op_mode;
uint8_t byteno;
uint32_t target_address, target_value, page_size;
uint32_t request_mem_size, alloc_mem_size, page_mask, request_mem_cnt;
bool show_binary;


void show_help(const char *func_name)
{
	printf("usage : %s 0x<address> [<count>]\n", func_name);
}

void show_info(void)
{
	if(target_address != 0) {
		printf("target address : 0x%x\n", target_address);
		if(is_read_mode)
			printf("request size   : %d byte%s\n", request_mem_size,
					request_mem_size == 1 ? "" : "s");
	}
	if(target_value != 0)
		printf("target value : 0x%x\n", target_value);

	printf("%d bit %s mode\n", byteno * 8,
			 is_read_mode ? "read" : 
			 is_write_mode ? "write" : "");
	printf("page size : %d\n", page_size);
	printf("alloc mem size : %d\n", alloc_mem_size);
}

void parse_argument(int argc, const char *argv[])
{
	int i;
	char *p, tmp[16];

	if(argc == 1){
		show_help(argv[0]);
		exit(0);
	}

	byteno = 4;
	op_mode = READ_MODE;
	for (i = 1; i< argc; i++) {
		if(argv[i] == NULL || *argv[i] == 0)
			continue;
		if(strcmp(argv[i], "-8") == 0)
			byteno = 1;
		if(strcmp(argv[i], "-16") == 0)
			byteno = 2;
		if(strcmp(argv[i], "-32") == 0)
			byteno = 4;
		if(strcmp(argv[i], "-h") == 0){
			show_help(argv[0]);
			exit(0);
		}
		if(strcmp(argv[i], "-b") == 0)
			show_binary = true;
		if(argv[i][0] == '0' && argv[i][1] == 'x'){
			p = strchr(argv[i], '=');
			if(p == NULL){
				op_mode = READ_MODE;
				target_address = strtol(argv[i], NULL, 16); 
				if(argv[i+1] != NULL && argv[i+1][0] != '-'){
					request_mem_cnt = strtol(argv[i+1], NULL, 10);
					request_mem_size = request_mem_cnt * byteno;
				}else{
					request_mem_cnt = 1;
					request_mem_size = 1;
				}
			}else{
				op_mode = WRITE_MODE;
				target_value = strtol((char *)(p+1), NULL, 16);
				memset(tmp, 0, 16);
				strncpy(tmp, argv[i], abs(p - argv[i]));
				target_address = strtol(tmp, NULL, 16); 
			}
		}
		if(strcmp(argv[i], "-s") == 0)
			op_mode = SHOW_MODE;
		if(strcmp(argv[i], "-m") == 0)
			op_mode = MONITOR_MODE;
	}

}

void show_binary_func(void *data, char cnt)
{
	int i;
	if(show_binary){
		for(i = cnt - 1; i >= 0; i--) {
			printf(BYTETOBINARYPATTERN, BYTETOBINARY(*((uint8_t *)data + i)));
			if(i != 0)
				printf("_");
		}
	}
	printf("\n");
}

void read_func(void *virt_addr)
{
	int i;
	uint8_t *data8;
	uint16_t *data16;
	uint32_t *data32;

	for (i = 0; i < request_mem_cnt; i++) {
		switch(byteno){
		case 1: 
			data8 = (uint8_t *)virt_addr + i;
			printf("0x%x = 0x%02x\t", target_address + i * byteno, *data8);
			show_binary_func(data8, 1);
			break;
		case 2: 
			data16 = (uint16_t *)virt_addr + i;
			printf("0x%x = 0x%04x\t", target_address + i * byteno, *data16);
			show_binary_func(data16, 2);
			break;
		case 4: 
			data32 = (uint32_t *)virt_addr + i;
			printf("0x%x = 0x%08x\t", target_address + i * byteno, *data32);
			show_binary_func(data32, 4);
			break;
		default:break;
		};
	}
	
}

#define BYTEMASK(x) ((1 << (8 * x)) - 1)
void monitor_func(void *address)
{
	static uint32_t old_data;
	uint32_t new_data;
	static bool init;
	while(1){
		old_data = *((uint32_t *)address) & BYTEMASK(byteno);
		if(!init) {
			init = true;
			continue;
		}else{
			usleep(50000);
			new_data = *((uint32_t *)address) & BYTEMASK(byteno);
			if(old_data != new_data) {
				printf("data changed : 0x%0x\t", new_data);
				show_binary_func(&new_data, byteno);
			}
		}
	}
}

int main(int argc, const char *argv[])
{
	int mem_dev;
	uint8_t *mem_pointer, tmp_pointer;
	void *virt_addr;

	printf("build time: %s %s\n",__TIME__,__DATE__);
	page_size = sysconf(_SC_PAGESIZE);
	page_mask = page_size - 1;

	parse_argument(argc, argv);

	alloc_mem_size = (((request_mem_size / page_size) + 1) * page_size);

	if(op_mode == SHOW_MODE){
		show_info();
		exit(0);
	}

	mem_dev = open("/dev/mem", O_RDWR | O_SYNC);
	if(mem_dev == -1) {
		perror("open /dev/mem fail ");
		exit(0);
	}
	

	mem_pointer = mmap(NULL,
			alloc_mem_size,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			mem_dev,
			(target_address & ~page_mask));
	
	if(mem_pointer == MAP_FAILED) {
		perror("mmap fail");
		goto done;
	}
	
	virt_addr = mem_pointer + (target_address & page_mask);

	switch(op_mode){
	case READ_MODE : read_func(virt_addr); break;
	case MONITOR_MODE : monitor_func(virt_addr); break;
	case WRITE_MODE : break;
	default: show_help(argv[0]); break;
	};
	

	munmap(mem_pointer, alloc_mem_size);
done:
	close(mem_dev);
	return 0;
}
