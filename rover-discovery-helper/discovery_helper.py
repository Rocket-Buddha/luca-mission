from __future__ import annotations

import argparse
import json
import socket
from datetime import datetime
from typing import Any


DEFAULT_UDP_HOST = "0.0.0.0"
DEFAULT_UDP_PORT = 4210
MAX_PACKET_SIZE = 65507


def now_string() -> str:
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")


def coerce_text(value: Any) -> str | None:
    if value is None:
        return None
    text = str(value).strip()
    return text or None


def parse_announcement(payload_text: str, source_ip: str, source_port: int) -> dict[str, Any]:
    json_payload: Any = None
    parse_error: str | None = None

    if payload_text:
        try:
            json_payload = json.loads(payload_text)
        except json.JSONDecodeError as exc:
            parse_error = f"{exc.msg} at char {exc.pos}"

    rover_ip = source_ip
    rover_id = None

    if isinstance(json_payload, dict):
        rover_ip = coerce_text(json_payload.get("ip")) or source_ip
        rover_id = (
            coerce_text(json_payload.get("id"))
            or coerce_text(json_payload.get("name"))
            or coerce_text(json_payload.get("rover_id"))
        )

    return {
        "received_at": now_string(),
        "source_ip": source_ip,
        "source_port": source_port,
        "rover_ip": rover_ip,
        "rover_id": rover_id,
        "raw_payload": payload_text,
        "json_payload": json_payload,
        "parse_error": parse_error,
    }


def print_announcement(record: dict[str, Any]) -> None:
    print()
    print("=" * 64)
    print(f"[{record['received_at']}] anuncio recibido")
    print(f"IP del rover: {record['rover_ip']}")
    if record["rover_id"]:
        print(f"ID del rover: {record['rover_id']}")
    print(f"IP origen UDP: {record['source_ip']}:{record['source_port']}")
    if record["parse_error"]:
        print(f"Nota: payload no parseable como JSON ({record['parse_error']})")
    if record["raw_payload"]:
        print(f"Payload: {record['raw_payload']}")
    else:
        print("Payload: <vacio>")
    print("=" * 64)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Listen for rover UDP announcements and print the discovered rover IP."
    )
    parser.add_argument("--udp-host", default=DEFAULT_UDP_HOST, help="UDP bind host. Default: %(default)s")
    parser.add_argument("--udp-port", type=int, default=DEFAULT_UDP_PORT, help="UDP bind port. Default: %(default)s")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.bind((args.udp_host, args.udp_port))

    print("Rover Discovery Helper")
    print(f"Escuchando anuncios UDP en {args.udp_host}:{args.udp_port}")
    print("Deja esta ventana abierta. Cierra la ventana o presiona Ctrl+C para terminar.")

    last_signature: tuple[str | None, str] | None = None

    try:
        while True:
            data, address = sock.recvfrom(MAX_PACKET_SIZE)
            payload_text = data.decode("utf-8", errors="replace").strip()
            record = parse_announcement(payload_text, address[0], address[1])
            signature = (record["rover_ip"], record["raw_payload"])

            if signature == last_signature:
                continue

            last_signature = signature
            print_announcement(record)
    except KeyboardInterrupt:
        print()
        print("[SYS] cierre solicitado")
    finally:
        sock.close()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
