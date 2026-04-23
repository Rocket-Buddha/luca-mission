# AGENTS

## Proposito

Esta carpeta es el paquete limpio para flashear el QD002 con 2 placas:

- `esp32-camera`: placa de la camara
- `esp32-body`: placa del cuerpo

No mezclar estos proyectos con los folders originales del tutorial, especialmente con `Leccion 8\\acebott-esp32-car-camera`, porque ese source original venia contaminado con codigo del body.

## Organizacion del paquete

La raiz de esta carpeta debe quedar reservada para los proyectos principales y la documentacion operativa.

Ubicaciones auxiliares ya definidas:

- `mission-control-ui\index.html`: panel local de Mision Luca
- `mission-control-ui\mission-logo-transparent.png`: asset local del logo del panel
- `probes\esp32-camera-dhcp-probe`: sketch de prueba para AP/DHCP de la camara
- `probes\esp32-body-motor-probe`: sketch de prueba del shield de motores
- `scripts\monitor-com5.ps1`: script de monitoreo serie para la placa body
- `scripts\monitor-com6.ps1`: script de monitoreo serie para la placa camara
- `logs\body-serial-log.txt`: salida del monitor serie de la placa body
- `logs\cam-serial-log.txt`: salida del monitor serie de la placa camara
- `wifi-profiles\esp32-car-open.xml`: perfil Wi-Fi abierto para `ESP32-Car`
- `wifi-profiles\esp32-car-wifi.xml`: perfil Wi-Fi WPA2 para `ESP32-Car`
- `wifi-profiles\esp32-car-dhcp-open.xml`: perfil Wi-Fi abierto para `ESP32-Car-DHCP`

Regla de orden:

- nuevos sketches de prueba -> `probes`
- panel web local y assets asociados -> `mission-control-ui`
- scripts auxiliares de soporte u operacion -> `scripts`
- logs o capturas de monitor serie -> `logs`
- perfiles Wi-Fi o XML auxiliares de Windows -> `wifi-profiles`
- no dejar este tipo de archivos sueltos en el root

## Estado validado

Los 2 proyectos compilan en esta maquina con:

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32 --warnings all .
```

## Mapeo de hardware

- `esp32-camera` -> modulo de camara basado en `ESP32-WROOM-32E`
- `esp32-body` -> placa del cuerpo `ESP-32 MAX v1.0`

## Identificacion de placas

- Camara: `ESP32-WROOM-32E`
- Body: `ESP-32 MAX v1.0`
- Shield de motores: `ACEBOTT-ESP32-Car-Shield-v1.1`

Cableado serie entre placas, tomado del PDF local:

- `camera pin 14 -> body TX`
- `camera pin 13 -> body RX`

## Regla operativa importante

Al grabar la placa `body`, primero hay que desconectar los cables `TX/RX` que la unen con la camara, y reconectarlos cuando termina el upload.

## Board/FQBN

El tutorial local indica `ESP32 Dev Module` para ambos proyectos.

Usar:

- Board IDE: `ESP32 Dev Module`
- FQBN CLI: `esp32:esp32:esp32`

## Dependencias

### `esp32-camera`

- Solo core `esp32`

### `esp32-body`

- `ESP32Servo`
- `EspSoftwareSerial`

`ACB_SmartCar_V2` y `ultrasonic` ya estan locales dentro del proyecto.

## Notas de mantenimiento

- Si se toca `esp32-camera/app_httpd.cpp`, mantener compatibilidad con el core `esp32` actual.
- `esp32-camera` debe quedar como backend del rover. No volver a embutir la UI HTML completa dentro del firmware.
- La UI oficial vive en `mission-control-ui`.
- Si se toca `esp32-body`, conservar los includes locales:
  - `ACB_SmartCar_V2.h`
  - `ultrasonic.h`
- No volver a depender de la libreria global `ultrasonic`, porque fue la causa de errores de API (`Init`, `Ranging`).
- El shield de motores no usa UART para mover ruedas. La referencia oficial del shield usa `DATA=5`, `SHCP=18`, `STCP=17`, `EN=16` y PWM en `19`; el ejemplo oficial tambien inicializa `23` como salida.

## Comandos utiles

### Camera

```powershell
cd "C:\Users\andre\Desktop\QD002-flash-listo\esp32-camera"
arduino-cli compile --fqbn esp32:esp32:esp32 --warnings all .
arduino-cli upload --fqbn esp32:esp32:esp32 -p COM6 .
```

### Body

```powershell
cd "C:\Users\andre\Desktop\QD002-flash-listo\esp32-body"
arduino-cli compile --fqbn esp32:esp32:esp32 --warnings all .
arduino-cli upload --fqbn esp32:esp32:esp32 -p COMx .
```

### Monitoreo serie

Scripts disponibles:

- `scripts\monitor-com5.ps1` -> guarda en `logs\body-serial-log.txt`
- `scripts\monitor-com6.ps1` -> guarda en `logs\cam-serial-log.txt`
