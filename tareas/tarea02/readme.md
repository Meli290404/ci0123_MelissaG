# tarea02 - UDP sobre IPv6

Instrucciones para compilar y probar el cliente/servidor UDP en IPv6.

## Archivos necesarios

- VSocket.h, VSocket.cc
- Socket.h, Socket.cc
- ipv6-udp-client.cc
- ipv6-udp-server.cc
- Makefile

## 1. Correccion en ipv6-udp-server.cc

La variable other debe ser del tipo sockaddr_in6 (28 bytes), no del
tipo generico sockaddr (16 bytes), ya que recvFrom necesita espacio
suficiente para una direccion IPv6 completa.

```
struct sockaddr_in6 other;
```

## 2. Direccion IP del cliente

En ipv6-udp-client.cc se dejo other.sin6_addr en in6addr_loopback
(::1) para probar en la propia maquina, en vez de in6addr_any.

## 3. Compilar

```
make clean
make
```

## 4. Ejecutar

No hace falta el laboratorio: se puede probar completo en la misma
maquina usando el loopback de IPv6 (::1).

Abrir dos terminales en la carpeta del proyecto.

Terminal 1 (servidor primero):

```
./ipv6-udp-server.out
```

Terminal 2 (cliente):

```
./ipv6-udp-client.out
```

## 5. Salida esperada

Cliente:

```
Client: Hello message sent.
Client message received: Hello from CI0123 server 2026-ii
```

Servidor:

```
Server: message received: Hello from client 2026-ii
Server: Hello message sent.
```

## 6. (Opcional) Ver los paquetes viajando

En una tercera terminal, antes de correr el cliente:

```
sudo tcpdump -i lo -n ip6 and udp port 1234
```

Deberian verse dos datagramas entre direcciones ::1.