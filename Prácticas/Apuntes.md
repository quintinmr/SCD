
## Sincronización básica en C++

**Tipos atómicos** tipos de datos enteros que se actuualizan de forma atómica.

**Objeto mutex** objetos parecios a los semáforos. Concepto de sección crítica.

Cuando queremos que en un programa multihebra hay dos hebras ejecutando el mismo código o trozos de código, pero que dichos trozos modifican una estructura de datos en memoria compartida. 

Si el progragamaor considera que la interfoliación de instruccionnes es incorrecta, se pueden usar los tipos **atómicos**. Tienen métodos para incrementarlas  y descrementarlas en un número de unidades determinado. Dichos métodos se implementan de forma atómica.Tienen garantizadas la atomicidad.

> atomic<T> a; --> {a.fetch_add(k), a.fetch_sub(k)}

El tipo **mutex** encapsula un valor lógico y una lista de hebras esperando. Cuando quiero ejecutar una sección crítica llamo a los dos únicos métodos públicos: lock (antes de la s.c. Mira si hay una hebra ejecutando la sección crítica. Si hay una hebra ejecutando, el cerrojo está cerrado) y unlock (cuando la hebra que está ejecutando la s.c llega aquí, se comprueba si hay otras hebras esperando ejecutar la s.c se desbloquea el cerrojo. Se libera el mutex). En el unlock si vamos a liberar más de una hebra, tenemos que tener algún criterio para librera las hebras (vale cualquier política de liberación en verdad).

**Ejemplo** cuando hacemos un **cout** en un programa multihebra las lineas se mezclan en el terminal y no sale bien. Así que podemos hacer el cout de forma atómica. Para ello, podemos usar un cerrojo. Así no se mezclan las hebras.

La diferencia entre el atómico y el mutex en tiempo se debe a los tiempos que tardan las funciones lock y unlock. Estos métodos tiene una implementación que tarda más y además tienen que estar bloqueando y desbloqueando las hebras.

> Resumen mutex: libre u ocupado. Lista de hebras esperando que puede estar o no vacía.


## Introducción a los semáforos.

Los **semáforos** son estructuras de datos en memoria compartida a los que solo se accede mediante dos operaciones. Es por ello que se parecen a los objetos mutex. Se diferencian en que la variable de estado de los semáforos puede tomar más valores. El valor que contienen los semáforos se denomina **valor del semáforo**. Las dos operaciones son:

* sem_wait: decrementa el valor del semáforo en una unidad. Comprueba si el valor del semáforo es cero; el valor no puede ser negativo.Conlleva potencialmente una espera si el valor del semáforo es cero (no puede decrementarlo más). Cuando el semáforo esta a 0 se dice que está a rojo.
* sem_signal: incrementa el valor del semáforo en una unidad. Cuando el incremento lleva al semáforo del 0 al 1, hay que ver si hay alguna hebra que estuviera esperando el valor uno. Si la hay, dicha hebra se desbloquea. 

Las transiciones que hacen sem_wait y sem_sigal son atómicas. El semáforo con el signal incrementa el valor su valor de forma atómica. El sem_wait, cuando el valor del semáforo vale mas de 0, lo decrementa de forma atómica. 

Si una hebra hace un sem_wait, valiendo el semáforo 0, la hebra se bloquea; aumenta el número de hebras esperando.

> El cambio del estado del semáforo es atómico.

La única forma de evitar una interfoliación es mediante la introducción de esperas.

> El sem_wait y el sem_signal ocurren de forma atómica. No pueden ejecutarse a la vez.

En una sección crítica de un programa multihebra, el número de procesos que pueden entrar a ejecutar la sección crítica viene dado por:

> $ 0 \leq 1+ \#F-\#I $

