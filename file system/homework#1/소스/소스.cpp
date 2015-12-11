#include <iostream>
#include <cstdio>
#include <fstream>
#include <ctime>//time()
#include <string>
#include <stdlib.h>
#include <algorithm>//sort()

#define OLD_MASTER_FILE "oldmaster.txt"
#define TRANSACTION_FILE "transaction.txt"
#define NEW_MASTER_FILE "newmaster.txt"
#define ERR_FILE "error.txt"
#define TRANSLEN 30
#define STRLEN 20

using namespace std;

//Trans log 구조체
class Trans{
private:
	int key;
	int t;
	char op; //operation
	char getMode(int num){
		if(num==0) return('i');
		else if(num==1) return('m');
		else return('d');
	}
public:
	static int counter; //default constructor
	Trans(){
		key = rand()%20+1;
		t=counter++;
		op = getMode(rand()%3);
	}
	Trans(Trans* tr){//copy constructor
		key=tr->key;
		t=tr->t;
		op=tr->op;
	}
	Trans(string str){//convert string to trans log
		sscanf(str.c_str(), "%d/t%d(%c)", &key, &t, &op);
	}
	void transToStr(char* str){//convert trans log to string
		sprintf(str, "%d/t%d(%c)", key, t, op);
	}
	int getKey() { return key; }
	int getTime() { return t; }
	char getOp() { return op; }
};
int Trans::counter=0; //counter초기화

//initialize master file(2,4,6,8,10,...)
void initMasterFile(){
	ofstream stream;
	stream.open(OLD_MASTER_FILE);
	for(int i=2; i<=20; i+=2)
		stream << i << "\n"; 
	stream.close();
}

//create 30 transaction log files
void makeTransactionLogFile(){
	srand(time(NULL));
	ofstream stream;
	stream.open(TRANSACTION_FILE);
	for(int i=0; i<TRANSLEN; i++){
		Trans* tr = new Trans();
		char str[100];
		tr->transToStr(str);
		stream << str << "\n";
		delete tr;
	}
	stream.close();
}

//function for sort compare
bool myFunction(string str1,string str2){
	int val1 = atoi(str1.c_str());
	int val2 = atoi(str2.c_str());
	return (val1<val2);
}

//function for sort transaction file
void sortTransactionFile(){
	ifstream stream; //read only
	stream.open(TRANSACTION_FILE);
	string arr[TRANSLEN];

	for(int i=0; i<TRANSLEN; i++)
		stream >> arr[i];

	sort(begin(arr), end(arr), myFunction);
	stream.close();

	ofstream stream2; //delete and write
	stream2.open(TRANSACTION_FILE);
	for(int i=0; i<TRANSLEN; i++)
		stream2 << arr[i] << "\n"; 
	stream2.close();
}

//store error msg at file
void storeErrMsg(int key, char errNum){
	fstream stream;
	stream.open(ERR_FILE, fstream::app);

	stream << "생성키" << key <<" : ";
	if(errNum=='d')
		stream << "마스터에 존재하지 않는 레코드의 삭제" << endl;
	else if(errNum=='m')
		stream << "마스터에 존재하지 않는 레코드의 수정"<< endl;
	else
		stream << "마스터에 이미 존재하는 레코드의 삽입" << endl;
	stream.close();
}

//function for return existance of key
int checkExist(int key, int exist, char opArr[20], int cnt){
	for(int i=0; i<cnt; i++){
		if( (opArr[i]=='i') && (exist==0)) { exist=1; }
		else if( (opArr[i]=='i') && (exist==1) ) storeErrMsg(key, opArr[i]);
		else if( (opArr[i]=='m') && (exist==0) ) storeErrMsg(key, opArr[i]);
		else if( (opArr[i]=='d') && (exist==0) ) storeErrMsg(key, opArr[i]);
		else if( (opArr[i]=='d') && (exist==1) ) exist=0;
	}
	return exist;
}

//merge function
void makeMerage(){
	fstream oldMaster, trans;
	ofstream newMaster, errF;
	oldMaster.open(OLD_MASTER_FILE);
	trans.open(TRANSACTION_FILE);
	newMaster.open(NEW_MASTER_FILE);
	errF.open(ERR_FILE);
	errF.close();

	int masterKey;
	oldMaster >> masterKey;

	string oldTransMsg, newTransMsg;
	trans >> oldTransMsg;
	Trans* oldLog = new Trans(oldTransMsg);
	
	while(oldMaster!=NULL){
		int transKey;
		if(oldLog!=NULL)
			transKey = oldLog->getKey();
		Trans* newLog;

		//key가 같은 transaction log의 operation(i/d/m)을 opArr에 저장.
		int checker = 1;
		char opArr[20];
		int cnt=0;
		if(oldLog!=NULL)
			opArr[cnt++]=oldLog->getOp();


		while(checker==1 && trans!=NULL){
			trans >> newTransMsg;
			newLog = new Trans(newTransMsg);
			if(transKey == newLog->getKey())
				opArr[cnt++]=newLog->getOp();
			else
				checker=0;
		}
		delete oldLog;
		oldLog = new Trans(newLog);
		if(newLog!=NULL)
			delete newLog;

		int reCheck =1;
		while(reCheck!=0){
			//비교하면서 error handling
			if( transKey < masterKey ){
				if (checkExist(transKey, 0, opArr, cnt) == 1)
					newMaster << transKey << endl;
				reCheck=0;
			}
			else if( transKey == masterKey ){
				if(checkExist(transKey, 1, opArr, cnt)==1)
					newMaster << transKey << endl;
				oldMaster >> masterKey;
				reCheck=0;
			}
			else if( transKey > masterKey ) {
				newMaster << masterKey << endl;
				oldMaster >> masterKey;
				if(oldMaster!=NULL)
					reCheck=1;
			}
		}

		while(trans==NULL && oldMaster!=NULL){
			newMaster << masterKey << endl;
			oldMaster >> masterKey;
		}
	}
	oldMaster.close();
	trans.close();
	newMaster.close();
}

void printfTransactionLogFile(){
	ifstream stream;
	stream.open(TRANSACTION_FILE);
	for(int i=1; i<=30; i++){
		string str;
		stream >> str;
		cout << "-" << i << ". 생성키 " << str << endl;
	}
	stream.close();
}

void printLogState(){
	ifstream stream;
	stream.open(TRANSACTION_FILE);
	cout << "로그상태 ";
	for(int i=1; i<=30; i++){
		string str;
		stream >> str;
		cout << str << " ";
	}
	 cout << endl;
	stream.close();
}

void printNewMaster(){
	ifstream stream;
	stream.open(NEW_MASTER_FILE);
	cout << "뉴 마스터: ";
	
	while(stream!=NULL){
		string num;
		stream>>num;
		cout << num << " ";
	}
	stream.close();
}

void printErrMsg(){	
	ifstream stream;
	stream.open(ERR_FILE);

	while(stream!=NULL){
		string str;
		getline(stream, str);
		cout << str << endl;
	}
	stream.close();
}

void main(){
	initMasterFile(); //마스터파일 초기화(2,4,6,....)
	makeTransactionLogFile();//난수 발생기를 이용해 30개의 log 생성
	
	cout << "출력1(트랜젝션로그파일의생성)" << endl;
	printfTransactionLogFile();
	printLogState();
	cout << endl;
	
	sortTransactionFile();
	cout << "출력2 (정렬된트렌젝션로그파일)" << endl;
	printLogState();
	cout << endl;

	makeMerage();//merge
	cout << "출력3 (뉴마스터파일)" << endl;
	printNewMaster();
	cout << endl;

	cout << "출력4 (오류메시지)" << endl;
	printErrMsg();
}
