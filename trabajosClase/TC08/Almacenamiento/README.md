# Sistema de almacenamiento — Gestión de Bodegas

Programa que lee y modifica el inventario de las dos bodegas (Bodega-1 y Bodega-2) guardadas en `cafeteria.dat`, sin pasar por HTTP ni por el protocolo de mensajes — es una herramienta de administración directa sobre el almacenamiento.

## Compilar

```
make
```

Genera el binario `gestionBodega`.

## Ejecutar

```
./gestionBodega
```

Si `cafeteria.dat` no existe en la carpeta, el programa lo crea automáticamente con Bodega-1, Bodega-2 y un catálogo de ejemplo, antes de mostrar el menú. Si ya existe (de una corrida anterior), lo abre tal cual quedó.

Menú disponible:

```
1) Listar bodegas
2) Listar categorías de una bodega
3) Listar productos de una categoría
4) Insertar / agregar stock a un producto
5) Extraer / quitar stock de un producto
6) Salir
```

Al pedir bodega o categoría, usar el id corto (`bod1`, `bod2`, `bebfr`, `caram`, etc.), no el nombre completo.

## Persistencia entre corridas

Los cambios que se hagan (insertar o extraer stock) quedan escritos en `cafeteria.dat`. Para comprobarlo: correr `./gestionBodega`, modificar algo, salir con la opción 6, y volver a correr `./gestionBodega` sin nada en medio — el cambio debe seguir ahí.

**`make clean` borra `cafeteria.dat` junto con el binario.** Es a propósito, para volver a probar la creación desde cero. No correrlo entre pruebas de persistencia, o se pierde todo lo insertado.
