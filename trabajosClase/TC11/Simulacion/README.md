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
