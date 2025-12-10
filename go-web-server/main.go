package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"sync"
	"time"

	"github.com/gorilla/websocket"
)

// State represents a single input frame from the Wii-like device.
type State struct {
	Pitch float64 `json:"pitch"`
	Yaw   float64 `json:"yaw"`
	Roll  float64 `json:"roll"`
	A     bool    `json:"a"`
	B     bool    `json:"b"`
	One   bool    `json:"1"`
	Two   bool    `json:"2"`
	Ts    int64   `json:"ts,omitempty"`
}

var (
	_upgrader = websocket.Upgrader{
		CheckOrigin:    func(r *http.Request) bool { return true },
		ReadBufferSize: 1024,
	}
)

// Global _latest state (mutex-protected)
var (
	_latestMu sync.Mutex
	_latest   = State{}
	_h        = newHub()
)

// hub broadcasts JSON messages to all connected display clients.
type hub struct {
	mu        sync.Mutex
	clients   map[*client]struct{}
	broadcast chan []byte
}

type client struct {
	conn *websocket.Conn
	send chan []byte
}

func newHub() *hub {
	h := &hub{
		clients:   make(map[*client]struct{}),
		broadcast: make(chan []byte, 16),
	}
	go h.run()
	return h
}

func (h *hub) run() {
	for msg := range h.broadcast {
		h.mu.Lock()
		for c := range h.clients {
			select {
			case c.send <- msg:
			default:
				// client is stuck; remove it
				close(c.send)
				delete(h.clients, c)
			}
		}
		h.mu.Unlock()
	}
}

func (h *hub) register(c *client) {
	h.mu.Lock()
	h.clients[c] = struct{}{}
	h.mu.Unlock()
}

func (h *hub) unregister(c *client) {
	h.mu.Lock()
	if _, ok := h.clients[c]; ok {
		delete(h.clients, c)
		close(c.send)
	}
	h.mu.Unlock()
	c.conn.Close()
}

func setLatest(s State) {
	s.Ts = time.Now().UnixMilli()
	_latestMu.Lock()
	_latest = s
	_latestMu.Unlock()

	b, err := json.Marshal(s)
	if err != nil {
		log.Printf("ERROR: setLatest: could not marshal: %v\n", err)
		return
	}

	select {
	case _h.broadcast <- b:
	default:
		log.Printf("WARNING: setLatest: broadcast channel full, dropping update\n")
	}
}

func getLatest() State {
	_latestMu.Lock()
	s := _latest
	_latestMu.Unlock()
	return s
}

// POST /wii accepts JSON body and updates state
func handlerWiiPost(w http.ResponseWriter, r *http.Request) {

	var s State
	if r.Body == nil {
		http.Error(w, "empty body", http.StatusBadRequest)
		log.Printf("DEBUG: empty body in /wii POST\n")
		return
	}

	dec := json.NewDecoder(r.Body)
	if err := dec.Decode(&s); err != nil {
		http.Error(w, "invalid json: "+err.Error(), http.StatusBadRequest)
		log.Printf("ERROR: invalid json in /wii POST: %v\n", err)
		return
	}

	setLatest(s)
	w.WriteHeader(http.StatusNoContent)
}

// GET /state returns last JSON state (for polling fallback)
func handlerState(w http.ResponseWriter, r *http.Request) {
	s := getLatest()
	w.Header().Set("Content-Type", "application/json")
	err := json.NewEncoder(w).Encode(s)
	if err != nil {
		log.Printf("ERROR: handlerState: could not encode json: %v\n", err)
	}
}

// WebSocket for display clients (server pushes updates)
func handlerWS(w http.ResponseWriter, r *http.Request) {
	conn, err := _upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Printf("ERROR: handlerWS: could not upgrade: %v\n", err)
		return
	}
	c := &client{
		conn: conn,
		send: make(chan []byte, 8),
	}

	_h.register(c)

	// Send current state immediately
	if b, err := json.Marshal(getLatest()); err == nil {
		c.send <- b
	}

	go writePump(_h, c)
	readPump(_h, c) // readPump blocks until connection closed
}

// writePump writes messages to the Websocket connection of a singular client.
func writePump(h *hub, c *client) {
	defer h.unregister(c)

	for msg := range c.send {
		if err := c.conn.SetWriteDeadline(time.Now().Add(1 * time.Second)); err != nil {
			log.Printf("ERROR: writePump setWriteDeadline: %v\n", err)
			return
		}

		if err := c.conn.WriteMessage(websocket.TextMessage, msg); err != nil {
			log.Printf("ERROR: writePump writeMessage: %v\n", err)
			return
		}
	}
}

// readPump reads messages from the Websocket connection from a singular client.
//
// We don't expect messages from display clients, but we read to detect
// connection closure.
func readPump(h *hub, c *client) {
	defer h.unregister(c)
	c.conn.SetReadLimit(1 << 10)

	for {
		messageType, _, err := c.conn.ReadMessage()
		if err != nil {
			log.Printf("ERROR: readPump: could not read message: %v\n", err)
			break
		}

		switch messageType {

		// Leave the loop on close message.
		case websocket.CloseMessage:
			break

		// Don't do anything else with other message types for now.
		default:
			log.Printf("DEBUG: readPump: ignoring message type %d\n", messageType)
		}
	}

}

// handlerWsInput is a Websocket endpoint for input producers (alt to HTTP POST).
func handlerWsInput(w http.ResponseWriter, r *http.Request) {
	conn, err := _upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Printf("ERROR: handlerWsInput: could not upgrade: %v\n", err)
		return
	}

	defer conn.Close()
	for {
		messageType, msg, err := conn.ReadMessage()
		if err != nil {
			log.Printf("ERROR: handlerWsInput: readMessage: %v\n", err)
			return
		}

		switch messageType {
		case websocket.TextMessage:
			var s State
			if err := json.Unmarshal(msg, &s); err != nil {
				log.Printf("ERROR: handlerWsInput invalid json: %v\n", err)
				continue
			}
			setLatest(s)

		case websocket.CloseMessage:
			log.Printf("DEBUG: handlerWsInput: received close message\n")
			return

		case websocket.PingMessage, websocket.PongMessage:
			// ignore
		default:
		}

	}
}

func rootHandler(w http.ResponseWriter, r *http.Request) {
	http.ServeFile(w, r, "static/index.html")
}

func main() {
	/*
	 - /wii - HTTP POST
	 - /ws-input - WebSocket
	*/
	http.HandleFunc("/wii", handlerWiiPost)
	http.HandleFunc("/ws-input", handlerWsInput)

	http.HandleFunc("/state", handlerState)
	http.HandleFunc("/ws", handlerWS)

	// Serve files under /static/
	fs := http.FileServer(http.Dir("./static"))
	http.Handle("/static/", http.StripPrefix("/static/", fs))
	http.HandleFunc("/", rootHandler)

	// TODO: Make this configurable from the command line.
	addr := ":8080"
	fmt.Println("Server listening on", addr)
	log.Fatal(http.ListenAndServe(addr, nil))
}
