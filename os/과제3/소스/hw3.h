#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SEMPERM 0600
#define TRUE 1
#define FALSE 0

#define key_m 2012343000 // 서버에서 작업할 때는 자기 학번 등을 이용하여 다른 사람의 키와 중복되지 않게 해야 함
#define key_okR key_m+1
#define key_okW key_m+2

int count(){
	int result;
	FILE *fstream = fopen("count.txt", "a+");
	while(!feof(fstream))
		fscanf(fstream, "%d", &result);
	fprintf(fstream, "%d\n", result+1);
	fclose(fstream);
	return result;
}

//semaphore object
typedef union semun{
	int val;
	struct semid_ds *buf; //sem_id : semaphore set associated with a semaphore ID.  There is one semaphore set per semaphore ID.
	ushort *array;
} semun;

int initsem (key_t semkey, int n) {
	int status = 0, semid;
	if ( (semid = semget (semkey, 1, SEMPERM | IPC_CREAT | IPC_EXCL)) == -1) { //semget : get a semaphore set identifier
		if (errno == EEXIST) //errno : number of last error
			semid = semget (semkey, 1, 0);
	}
	else{
		semun arg;
		arg.val = n;
		status = semctl(semid, 0, SETVAL, arg); //semctl : control operation
	}
	if (semid == -1 || status == -1) {
		perror("initsem failed");
		return (-1);
	}
	return (semid);
}

int p (int semid) {
	struct sembuf p_buf;
	p_buf.sem_num = 0;
	p_buf.sem_op = -1;
	p_buf.sem_flg = SEM_UNDO;
	if (semop(semid, &p_buf, 1) == -1) {
		perror ("p(semid) failed");
		exit(1);
	}
	return (0);
}

int v (int semid)
{
	struct sembuf v_buf;
	v_buf.sem_num = 0;
	v_buf.sem_op = 1;
	v_buf.sem_flg = SEM_UNDO;
	if (semop(semid, &v_buf, 1) == -1)
	{
		perror ("v(semid) failed");
		exit(1);
	}
	return (0);
}

void Acquire(int mutex){
	p(mutex);
}

void Release(int mutex){
	v(mutex);
}

void wait(int semid, int lock){
	v(lock);
	p(semid);
	p(lock);
}

void signal(int semid){
	v(semid);
}

void broadcast(int semid, int num){
	int i;
	for(i=0; i<num; i++){
		v(semid);
	}
}

int ActvieWriter(int num){
	char str[50];
	int val;
	time_t t;
	FILE *fstream = fopen("AW.txt", "a+");
	if(fstream == NULL)
		exit(1);
	while(!feof(fstream))
		fgets(str, 50, fstream);
	val=atoi(str);

	if(num==0){
		fclose(fstream);
		return val;
	}
	else{
		time(&t);
		fprintf(fstream, "%d %d %d %s", val+num, getpid(), count(), ctime(&t));
		fclose(fstream);
		return 0;
	}

}

int WaitingWriter(int num){
	char str[50];
	int val;
	time_t t;
	FILE *fstream = fopen("WW.txt", "a+");
	if(fstream == NULL)
		exit(1);
	while(!feof(fstream))
		fgets(str, 50, fstream);
	val=atoi(str);

	if(num==0){
		return val;
		fclose(fstream);
	}
	else{
		time(&t);
		fprintf(fstream, "%d %d %d %s", val+num, getpid(), count(), ctime(&t));
		fclose(fstream);
		return 0;
	}
}

int ActiveReader(int num){
	char str[50];
	int val;
	time_t t;
	FILE *fstream = fopen("AR.txt", "a+");
	if(fstream == NULL)
		exit(1);
	while(!feof(fstream))
		fgets(str, 50, fstream);
	val=atoi(str);

	if(num==0){
		return val;
		fclose(fstream);
	}
	else{
		time(&t);
		fprintf(fstream, "%d %d %d %s", val+num, getpid(), count(), ctime(&t));
		fclose(fstream);
		return 0;
	}
}

int WaitingReader(int num){
	char str[50];
	int val;
	time_t t;
	FILE *fstream = fopen("WR.txt", "a+");
	if(fstream == NULL)
		exit(1);
	while(!feof(fstream))
		fgets(str, 50, fstream);
	val=atoi(str);

	if(num==0){
		return val;
		fclose(fstream);
	}
	else{
		time(&t);
		fprintf(fstream, "%d %d %d %s", val+num, getpid(), count(), ctime(&t));
		fclose(fstream);
		return 0;
	}
}

