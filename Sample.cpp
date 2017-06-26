#include <string.h>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <thread>
#include "Sample.h"

#define TERMINATE_TIME 10

//length of key and value
const unsigned int KEY_LENGTH = 2;
const unsigned int VALUE_LENGTH = 10;

std::atomic<bool> run{ true };

std::mutex gMutex;
std::condition_variable gCV1;

//stack container pointer
my_pair* HEAD = NULL;

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
	char* _value = (char *)malloc((VALUE_LENGTH + 1) * sizeof(char));
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

	std::cout << std::endl <<"Release Resource." <<std::endl;
	for (_ptr = HEAD; _ptr;)
	{
		_tail = _ptr;
		if (_ptr->next) {
			_ptr = _ptr->next;
			free(_tail);
		}
		else{
			free(_tail);
			break;
		}
	}
}

//Random create string pair to push to stack
void random_push(){
	char key[KEY_LENGTH+1];
	char value[VALUE_LENGTH+1];
	char temp[2];
	time_t t;
	my_pair *_ptr;

	int count = 0;

	srand((unsigned)time(&t));

	while (run){

		std::unique_lock<std::mutex> mLock(gMutex);

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
		std::cout << std::endl << std::endl << "PUSH item KEY: " << key << "  Value: " << value << std::endl << std::endl;
		if (!push(key, value)){//if push fail, destory container and restart.
			destructor_pair();
		}
//		else delay(10000);
		count++;
		mLock.unlock();
		if (count > 5) {
			count = 0;
			gCV1.notify_one();
		}
		else delay(10);
	}
	std::cout << std::endl << std::endl <<"PUSH Thread is terminated." << std::endl <<std::endl;
}
//Pop entry from stack
void recursive_pop(){
	char *value;
	while (run){

		std::unique_lock<std::mutex> mLock(gMutex);
		gCV1.wait(mLock);
		while ( HEAD && (value = pop()) != NULL){
			std::cout << "POP Value:" << value << std::endl;
//			delay(5000);
		}
		mLock.unlock();
	}
	std::cout << std::endl << std::endl << "POP Thread is terminated." << std::endl << std::endl;
}
//show all entries in stack
void stack_printf(){
	my_pair *_ptr = HEAD;
	std::cout << std::endl << std::endl << "Top  of stack." << std::endl << std::endl;
	for (; _ptr && (_ptr != NULL); _ptr = _ptr->next)
	{
		std::cout << "KEY: " << _ptr->key << "   Value: " << _ptr->value << std::endl;
	}
	std::cout << std::endl << "END  of stack." << std::endl;
//	delay(10000);
}

int main(int argc, char** argv){
	signal(SIGINT, CtrlHandler);

	auto start = std::chrono::system_clock::now();

	std::thread mThread1(random_push); //thread 1 for push entries

	std::thread mThread2(recursive_pop); // thread 2 for pop entries

	while (run){//printf entries in stack
		if (HEAD && HEAD != 0) {
			std::unique_lock<std::mutex> mLock(gMutex);
			stack_printf();
			mLock.unlock();
		}
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
			std::cout << std::endl << std::endl << "Stack is empty!" << std::endl << std::endl;
		}
		auto end = std::chrono::system_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		std::cout	<< std::endl << std::endl << "TIME is " << double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den
					<< std::endl <<std::endl;
		if ((double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den) > TERMINATE_TIME) run = !run;
		delay(10);
	}
//	std::cout << std::endl << std::endl << "wait for thread1!" << std::endl << std::endl;
	mThread1.join();
//	std::cout << std::endl << std::endl << "wait for thread2!" << std::endl << std::endl;
	gCV1.notify_all();
	mThread2.join();
//	std::cout << std::endl << std::endl << "Destroy stack!" << std::endl << std::endl;
	destructor_pair();
	getchar();
	return 0;
}