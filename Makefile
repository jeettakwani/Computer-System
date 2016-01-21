check:clean libckpt.a hello.c
	gcc -g3 -O0 -static -Wl,--allow-multiple-definition -L. -Wl,--whole-archive -lckpt -Wl,--no-whole-archive -o hello hello.c
	  (sleep 3 && pgrep -n hello | xargs kill -12 &&  sleep 2 && pkill -9 hello && make restart) & ./hello


libckpt.a:check_point.o
	ar -cvr libckpt.a check_point.o

check_point.o: 
	cc -g3 -O0 -std=gnu99 -c check_point.c

restart:myrestart.o
	./myrestart checkpoint_image

myrestart.o:
	 gcc -std=gnu99 -g3 -O0 -static -Wl,-Ttext-segment=5000000 -Wl,-Tdata=5100000 -Wl,-Tbss=5200000 -o myrestart myrestart.c

clean:
	-rm file_*
	-rm context_image
	-rm checkpoint_image

dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar
    dir=`basename $$PWD`; ls -l ../$$dir.tar.gz
