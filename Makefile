
CC = gcc
CFLAG = -Wall -O3
SRC = dependent-fssize.c
HDR = 
OBJ = $(SRC:%.c=%.o)
TARGET = dependent-fssize.so
MODULEPATH = /etc/zabbix/modules

.SUFFIXES: .c .o

$(TARGET): $(OBJ)
	$(CC) -shared -o $@ $(OBJ) -fPIC

$(OBJ): $(HDR)

.c.o:
	$(CC) -c $< -I../zabbix-src/include/ -fPIC -c $(CFLAG)

clean:
	rm -f $(TARGET) $(OBJ)

install:$(TARGET)
	install -C $(TARGET) $(MODULEPATH)

restart:$(TARGET)
	service zabbix-agent restart
	service zabbix-agent status
	tail /var/log/zabbix/zabbix_agentd.log

test:
	md5sum  $(MODULEPATH)/$(TARGET) ./$(TARGET) || :
	zabbix_agentd --print 2>/dev/null |grep -e vfs.fs.size.master

test2:
	md5sum  $(MODULEPATH)/$(TARGET) ./$(TARGET) || :
	zabbix_agentd --test vfs.fs.size[/,total]
	zabbix_agentd --test vfs.fs.size[/,free]
	zabbix_agentd --test vfs.fs.size[/,used]
	zabbix_agentd --test vfs.fs.size[/,pfree]
	zabbix_agentd --test vfs.fs.size[/,pused]
	zabbix_get -s 127.0.0.1 -k  vfs.fs.size.master[/] |jq .


