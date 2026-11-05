# Simulación TicAmazon con acceso al almacenamiento

Simulación con hilos y un buzón de System V (sin sockets). Cada isla guarda sus bodegas en su propio **filesystem de un solo archivo**:

| Isla | Archivo | Bodegas (archivos de texto dentro del FS) |
|---|---|---|
| 4 | `fs.dat.ci0123` | `Bodega1` (BebidasCalientes, Postres), `Bodega2` (Caramelos, Reposteria) |
| 5 | `fs5.dat.ci0123` | `Bodega3` (helados, snacks) |

Cada producto es una línea `nombreBodega,categoria,producto,cantidad,precio`, por ejemplo `Bodega1,Postres,tarta,10,2200.00`.

## Qué hace cada hilo de bodega con el almacenamiento

| Mensaje | Acceso al FS |
|---|---|
| `60` (anuncio) y `10` | lee las líneas de su archivo y saca las categorías |
| `20/<categoria>` | lee las líneas de esa categoría |
| `50/<producto>/<cantidad>` | lee el archivo, descuenta el stock y lo vuelve a escribir (si no hay stock no escribe) |

Cada acceso se rotula así:

```
    Bodega1   <-> fs.dat.ci0123   descuenta 3 de capuchino y reescribe la bodega, quedan 35 (i-nodo 3, 151 bytes, bloques 5)
```

`Bodega1` y `Bodega2` son dos hilos que comparten el mismo archivo; `BodegaFS` toma el mutex del `Filesystem` en cada operación, así que no se pisan.

## Compilar y ejecutar

```
make
./simulacion        # crea los dos archivos con el stock inicial y corre la simulacion
./simulacion -c     # conserva los archivos de la corrida anterior (el stock sigue descontado)
make clean          # borra el ejecutable, los dos archivos y la cola de mensajes
```

Al final se abre cada archivo con un `Filesystem` nuevo y se imprime cada bodega tal como quedó en disco. Ahí se comprueba que las reservas quedaron guardadas (por ejemplo `capuchino` pasa de 40 a 35 y `tarta` de 10 a 0). Con `-c` la segunda corrida arranca desde esos valores: `tarta` responde `32` con 0.

Para ver el archivo por dentro: `hexdump -C fs.dat.ci0123 | less`.

## Cambios respecto a la versión anterior

- `Bodega` ya no recibe las categorías ni el vector de productos: recibe el `Filesystem` de su isla y el nombre del archivo, busca su nombre en el directorio y trabaja sobre su i-nodo.
- El `Bodega` del filesystem se renombró a `BodegaFS` para no chocar con la clase `Bodega` de la simulación.
- Archivos agregados del filesystem: `FileSystem`, `Bitmap`, `BodegaFS`, `AdministradorBodegas`, `Formatos.h`, `Inventario.h`.
- `Bitacora` tiene `rotularDisco()` para los accesos al almacenamiento.
- `BebidasFrias` ya no aparece: en el FS una categoría existe solo si alguna línea la menciona, y no tenía productos. El caso del cliente sigue dando `23`, ahora porque ninguna bodega la atiende.
- La descripción de los productos no se guarda, porque la línea de la bodega no tiene ese campo (el protocolo tampoco la usa).
