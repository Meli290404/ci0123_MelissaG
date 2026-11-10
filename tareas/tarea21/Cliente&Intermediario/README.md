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

## Descubrimiento entre islas (protocolo mancomunado)

Para hablar con los intermediarios de otras islas se usa el protocolo mancomunado del grupo 2: `TIPO/campos`, sin origen ni destino. El intermediario abre un segundo socket UDP en el puerto **9090**, ligado a todas las interfaces y con broadcast habilitado.

| Tipo | Formato | Cuándo |
|---|---|---|
| **60** ANNOUNCE | `60/ID_MSG/INT_04/<ip privada>/9090` | Al arrancar (el mismo 60 dos veces) y cada vez que aparece una isla nueva |
| **61** ANNOUNCE_BYE | `61/INT_04` | Al apagar el intermediario con `Ctrl+C` o `kill` |

Los dos van al broadcast dirigido de cada isla: `172.16.123.15, .31, .47, .63, .79, .95, .111`. Nunca a `255.255.255.255`.

Hilos nuevos del intermediario:

- **`escucharIslas`**: espera en `recvFrom` en el UDP 9090. Con un 60 válido registra la isla en la `TablaIslas`; con un 61 la deja de baja. Lo que no cumple el formato se descarta (por UDP no hay conexión por donde contestar un 02).
- **`anunciarAlArrancar`**: manda el 60 inicial.
- **`esperarApagado`**: espera la señal con `sigwait` y manda el 61 antes de terminar.

Reglas:

- **Duplicados y ecos**: cada 60 lleva un `ID_MSG` distinto. Si llega otra vez el mismo `id_isla/ID_MSG` en menos de 60 s, se descarta. El eco del propio 60 se descarta por el `id_isla`. El `ID_MSG` arranca en un número al azar, para que un intermediario que se reinicia no parezca repetido.
- **Isla que arranca después**: cuando llega el 60 de una isla **nueva**, el intermediario se vuelve a anunciar, porque esa isla no oyó el 60 inicial. Las islas que ya lo conocían lo reciben como igual y no contestan, así que no se forma un bucle.
- **Sin heartbeat**: una isla queda `activa` hasta que manda su 61.

La tabla de islas se ve en `/rutas` y en el comando `RUTAS` del cliente.

## Atención a otras islas (TCP 9090)

Las otras islas se conectan por TCP a la IP y el puerto que van en nuestro 60. Cada conexión trae una solicitud y recibe una respuesta. El hilo `aceptarIslas` acepta las conexiones y crea un hilo `atenderIsla` por cada una. `AtencionIsla` traduce cada mensaje al protocolo interno:

| Llega | Qué se hace adentro | Respuesta |
|---|---|---|
| `10` | categorías de la tabla de rutas, en minúsculas y sin repetir | `01/cafes,tes,...` |
| `20/categoria` | `20` interno al servidor dueño de la ruta | `01/n/producto,precio,stock;...`, `01/0` si está vacía, `03/categoria` si no existe |
| `50/producto/cantidad` | busca el producto en los servidores y lo aparta con el `50` interno | `01`, `02/103` si no alcanza, `03/producto` si no existe |
| `40/n/producto,cantidad;...` | revisa que cada producto tenga reserva vigente de esa isla | `01/total/n/producto,cantidad,subtotal;...` o `02/104` |
| otro tipo | — | `02/101` |
| mensaje roto o campos de más o de menos | — | `02/100` |
| valor que no cumple la expresión regular | — | `02/102` |

- **Mayúsculas**: categorías y productos se buscan en minúsculas, así que `20/CAFES` y `50/espresso/1` encuentran `cafes` y `Espresso`.
- **Reservas**: se guardan por IP de la isla y duran 60 s. Si vencen sin compra, el hilo `vencerReservas` le devuelve el stock al servidor con el mensaje interno **52**. Si al reservar el servidor aparta menos de lo pedido, también se devuelve lo apartado y se contesta `02/103`.
- **Fin del mensaje**: se lee hasta el `\n`, hasta que la otra isla cierre su lado o hasta un segundo sin datos. Las respuestas se mandan terminadas en `\n`.

Mensajes internos nuevos, entre intermediario y servidor de productos:

| Tipo | Formato | Para qué |
|---|---|---|
| **52** | `INT_04\|SERV_NN/52/<producto>/<cantidad>` | Devolver stock reservado que no se usó |
| **53** | `SERV_NN\|INT_04/53/<producto>/<cantidad devuelta>` | Respuesta al 52 (o `27` si el producto ya no existe) |

Para probar sin otra isla, con `nc`:

```
echo "10" | nc <ip privada> 9090
echo "20/cafes" | nc <ip privada> 9090
echo "50/espresso/2" | nc <ip privada> 9090
echo "40/1/espresso,2" | nc <ip privada> 9090
```

## El cliente compra en otras islas

Cuando algo no está en la isla 4, el intermediario se lo pide a las otras islas con el protocolo mancomunado. `ConexionIsla` manda cada mensaje por TCP con timeout de 5 s y hasta 3 intentos, como recomienda el protocolo. Si una isla no contesta, queda `sin-respuesta` en la tabla de islas y no se vuelve a usar hasta su próximo 60.

| El cliente pide | Qué hace el intermediario |
|---|---|
| `CATEGORIAS` | junta las categorías de la isla 4 y las de cada isla activa (`10`). Las que la isla 4 ya tiene no se repiten; las demás salen con `(de INT_NN)` |
| `<categoria>` | si la isla 4 no tiene productos de esa categoría, le pide un `20` a cada isla que la tiene. Los productos salen con `isla: INT_NN` |
| `AGREGAR <producto> <n>` | si ningún servidor de la isla 4 lo tiene, lo busca en lo que las islas ya mostraron (si no lo ha visto, les pide sus productos) y lo reserva con un `50`. Como el `50` es todo o nada, si la isla tiene menos se pide lo que tiene |
| `FACTURA` | lo de la isla 4 se cobra como siempre. Lo de cada otra isla se compra con un `40` a esa isla y se usan los subtotales que ella cobra. Si la reserva venció (`02/104`), se vuelve a reservar y se intenta el `40` otra vez; si no se puede comprar, la línea sale `no se pudo comprar` y no suma al total |

Cuando se descubre una isla nueva, el intermediario le pide sus categorías en un hilo aparte (`conocerIsla`), para no detener al hilo que escucha el UDP. `/rutas` muestra cuántas categorías tiene cada isla.

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
./intermediario <ip clientes> <ip red de servidores> [broadcast] [-isla NN] [-islas b1,b2,...] [-puerto N] [-http N]
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

Navegador: `http://<ip>:8080/` abre el menú, con un botón por página. Todas las páginas tienen arriba una barra para moverse entre ellas.

| Página | Qué muestra |
|---|---|
| `/` | Menú, con cuántos servidores e islas hay activos |
| `/inventario` | Todos los productos: una sección por servidor de la isla y una por cada otra isla |
| `/categorias` | Las categorías de la isla y de las otras islas; cada una lleva a sus productos |
| `/servidores` | Los servidores de productos descubiertos y su estado |
| `/rutas` | La tabla de rutas completa (servidores, rutas por categoría y otras islas) |
| `/islas` | Las otras islas descubiertas, su estado y sus categorías |

`/servidores`, `/rutas` e `/islas` se recargan solas cada 2 segundos.

### Probar varias islas en una sola computadora

Cada intermediario se identifica con `-isla`, atiende clientes en otro puerto con `-http` y anuncia otro puerto con `-puerto`. `-islas` cambia la lista de broadcasts por el de la red de la computadora (el número después de `brd` en `ip -4 addr`):

```
./intermediario <ip> <ip> -isla 04 -islas <brd> -http 8080 -puerto 9090
./intermediario <ip> <ip> -isla 05 -islas <brd> -http 8081 -puerto 9190
./intermediario <ip> <ip> -isla 06 -islas <brd> -http 8082 -puerto 9290
```

Después de arrancar las tres, cada una tiene a las otras dos en `http://<ip>:808X/rutas`. Con `Ctrl+C` en la 05, las otras la muestran `de-baja`; al volver a levantarla, aparece `activa` otra vez.

En el laboratorio no hacen falta las opciones: `./intermediario <ip publica> 172.16.123.6X` anuncia como `INT_04` a las siete islas.

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
- **Las otras islas no aparecen o no se pueden conectar**: el firewall debe dejar pasar UDP y TCP 9090 (`sudo ufw allow 9090`), y el switch L3 debe permitir el broadcast dirigido hacia cada VLAN.
- **El cliente dice `el intermediario todavia no ha descubierto ningun servidor`**: ningún servidor contestó el 82. Revisar que estén en la misma red y que el firewall deje pasar UDP 9091 y 9092 (`sudo ufw allow 9091/udp`, `sudo ufw allow 9092/udp`).
- **`Address already in use`** en TCP: ya hay un servidor con ese mismo número. Cada servidor necesita un número distinto.
- **Dos servidores en la misma computadora**: los dos escuchan UDP 9092 (gracias a `SO_REUSEADDR`) y a los dos les llega el 82. El 81, que va directo, solo le llega a uno, así que el otro sigue mandando su 80 hasta 3 veces; no afecta, porque igual queda registrado.
- **`cargarInventario.sh` dice que el archivo ya existe**: borrarlo antes con `rm -f`.
