# Rover Discovery Helper

Helper local para recibir anuncios UDP del rover y mostrar en pantalla la IP descubierta.

## Objetivo

Este proyecto escucha un puerto UDP local y, cuando llega un anuncio del rover, muestra la IP en la ventana:

1. escucha un puerto UDP local
2. toma la IP de origen del paquete
3. si el JSON incluye `ip`, la usa; si no, usa la IP origen del paquete
4. imprime la IP del rover en pantalla

## Requisitos

- Windows con Python 3.13 o similar
- Firewall permitiendo el puerto UDP configurado

No usa dependencias externas.

## Ejecutar

Desde esta carpeta:

```powershell
cd "C:\Users\andre\Desktop\QD002-flash-listo\rover-discovery-helper"
py .\discovery_helper.py
```

Tambien se puede iniciar con doble click en:

- `iniciar-rover-discovery-helper.cmd`

Puertos por defecto:

- UDP listener: `0.0.0.0:4210`

Parametros opcionales:

```powershell
py .\discovery_helper.py --udp-port 49000
```

El launcher `.cmd` tambien reenvia argumentos si lo queres correr desde consola:

```powershell
.\iniciar-rover-discovery-helper.cmd --udp-port 49000
```

## Formato recomendado del paquete UDP

El helper acepta texto plano o JSON. Si el paquete no incluye `ip`, usa igual la IP de origen del socket UDP.

Payload recomendado:

```json
{
  "id": "uca-rover",
  "stream_port": 81,
  "telemetry_port": 82
}
```

Payload mas completo:

```json
{
  "id": "uca-rover",
  "ip": "192.168.1.34",
  "ports": {
    "http": 80,
    "stream": 81,
    "telemetry": 82
  }
}
```

## Prueba manual desde Windows

Esto simula un anuncio UDP local:

```powershell
$payload = '{"id":"uca-rover","stream_port":81,"telemetry_port":82}'
$udp = [System.Net.Sockets.UdpClient]::new()
$bytes = [System.Text.Encoding]::UTF8.GetBytes($payload)
$udp.Send($bytes, $bytes.Length, "127.0.0.1", 4210) | Out-Null
$udp.Dispose()
```

La ventana del helper deberia mostrar algo como:

```text
================================================================
[2026-04-29 23:40:12] anuncio recibido
IP del rover: 192.168.1.34
ID del rover: uca-rover
IP origen UDP: 192.168.1.34:4210
Payload: {"id":"uca-rover","stream_port":81,"telemetry_port":82}
================================================================
```
