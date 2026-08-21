# TC02 - UDP sobre IPv4

Instrucciones para compilar y probar el cliente/servidor UDP en IPv4.

## Archivos necesarios

- VSocket.h, VSocket.cc
- Socket.h, Socket.cc
- ipv4-udp-client.cc
- ipv4-udp-server.cc
- Makefile

## 1. Compilar

```
make clean
make
```

## 2. Direccion IP del cliente

En ipv4-udp-client.cc se usa la IP 127.0.0.1 para probar en la propia
maquina. Queda comentada la IP del servidor de Python del laboratorio
(10.1.35.50) por si se necesita usar mas adelante.

## 3. Ejecutar

Abrir dos terminales en la carpeta del proyecto.

Terminal 1 (servidor primero):

```
./ipv4-udp-server.out
```

Queda esperando (bloqueado en recvFrom) hasta que le llegue un mensaje.

Terminal 2 (cliente):

```
./ipv4-udp-client.out
```

## 4. Salida esperada

Cliente:

```
Client: Hello message sent.
Client message received: Hello 2026-ii from CI0123 server
```

Servidor:

```
Server: message received: Hello 2026-ii from CI0123 client
Server: Hello message sent.
```

## 5. (Opcional) Ver los paquetes viajando

En una tercera terminal, antes de correr el cliente:

```
sudo tcpdump -i lo -n udp port 1234
```

Deberian verse dos datagramas: uno del cliente al puerto 1234, y la
respuesta del servidor de vuelta al puerto efimero del cliente.