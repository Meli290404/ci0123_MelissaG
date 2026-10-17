# TicAmazon — Servidor de productos, Intermediario y Cliente

Este módulo tiene cuatro programas y un script:

- **`servidorProductos`**: es el único que abre `cafeteria.dat`. Atiende al intermediario con el protocolo interno de la isla (mensajes 10, 20 y 50) por TCP en el puerto 9091 y lee las bodegas del archivo en cada mensaje. Cada solicitud se atiende en un hilo nuevo.
- **`intermediario`**: servidor HTTP en el puerto 8080. Atiende al cliente y al navegador, le pide los datos al servidor de productos y guarda el carrito de cada sesión. La factura se arma con lo ya reservado, sin volver a consultar la bodega.
- **`cliente`**: cliente de línea de comandos. Pide un número de sesión al intermediario y le hace peticiones HTTP (`/categorias`, `/productos`, `/agregar`, `/factura`).
- **`adminBodegas`**: programa independiente para crear el archivo y agregar o eliminar bodegas, categorías y productos.
- **`cargarInventario.sh`**: crea `cafeteria.dat` con 2 bodegas, 13 categorías y 104 productos, usando `adminBodegas`.

```
cliente / navegador --HTTP :8080--> intermediario --TCP :9091--> servidorProductos --> cafeteria.dat
```

## Compilar

Parado en esta carpeta:

```
make
```

Genera cuatro binarios: `servidorProductos`, `intermediario`, `cliente` y `adminBodegas`.

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

## Terminar y limpiar

`Ctrl+C` en la terminal del intermediario y en la del servidor de productos. Después:

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