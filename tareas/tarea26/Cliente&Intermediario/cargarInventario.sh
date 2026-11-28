#!/bin/bash
# crea el sistema de archivos del servidor de productos 01 con dos bodegas, 13 categorias y 104 productos
# usando adminBodegas. cada bodega es un archivo de texto dentro del archivo; cada producto es una linea
#   nombreBodega,categoria,producto,cantidad,precio
# uso: ./cargarInventario.sh [archivo]   (por defecto fs1.dat.ci0123)

ARCHIVO=${1:-fs1.dat.ci0123}
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

# bodega 1: bebidas y comidas
bodega Bodega-1

producto Bodega-1 cafes            Espresso          40 110000
producto Bodega-1 cafes            Americano         40 120000
producto Bodega-1 cafes            Capuchino         35 160000
producto Bodega-1 cafes            Latte             35 170000
producto Bodega-1 cafes            Mocaccino         25 190000
producto Bodega-1 cafes            Macchiato         25 165000
producto Bodega-1 cafes            CafeConLeche      30 140000
producto Bodega-1 cafes            Cortado           20 135000

producto Bodega-1 bebidasFrias     CafeFrio          20 150000
producto Bodega-1 bebidasFrias     Frappe            25 210000
producto Bodega-1 bebidasFrias     Limonada          15 100000
producto Bodega-1 bebidasFrias     TeFrio            30 110000
producto Bodega-1 bebidasFrias     Batido-Fresa      18 180000
producto Bodega-1 bebidasFrias     Batido-Mango      18 180000
producto Bodega-1 bebidasFrias     Horchata          22 120000
producto Bodega-1 bebidasFrias     AguaPipa          12 130000

producto Bodega-1 bebidasCalientes CafeCaliente      30 140000
producto Bodega-1 bebidasCalientes Chocolate         25 160000
producto Bodega-1 bebidasCalientes TeVerde           30 110000
producto Bodega-1 bebidasCalientes TeNegro           30 110000
producto Bodega-1 bebidasCalientes Manzanilla        20 100000
producto Bodega-1 bebidasCalientes AguaDulce         20  90000
producto Bodega-1 bebidasCalientes ChaiLatte         15 190000
producto Bodega-1 bebidasCalientes Matcha            15 210000

producto Bodega-1 desayunos        GalloPinto        20 350000
producto Bodega-1 desayunos        Huevos-Revueltos  20 280000
producto Bodega-1 desayunos        Panqueques        15 300000
producto Bodega-1 desayunos        Waffles           15 320000
producto Bodega-1 desayunos        Tostadas          25 180000
producto Bodega-1 desayunos        Omelette          15 330000
producto Bodega-1 desayunos        Avena             20 200000
producto Bodega-1 desayunos        Granola           20 220000

producto Bodega-1 emparedados      Sandwich-Jamon    20 260000
producto Bodega-1 emparedados      Sandwich-Pollo    20 290000
producto Bodega-1 emparedados      Club-Sandwich     15 340000
producto Bodega-1 emparedados      Croissant-Jamon   18 270000
producto Bodega-1 emparedados      Bagel-Queso       18 240000
producto Bodega-1 emparedados      Panini            15 310000
producto Bodega-1 emparedados      Wrap-Pollo        15 300000
producto Bodega-1 emparedados      Wrap-Vegetal      15 280000

producto Bodega-1 ensaladas        Ensalada-Cesar    12 380000
producto Bodega-1 ensaladas        Ensalada-Griega   12 360000
producto Bodega-1 ensaladas        Ensalada-Frutas   15 250000
producto Bodega-1 ensaladas        Ensalada-Atun     10 370000
producto Bodega-1 ensaladas        Ensalada-Caprese  10 390000
producto Bodega-1 ensaladas        Ensalada-Pollo    12 380000
producto Bodega-1 ensaladas        Ensalada-Verde    15 280000
producto Bodega-1 ensaladas        Ensalada-Pasta    12 330000

producto Bodega-1 postres          Tiramisu          10 250000
producto Bodega-1 postres          Cheesecake        10 260000
producto Bodega-1 postres          Flan              15 150000
producto Bodega-1 postres          Brownie           20 120000
producto Bodega-1 postres          TresLeches        12 200000
producto Bodega-1 postres          Pie-Limon         10 220000
producto Bodega-1 postres          Mousse            10 210000
producto Bodega-1 postres          Arroz-con-Leche   15 140000

# bodega 2: dulces, panes y snacks
bodega Bodega-2

producto Bodega-2 caramelos        Chupeta           50  30000
producto Bodega-2 caramelos        Menta             80  20000
producto Bodega-2 caramelos        Gomitas           60  50000
producto Bodega-2 caramelos        Caramelo-Fresa    70  25000
producto Bodega-2 caramelos        Caramelo-Leche    70  25000
producto Bodega-2 caramelos        Toffee            40  35000
producto Bodega-2 caramelos        Malvaviscos       40  60000
producto Bodega-2 caramelos        Regaliz           30  40000

producto Bodega-2 reposteria       PanDulce          25  80000
producto Bodega-2 reposteria       Galleta-Avena     40  50000
producto Bodega-2 reposteria       Galleta-Chispas   40  55000
producto Bodega-2 reposteria       Muffin-Arandano   20 120000
producto Bodega-2 reposteria       Muffin-Choco      20 120000
producto Bodega-2 reposteria       Dona-Glaseada     30  90000
producto Bodega-2 reposteria       Cupcake           25 110000
producto Bodega-2 reposteria       Rollo-Canela      20 130000

producto Bodega-2 panes            Baguette          15 150000
producto Bodega-2 panes            Pan-Integral      15 180000
producto Bodega-2 panes            Pan-Centeno       10 190000
producto Bodega-2 panes            Pan-Queso         30  70000
producto Bodega-2 panes            Pan-Coco          25  80000
producto Bodega-2 panes            Bolillo           40  30000
producto Bodega-2 panes            Croissant         25 100000
producto Bodega-2 panes            Ciabatta          12 160000

producto Bodega-2 chocolates       Bombones          20 250000
producto Bodega-2 chocolates       Trufa             20 180000
producto Bodega-2 chocolates       Barra-Chocolate   30 120000
producto Bodega-2 chocolates       Chocolate-Blanco  25 130000
producto Bodega-2 chocolates       Chocolate-Amargo  25 140000
producto Bodega-2 chocolates       Almendras-Choco   20 200000
producto Bodega-2 chocolates       Mani-Choco        30 110000
producto Bodega-2 chocolates       Pasas-Choco       30 100000

producto Bodega-2 snacks           Papas-Tostadas    40  90000
producto Bodega-2 snacks           Platanitos        40  80000
producto Bodega-2 snacks           Palomitas         30  70000
producto Bodega-2 snacks           Mani-Salado       35  60000
producto Bodega-2 snacks           Nachos            25 150000
producto Bodega-2 snacks           Tortillitas       30  85000
producto Bodega-2 snacks           Pretzels          25  95000
producto Bodega-2 snacks           Barra-Granola     35  75000

producto Bodega-2 helados          Helado-Vainilla   20 120000
producto Bodega-2 helados          Helado-Choco      20 120000
producto Bodega-2 helados          Helado-Fresa      20 120000
producto Bodega-2 helados          Paleta-Mango      30  60000
producto Bodega-2 helados          Paleta-Limon      30  60000
producto Bodega-2 helados          Cono-Doble        15 160000
producto Bodega-2 helados          Sundae            12 220000
producto Bodega-2 helados          Banana-Split      10 300000

echo "listo: $ARCHIVO con dos bodegas"
"$ADMIN" "$ARCHIVO" libres