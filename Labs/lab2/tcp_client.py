"""
EE 250L Fall 2026 - Lab 2, Part 2
Python TCP client starter code. Complete the TODOs below.

Usage:
python3 tcp_client.py <SERVER_IP_OR_HOSTNAME> <PORT>

Example:
python3 tcp_client.py 127.0.0.1 5000

Expected exchange:
1. Connect to tcp_server.py
2. Ask the user for a short message
3. Send the message
4. Receive and print the server reply
5. Close the TCP connection
"""

import socket
import sys

## constants:
MAX_SOCKET_RECEIVE_SIZE = 256

def main():
    if len(sys.argv) != 3:
        print(f"Usage: python3 {sys.argv[0]} <SERVER_IP_OR_HOSTNAME> <PORT>")
        sys.exit(1)

    server_host = sys.argv[1]
    try:
        server_port = int(sys.argv[2])
    except ValueError:
        print("ERROR: PORT must be an integer.")
        sys.exit(1)

    # Initialize variable to None first to protect the finally block scope
    sock = None

    try:
        # TODO 1: Create an IPv4 TCP socket.
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        # TODO 2: Connect the socket to (server_host, server_port).
        # Note the double parentheses to pass host and port as a single tuple argument
        sock.connect((server_host, server_port))

        ## message from the user that shall be sent to server
        message = input("Enter a short message: ")

        # TODO 3: Convert the string to bytes and send it over the TCP connection.
        sock.sendall(message.encode('utf-8'))

        # TODO 4: Receive up to 256 bytes from the server, decode them as UTF-8, and print the response.
        response = sock.recv(MAX_SOCKET_RECEIVE_SIZE)
        print("Server replied:", response.decode('utf-8'))

    except socket.error as e:
        print(f"Socket or network error occurred: {e}")

    finally:
        # TODO 5: Close the socket if it was successfully created.
        if sock is not None:
            sock.close()
            print("socket closed")
        else:
            print("socket does not exist")

if __name__ == "__main__":
    main()
