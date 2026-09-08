# Reporte Comparativo: SIQP vs Protocolo TICAMAZON

## Wilson Chavarría Miranda C22114 GRUPO 2 ISLA 2

## Melissa Garita Chacón C23186 GRUPO 2 ISLA 4

## Introducción

Este reporte compara dos protocolos evaluados: **SIQP** y **Protocolo TICAMAZON**, en función de su complejidad sintáctica, curva de aprendizaje, y mecanismos de identificación de clientes, servidores y mensajes.

---

## SIQP

### Complejidad y curva de aprendizaje

- Tiene una sintaxis más extensa, lo que lo vuelve más complejo.
- Esta mayor complejidad implica una carga de aprendizaje más pesada.
- Presenta demasiados elementos, lo que genera una **carga cognitiva inicial muy grande**.

### Identificación de clientes y servidores

- Cuenta con **identificadores para cada cliente y cada servidor**.
- Esto permite que, ante múltiples requests provenientes de distintos clientes y distintos servidores, siempre se pueda determinar con claridad a quién pertenece cada uno (no se pierde la trazabilidad del origen).

### Identificación de mensajes

- Cuenta con un **identificador por mensaje**, el cual se envía y se recibe con el mismo identificador.
- Esto permite que, aunque se envíen muchos mensajes, siempre se pueda determinar a qué mensaje corresponde cada respuesta.

---

## Protocolo TICAMAZON

### Complejidad y curva de aprendizaje

- Tiene una sintaxis simple.
- Esta simplicidad facilita ponerlo a funcionar desde cero, en comparación con SIQP.

---

## Comentarios adicionales
- Se terminó eligiendo el protocolo SIQP ya que está más completo, sin embargo, se le quitó la parte de poder agregar categorías y productos ya que era más complejo de implementar.

- Agregué el archivo que los compañeros crearon en donde se explica con detalle su protocolo para que se pueda entender mejor. 

---
