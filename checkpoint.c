#include<signal.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<assert.h>
#include<ucontext.h>
#include<fcntl.h>
#include<math.h>

/* character array to store
pid in /poc/$$pid/maps which 
is used  stored in a variable
to access the memory maps
also used for /proc/$pid/mem*/

char file_name[100];
char file_name_1[100];

/* to store context of this 
program*/
ucontext_t context;
int after_restart = 0;

/* function declaration */
void signal_handler(int signum);
void check_point();
void readchar(char*);
void readAddress(char[],char[],char[],char[]);
int nDigits(unsigned long long int i);
unsigned long long int convertToDecimal(char address[]);
void get_context();

/* a way to define constructor 
which is initialized when the user process 
starts without initializng in the user process*/
__attribute__((constructor))
void myconstructor()
{
  signal(SIGUSR2, signal_handler);
}

/* signal handler which is called 
when the process recieves SIGUSR2
and function check_point 
is called */
void signal_handler(int signum)
{
  if(signum != 12)
  {
    kill(getpid(),SIGINT);
  }
  check_point();
  get_context();
}

/*This function reads one line at 
time from file /proc/pid/maps and
calls function readchar to parse 
each line.
*/
void check_point()
{
  FILE *fp;
  char line[2048] = "";
  sprintf(file_name, "/proc/%d/maps", getpid());
  fp = fopen(file_name, "r");
  if(fp==NULL){
    perror("maps");
  }
  char c [10];
  while(fgets(line,2048,fp) != NULL)
    {
      readchar(line);
    }
  fclose(fp);

  FILE *global;
  int *p = &after_restart;
  global = fopen("global", "wb");
  fwrite(&p,1, sizeof(int *), global);
  fclose(global);
}

void get_context()
{
  getcontext(&context);
  if(after_restart == 0)
  {
    FILE *ctx;
    ctx = fopen("context_image", "wb");
    fwrite(&context, 1, sizeof(ucontext_t), ctx);
    fclose(ctx);
  }
}

/* This function is used to parse
the line and read the start address
end address permission and name of 
the seciton. These arrays are passed
to function readAddress.
*/
void readchar(char line[])
{
  int i =0, j=0;
  int s=0,e=0,r=0;
  char c;
  char start_address[30] ="";
  char end_address[30] = "";
  char permission[5] = "";
  char section [50] = "";
  while(line[i] != '\n')
    {
      if(line[i] != '-' && !s)
      {
        while(line[i] != '-')
        {
          c = line[i];
          //printf("%c", c);
          start_address[i] = c;
          i++;
        }
      s = 1;
      start_address[i] = '\0';
      i++;
      }
      if(line[i] != ' ' && !e)
      {
      while(line[i] != ' ')
      {
        c = line[i];
        //printf("%c", c);
        end_address[j] = c;
        j++;
        i++;
      }
      e = 1;
      i++;
      end_address[j] = '\0';
    }
      if((line[i] == 'r' || line[i] == '-') 
        && (line[i+1] == 'w' || line[i+1] == 'x' || line[i+1] == '-') && !r)
      {
        permission[0] = line[i];
        permission[1] = line[i+1];
        permission[2] = line[i+2];
        permission[3] = line[i+3];
        permission[4] = '\0';
        i = i+4;
      }
      r = 1;

      if(line[i] == '/')
      {
        int j =0;
        while(line[i] != '\n')
        {
          c = line[i];
          //printf("%c", c);
          if (c == '/')
            c = '_';
          section[j] = c;
          i++;
          j++;
        }
        section[j] = '\0';
        break;
      }
      if(line[i] == '[')
      {
        i++;
        int j =0;
        while(line[i] != ']')
        {
          c = line[i];
          //printf("%c", c);
          section[j] = c;
          i++;
          j++;
        }
        section[j] = '\0';
      }
      i++;
    }
    //if(strcmp(section,"vsyscall"))
      readAddress(start_address, end_address, permission, section);
}


/* this function is used to convert start
and end address into decimal. this decimal value
of start address is used as a starting offset in 
file /proc/pid/mem and decimal value of end address 
is used as number of bytes to be read from the file.
This is also used to get the context and save it to 
a file.
*/
void readAddress(char start_address[],char end_address[],
		 char permission[], char section[])
{


  int len_of_start_address = strlen(start_address);
  int len_of_end_address = strlen(end_address);
  int len_permission = strlen(permission);
  int len_section = strlen(section);

  unsigned long long int decimal_value_start_address = 0;
  decimal_value_start_address = convertToDecimal(start_address);

  unsigned long long int decimal_value_end_address = 0;
  decimal_value_end_address = convertToDecimal(end_address); 
 
 int f;
 sprintf(file_name_1, "/proc/%d/mem", getpid());
 f = open(file_name_1, O_RDONLY);
 
 unsigned long long int difference = decimal_value_end_address - decimal_value_start_address;

char s_char[100];
int d = nDigits(difference);
snprintf(s_char,d+1,"%llu",difference);

 if(lseek(f, decimal_value_start_address, SEEK_SET)== -1)
  perror("lseek failed");

char something[100];

sprintf(something, "file_%s", section);

int section_file;
if(access(something,F_OK) != -1)
{
  
  sprintf(something, "file_%s_2", section);
  section_file = open(something, O_CREAT| O_WRONLY | O_APPEND,0777);
}
else{
section_file = open(something, O_CREAT| O_WRONLY | O_APPEND,0777);
}

int check_point_image_file;
check_point_image_file = open("checkpoint_image", O_CREAT| O_WRONLY | O_APPEND,0777);

write(check_point_image_file,something,strlen(something));
write(check_point_image_file," ",1);
write(check_point_image_file,start_address,len_of_start_address);
write(check_point_image_file," ",1);
write(check_point_image_file,end_address,len_of_end_address);
write(check_point_image_file," ",1);
write(check_point_image_file,permission,len_permission);
write(check_point_image_file," ",1);
write(check_point_image_file,s_char,d);
write(check_point_image_file," ",1);
if(strcmp(section,"vsyscall"))
{
  write(check_point_image_file,"\n",1);
}
else
{
  write(check_point_image_file,"\0",1);
}

void *buf[difference];

if(read(f, buf, difference) == -1)
  perror("read failed");

close(f);

if(write(section_file,buf,difference) == -1)
  perror("write");

close(check_point_image_file);
close(section_file);

  }

int nDigits(unsigned long long int i) {
    int n = 1;
    while (i > 9) {
        n++;
        i /= 10;
    }
    return n;
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
