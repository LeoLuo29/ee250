"""
EE 250L Fall 2026 - Lab 2
TCP server used in BOTH Part 2 and Part 3.

Usage:
    python3 tcp_server.py <PORT>

Example:
    python3 tcp_server.py 5000

Do not modify this file unless instructed by course staff.
"""

import socket
import sys

ACK_MESSAGE = "I got your message"


def main():
    if len(sys.argv) != 2:
        print(f"Usage: python3 {sys.argv[0]} <PORT>")
        sys.exit(1)

    try:
        port = int(sys.argv[1])
    except ValueError:
        print("ERROR: PORT must be an integer.")
        sys.exit(1)

    if not (1024 <= port <= 65535):
        print("ERROR: Please use an unprivileged TCP port from 1024 to 65535.")
        sys.exit(1)

    # AF_INET = IPv4, SOCK_STREAM = TCP
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_sock:
        # Makes restarting the lab server less annoying after Ctrl+C.
        server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        # Bind to all IPv4 interfaces so the same server can accept:
        #   Part 2: localhost clients
        #   Part 3: ESP32-S3 clients arriving through the VM network interface
        server_sock.bind(("0.0.0.0", port))
        server_sock.listen(5)

        print(f"TCP server listening on 0.0.0.0:{port}")
        print("Press Ctrl+C to stop the server.")

        try:
            while True:
                client_sock, client_addr = server_sock.accept()
                with client_sock:
                    print(f"\nConnection from {client_addr[0]}:{client_addr[1]}")

                    data = client_sock.recv(256)
                    if not data:
                        print("Client connected but sent no data.")
                        continue

                    message = data.decode("utf-8", errors="replace")
                    print(f"Received: {message}")

                    client_sock.sendall(ACK_MESSAGE.encode("utf-8"))
                    print(f"Sent: {ACK_MESSAGE}")

        except KeyboardInterrupt:
            print("\nServer stopped.")


if __name__ == "__main__":
    main()
