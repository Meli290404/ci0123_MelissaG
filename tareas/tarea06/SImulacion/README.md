# Tarea06

## Que hace esta simulacion

Simula la primera etapa de TicAmazon (equipo 4, categoria cafeteria) usando IPC de System V (colas de mensajes) en vez de red real. Hay tres hilos dentro del mismo proceso, cada uno representando un componente del sistema:

- **Cliente**: manda un guion fijo de peticiones (RE_CAT, RE_PROD, ADD_CART, GET_FACT) y muestra las respuestas.
- **Intermediario**: recibe las peticiones del cliente, valida los campos contra las expresiones regulares del protocolo, y reenvia lo que corresponda a la bodega. Tambien maneja el carrito de compras y arma la factura proforma.
- **Bodega**: tiene el inventario de productos y responde las consultas de listado, busqueda y disponibilidad de stock que le llegan del intermediario.

Los tres hilos se comunican a traves de un solo `Buzon` (una cola de mensajes de System V) compartido, usando un numero de canal distinto segun quien le habla a quien (`origen*10 + destino`). Cada mensaje que se envia se imprime en pantalla con su origen, destino y una breve explicacion.

## Como compilar

```
make
```

Esto genera el ejecutable `simulacion`.

## Como ejecutar

```
./simulacion
```

Corre el guion de pruebas completo y termina solo, imprimiendo cada mensaje intercambiado entre los tres componentes.

## Limpieza

```
make clean
```

Borra el ejecutable y, de paso, elimina la cola de mensajes de System V si quedo viva de una corrida anterior que no terminó bien (por ejemplo si el programa crasheo antes de que el destructor de `Buzon` pudiera eliminarla).
