// Alumno: Ricardo Ruiz Fernández de Alba
// SCD - Doble Grado en Ingeniería Informática y Matemáticas 

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "scd.h"

using namespace std ;
using namespace scd ;

// numero de fumadores 

const int num_fumadores = 3 ;

int num_fumados[num_fumadores] = {0}; // Array que almacena el numero de veces que ha fumado cada fumador.

int num_fumador_sanitaria = 0; // Almacena el número de fumador que ha desbloqueado a la hebra sanitarias

Semaphore mostr_vacio(1), // Semáforo que toma 1 cuando el mostrador está vacío y 0 en caso contrario.
          ingr_disp[3] = {0, 0, 0}; // Semáforo que indica si el ingrediente i está disponible

Semaphore ocupada_sanitaria(0), // Semáforo que indica que la hebra sanitaria está ocupada.
          libre_sanitaria(0); // Semáforo que indica que la hebra sanitaria ha sido desbloqueada

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;

   return num_ingrediente ;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
   while (true)
   {
      int i = producir_ingrediente();
      mostr_vacio.sem_wait(); // Esperar a que el mostrador esté vacío
         cout << "puesto ingr.: " << i << endl;
      ingr_disp[i].sem_signal(); // El ingrediente i está sobre el mostrador.
   }
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
   

   num_fumados[num_fumador]++; // Incrementar el número de veces que ha fumado el fumador num_fumador

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
   while (true)
   {
      ingr_disp[num_fumador].sem_wait(); // Esperar a que el ingrediente i esté sobre el mostrador.

         if (num_fumados[num_fumador] == 5)
         {
            num_fumador_sanitaria = num_fumador; // num_fumador que ha desbloqueado a la hebra sanitaria
            libre_sanitaria.sem_signal();  
            ocupada_sanitaria.sem_wait();
            cout << "Soy el fumador " << num_fumador << " y me han llamado vicioso" << endl;
         }
         
            cout << "retirado ingr.: " << num_fumador << endl;

      mostr_vacio.sem_signal(); // El mostrador ha quedado vacío tras retirar el fumador su ingrediente.

      fumar(num_fumador);
   }
}

//----------------------------------------------------------------------
// función que ejecuta la hebra sanitaria

void funcion_hebra_sanitaria()
{
   while (true)
   {
      libre_sanitaria.sem_wait(); // Esperar a que haya fumado 5 veces y haya sido desbloqueado para fumar por sexta vez.
         cout << "FUMAR MATA: ya lo sabes, fumador " << num_fumador_sanitaria << endl;
      ocupada_sanitaria.sem_signal(); // Desbloquear al fumador
   }
}

//----------------------------------------------------------------------

int main()
{
   // declarar hebras y ponerlas en marcha
   thread hebra_estanquero(funcion_hebra_estanquero),
          hebra_sanitaria(funcion_hebra_sanitaria),
          hebra_fumador[num_fumadores];
   
   
   for (int i=0; i < num_fumadores; i++)
      hebra_fumador[i] = thread(funcion_hebra_fumador, i);
   
   hebra_estanquero.join();
   hebra_sanitaria.join();

   for (int i=0; i < num_fumadores; i++)
      hebra_fumador[i].join();
   
}
