#declare variables

CC = g++
INSTDIR = /home/colin/git_home/VC/install
INCLUDE = .
#CFLAGS = -g -Wall -ansi -std=c++11
CFLAGS = -g -Wl,--no-as-needed -ansi -std=c++11
LIBS += 


.PHONY: install clean all

all: Sample.cpp Sample.h
	$(CC) -I$(INCLUDE) $(CFLAGS) Sample.cpp Sample.h -o sample.out -lpthread

clean: sample.out
	rm -f sample.out; \
	rm -f $(INSTDIR)/sample.out
install: sample.out
	@if [ -d $(INSTDIR) ]; \
	then \
	    cp sample.out $(INSTDIR); \
	    chmod a+x $(INSTDIR)/sample.out;\
	    chmod og-w $(INSTDIR)/sample.out;\
	    echo "Installed in $(INSTDIR)";\
	else \
	    echo "Sorry, $(INSTDIR) does not exist";\
	fi
