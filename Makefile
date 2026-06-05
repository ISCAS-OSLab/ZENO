.PHONY: all build run test clean install clean_log

all: build

JOBS ?= $(shell nproc 2>/dev/null || echo 1)

build: 
	cmake -B build -S src
	cmake --build build --parallel $(JOBS)

BUILDDIR := build

install: 
	sudo cmake --install build

run:
	fallocate -l 540M /dev/shm/ipcshm
	chmod a+rwx /dev/shm/ipcshm
	./build/tee_server >> /dev/null 2>&1 &

stop:
	pkill -9 tee_server

test:
	sudo -u postgres pg_prove test/unit_test.sql

clean_log:
	sudo rm -rf /tmp/kvmap_* /tmp/*.log
	rm -rf sat/*.log

clean:
	rm -rf build
