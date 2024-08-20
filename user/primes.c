#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void primes(int p[2]){
    int buffer;
    int buffer1;
    int p1[2];
    
    close(p[1]);
    if(read(p[0],&buffer,sizeof(buffer))==0){
        close(p[0]);
        exit(0);
    }
    printf("prime %d\n",buffer);

    pipe(p1);
    if(fork()==0){
        close(p[0]);
        primes(p1);
    }
    else{
        close(p1[0]);
        while(read(p[0],&buffer1,sizeof(buffer))>0){
            if(buffer1%buffer!=0){
                write(p1[1],&buffer1,sizeof(buffer1));
            }
        }
        close(p[0]);
        close(p1[1]);
        wait(0);
        exit(0);
    }

}
int main(int argc, char *argv[])
{
    int p[2];
    pipe(p);
    
    if(fork()==0){
        primes(p);
    }
    else{
        close(p[0]);
        for(int i=2;i<36;i++){
            write(p[1],&i,4);
        }
        close(p[1]);

        wait(0);
        exit(0);
    }
    exit(0);
}