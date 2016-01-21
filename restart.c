#include <stdio.h>
#include <ucontext.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <unistd.h>

ucontext_t read_context;
FILE *f;
void *stack_pointer;

char checkpoint_image[100];

void memory_restore();
void unmap_stack();
void change_stack_address(char []);
unsigned long long int convertToDecimal(char address[]);
void checkpoint_image_restore();
void set_context();

int main(int argc, char *argv[])
{
	for (int i=0; i < strlen(argv[1]); i++)
	{
		checkpoint_image[i] = argv[1][i];
	}

	stack_pointer = mmap((void *)(0x5300000),8192,PROT_WRITE|PROT_READ|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);

	if(stack_pointer == MAP_FAILED)
	{
		perror("mmap");
	}

	asm volatile ("mov %0, %%rsp" :: "g" (stack_pointer+7192) : "memory");

	memory_restore();
}

void memory_restore()
{
	unmap_stack();

	checkpoint_image_restore();

	set_context();

}

void unmap_stack()
{
	FILE *fp;
	char line[2048] = " ";

	fp = fopen("/proc/self/maps", "r");
	if(fp == NULL)
	{
		perror("maps");
	}

	while(fgets(line,2048,fp) != NULL)
	{
		if(strstr(line, "stack"))
		{
			change_stack_address(line);
			break;
		}
	}
	fclose(fp);
}

void change_stack_address(char line[])
{
	char *start_adrs, *end_adrs, *object, *l;
	char delimit[] = " -";
	unsigned long long int decimal_start_adrs = 0;
	unsigned long long int decimal_end_adrs = 0;
	l = line;

	object = strsep(&l,delimit);
	start_adrs = object;

	object = strsep(&l,delimit);
	end_adrs = object;

	decimal_start_adrs = convertToDecimal(start_adrs);

	decimal_end_adrs = convertToDecimal(end_adrs);

	unsigned long long int difference = decimal_end_adrs - decimal_start_adrs -1;

	if(munmap((void *)decimal_start_adrs, difference) == -1)
		perror("munmap");
}

unsigned long long int convertToDecimal(char address[])
{
  char c;
  unsigned long long int v = 0;
  int i =0;
  while (1) {
    c = address[i];
      if ((c >= '0') && (c <= '9')) c -= '0';
      else if ((c >= 'a') && (c <= 'f')) c -= 'a' - 10;
      else if ((c >= 'A') && (c <= 'F')) c -= 'A' - 10;
      else break;
      v = v * 16 + c;
      i++;
    }
    return v;
}

void checkpoint_image_restore()
{
	char line[2048] = "";
	FILE *fp;
	char delimit[] = " ";

	fp = fopen("checkpoint_image", "r");
	if(fp == NULL)
		perror("fopen");

	while(fgets(line,2048,fp) != NULL)
	{
		if(strstr(line,"vsyscall"))
			break;
		unsigned long long int size;
		unsigned long long int start;
		int offset = 0;
		int file = 0;

		char *object = " ", *l;

		l = line;

		object = strsep(&l,delimit);
		file = open(object, O_RDONLY);

		object = strsep(&l,delimit);
		start = convertToDecimal(object);

		object = strsep(&l,delimit);

		object = strsep(&l,delimit);

		object = strsep(&l,delimit);
		size = atoi(object);

		//snprintf(size,"%s",object);
		//mprotect(,size,PROT_WRITE);

		void *map_address = mmap((void *)start,size,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_FIXED,file,offset);
		if(map_address == MAP_FAILED)
			perror("mmap");

		close(file);
	}
	fclose(fp);
}

void set_context()
{
	int *after_restart;
	FILE *global;
	global = fopen("global", "r");
	fread(&after_restart,1, sizeof(int *),global);
	*after_restart = 1;
	fclose(global);
	
	f = fopen("context_image", "r");
	fread(&read_context,1, sizeof(ucontext_t),f);
	fclose(f);
	setcontext(&read_context);
}
