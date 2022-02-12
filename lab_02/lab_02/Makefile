CFILES := $(wildcard *.c)
OBJECTS := $(CFILES:%.c=%.o)


app.exe: $(OBJECTS) *.h
	gcc $(OBJECTS) -o app.exe

%.o: %.c *.h
	gcc  -c $<

run: app.exe
	./app.exe
	
# .PHONY - чтобы не было конфликтов с файлами
.PHONY : clean 
clean:
	$(RM) *.o *.exe