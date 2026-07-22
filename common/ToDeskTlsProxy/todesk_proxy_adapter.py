#!/usr/bin/env python3
import argparse
import select
import socket
import socketserver
import sys
import threading
import urllib.parse


MAX_HEADER_BYTES = 65536
BUFFER_SIZE = 65536


def log(message):
    print(message, flush=True)


def recv_exact(sock, size):
    chunks = []
    remaining = size
    while remaining:
        chunk = sock.recv(remaining)
        if not chunk:
            raise OSError("unexpected EOF")
        chunks.append(chunk)
        remaining -= len(chunk)
    return b"".join(chunks)


def parse_host_port(value, default_port=None):
    value = value.strip()
    if value.startswith("["):
        end = value.find("]")
        if end < 0:
            raise ValueError("invalid IPv6 host")
        host = value[1:end]
        rest = value[end + 1 :]
        if rest.startswith(":"):
            port = int(rest[1:])
        elif default_port is not None:
            port = default_port
        else:
            raise ValueError("missing port")
        return host, port

    if value.count(":") == 1:
        host, port_text = value.rsplit(":", 1)
        return host, int(port_text)

    if default_port is None:
        raise ValueError("missing port")
    return value, default_port


def socks5_connect(socks_host, socks_port, target_host, target_port):
    upstream = socket.create_connection((socks_host, socks_port), timeout=15)
    try:
        upstream.sendall(b"\x05\x01\x00")
        response = recv_exact(upstream, 2)
        if response != b"\x05\x00":
            raise OSError("SOCKS5 no-auth negotiation failed")

        encoded_host = target_host.encode("idna")
        if len(encoded_host) > 255:
            raise OSError("target host is too long")
        request = (
            b"\x05\x01\x00\x03"
            + bytes([len(encoded_host)])
            + encoded_host
            + int(target_port).to_bytes(2, "big")
        )
        upstream.sendall(request)

        header = recv_exact(upstream, 4)
        if header[0] != 5:
            raise OSError("invalid SOCKS5 response")
        if header[1] != 0:
            raise OSError("SOCKS5 connect failed with code {}".format(header[1]))

        atyp = header[3]
        if atyp == 1:
            recv_exact(upstream, 4)
        elif atyp == 3:
            size = recv_exact(upstream, 1)[0]
            recv_exact(upstream, size)
        elif atyp == 4:
            recv_exact(upstream, 16)
        else:
            raise OSError("invalid SOCKS5 address type")
        recv_exact(upstream, 2)
        return upstream
    except Exception:
        upstream.close()
        raise


def pipe(src, dst):
    try:
        while True:
            data = src.recv(BUFFER_SIZE)
            if not data:
                break
            dst.sendall(data)
    except OSError:
        pass
    finally:
        try:
            dst.shutdown(socket.SHUT_WR)
        except OSError:
            pass


def relay(client, upstream, initial_to_upstream=b""):
    if initial_to_upstream:
        upstream.sendall(initial_to_upstream)
    worker = threading.Thread(target=pipe, args=(client, upstream), daemon=True)
    worker.start()
    pipe(upstream, client)
    worker.join(timeout=2)


class ProxyHandler(socketserver.BaseRequestHandler):
    def handle(self):
        client = self.request
        client.settimeout(30)
        first = client.recv(1, socket.MSG_PEEK)
        if not first:
            return

        if first in (b"\x04", b"\x05"):
            self.handle_socks_passthrough(client)
            return

        self.handle_http_proxy(client)

    def handle_socks_passthrough(self, client):
        upstream = socket.create_connection(
            (self.server.socks_host, self.server.socks_port), timeout=15
        )
        log("SOCKS passthrough from {}".format(self.client_address[0]))
        try:
            relay(client, upstream)
        finally:
            upstream.close()

    def handle_http_proxy(self, client):
        header = b""
        while b"\r\n\r\n" not in header and len(header) < MAX_HEADER_BYTES:
            chunk = client.recv(4096)
            if not chunk:
                return
            header += chunk

        if b"\r\n\r\n" not in header:
            self.send_error(client, 431, "Request Header Fields Too Large")
            return

        head, buffered_body = header.split(b"\r\n\r\n", 1)
        lines = head.decode("iso-8859-1", errors="replace").split("\r\n")
        if not lines or len(lines[0].split()) != 3:
            self.send_error(client, 400, "Bad Request")
            return

        method, target, version = lines[0].split()
        headers = lines[1:]

        try:
            if method.upper() == "CONNECT":
                host, port = parse_host_port(target)
                upstream = socks5_connect(
                    self.server.socks_host, self.server.socks_port, host, port
                )
                log("HTTP CONNECT {}:{}".format(host, port))
                client.sendall(b"HTTP/1.1 200 Connection Established\r\n\r\n")
                relay(client, upstream, buffered_body)
            else:
                host, port, rewritten_target = self.parse_http_target(
                    method, target, headers
                )
                upstream = socks5_connect(
                    self.server.socks_host, self.server.socks_port, host, port
                )
                log("HTTP {} {}:{}".format(method.upper(), host, port))
                clean_headers = [
                    line
                    for line in headers
                    if not line.lower().startswith("proxy-connection:")
                ]
                request = (
                    "{} {} {}\r\n".format(method, rewritten_target, version)
                    + "\r\n".join(clean_headers)
                    + "\r\n\r\n"
                ).encode("iso-8859-1", errors="replace")
                relay(client, upstream, request + buffered_body)
        except Exception as exc:
            log("proxy error: {}".format(exc))
            self.send_error(client, 502, "Bad Gateway")
        finally:
            try:
                upstream.close()
            except Exception:
                pass

    def parse_http_target(self, method, target, headers):
        parsed = urllib.parse.urlsplit(target)
        if parsed.scheme and parsed.hostname:
            port = parsed.port or (443 if parsed.scheme == "https" else 80)
            path = urllib.parse.urlunsplit(("", "", parsed.path or "/", parsed.query, ""))
            return parsed.hostname, port, path

        host_header = None
        for line in headers:
            if line.lower().startswith("host:"):
                host_header = line.split(":", 1)[1].strip()
                break
        if not host_header:
            raise ValueError("missing Host header")
        host, port = parse_host_port(host_header, 80)
        return host, port, target or "/"

    def send_error(self, client, code, reason):
        body = "{} {}\n".format(code, reason).encode("ascii")
        response = (
            "HTTP/1.1 {} {}\r\n".format(code, reason)
            + "Content-Length: {}\r\n".format(len(body))
            + "Connection: close\r\n"
            + "\r\n"
        ).encode("ascii") + body
        try:
            client.sendall(response)
        except OSError:
            pass


class ThreadingTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    allow_reuse_address = True
    daemon_threads = True


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--listen-host", default="127.0.0.1")
    parser.add_argument("--listen-port", type=int, default=52030)
    parser.add_argument("--socks-host", default="127.0.0.1")
    parser.add_argument("--socks-port", type=int, default=52031)
    args = parser.parse_args()

    with ThreadingTCPServer((args.listen_host, args.listen_port), ProxyHandler) as server:
        server.socks_host = args.socks_host
        server.socks_port = args.socks_port
        log(
            "ToDesk local HTTP/SOCKS proxy listening on "
            "{}:{}, upstream SOCKS {}:{}".format(
                args.listen_host,
                args.listen_port,
                args.socks_host,
                args.socks_port,
            )
        )
        server.serve_forever()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(0)
