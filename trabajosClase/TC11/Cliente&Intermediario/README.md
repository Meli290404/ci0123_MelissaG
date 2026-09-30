# TicAmazon — Intermediario + Bodega y Cliente

Este módulo tiene dos programas:

- **`intermediario`**: proceso único que hace de Intermediario (servidor HTTP para el Cliente) y de Bodega (almacenamiento en `cafeteria.dat`) al mismo tiempo. Atiende cada solicitud en un hilo nuevo.
- **`clienteTicAmazon`**: cliente de línea de comandos que arma los mensajes del protocolo TicAmazon y los manda por HTTP al Intermediario.

## Compilar

Parado en esta carpeta:

```
make
```

Genera dos binarios: `intermediario` y `clienteTicAmazon`.

## Ejecutar

**1. Levantar el Intermediario** (primera terminal):

```
./intermediario
```

Al arrancar crea `cafeteria.dat` desde cero, registra Bodega-1 y Bodega-2, y las llena con las categorías y productos de ejemplo. Queda escuchando en el puerto 8080 — esta terminal no se cierra mientras se prueba; ahí van apareciendo los mensajes `[recibido]`/`[respondido]` de cada solicitud.

**2. Correr el Cliente** (segunda terminal, misma carpeta):

```
./clienteTicAmazon
```

`HOST_SERVIDOR` en `Cliente.cc` ya apunta a `127.0.0.1` para pruebas locales. Comandos disponibles:

```
CATEGORIAS
<idCategoria>            (ej. bebfr, bebca, postr, caram, repos)
PRODUCTO <nombre>
AGREGAR <nombre> <cantidad>
FACTURA
salir
```

Ejemplo de sesión:

```
> CATEGORIAS
> bebfr
> PRODUCTO CafeFrio
> AGREGAR CafeFrio 2
> AGREGAR Chupeta 5
> FACTURA
```

Se pueden abrir varias terminales con `clienteTicAmazon` a la vez contra el mismo `intermediario`, ya que cada solicitud se atiende en su propio hilo.

## Terminar y limpiar

`Ctrl+C` en la terminal del Intermediario para cerrarlo (no hay cierre ordenado con `DEATH` todavía). Después:

```
make clean
```

## Problemas comunes

- **`Address already in use`** al reiniciar `intermediario` muy rápido: esperar unos segundos, o cambiar `PUERTO_SERVIDOR` en `Intermediario.cc`.
- **El cliente dice `ERR_COMM` en todo**: confirmar que `intermediario` está corriendo antes de lanzar el cliente, y que `PUERTO_SERVIDOR` coincide en `Intermediario.cc` y `Cliente.cc`.
