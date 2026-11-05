#!/bin/bash
# inventario del segundo servidor de productos (SERV_02): dos bodegas con frutas, verduras y helados.
# "helados" tambien esta en el primer servidor y Banana-Split esta en los dos, para probar que el
# intermediario busca en otro servidor cuando al primero no le alcanza.
# cada bodega es un archivo de texto dentro del archivo; cada producto es una linea
#   nombreBodega,categoria,producto,cantidad,precio
# uso: ./cargarInventario2.sh [archivo]   (por defecto fs2.dat.ci0123)

ARCHIVO=${1:-fs2.dat.ci0123}
ADMIN=./adminBodegas

if [ ! -x "$ADMIN" ]; then
    echo "no se encontro $ADMIN, compile primero con make"
    exit 1
fi
if [ -e "$ARCHIVO" ]; then
    echo "$ARCHIVO ya existe; borrelo primero si quiere crearlo de nuevo"
    exit 1
fi

# cada llamada se detiene en el primer error para no dejar el archivo a medias sin avisar
correr() {
    "$ADMIN" "$ARCHIVO" "$@" > /dev/null || { echo "fallo: $ADMIN $ARCHIVO $*"; exit 1; }
}

# la categoria no se crea aparte: existe en cuanto un producto la usa
bodega()    { correr agregar "$1"; }
producto()  { correr producto "$1" "$2" "$3" "$4" "$5"; }   # bodega categoria nombre cantidad precioCentavos

correr crear TicAmazon

# bodega 3: frutas y verduras
bodega Bodega-3

producto Bodega-3 frutas           Banano            100  20000
producto Bodega-3 frutas           Manzana            60  45000
producto Bodega-3 frutas           Naranja            80  25000
producto Bodega-3 frutas           Pina               30  90000
producto Bodega-3 frutas           Papaya             25  80000
producto Bodega-3 frutas           Fresas             40 120000

producto Bodega-3 verduras         Papa               90  15000
producto Bodega-3 verduras         Zanahoria          70  12000
producto Bodega-3 verduras         Tomate             60  18000
producto Bodega-3 verduras         Lechuga            40  30000
producto Bodega-3 verduras         Brocoli            30  45000

# bodega 4: helados (la misma categoria que en el primer servidor)
bodega Bodega-4

producto Bodega-4 helados          Banana-Split       15 300000
producto Bodega-4 helados          Helado-Coco        20 120000
producto Bodega-4 helados          Helado-Cafe        20 125000
producto Bodega-4 helados          Paleta-Fresa       30  60000

echo "listo: $ARCHIVO con dos bodegas"
"$ADMIN" "$ARCHIVO" libres
