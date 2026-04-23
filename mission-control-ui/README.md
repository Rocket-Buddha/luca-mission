# Mission Control UI

Panel local de `Mision Luca` separado del firmware del robot.

## Uso

1. Conectarse al Wi-Fi del rover `ESP32-Car`.
2. Abrir [index.html](./index.html) localmente.

Si el navegador se pone estricto con `file://`, servir esta carpeta por `localhost`:

```powershell
cd "C:\Users\andre\Desktop\QD002-flash-listo\mission-control-ui"
py -m http.server 8080
```

Y abrir:

- `http://localhost:8080`

## Endpoints del rover

- control: `http://192.168.4.1/control`
- stream: `http://192.168.4.1:81/stream`
- capture: `http://192.168.4.1:81/capture`
- telemetry SSE: `http://192.168.4.1:82/telemetry`

## Nota

El firmware ya no sirve la UI HTML ni el logo. Esta carpeta es la fuente oficial del panel.
