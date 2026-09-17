# tarea09 - Sockets SSL/TLS IPv6 (fork / hilos)

CI0123 Proyecto Integrador de Redes y Sistemas Operativos - 2026-ii

## Qué hace

Es la misma funcionalidad SSL/TLS de la versión IPv4, pero usando sockets
**IPv6** (`AF_INET6`). Reutiliza las mismas clases `VSocket`, `Socket` y
`SSLSocket` (esta última no necesitó ningún cambio, ya soportaba IPv6). Hay
dos versiones del servidor:

- **`SSLForkServer6`**: crea un proceso hijo (`fork`) por cada cliente.
- **`SSLThreadServer6`**: crea un hilo (`std::thread`) por cada cliente.

`SSLClient6` es el cliente de prueba: se conecta por IPv6, hace el
handshake TLS, manda un mensaje y muestra la respuesta.

## Compilar

Primero se necesita un certificado autofirmado (una sola vez, es el mismo
para IPv4 e IPv6):

```bash
make cert
```

Esto genera `ci0123.pem` (certificado) y `key0123.pem` (llave privada).

```bash
make
```

Genera `SSLForkServer6.out`, `SSLThreadServer6.out` y `SSLClient6.out`.

```bash
make clean
```

Borra los binarios y objetos compilados (no borra los `.pem`).

## Ejecutar

En una terminal, levantar **uno** de los dos servidores:

```bash
./SSLForkServer6.out 4433
# o
./SSLThreadServer6.out 4433
```

En otra terminal, el cliente (`::1` es el equivalente IPv6 de `127.0.0.1`,
o sea localhost):

```bash
./SSLClient6.out ::1 4433 "hola mundo"
```

**Notas:**
- En Ubuntu hace falta `sudo apt install libssl-dev` antes de compilar.
- Si la IPv6 del servidor es *link-local* (empieza con `fe80::`), hay que
  indicar también la interfaz de red, por ejemplo `fe80::1234%eth0`.