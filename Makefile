#declare variables
CC = gcc
CPP = g++
INSTDIR = /home/colin/git_home/VC/install
INCLUDE = .
#CFLAGS = -g -Wall -ansi -std=c++11
CFLAGS = -g -Wl,--no-as-needed -ansi -std=gnu99
CPPFLAGS = -g -Wl,--no-as-needed -ansi -std=c++11
LIBS += 


.PHONY: install clean all

all: Sample.cpp Sample.h
	$(CC) -I$(INCLUDE) $(CFLAGS) Sample.c Sample.h -o sample.out -lpthread
	$(CPP) -I$(INCLUDE) $(CPPFLAGS) Sample.cpp Sample.h -o sample_2.out -lpthread
clean: sample.out
	rm -f sample.out; \
	rm -f sample_2.out; \
	rm -f $(INSTDIR)/sample.out; \
	rm -f $(INSTDIR)/sample_2.out
install: sample.out
	@if [ -d $(INSTDIR) ]; \
	then \
	    cp sample.out $(INSTDIR); \
	    cp sample_2.out $(INSTDIR); \
	    chmod a+x $(INSTDIR)/sample.out;\
	    chmod og-w $(INSTDIR)/sample.out;\
	    chmod a+x $(INSTDIR)/sample_2.out;\
            chmod og-w $(INSTDIR)/sample_2.out;\
	    echo "Installed in $(INSTDIR)";\
	else \
	    echo "Sorry, $(INSTDIR) does not exist";\
	fi
