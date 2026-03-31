import socket
import ssl

# TCP target for our local wolfSSL server.
HOST = "127.0.0.1"
PORT = 4433


def main():
    # Stage 1: Build an SSL context configured for server authentication.
    # `cafile` points to the CA certificate that signed `server-cert.pem`.
    context = ssl.create_default_context(ssl.Purpose.SERVER_AUTH, cafile="ca-cert.pem")
    # Hostname checks are disabled for this local demo.
    # Keep this True in production and match certificate SAN/CN correctly.
    context.check_hostname = False

    # Stage 2: Open a plain TCP connection to the server.
    with socket.create_connection((HOST, PORT)) as sock:
        # Stage 3: Upgrade the TCP socket to TLS and perform handshake.
        # server_hostname is still supplied for SNI and certificate context.
        with context.wrap_socket(sock, server_hostname="localhost") as tls_sock:
            # Stage 4: Send encrypted application data.
            tls_sock.sendall(b"Hello from Python TLS client")
            # Stage 5: Read encrypted reply from server, then decode for display.
            response = tls_sock.recv(1024)
            print("Server replied:", response.decode("utf-8", errors="replace"))


if __name__ == "__main__":
    # Stage 0: Program entry point.
    main()
