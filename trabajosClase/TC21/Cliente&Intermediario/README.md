# TicAmazon — Servidores de productos, Intermediario y Cliente (TC #18 y Tarea #21)

Este módulo tiene cuatro programas y dos scripts:

- **`servidorProductos`**: cada servidor abre su propio sistema de archivos (`fs1.dat.ci0123`, `fs2.dat.ci0123`, …). Se identifica como `SERV_NN` y atiende al intermediario por TCP en el puerto `9050 + NN` (mensajes 10, 20 y 50). Al arrancar se anuncia por **broadcast UDP** para que el intermediario lo encuentre.
- **`intermediario`**: servidor HTTP en el puerto 8080. Descubre los servidores de productos por **broadcast UDP**, los guarda en su **tabla de rutas** y reparte las consultas entre ellos. Guarda el carrito de cada sesión.
- **`cliente`**: cliente de línea de comandos que le hace peticiones HTTP al intermediario.
- **`adminBodegas`**: programa independiente para crear el archivo y agregar o eliminar bodegas y productos.
- **`cargarInventario.sh`** / **`cargarInventario2.sh`**: crean el archivo de un servidor con sus bodegas.

```
                                          +--TCP :9051--> servidorProductos 1 --> fs1.dat.ci0123
cliente / navegador --HTTP :8080--> intermediario
                                          +--TCP :9052--> servidorProductos 2 --> fs2.dat.ci0123
                         (descubrimiento: UDP broadcast, puertos 9091 y 9092)
```

## Qué cambió para la TC #18

| Requisito | Cómo se cumple |
|---|---|
| El intermediario descubre al servidor por broadcast | El intermediario manda `82` a `<broadcast>:9092` al arrancar y cada 10 s. Cada servidor contesta `83` directo a la IP del intermediario. |
| El servidor descubre al intermediario por broadcast | Al arrancar, el servidor manda `80` a `<broadcast>:9091` (hasta 3 veces). El intermediario lo registra y contesta `81`. |
| UDP con un hilo esperando en `recvFrom` | Cada programa tiene un socket UDP (`Socket('d')`) con `SO_BROADCAST` y un hilo (`escucharServidores` / `escucharIntermediario`) bloqueado en `recvFrom`; las respuestas salen con `sendTo`. |
| Direcciones de broadcast de cada isla | Se calcula sola desde la IP con `broadcastDe()` (en la isla 4 da `172.16.123.79`). También se puede pasar como último argumento. |
| FS en un solo archivo con varias bodegas, directorio y manejo de espacio | `fs.dat.ci0123`: superbloque, mapa de bits, i-nodo del directorio y un i-nodo por bodega, todo en un archivo de 1024 bloques de 256 bytes. |
| Bodegas como archivos de texto, una línea por producto | `Bodega-3,frutas,Banano,100,200.00` |
| **Extra: tabla de rutas** | Clase `TablaRutas`: qué servidores hay, en qué IP y puerto, qué categorías tiene cada uno y si siguen vivos. Se ve en `/rutas` y con el comando `RUTAS`. |

### Mensajes nuevos del descubrimiento (UDP)

Siguen el mismo formato `ORIGEN|DESTINO/TIPO/campos` del protocolo de la isla. Los números 80–83 son los del proyecto grupal.

| Tipo | Quién → quién | Formato | Cuándo |
|---|---|---|---|
| **80** | servidor → broadcast:9091 | `SERV_NN\|BROADCAST/80/<puerto tcp>/<count>/<cat,cat,...>` | El servidor arranca y busca al intermediario |
| **81** | intermediario → servidor:9092 | `INT_04\|SERV_NN/81/<nuevo\|actualizado>` | Confirma que lo agregó a la tabla de rutas |
| **82** | intermediario → broadcast:9092 | `INT_04\|BROADCAST/82` | El intermediario busca servidores (al arrancar y cada 10 s) |
| **83** | servidor → intermediario:9091 | `SERV_NN\|INT_04/83/<puerto tcp>/<count>/<cat,cat,...>` | Respuesta al 82 |
| **72** | servidor → intermediario:9091 | `SERV_NN\|INT_04/72` | El servidor se apaga (`Ctrl+C` o `kill`); si todavía no conoce al intermediario, va por broadcast |

La IP de la ruta se toma de la dirección de donde llegó el datagrama, por eso no viaja en el mensaje.

### Tabla de rutas

Cada fila es un servidor: `id`, `ip`, `puerto`, `categorías`, `último contacto` y estado:

- **activo**: contestó el último 82 (o anunció su 80) hace menos de 30 s.
- **vencido**: lleva más de 30 s sin contestar el 82 (se apagó).
- **sin-respuesta**: el intermediario le escribió por TCP y no contestó. No se usa hasta su próximo 83.

Cuando llega un **72** el servidor se saca de la tabla de una vez, sin esperar los 30 s. Solo se acepta si viene de la misma IP que el servidor tiene en la tabla.

### Reglas de la isla

- **Cada servidor tiene categorías distintas.** Si un servidor anuncia una categoría que ya tiene otro servidor activo, se registra igual, pero el intermediario avisa en pantalla y la ruta de esa categoría se queda con el que llegó primero. En `/rutas` y en `RUTAS` esa fila sale como `repetida (ruta de SERV_NN)`.
- **Un solo intermediario por isla.** Cada servidor se queda con el primer intermediario que le contesta (81 u 82). Si le llega un 82 de otro intermediario con otra IP, avisa una vez y no le contesta. Si el primero pasa 30 s sin mandar el 82, se da por caído y el servidor acepta al nuevo.

Cómo la usa el intermediario:

- **`/categorias`**: le pide el 10 a cada servidor activo y junta las categorías sin repetir.
- **`/productos`**: le pregunta al servidor que tiene la ruta de la categoría. Si no tiene productos, pregunta a los demás (si un producto está en dos servidores suma su stock).
- **`/agregar`**: pide la reserva (50) a un servidor; si no tiene el producto o no le alcanza, le pide lo que falta al siguiente.
- **`/`** (navegador): una sección por servidor.
- **`/rutas`**: la tabla de rutas; se recarga sola cada 2 segundos.

### Cambios al diseño que conviene conocer

1. **El almacenamiento pasó de binario a texto** (lo pide la TC): cada bodega es un archivo de texto con su i-nodo.
2. **El intermediario pasó de un servidor fijo a varios**: antes recibía la IP del servidor por argumento y usaba un solo `ConexionBodega`. Ahora `AtencionCliente` recibe la `TablaRutas` y crea un `ConexionBodega` por consulta, hacia el servidor que toque. `ConexionBodega` no cambió.
3. **El id del servidor ya no es una constante**: `ID_SERVIDOR` se quitó de `Protocolo.h`; ahora sale del número que recibe `servidorProductos` (`idServidor(numero)`) y se le pasa a `AtencionBodega`.
4. **El puerto TCP del servidor cambió** de 9091 a `9050 + NN`, para poder correr varios servidores en una sola computadora. El 9091 ahora es el puerto UDP del intermediario.
5. **`servidorProductos` ya no crea datos de ejemplo**: si el archivo no existe, pide correr el script.

Clases y archivos nuevos: `TablaRutas.h/.cc`, `Descubrimiento.h/.cc`. Métodos nuevos en `VSocket`: `HabilitarBroadcast()` y `ArmarDireccion()`; `Bind(int)` ahora usa `SO_REUSEADDR`.

## Compilar

```
make
```

## Preparar el inventario

Cada servidor de productos tiene su propio archivo:

```
rm -f fs1.dat.ci0123 fs2.dat.ci0123
./cargarInventario.sh  fs1.dat.ci0123     # SERV_01: Bodega-1 y Bodega-2, 13 categorías
./cargarInventario2.sh fs2.dat.ci0123     # SERV_02: Bodega-3 y Bodega-4 (frutas, verduras, heladeria)
```

Si se corre sin argumento, cada script usa su nombre por defecto (`fs1.dat.ci0123` y `fs2.dat.ci0123`). Con un solo servidor se puede usar `fs.dat.ci0123`, que es el nombre por defecto de `servidorProductos`.

## Buscar la IP

Los programas no aceptan `127.0.0.1`:

```
ip -4 addr
```

La IP es el número después de `inet` y el broadcast el número después de `brd`.

## Ejecutar

```
./servidorProductos <numero> <ip> [archivo] [broadcast]
./intermediario <ip clientes> <ip red de servidores> [broadcast]
./cliente <ip del intermediario>
```

**En una sola computadora** (cuatro terminales). El orden no importa; este es el que muestra las dos formas de descubrimiento:

```
./servidorProductos 1 <ip> fs1.dat.ci0123     # arranca solo: manda 80 y nadie contesta
./intermediario <ip> <ip>                     # manda 82 y SERV_01 contesta 83
./servidorProductos 2 <ip> fs2.dat.ci0123     # manda 80 y el intermediario contesta 81
./cliente <ip>
```

**En el laboratorio (isla 4)**: el intermediario recibe la IP pública para los clientes y la IP de la red privada `172.16.123.64/28`; el broadcast sale solo (`172.16.123.79`):

```
./intermediario <ip publica> 172.16.123.6X
./servidorProductos 1 172.16.123.6Y fs1.dat.ci0123
```

Si `broadcastDe` no encuentra la interfaz, se pasa a mano: `./intermediario <ip publica> 172.16.123.6X 172.16.123.79`.

Comandos del cliente:

```
CATEGORIAS
<categoria>
AGREGAR <nombre> <cantidad>
FACTURA
RUTAS
salir
```

Prueba sugerida con los dos servidores:

```
> RUTAS                       (SERV_01 y SERV_02 activos, con sus categorías)
> CATEGORIAS                  (las 16 categorías de los dos servidores)
> heladeria                   (Banana-Split de SERV_02 con stock 15; SERV_01 tiene otras 10 en helados)
> AGREGAR Banana-Split 20     (10 de SERV_01 y 10 de SERV_02)
> FACTURA
```

Luego apagar `servidorProductos 2` con `Ctrl+C`: manda el 72 y el intermediario lo saca de la tabla en ese momento (`RUTAS` ya no lo muestra). Al volver a levantarlo, su 80 lo agrega otra vez sin reiniciar el intermediario. Si en cambio se mata con `kill -9`, no alcanza a avisar: el intermediario lo marca `sin-respuesta` en la siguiente consulta, o `vencido` a los 30 s.

Para probar una categoría repetida se puede crear un tercer servidor con `frutas`:

```
./adminBodegas fs3.dat.ci0123 crear Prueba
./adminBodegas fs3.dat.ci0123 agregar Bodega-5
./adminBodegas fs3.dat.ci0123 producto Bodega-5 frutas Mango 10 50000
./servidorProductos 3 <ip> fs3.dat.ci0123
```

El intermediario muestra `aviso: SERV_03 repite la categoria frutas (SERV_02)` y la ruta de `frutas` sigue siendo de SERV_02. Si SERV_02 se apaga, la ruta pasa a SERV_03.

Navegador: `http://<ip>:8080/` (inventario por servidor) y `http://<ip>:8080/rutas` (tabla de rutas).

## Administrar las bodegas

```
./adminBodegas fs1.dat.ci0123 listar
./adminBodegas fs1.dat.ci0123 libres
./adminBodegas fs1.dat.ci0123 agregar <bodega>
./adminBodegas fs1.dat.ci0123 eliminar <bodega>
./adminBodegas fs1.dat.ci0123 producto <bodega> <categoria> <nombre> <cantidad> <precioCentavos>
./adminBodegas fs1.dat.ci0123 descontar <producto> <cantidad>
./adminBodegas fs1.dat.ci0123 inodos
./adminBodegas fs1.dat.ci0123 ver <bodega>
```

Una categoría nueva se anuncia en el siguiente 83 (máximo 10 s); el servidor no necesita reiniciarse.

Para ver el archivo por dentro: `hexdump -C fs1.dat.ci0123 | less`.

## Problemas comunes

- **`no se encontro la direccion de broadcast`**: la IP no es de ninguna interfaz. Revisar con `ip -4 addr` o pasar el broadcast como último argumento.
- **El cliente dice `el intermediario todavia no ha descubierto ningun servidor`**: ningún servidor contestó el 82. Revisar que estén en la misma red y que el firewall deje pasar UDP 9091 y 9092 (`sudo ufw allow 9091/udp`, `sudo ufw allow 9092/udp`).
- **`Address already in use`** en TCP: ya hay un servidor con ese mismo número. Cada servidor necesita un número distinto.
- **Dos servidores en la misma computadora**: los dos escuchan UDP 9092 (gracias a `SO_REUSEADDR`) y a los dos les llega el 82. El 81, que va directo, solo le llega a uno, así que el otro sigue mandando su 80 hasta 3 veces; no afecta, porque igual queda registrado.
- **`cargarInventario.sh` dice que el archivo ya existe**: borrarlo antes con `rm -f`.
