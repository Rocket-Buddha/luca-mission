# QD002 listo para grabar

Este paquete deja separado el firmware de las 2 placas del carro:

- `esp32-camera`: firmware de la placa de la camara
- `esp32-body`: firmware de la placa que maneja motores, servos y sensores del cuerpo
- `mission-control-ui`: panel local del rover, separado del firmware

Los sketches auxiliares de prueba quedaron agrupados en:

- `probes\esp32-camera-dhcp-probe`
- `probes\esp32-body-motor-probe`

Los archivos auxiliares que no forman parte del firmware principal quedaron separados en:

- `mission-control-ui`: UI local para operar el rover desde notebook/PC
- `scripts`: utilidades PowerShell de soporte
- `logs`: capturas de monitor serie
- `wifi-profiles`: perfiles Wi-Fi de Windows para las redes del ESP32

Archivos actuales:

- `scripts\monitor-com5.ps1`: monitor serie de la placa body
- `scripts\monitor-com6.ps1`: monitor serie de la placa camara
- `mission-control-ui\index.html`: panel local de control y video
- `logs\body-serial-log.txt`: generado por `scripts\monitor-com5.ps1`
- `logs\cam-serial-log.txt`: generado por `scripts\monitor-com6.ps1`
- `wifi-profiles\esp32-car-open.xml`: perfil abierto para `ESP32-Car`
- `wifi-profiles\esp32-car-wifi.xml`: perfil WPA2 para `ESP32-Car`
- `wifi-profiles\esp32-car-dhcp-open.xml`: perfil abierto para `ESP32-Car-DHCP`

Los 2 proyectos fueron compilados y validados en esta maquina con `arduino-cli`.

## Placa a usar en Arduino IDE

Segun el tutorial local de ACEBOTT, para ambos proyectos hay que seleccionar:

- `Tools > Board > esp32 > ESP32 Dev Module`

FQBN equivalente en CLI:

- `esp32:esp32:esp32`

## Dependencias

### `esp32-camera`

No requiere librerias extra fuera del core `esp32`.

### `esp32-body`

Requiere estas librerias instaladas en Arduino:

- `ESP32Servo`
- `EspSoftwareSerial`

Comando CLI si hiciera falta:

```powershell
arduino-cli lib install ESP32Servo EspSoftwareSerial
```

Nota:

- `ACB_SmartCar_V2` ya quedo copiada dentro del proyecto.
- `ultrasonic` ya quedo implementada localmente dentro del proyecto.

## Orden recomendado de grabacion

### 1. Grabar la placa de la camara

Proyecto:

- `esp32-camera\esp32-camera.ino`

Comando probado:

```powershell
cd "C:\Users\andre\Desktop\QD002-flash-listo\esp32-camera"
arduino-cli compile --fqbn esp32:esp32:esp32 --warnings all .
arduino-cli upload --fqbn esp32:esp32:esp32 -p COM6 .
```

Notas:

- En esta maquina la camara aparecio en `COM6`.
- Si el puerto cambia, reemplazar `COM6`.
- Despues de grabar, presionar `RESET` en la placa camara.

### 2. Grabar la placa del cuerpo

Proyecto:

- `esp32-body\esp32-body.ino`

Comando probado para compilar:

```powershell
cd "C:\Users\andre\Desktop\QD002-flash-listo\esp32-body"
arduino-cli compile --fqbn esp32:esp32:esp32 --warnings all .
```

Antes de grabar el body:

- Desconectar los 2 cables de comunicacion entre camara y body.

Esto coincide con el material local de ACEBOTT, que indica que al grabar la placa base del carro hay que desconectar primero `TX` y `RX` de la camara y volver a conectarlos al terminar.

El cableado entre placas, segun el PDF local de construccion, es:

- `camara pin 14 -> TX` de la placa body
- `camara pin 13 -> RX` de la placa body

Entonces, para grabar el body:

1. Apagar el carro.
2. Desconectar esos 2 cables `TX/RX` que unen camara y body.
3. Conectar la placa body al USB.
4. Grabar el firmware.
5. Volver a conectar `camara pin 14 -> TX` y `camara pin 13 -> RX`.

Comando de upload:

```powershell
cd "C:\Users\andre\Desktop\QD002-flash-listo\esp32-body"
arduino-cli upload --fqbn esp32:esp32:esp32 -p COMx .
```

Cambiar `COMx` por el puerto real de la placa body.

## Puesta en marcha

Despues de grabar ambas placas:

1. Reconectar `TX/RX` entre camara y body.
2. Encender el carro.
3. La camara crea el WiFi `ESP32-Car`.
4. Clave por defecto: `12345678`.

## Estado del paquete

### `esp32-camera`

- Compila en esta maquina.
- Quedo adaptado para el core `esp32` actual.
- Solo expone servicios del rover: control, video/captura y telemetria.

### `mission-control-ui`

- Vive fuera del firmware.
- Usa los mismos servicios HTTP del rover.
- Es la ubicacion oficial de la UI; no volver a meter HTML grande dentro de `esp32-camera`.

### `esp32-body`

- Compila en esta maquina.
- Quedo desacoplado de la libreria `ultrasonic` equivocada.
- Lleva copia local de `ACB_SmartCar_V2`.

## Importante

No usar como referencia el `acebott-esp32-car-camera` roto de `Lección 8` del paquete original. Ese folder venia mezclado con logica del body. En esta carpeta del escritorio ya quedaron separados los proyectos correctos para cada placa.
