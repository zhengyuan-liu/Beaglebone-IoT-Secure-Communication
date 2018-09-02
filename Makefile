# NAME: Zhengyuan Liu
# EMAIL: zhengyuanliu@ucla.edu
#
# Makefile

default:
	@gcc -Wall -Wextra -lmraa -lm -o lab4c_tcp lab4c_tcp.c
	@gcc -Wall -Wextra -lmraa -lm -lssl -lcrypto -o lab4c_tls lab4c_tls.c

clean:
	@rm -f lab4c.tar.gz lab4c_tcp lab4c_tls

dist:
	@tar -cvzf lab4c.tar.gz lab4c_tcp.c lab4c_tls.c Makefile README
