package main

import (
	"fmt"
	"io"
	"log"
	"net"
	"os"
)

var logger = log.New(os.Stderr, "", log.LstdFlags)

func main() {
	if len(os.Args) < 2 {
		fmt.Fprintln(os.Stderr, "usage: goclient <requested_file_path>")
		os.Exit(1)
	}

	path := os.Args[1]
	file, err := os.OpenFile(path, os.O_CREATE | os.O_WRONLY, 0o744)
	if err != nil {
		logger.Print("Error opening file for write: ", err)
		os.Exit(1)
	}
	
	conn, err := net.Dial("tcp", "localhost:8080")
	if err != nil {
		fmt.Fprintln(os.Stderr, "goclient: unable to connect:", err)
		os.Exit(1)
	}
	defer conn.Close()

	fmt.Println("Connected to the server")

	if _, err := conn.Write([]byte(path)); err != nil {
		fmt.Fprintln(os.Stderr, "Failed to request open:", err)
		os.Exit(1)
	}

	_, err = io.Copy(file, conn)
	if err == io.EOF {
		log.Println("Download has finished...")
	}
	
	fmt.Println("Server closed the connection")
}
