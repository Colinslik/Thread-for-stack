#include <string.h>
#include <time.h>
#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32) || defined(__WINDOWS__) || defined(__TOS_WIN__)
#include <windows.h>
#define TERMINATE_TIME 100
#else
#include <pthread.h>
#define TERMINATE_TIME 0.01
#endif
#include "Sample.h"

//length of key and value
#define KEY_LENGTH 2
#define VALUE_LENGTH 10
\

//stack container pointer
my_pair* HEAD = NULL;

bool run = 1;

//stack push
bool push(char* key, char* value){

	my_pair *_ptr = HEAD;
	char* _key = (char *)malloc(KEY_LENGTH * sizeof(char));
	char* _value = (char *)malloc(VALUE_LENGTH * sizeof(char));

	strncpy(_key, key, KEY_LENGTH);
	_key[KEY_LENGTH] = '\0';
	strncpy(_value, value, VALUE_LENGTH);
	_value[VALUE_LENGTH] = '\0';
	if (!_ptr){//if Head pointer has already been initialized.
		if ((_ptr = (my_pair *)malloc(sizeof(my_pair))) != NULL)
		{
			_ptr->key = _key;
			_ptr->value = _value;
			_ptr->next = NULL;
			HEAD = _ptr;
			return 1;
		}
		return 0;
	}
	else{//if Head pointer has not yet been initialized.
		my_pair *_ptr = HEAD;
		for (; _ptr && _ptr->next != NULL; _ptr = _ptr->next);
		if ((_ptr->next = (my_pair *)malloc(sizeof(my_pair))) != NULL)
		{
			_ptr = _ptr->next;
			_ptr->key = _key;
			_ptr->value = _value;
			_ptr->next = NULL;
			return 1;
		}
		return 0;
	}
}
//stack pop
char* pop(){
	char* _value = (char *)malloc(VALUE_LENGTH * sizeof(char));
	my_pair *_ptr = HEAD, *_pre_ptr = HEAD;
	if (_ptr->next != NULL) {
		_pre_ptr = _ptr;
		_ptr = _ptr->next;
	}
	for (; _ptr->next != NULL; _ptr = _ptr->next) _pre_ptr = _pre_ptr->next;
	strncpy(_value, _ptr->value, VALUE_LENGTH);
	_value[VALUE_LENGTH] = '\0';
	_pre_ptr->next = NULL;
	free(_ptr);
	if (_pre_ptr == _ptr) HEAD = NULL;//if pop the last entry, let Head pointer set to NULL
	return _value;
}

//pair struct search method by key
my_pair* pair_find(char *key){
	my_pair *_ptr = HEAD;
	if (_ptr){
		for (; _ptr != NULL; _ptr = (*_ptr).next){
			if (strcmp((*_ptr).key, key) == 0) return _ptr;
		}
	}
	return NULL;
}

//free resource accupied by stack container
void destructor_pair(){
	my_pair *_ptr, *_tail;

	printf("\nRelease Resource.\n");
	for (_ptr = HEAD; _ptr != NULL; _ptr = _ptr->next)
	{
		_tail = _ptr;
		free(_tail);
	}
}

//Random create string pair to push to stack
void* random_push(){
	char key[KEY_LENGTH+1];
	char value[VALUE_LENGTH+1];
	char temp[2];
	time_t t;
	my_pair *_ptr;

	srand((unsigned)time(&t));

	while (run){
		strcpy(key, "\0");
		strcpy(value, "\0");
		strcpy(temp, "\0");
		_ptr = HEAD;
		for (unsigned i = 0; i < 10; i++){
			for (unsigned j = 0; j < KEY_LENGTH; j++){
				temp[0] = (char)('A' + (rand() % 26));
				temp[1] = '\0';
				strcat(key, temp);
			}
			if ((_ptr = pair_find(key)) == NULL) break; //check if key exist or not
			else {
				strcpy(key, "\0");
			}
		}
		strcpy(temp, "\0");
		for (unsigned i = 0; i < VALUE_LENGTH; i++){
			temp[0] = (char)('A' + (rand() % 26));
			temp[1] = '\0';
			strcat(value, temp);
		}
		printf("\n\nPUSH item KEY: %s   Value: %s\n\n", key, value);
		if (!push(key, value)){//if push fail, destory container and restart.
			destructor_pair();
		}
		else delay(2000);
	}
}
//Pop entry from stack
void* recursive_pop(){
	char *value;
	while (run){
		if ( HEAD && (value = pop()) != NULL){
			printf("POP Value: %s\n", value);
			delay(1000);
		}
		else delay(10000);
	}
}
//show all entries in stack
void stack_printf(){
	my_pair *_ptr = HEAD;
	printf("\n\nTop  of stack.\n\n");
	for (; _ptr && (_ptr != NULL); _ptr = _ptr->next)
	{
		printf("KEY: %s    Value: %s\n", _ptr->key, _ptr->value);
	}
	printf("\nEND  of stack.\n");
	delay(5000);
}

#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32) || defined(__WINDOWS__) || defined(__TOS_WIN__)
DWORD WINAPI Thread1(LPVOID lpParam){
	random_push();
	printf("\n\nPUSH Thread is terminated.\n\n");
	return 0;
}
DWORD WINAPI Thread2(LPVOID lpParam){
	recursive_pop();
	printf("\n\nPOP Thread is terminated.\n\n");
	return 0;
}
#endif

int main(int argc, char** argv){

#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32) || defined(__WINDOWS__) || defined(__TOS_WIN__)
	HANDLE hThread1, hThread2;
	DWORD threadID1, threadID2;
	hThread1 = CreateThread(NULL, // security attributes ( default if NULL )
		0, // stack SIZE default if 0
		Thread1, // Start Address
		NULL, // input data
		0, // creational flag ( start if  0 )
		&threadID1); // thread ID

	hThread2 = CreateThread(NULL, // security attributes ( default if NULL )
		0, // stack SIZE default if 0
		Thread2, // Start Address
		NULL, // input data
		0, // creational flag ( start if  0 )
		&threadID2); // thread ID
#else
	void *ret;
	pthread_t thread1;
	pthread_t thread2;

	pthread_create(&thread1, NULL, random_push, NULL); //thread 1 for push entries
	//mThread1.join();

	pthread_create(&thread2, NULL, recursive_pop, NULL); // thread 2 for pop entries
	//mThread2.join();
#endif
	clock_t start, now;
	signal(SIGINT, CtrlHandler);

	start = clock();

	while (run){//printf entries in stack
		if(HEAD && HEAD != 0) stack_printf();
/*		{
			[=](my_pair *_ptr) {
				for (; _ptr != NULL; _ptr = _ptr->next)
				{
					std::cout << "   PRINTER  KEY: " << _ptr->key << "   Value: " << _ptr->value  <<"   PTR: " <<_ptr << std::endl;
					delay(100);
				}
				std::cout << std::endl << std::endl << std::endl << " END  of  PRINTER " << std::endl << std::endl << std::endl;
			}(HEAD);
		*/
		else {
			printf("\n\nStack is empty!\n\n");
			delay(5000);
		}
		now = clock();
		printf("\n\nTIME is %f\n\n", (now - start) / (double)(CLOCKS_PER_SEC));
		if (((now - start) / (double)(CLOCKS_PER_SEC)) > TERMINATE_TIME) run = !run;
	}

#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32) || defined(__WINDOWS__) || defined(__TOS_WIN__)
	WaitForSingleObject(hThread1, INFINITE);
	CloseHandle(hThread1);
	WaitForSingleObject(hThread2, INFINITE);
	CloseHandle(hThread2);
#else
	pthread_join(thread1, &ret);
	pthread_join(thread2, &ret);
#endif
	destructor_pair();
	printf("\n\nTerminate.\n\n");
	getchar();
	return 0;
}