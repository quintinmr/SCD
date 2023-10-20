# Práctica 1

* Se introcuen esperas bloqueadas de una duración determinada aleatoria, entre producir y consumir, para dotar de realismo al programa.

* Para dejar una espera bloqueada durante una duración concreta, usamos la función:

> chrono::milliseconds(duracion (aleatorio<a,b>));
> this_thread::sleep_for (duracion);

La gracia del productor-consumidor es que el productor puede estar produciendo el siguiente valor mientras que el consumidos está consumiendo el valor anterior. AHí es donde estamos aprovechando el paralelismo potencial.

Para acelerar el proceso, se usan múltiples variables compartidas (un array de variables compartidas (buffer)). De esta forma, el productor podrá producir muchos valores pendientes de leer por el consumidor. Cuando se llene el vector, estaremos ante un escenario igual que cuando solo hay una única variable compartida que se produce y hasta que no se cosuma no se vuelve a escribir.

* Ocupación del buffer: $\#L \leq \#E$. Nunca puede ser la ocupaciṕn superior al tamaño fijo del buffer:

> $\#E - \L \leq k$


Cuando tenemos múltiples productores y consumidores, cada productor produce un rango de los valores (se reparten la tarea en partes iguales: si el vector tiene 1000 valores, el primer productor produce los 250 primeros, el segundo los 250 segundos, etc.). Cada productor va por un sitio y eso se apunta en un array. Cada productor accede a una componente de un array. Utilizo semáforos. Puedo utilizar los mismos semáforos, pero tengo que programarlo en condiciones:

*  Dos o más hebras pueden intentar acceder a **primera_libre** (LIFO)
* Dos o más productores pueden acceder a **primera_libre** y dos o más consumidores pueden acceder simultáneamente a **primera_ocupada** (FIFO).
