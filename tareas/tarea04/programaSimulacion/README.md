# Trabajo en clase #4 — Programas de simulación del protocolo

Dos programas independientes que simulan la interacción entre el servidor de productos (bodega) y el servidor intermediario, comunicados por IPC (cola de mensajes System V, `Buzon`). Cada mensaje intercambiado se imprime en pantalla con un rótulo `[Origen -> Destino] mensaje  explicación`.

## Compilar

```bash
make
```

Genera dos ejecutables: `bodega` e `intermediario`.

## Ejecutar

Requiere **dos terminales abiertas al mismo tiempo**, en el orden indicado:

**Terminal 1** (arrancar primero, se queda esperando):
```bash
./bodega
```

**Terminal 2** (arrancar después, mientras la 1 sigue corriendo):
```bash
./intermediario
```

El intermediario manda un listado de productos y dos consultas puntuales, muestra las respuestas, y al final avisa el cierre — ambos procesos terminan solos.

## Limpiar

```bash
make clean
```
