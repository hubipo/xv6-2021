#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
   int p1[2];
   int p2[2];

   pipe(p1);
   pipe(p2);
   
   if(fork()==0){
    char buffer[1];
    close(p1[1]);
    close(p2[0]);

    read(p1[0],buffer,1);
    printf("%d: received ping\n",getpid());
    
    write(p2[1],"L",1);
   }
   else{
    char buffer[1];
    close(p2[1]);
    close(p1[0]);

    write(p1[1],"L",1);

    read(p2[0],buffer,1);
    printf("%d: received pong\n",getpid());

   }
    exit(0);
}