clean:
	rm -rf build
.PHONY: clean 

build: 
	rm -rf build && mkdir build && cd build && cmake ..
.PHONY: build

compile:
	cd build && make wiimote
.PHONY: compile

run: compile
	./build/wiimote
.PHONY: run

all: clean build compile
.PHONY: all

help:
	@echo "... clean    - Remove build directory"
	@echo "... c        - Create build directory"
	@echo "... compile  - Compile the wiimote project"
	@echo "... run      - Compile and execute the wiimote executable."
	@echo "... all      - Clean and create build directory"
.PHONY: help