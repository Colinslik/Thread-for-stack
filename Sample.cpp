#include <string.h>
#include <time.h>
#include <mutex>
#include "Sample.h"

//length of key and value
const unsigned int KEY_LENGTH = 2;
const unsigned int VALUE_LENGTH = 10;

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
	for (_ptr = HEAD; _ptr != NULL; _ptr = _ptr->next)
	{
		_tail = _ptr;
		free(_tail);
	}
}

//Random create string pair to push to stack
void random_push(){
	char key[KEY_LENGTH];
	char value[VALUE_LENGTH];
	char temp[2];
	time_t t;
	my_pair *_ptr;

	srand((unsigned)time(&t));

	while (1){
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
		else delay(10000);
	}
}
//Pop entry from stack
void recursive_pop(){
	char *value;
	while (1){
		if ( HEAD && (value = pop()) != NULL){
			std::cout << "POP Value:" << value << std::endl;
			delay(5000);
		}
		else delay(60000);
	}
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
	delay(10000);
}

int main(int argc, char** argv){
	signal(SIGINT, CtrlHandler);

	std::thread mThread1(random_push); //thread 1 for push entries
	//mThread1.join();

	std::thread mThread2(recursive_pop); // thread 2 for pop entries
	//mThread2.join();

	while (1){//printf entries in stack
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
			std::cout << std::endl << std::endl << "Stack is empty!" << std::endl << std::endl;
			delay(10000);
		}
	}
	destructor_pair();
	getchar();
	return 0;
}