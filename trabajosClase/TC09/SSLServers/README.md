# TC09 - Sockets SSL/TLS IPv4 (fork / hilos)

CI0123 Proyecto Integrador de Redes y Sistemas Operativos - 2026-ii

## Qué hace

Agrega la clase `SSLSocket` (deriva de `VSocket`) para armar un servidor
"mirror" que habla TCP IPv4 cifrado con SSL/TLS en vez de texto plano. Hay
dos versiones del servidor:

- **`SSLForkServer`**: crea un proceso hijo (`fork`) por cada cliente.
- **`SSLThreadServer`**: crea un hilo (`std::thread`) por cada cliente.

`SSLClient` es el cliente de prueba: se conecta, hace el handshake TLS,
manda un mensaje y muestra la respuesta (además del cifrado usado y el
certificado del servidor).

## Compilar

Primero se necesita un certificado autofirmado (una sola vez):

```bash
make cert
```

Esto genera `ci0123.pem` (certificado) y `key0123.pem` (llave privada).

```bash
make
```

Genera `SSLForkServer.out`, `SSLThreadServer.out` y `SSLClient.out`.

```bash
make clean
```

Borra los binarios y objetos compilados (no borra los `.pem`).

## Ejecutar

En una terminal, levantar **uno** de los dos servidores:

```bash
./SSLForkServer.out 4433
# o
./SSLThreadServer.out 4433
```

En otra terminal, el cliente:

```bash
./SSLClient.out 127.0.0.1 4433 "hola mundo"
```

**Nota:** en Ubuntu hace falta `sudo apt install libssl-dev` antes de
compilar, o va a tirar `fatal error: openssl/ssl.h: No such file or
directory`.