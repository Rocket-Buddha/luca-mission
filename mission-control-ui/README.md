# Mission Control UI

Panel local de `Mision Luca` separado del firmware del robot.

## Uso

1. Conectar la notebook/PC al mismo Wi-Fi que usa el rover (`Personal-F0C`).
2. Ejecutar `rover-discovery-helper` y anotar la IP mostrada del rover.
3. Abrir [index.html](./index.html) con `?rover=<ip-del-rover>`.

Ejemplo:

- `file:///C:/Users/andre/Desktop/QD002-flash-listo/mission-control-ui/index.html?rover=192.168.1.34`

Si el navegador se pone estricto con `file://`, servir esta carpeta por `localhost`:

```powershell
cd "C:\Users\andre\Desktop\QD002-flash-listo\mission-control-ui"
py -m http.server 8080
```

Y abrir:

- `http://localhost:8080/?rover=192.168.1.34`

La UI recuerda el ultimo `?rover=` usado y lo guarda en `localStorage`.
Si no le pasas `?rover=` y no hay IP guardada, te pide la IP o hostname del rover al abrir.

## Endpoints del rover

- control: `http://<ip-del-rover>/control`
- stream: `http://<ip-del-rover>:81/stream`
- capture: `http://<ip-del-rover>:81/capture`
- telemetry SSE: `http://<ip-del-rover>:82/telemetry`

## Nota

El firmware ya no sirve la UI HTML ni el logo. Esta carpeta es la fuente oficial del panel.
