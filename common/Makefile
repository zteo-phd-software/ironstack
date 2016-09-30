CC = g++-4.8
CCOPTS = -c -g -Wall -Wformat-nonliteral -ggdb -funsigned-char -fno-exceptions -std=c++11 -pg
LINKOPTS = -g -pthread -pg -std=c++11

all: autobuf.o \
	autobuf_packer.o \
	bounded_buffer.o \
	common_utils.o \
	common_utils_oop.o \
	csv_parser.o \
	filesystem.o \
	gui.o \
	ip_address.o \
	ip_port.o \
	mac_address.o \
	rate_meter.o \
	serial.o \
	switch_telnet.o \
	tcp.o \
	telnet_client.o \
	timed_barrier.o \
	timer.o \
	uuid.o \
	z_allocator.o

telnet_test: autobuf.o \
	telnet_test.o \
	ip_address.o \
	ip_port.o \
	tcp.o \
	switch_telnet.o
	$(CC) $(LINKOPTS) -o $@ $^

string_utils.o: string_utils.cpp string_utils.h
	$(CC) $(CCOPTS) -o $@ string_utils.cpp

telnet_test.o: telnet_test.cpp
	$(CC) $(CCOPTS) -o $@ telnet_test.cpp

autobuf.o: autobuf.cpp autobuf.h
	$(CC) $(CCOPTS) -o $@ autobuf.cpp

autobuf_packer.o: autobuf_packer.cpp autobuf_packer.h
	$(CC) $(CCOPTS) -o $@ autobuf_packer.cpp

bounded_buffer.o: bounded_buffer.cpp bounded_buffer.h
	$(CC) $(CCOPTS) -o $@ bounded_buffer.cpp

common_utils.o: common_utils.cpp common_utils.h
	$(CC) $(CCOPTS) -o $@ common_utils.cpp

common_utils_oop.o: common_utils_oop.cpp common_utils_oop.h
	$(CC) $(CCOPTS) -o $@ common_utils_oop.cpp

csv_parser.o: csv_parser.cpp csv_parser.h
	$(CC) $(CCOPTS) -o $@ csv_parser.cpp

filesystem.o: filesystem.cpp filesystem.h
	$(CC) $(CCOPTS) -o $@ filesystem.cpp

gui.o: gui.cpp gui.h
	$(CC) $(CCOPTS) -o $@ gui.cpp

ip_address.o: ip_address.cpp ip_address.h
	$(CC) $(CCOPTS) -o $@ ip_address.cpp

ip_port.o: ip_port.cpp ip_port.h
	$(CC) $(CCOPTS) -o $@ ip_port.cpp

# disabled because serializable has been deprecated
#kv_store.o: kv_store.cpp kv_store.h
#	$(CC) $(CCOPTS) -o $@ kv_store.cpp

mac_address.o: mac_address.cpp mac_address.h
	$(CC) $(CCOPTS) -o $@ mac_address.cpp

rate_meter.o: rate_meter.cpp rate_meter.h
	$(CC) $(CCOPTS) -o $@ rate_meter.cpp

serial.o: serial.cpp serial.h
	$(CC) $(CCOPTS) -o $@ serial.cpp

switch_telnet.o: switch_telnet.cpp switch_telnet.h
	$(CC) $(CCOPTS) -o $@ switch_telnet.cpp

tcp.o: tcp.cpp tcp.h
	$(CC) $(CCOPTS) -o $@ tcp.cpp

tcp_utils.o: tcp_utils.cpp tcp_utils.h
	$(CC) $(CCOPTS) -o $@ tcp_utils.cpp

telnet_client.o: telnet_client.cpp telnet_client.h
	$(CC) $(CCOPTS) -o $@ telnet_client.cpp

timed_barrier.o: timed_barrier.cpp timed_barrier.h
	$(CC) $(CCOPTS) -o $@ timed_barrier.cpp

timer.o: timer.cpp timer.h
	$(CC) $(CCOPTS) -o $@ timer.cpp

uuid.o: uuid.cpp uuid.h
	$(CC) $(CCOPTS) -o $@ uuid.cpp

z_allocator.o: z_allocator.cpp z_allocator.h
	$(CC) $(CCOPTS) -o $@ z_allocator.cpp

clean:
	rm -rf *.o

cleandump:
	rm -rf core.*
