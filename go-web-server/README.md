# Go Web Server

> You will need to have `Go`, `npm`, and `Node.js` installed in order to compile
and run the Go web server.

- [Installing Go](https://go.dev/doc/install)
- [Installing Node.js and npm](https://docs.npmjs.com/downloading-and-installing-node-js-and-npm)


## Run the Web Server

### With Make
1. Run the following command: 
```
make all
```
2. Navigate to http://localhost:8080 in a web browser.

### Without Make

1. Run the following command to compile the Typescript files, followed by the
Go binary and then finally run the Go binary, which hosts the web server.

```
npm run build && go build && ./wiigo
```

2. Navigate to http://localhost:8080 in a web browser.


### See the Dot Move

You can send an HTTP POST request to the `/wii` endpoint in order to see the 
dot move about the screen.

Example:
```bash
curl -X POST -H 'Content-Type: application/json'   -d '{"pitch":-120,"yaw":-120,"roll":10,"a":true,"b":false,"1":true,"2":false}'   http://localhost:8080/wii
```