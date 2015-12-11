#include "hw3.h"

void main(int argc, char **argv){
	int i;
	int lock, reader, writer;
	sleep(atoi(argv[1]));
   	if ((lock = initsem(key_m,1)) < 0)    // lock semaphore 연결.(없으면 초기값을 1로 주면서 새로 만들어서 연결한다.)
		exit(1);
   	if ((reader = initsem(key_okR,1)) < 0)    // reader semaphore 연결.(없으면 초기값을 1로 주면서 새로 만들어서 연결한다.)
		exit(1);
   	if ((writer = initsem(key_okW,1)) < 0)    // writer semaphore 연결.(없으면 초기값을 1로 주면서 새로 만들어서 연결한다.)
		exit(1);

	Acquire(lock);   // Lock Acquire()
   	while( (ActvieWriter(0) + WaitingWriter(0)) > 0 ){
   		WaitingReader(1);
   		wait(reader, lock);
   		WaitingReader(-1);
   	}
   	ActiveReader(1);
   	Release(lock);

   	//Perform actual read-only access AccessDatabase(ReadOnly);
	sleep(atoi(argv[2]));

   	Acquire(lock);
	ActiveReader(-1);
   	if( ActiveReader(0)==0 && WaitingWriter(0)>0 )
   		signal(writer);
   	Release(lock);

}

