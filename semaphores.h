#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>

/*API: This is a library which implements binary semaphores. To use this libary
 * you need to have a variable which will be integer (ex int semid).
 * create semaphore : semid=init(semid,0) (the second argument is value in which the semaphore is initialized)
 * up : up(semid);
 * down: down(semid);
 * destroy: destroy(semid);
 * */
int init(int semid,int initial_value) {
    semid = semget(IPC_PRIVATE,1,S_IRWXU);
    semctl(semid,0,SETVAL,initial_value); // initial semaphore to initial_value value
    return semid;
}
void down(int semid) {
    int ret;
    struct sembuf op;

	op.sem_num=0;
	op.sem_flg=0;
	op.sem_op=-1;
	ret=semop(semid,&op,1);
    if(ret==-1){
        printf("Error in mysem_down().");
		exit(1);
    }
}
int up(int semid) {

    int ret;
    struct sembuf op;

    op.sem_num=0;
    op.sem_flg=0;
    op.sem_op=1;
    ret=semop(semid,&op,1);
    if(ret==-1){
        printf("[ERROR MESSAGE]::mysem_up() failed!");
        return ret;
    }
    ret=semctl(semid,0,GETVAL);
    if(ret>1){
        printf("[ERROR MESSAGE]::semaphore is already 1.");
        return(ret);
    }

    return(ret);
}
void destroy(int semid) {
    semctl(semid,0,IPC_RMID);
}
