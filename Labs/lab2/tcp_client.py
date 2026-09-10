"""
EE 250L Fall 2026 - Lab 2, Part 2
Python TCP client starter code.

Complete the TODOs below.

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

    # TODO 1:
    # Create an IPv4 TCP socket.
    #
    # Hint:
    #   socket.AF_INET   -> IPv4
    #   socket.SOCK_STREAM -> TCP
    #
    # Replace the next line with your code.
    sock = None

    try:
        # TODO 2:
        # Connect the socket to (server_host, server_port).
        #
        # Replace "pass" with your code.
        pass

        message = input("Enter a short message: ")

        # TODO 3:
        # Convert the string to bytes and send it over the TCP connection.
        # sendall() is convenient in Python because it keeps sending until
        # all bytes have been handed to the socket or an error occurs.
        #
        # Replace "pass" with your code.
        pass

        # TODO 4:
        # Receive up to 256 bytes from the server, decode them as UTF-8,
        # and print the response.
        #
        # Replace the next two lines with your code.
        response = None
        print("Server replied:", response)

    finally:
        # TODO 5:
        # Close the socket if it was successfully created.
        #
        # Replace "pass" with your code.
        pass


if __name__ == "__main__":
    main()
