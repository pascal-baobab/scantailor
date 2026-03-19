# Telegram Bot API — DPI Bypass Workaround

## Problema

Alcuni firewall (es. MizarFW/Zyxel) bloccano le connessioni a `api.telegram.org` tramite **Deep Packet Inspection (DPI)** del campo **SNI** nel TLS ClientHello.

**Sintomi:**
- `ping api.telegram.org` -> OK (ICMP non bloccato)
- TCP connect a porta 443 -> OK (SYN/ACK in ~30ms)
- TLS handshake con SNI `api.telegram.org` -> **TIMEOUT**
- TLS handshake senza SNI -> OK
- `curl https://api.telegram.org/...` -> timeout
- `curl https://www.google.com` -> funziona

**Diagnosi rapida (Python):**
```python
import socket, ssl, time

ip = '149.154.167.220'  # Telegram DC2
s = socket.socket(); s.settimeout(8); s.connect((ip, 443))
ctx = ssl.create_default_context()
ctx.check_hostname = False; ctx.verify_mode = ssl.CERT_NONE

# CON SNI -> timeout
try:
    ss = ctx.wrap_socket(s, server_hostname='api.telegram.org')
    print("SNI: OK")
except: print("SNI: BLOCKED")

# SENZA SNI -> OK
s2 = socket.socket(); s2.settimeout(8); s2.connect((ip, 443))
try:
    ss = ctx.wrap_socket(s2, server_hostname=None)
    print("No SNI: OK")
except: print("No SNI: BLOCKED")
```

## Soluzione

Connettere direttamente all'IP del server Telegram senza inviare SNI:

### Per python-telegram-bot v22+

```python
from telegram.request import HTTPXRequest
from telegram.ext import ApplicationBuilder

TELEGRAM_IP = "149.154.167.220"  # Telegram DC2 (Amsterdam)

request = HTTPXRequest(
    httpx_kwargs={
        "verify": False,
        "headers": {"Host": "api.telegram.org"},
    },
)

app = (
    ApplicationBuilder()
    .token("YOUR_BOT_TOKEN")
    .base_url(f"https://{TELEGRAM_IP}/bot")
    .base_file_url(f"https://{TELEGRAM_IP}/file/bot")
    .request(request)
    .get_updates_request(HTTPXRequest(
        httpx_kwargs={
            "verify": False,
            "headers": {"Host": "api.telegram.org"},
        },
    ))
    .build()
)
```

### Per httpx (diretto)

```python
import httpx

client = httpx.AsyncClient(
    base_url=f"https://149.154.167.220",
    verify=False,
    headers={"Host": "api.telegram.org"},
    timeout=10.0,
)
resp = await client.get("/botTOKEN/getMe")
```

### Per urllib (stdlib)

```python
import socket, ssl, json

ip, host = '149.154.167.220', 'api.telegram.org'
s = socket.socket(); s.settimeout(10); s.connect((ip, 443))
ctx = ssl.create_default_context()
ctx.check_hostname = False; ctx.verify_mode = ssl.CERT_NONE
ss = ctx.wrap_socket(s, server_hostname=None)  # No SNI!

request = f'GET /botTOKEN/getMe HTTP/1.1\r\nHost: {host}\r\nConnection: close\r\n\r\n'
ss.send(request.encode())
data = b''
while True:
    chunk = ss.recv(4096)
    if not chunk: break
    data += chunk
body = data.decode().split('\r\n\r\n', 1)[1]
print(json.loads(body))
```

## Perche funziona

1. **IP diretto** (149.154.167.220): bypassa il DNS locale che potrebbe non risolvere `api.telegram.org`
2. **Niente SNI**: httpx non invia SNI quando l'host e' un IP address, quindi il firewall non vede `api.telegram.org` nel TLS handshake
3. **Header Host**: il server Telegram ha bisogno di `Host: api.telegram.org` nell'HTTP request per servire l'API correttamente (altrimenti ritorna 302)
4. **verify=False**: necessario perche il certificato SSL e' emesso per `api.telegram.org`, non per l'IP

## IP Telegram Bot API

| Datacenter | IP | Localita |
|---|---|---|
| DC1 | 149.154.167.198 | (puo non funzionare) |
| DC2 | 149.154.167.220 | Amsterdam (consigliato) |
| DC4 | 149.154.167.91 | Amsterdam |
| DC5 | 91.108.56.130 | Amsterdam |

## Soluzione permanente (admin)

Chiedere all'admin di rete di:
- Aggiungere `api.telegram.org` alla whitelist SNI del DPI
- Oppure: permettere TCP/443 verso 149.154.160.0/20

Vedi `ISTRUZIONI_ADMIN_FIREWALL.txt` per le istruzioni complete.
