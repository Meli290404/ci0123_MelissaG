# TC07 - Sockets TCP (fork / hilos)

## Qué hace

Implementa las clases `VSocket` (base) y `Socket` (TCP/IPv4) para armar un
servidor tipo "mirror": recibe un mensaje del cliente y se lo devuelve tal
cual. Hay dos versiones del servidor, cada una atendiendo cada conexión de
forma distinta:

- **`ForkMirrorServer`**: crea un proceso hijo (`fork`) por cada cliente.
- **`ThreadMirrorServer`**: crea un hilo (`std::thread`) por cada cliente.

`MirrorClient` es el cliente que se conecta, manda un mensaje y muestra la
respuesta.

## Compilar

```bash
make
```

Genera `ForkMirrorServer.out`, `ThreadMirrorServer.out` y `MirrorClient.out`.

```bash
make clean
```

Borra los binarios y objetos compilados.

## Ejecutar

En una terminal, levantar **uno** de los dos servidores (quedan esperando
conexiones, se detienen con `Ctrl+C`):

```bash
./ForkMirrorServer.out
# o
./ThreadMirrorServer.out
```

En otra terminal, correr el cliente (el mensaje entre comillas es opcional):

```bash
./MirrorClient.out "hola mundo"
```

Por defecto usan el puerto `2026` en `127.0.0.1`. Si se va a probar entre dos
computadoras, cambiar la IP en `MirrorClient.cc` por la del equipo donde
corre el servidor y recompilar.
