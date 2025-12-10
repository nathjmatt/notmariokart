# Build the frontend (Typescript).
buildt:
	npm run build

# Build the backend (Go).
buildgo:
	go build -o wiigo ./main.go

# Build the frontend and the backend.
build: buildt buildgo

# Execute the Go binary.
run:
	./wiigo

clean:
	rm -f wiigo
	rm -f ./static/app.js
	rm -f ./static/app.js.map

all: clean build run