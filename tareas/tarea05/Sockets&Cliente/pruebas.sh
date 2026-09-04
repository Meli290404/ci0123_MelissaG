#!/bin/bash
# pruebas.sh - casos prueba cliente TicAmazon
# Equipo 4
#
# uso: ./pruebas.sh [-v]
#deja el print en resultados.txt

CLIENTE=./Client
SALIDA=resultados.txt

numero=0
pasaron=0
fallaron=0

detallado=0
[ "$1" = "-v" ] && detallado=1

if [ ! -x "$CLIENTE" ]; then
    echo "no se encuentra $CLIENTE, compile primero con make"
    exit 1
fi

{
    echo "casos de prueba - cliente TicAmazon - equipo 4"
    echo "servidor: os.ecci.ucr.ac.cr  -  $(date '+%Y-%m-%d %H:%M')"
    echo
} > "$SALIDA"

# separa los casos por grupo, en pantalla y en el archivo
grupo() {
    echo
    echo "-- $1 --"
    {
        echo
        echo "== $1 =="
        echo
    } >> "$SALIDA"
}

# probar <desc> <entrada> <esperado>
# entrada puede traer varias lineas si se manda con $'linea1\nlinea2'
probar() {
    desc="$1"; entrada="$2"; esperado="$3"
    numero=$(( numero + 1 ))

    salida=$( printf '%s\nsalir\n' "$entrada" | $CLIENTE 2>/dev/null )
    limpia=$( echo "$salida" | sed -n '/\[Cliente/,$p' | sed 's/^> *//' | grep -v '^---Fin de pruebas---$' )

    if echo "$salida" | grep -q "$esperado"; then
        veredicto="ok"
        pasaron=$(( pasaron + 1 ))
    else
        veredicto="FALLA"
        fallaron=$(( fallaron + 1 ))
    fi

    printf "%2s %-6s %-28s %s\n" "$numero" "$veredicto" "$desc" "$entrada"

    if [ "$detallado" -eq 1 ]; then
        echo "$limpia" | sed 's/^/     /'
    else
        echo "$limpia" | grep -E '^\[|^  \(' | sed 's/^/     /'
    fi

    {
        echo "caso $numero - $desc"
        echo "entrada : $entrada"
        echo "esperado: contiene \"$esperado\""
        echo "veredicto: $veredicto"
        echo "salida:"
        echo "$limpia" | sed 's/^/    /'
        echo
    } >> "$SALIDA"
}

echo "pruebas cliente TicAmazon - equipo 4"

grupo "pruebas correctas"
probar "categoria del equipo"      "Isla 4"                "PROD_LIST [0-9]"
probar "categoria con productos"   "Alimentos y bebidas"   "PROD_LIST [0-9]"
probar "respuesta grande"          "Salud y belleza"       "PROD_LIST [0-9]"
probar "conexion por SSL"          "Isla 4"                "SSL/HTTPS"
probar "buscar producto, varios matches" "PRODUCTO manga"  "coincidencia(s)"

grupo "carrito y factura"
probar "agregar al carrito"        "AGREGAR manga 2"       "PROD_ADDED"
probar "agregar y ver factura"     $'AGREGAR manga 2\nFACTURA' "TOTAL"
probar "factura sin nada en el carrito" "FACTURA"           "sin productos"

grupo "manejo de errores"
probar "categoria valida pero vacia" "Maquinaria"          "PROD_LIST_EMPT"
probar "tildes y espacios en la url" "Hogar y decoración"  "PROD_LIST_EMPT"
probar "categoria inexistente"     "Cafeteria"             "PROD_LIST_EMPT"
probar "producto que no existe"    "PRODUCTO xyz"          "PROD_NOT_AVAIL"
probar "agregar producto que no existe" "AGREGAR xyz 1"    "PROD_NOT_AVAIL"
probar "servidor no responde"      "ERROR"                 "ERR_COMM Servidor"
probar "codigo HTTP distinto de 200" "RUTA /TicAmazon/noexiste.php" "ERR_HTTP codigo=404"

echo
printf "total %s, pasaron %s, fallaron %s\n" "$numero" "$pasaron" "$fallaron"
echo "detalle en $SALIDA"

{
    echo
    echo "resumen: total $numero, pasaron $pasaron, fallaron $fallaron"
} >> "$SALIDA"

[ "$fallaron" -eq 0 ]