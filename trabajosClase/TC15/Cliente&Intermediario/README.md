# TicAmazon — Servidor de productos, Intermediario y Cliente

Este módulo tiene cuatro programas y un script:

- **`servidorProductos`**: es el único que abre `cafeteria.dat`. Atiende al intermediario con el protocolo interno de la isla (mensajes 10, 20 y 50) por TCP en el puerto 9091 y lee las bodegas del archivo en cada mensaje. Cada solicitud se atiende en un hilo nuevo.
- **`intermediario`**: servidor HTTP en el puerto 8080. Atiende al cliente y al navegador, le pide los datos al servidor de productos y guarda el carrito de cada sesión. La factura se arma con lo ya reservado, sin volver a consultar la bodega.
- **`cliente`**: cliente de línea de comandos. Pide un número de sesión al intermediario y le hace peticiones HTTP (`/categorias`, `/productos`, `/agregar`, `/factura`).
- **`adminBodegas`**: programa independiente para crear el archivo y agregar o eliminar bodegas, categorías y productos.
- **`pruebaBroadcast`**: programa de prueba para los métodos de broadcast de `VSocket`. Envía y recibe mensajes UDP dirigidos a la dirección broadcast de una red.
- **`cargarInventario.sh`**: crea `cafeteria.dat` con 2 bodegas, 13 categorías y 104 productos, usando `adminBodegas`.

```
cliente / navegador --HTTP :8080--> intermediario --TCP :9091--> servidorProductos --> cafeteria.dat
```

## Compilar

Parado en esta carpeta:

```
make
```

Genera cinco binarios: `servidorProductos`, `intermediario`, `cliente`, `adminBodegas` y `pruebaBroadcast`.

## Preparar el inventario

```
rm -f cafeteria.dat
./cargarInventario.sh
```

Debe terminar con `listo: cafeteria.dat con dos bodegas` y `bloques libres: 993 de 1024`. El script no sobrescribe un `cafeteria.dat` que ya exista.

Si no se corre el script, `servidorProductos` crea `cafeteria.dat` al arrancar con una carga pequeña de ejemplo (2 bodegas y 7 productos). Si el archivo ya existe y es válido, lo usa tal cual.

## Buscar la IP

Los programas no aceptan `127.0.0.1` ni `localhost`: hay que usar la IP de la computadora.

```
ip -4 addr
```

La IP es el número después de `inet` en la interfaz de red (por ejemplo `wlp3s0`), sin el `/24`. No es el número después de `brd`, que es la dirección de broadcast.

En los siguientes pasos se usa `<ip>`.

## Ejecutar

Se necesitan tres terminales abiertas en esta carpeta. Las dos primeras no se cierran mientras se prueba; ahí van apareciendo los mensajes de cada solicitud.

**1. Levantar el servidor de productos** (primera terminal):

```
./servidorProductos <ip>
```

Muestra `[INT -> SERV]` y `[SERV -> INT]` por cada mensaje del protocolo interno.

**2. Levantar el intermediario** (segunda terminal):

```
./intermediario <ip> <ip>
```

La primera IP es donde escucha a los clientes y la segunda es la del servidor de productos. En una sola computadora son la misma; en el laboratorio serán la de la red pública y la de la red privada.

**3. Correr el cliente** (tercera terminal):

```
./cliente <ip>
```

Al arrancar debe mostrar `sesion 1`. Comandos disponibles:

```
CATEGORIAS
<idCategoria>            (ej. cafes, bebfr, bebca, ensal, postr, caram, helad)
AGREGAR <nombre> <cantidad>
FACTURA
salir
```

Ejemplo de sesión:

```
> CATEGORIAS
> ensal
> AGREGAR Ensalada-Caprese 2
> AGREGAR Bombones 3
> AGREGAR Banana-Split 20
> FACTURA
```

`AGREGAR` reserva el producto en el servidor de productos y descuenta el stock de `cafeteria.dat`. Si no hay suficiente, reserva lo que haya (en el ejemplo, 10 Banana-Split de los 20 pedidos). La factura del ejemplo da `TOTAL: 45300.00`.

Se pueden abrir varias terminales con `cliente` a la vez contra el mismo `intermediario`: cada una recibe su propio número de sesión y su propio carrito.

**4. Ver el inventario en el navegador**:

```
http://<ip>:8080/
```

Muestra todas las categorías con sus productos, precios y stock. Desde un teléfono funciona si está en la misma red.

## Administrar las bodegas

`adminBodegas` trabaja directo sobre el archivo, sin pasar por la red:

```
./adminBodegas cafeteria.dat crear <nombreNegocio>
./adminBodegas cafeteria.dat listar
./adminBodegas cafeteria.dat libres
./adminBodegas cafeteria.dat agregar <idBodega> <nombre>
./adminBodegas cafeteria.dat eliminar <idBodega>
./adminBodegas cafeteria.dat categoria <idBodega> <idCategoria> <nombre>
./adminBodegas cafeteria.dat producto <idBodega> <idCategoria> <nombre> <cantidad> <precioCentavos>
./adminBodegas cafeteria.dat descontar <producto> <cantidad>
```

- **`idBodega`**: de 1 a 4 letras o dígitos.
- **`idCategoria`**: de 3 a 5 letras.
- **Nombres de bodega y de categoría**: hasta 15 caracteres, sin `/ , ; | :`.
- **Nombre de producto**: hasta 23 letras, dígitos o guion.
- **Precio**: va en centavos (`150000` es 1500.00). Los nombres con espacios van entre comillas, por ejemplo `"Bebidas frias"`.

Para ver el archivo por dentro:

```
hexdump -C cafeteria.dat | less
```

## Broadcast en las clases de sockets

`VSocket` tiene cuatro métodos nuevos para trabajar con direcciones broadcast. Solo sirven con sockets de datagramas (`Socket('d')`) en IPv4, porque broadcast no existe en TCP ni en IPv6.

- **`EnableBroadcast()`**: activa en el socket la opción de broadcast. Sin esto, Linux no deja enviar a una dirección broadcast y `sendTo` falla con `Permission denied`.
- **`sendTo(buffer, tamaño, ip, puerto)`**: envía un datagrama a una IP y puerto escritos como texto, por ejemplo `"172.16.123.79"` y `9090`.
- **`recvFrom(buffer, tamaño, ipOrigen, puertoOrigen)`**: recibe un datagrama y deja en `ipOrigen` y `puertoOrigen` los datos de quien lo envió. Con broadcast no se sabe de antemano quién va a enviar, así se puede contestarle directo.
- **`VSocket::BroadcastAddress(ip, prefijo)`**: calcula la dirección broadcast de una red. Por ejemplo, `("172.16.123.67", 28)` devuelve `172.16.123.79`.

El receptor usa el `Bind(ip, puerto)` que ya existía, pero con la dirección broadcast de la red y no con su propia IP. Si se hace bind a la IP propia, Linux no le entrega los mensajes enviados a la broadcast.

Direcciones broadcast de las redes que se usan:

```
isla 4     172.16.123.64/28   ->  172.16.123.79
isla 3     172.16.123.48/28   ->  172.16.123.63
pública    10.1.35.0/24       ->  10.1.35.255
```

### Probar el broadcast

Calcular la dirección broadcast de una red (el prefijo es el número después de la `/` en `ip -4 addr`):

```
./pruebaBroadcast calcular 172.16.123.67 28
```

Debe mostrar `172.16.123.79`.

**1. Levantar el receptor** (primera terminal, o en otra computadora de la isla):

```
./pruebaBroadcast receptor 172.16.123.79 9090
```

Muestra `[receptor] esperando en 172.16.123.79:9090` y se queda esperando.

**2. Enviar un mensaje** (segunda terminal):

```
./pruebaBroadcast emisor 172.16.123.79 9090 "60/INT_04/172.16.123.67"
```

El emisor muestra `[emisor] 23 bytes enviados a 172.16.123.79:9090` y el receptor:

```
[receptor] de 172.16.123.67:51032 -> 60/INT_04/172.16.123.67
```

La IP es la de la computadora que envió y el puerto lo escoge el sistema en cada envío, así que puede ser otro número.

Se pueden levantar varios receptores en el mismo puerto, en la misma o en distintas computadoras de la isla: un solo envío le llega a todos.

Para probar en una sola computadora fuera del laboratorio, se usa la broadcast de la red de la casa, por ejemplo `192.168.100.255` si la IP es `192.168.100.14/24`.

## Terminar y limpiar

`Ctrl+C` en la terminal del intermediario, en la del servidor de productos y en la del receptor de broadcast. Después:

```
make clean
```

Los carritos viven en la memoria del intermediario, así que se pierden si se reinicia. El stock reservado ya quedó descontado en `cafeteria.dat`. Para volver al inventario completo:

```
rm -f cafeteria.dat
./cargarInventario.sh
```

## Problemas comunes

- **`no se permite loopback ni INADDR_ANY`**: se usó `127.0.0.1`. Usar la IP de `inet`.
- **El cliente dice `no se pudo conectar con el intermediario`**: confirmar que `intermediario` está corriendo y que se usó la misma IP en los dos programas. Revisar también que no sea la IP de `brd`.
- **El cliente dice `el servidor de productos no responde (2 reintentos)`**: `servidorProductos` no está corriendo o la segunda IP del intermediario no es la suya. En cuanto se levanta, el intermediario vuelve a funcionar sin reiniciarlo.
- **`Address already in use`**: ya hay otro `servidorProductos` o `intermediario` corriendo en ese puerto. Cerrarlo o buscarlo con `ss -ltnp | grep -E "8080|9091"`.
- **Un comando escrito no hace nada**: se escribió en la terminal donde corre un servidor. Abrir otra terminal.
- **La IP cambió**: pasa al cambiar de red WiFi. Volver a revisar con `ip -4 addr`.
- **El teléfono no carga la página**: tiene que estar en la misma red. Si el firewall está activo (`sudo ufw status`), abrir el puerto con `sudo ufw allow 8080/tcp`.
- **`cargarInventario.sh` dice que el archivo ya existe**: borrarlo antes con `rm -f cafeteria.dat`.
- **El receptor de broadcast no recibe nada**: revisar que el receptor use la dirección broadcast (la de `brd`) y no su propia IP, y que emisor y receptor usen el mismo puerto. Si el firewall está activo, abrir el puerto con `sudo ufw allow 9090/udp`.
- **El broadcast no llega a otra isla**: enviar a la broadcast de otra red (por ejemplo `172.16.123.63`) solo funciona si el switch L3 tiene `ip directed-broadcast` en la interfaz VLAN de esa isla. Cisco lo bloquea por defecto.
- **`sendTo error: Permission denied`**: el socket no llamó a `EnableBroadcast()` antes de enviar.