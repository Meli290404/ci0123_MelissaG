# TC07 - Sockets TCP IPv6 (fork / hilos)

CI0123 Proyecto Integrador de Redes y Sistemas Operativos - 2026-ii

## Qué hace

Es la misma funcionalidad de la tarea de IPv4, pero usando sockets **IPv6**
(`AF_INET6`). Reutiliza las mismas clases `VSocket` y `Socket`, que ahora
arman la estructura de dirección correcta (`sockaddr_in6`) según la bandera
IPv6 del socket. El servidor sigue siendo un "mirror": recibe un mensaje del
cliente y se lo devuelve tal cual. Hay dos versiones del servidor:

- **`ForkMirrorServer6`**: crea un proceso hijo (`fork`) por cada cliente.
- **`ThreadMirrorServer6`**: crea un hilo (`std::thread`) por cada cliente.

`MirrorClient6` es el cliente que se conecta, manda un mensaje y muestra la
respuesta.

## Compilar

```bash
make
```

Genera `ForkMirrorServer6.out`, `ThreadMirrorServer6.out` y
`MirrorClient6.out`.

```bash
make clean
```

Borra los binarios y objetos compilados.

## Ejecutar

En una terminal, levantar **uno** de los dos servidores (quedan esperando
conexiones, se detienen con `Ctrl+C`):

```bash
./ForkMirrorServer6.out
# o
./ThreadMirrorServer6.out
```

En otra terminal, correr el cliente (el mensaje entre comillas es opcional):

```bash
./MirrorClient6.out "hola mundo"
```

Por defecto usan el puerto `2026` y `::1` (el equivalente IPv6 de
`127.0.0.1`, o sea localhost). Si se va a probar entre dos computadoras,
cambiar la dirección en `MirrorClient6.cc` por la IPv6 real del equipo donde
corre el servidor y recompilar.

**Nota:** si la IPv6 del servidor es *link-local* (empieza con `fe80::`),
hay que indicarle también la interfaz de red, por ejemplo `fe80::1234%eth0`,
porque una dirección link-local sola no le dice al sistema por cuál interfaz
salir.
