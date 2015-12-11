#include <stdio.h>
#include <stdlib.h>

void main(){
	int i;
	for(i=0; i<1000; i++){
		FILE *fp = fopen("data.txt", "a+");
		int data;
		if( fp==NULL ){
			perror("file opening err");
			exit(1);
		}
		while(!feof(fp))
			fscanf(fp, "%d", &data);
		fprintf(fp, "%d\n", data+1);
		fclose(fp);
	}

}
