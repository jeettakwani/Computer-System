#define  _GNU_SOURCE
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/fcntl.h>


const int long_size = sizeof(long);

long freespaceaddr(pid_t pid)
{
    FILE *fp;
    char filename[30];
    char line[85];
    long addr;
    char str[20];
    sprintf(filename, "/proc/%d/maps", pid);
    fp = fopen(filename, "r");
    if(fp == NULL)
        exit(1);
    while(fgets(line, 85, fp) != NULL) {
        sscanf(line, "%lx-%*lx %*s %*s %s", &addr,
               str, str, str, str);
        if(strcmp(str, "00:00") == 0)
            break;
    }
    fclose(fp);
    return addr;
}

int main(int argc, char *argv[])
{
    struct user_regs_struct oldregs, regs;
    unsigned long  pid, addr, save[42500];
    siginfo_t      info;
    char           dummy;
    extern char    etext;

    if (sscanf(argv[1], " %lu %c", &pid, &dummy) != 1 || pid < 1UL) {
        fprintf(stderr, "%s: Invalid process ID.\n", argv[1]);
        return 1;
    }

    if (ptrace(PTRACE_ATTACH, (pid_t)pid, NULL, NULL)) {
        fprintf(stderr, "Cannot attach to process %lu: %s.\n", pid, strerror(errno));
        return 1;
    }

    waitid(P_PID, (pid_t)pid, &info, WSTOPPED);

    if (ptrace(PTRACE_GETREGS, (pid_t)pid, NULL, &oldregs)) {
        fprintf(stderr, "Cannot get register state from process %lu: %s.\n", pid, strerror(errno));
        ptrace(PTRACE_DETACH, (pid_t)pid, NULL, NULL);
        return 1;
    }


    struct stat file_size;
    if(stat("execute", &file_size) == -1)
    {
        printf("Error with file size");
    }
    
    char bytes[file_size.st_size];

    addr = freespaceaddr((pid_t)pid);
   
    int fd =0;
    fd = open("execute", O_RDONLY);
    /*void *map_address = mmap((void *)addr,file_size.st_size,
        PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_FIXED,fd,0);
    
    if(map_address == MAP_FAILED)
        perror("Map");*/
    //close(fd);
    
    
    if (ptrace(PTRACE_POKETEXT, (pid_t)pid, (void *)(addr + 0UL), (void *)0x2c6f6c6c6548050fULL) ||
        ptrace(PTRACE_POKETEXT, (pid_t)pid, (void *)(addr + 8UL), (void *)0x0a21646c726f7720ULL)) {
        fprintf(stderr, "Cannot modify process %lu code: %s.\n", pid, strerror(errno));
        ptrace(PTRACE_DETACH, (pid_t)pid, NULL, NULL);
        return 1;
    }

    /* Modify process registers, to execute the just inserted code. */
    regs = oldregs;
    regs.rip = addr;
    regs.rax = __NR_mmap;
    regs.rbx = (void *)0x50000000;
    regs.rcx = file_size.st_size;
    regs.rdx = PROT_READ|PROT_WRITE|PROT_EXEC;
    regs.rsi = MAP_PRIVATE|MAP_FIXED;
    regs.rdi = fd;
    regs.r10 = 0;
    if (ptrace(PTRACE_SETREGS, (pid_t)pid, NULL, &regs)) {
        fprintf(stderr, "Cannot set register state from process %lu: %s.\n", pid, strerror(errno));
        ptrace(PTRACE_DETACH, (pid_t)pid, NULL, NULL);
        return 1;
    }

    if (ptrace(PTRACE_CONT, (pid_t)pid, NULL, NULL)) {
        fprintf(stderr, "Cannot execute injected code to process %lu: %s.\n", pid, strerror(errno));
        ptrace(PTRACE_DETACH, (pid_t)pid, NULL, NULL);
        return 1;
    }

    waitid(P_PID, (pid_t)pid, &info, WSTOPPED);

    if (ptrace(PTRACE_SETREGS, (pid_t)pid, NULL, &oldregs)) {
        fprintf(stderr, "Cannot reset register state from process %lu: %s.\n", pid, strerror(errno));
        ptrace(PTRACE_DETACH, (pid_t)pid, NULL, NULL);
        return 1;
    }

    if (ptrace(PTRACE_DETACH, (pid_t)pid, NULL, NULL)) {
        fprintf(stderr, "Cannot detach from process %lu: %s.\n", pid, strerror(errno));
        return 1;
    }

    close(fd);
    fprintf(stderr, "Done.\n");
    return 0;
}
