/*
    Tenemos un número arbitrario de hebras estanquero, que realizan la misma función
    que en el programa original (producir un ingrediente de los 3 posibles y ponerlo
    encima del mostrador, imprimiendo un mensaje informando de su acción). Pero ahora
    hay espacio para 4 items de cada ingrediente, de tal forma que cuando ya haya 4
    items del ingrediente que se quiere poner, los estanqueros que quieran poner ese
    ingrediente se bloquean.
*/
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

const unsigned 
          num_fumadores             = 3,
          num_estanqueros           = 4;

Semaphore mostr_vacio[num_fumadores]        = {4,4,4},   // Semáforos para controlan los tres espacios que hay en el mostrador
          ingr_disp[3]                      = {0,0,0},   // Semáforo que controla la disponibilidad del ingrediente i en el mostrador
          exclusion_mutua                   = 1;         // Semáforo que controla la salida por pantalla

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   sem_wait(exclusion_mutua);
   cout << "Estanquero empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;
   sem_signal(exclusion_mutua);
   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   sem_wait(exclusion_mutua);
   cout << "Estanquero termina de producir ingrediente " << num_ingrediente << endl;
   sem_signal(exclusion_mutua);

   return num_ingrediente ;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero( int num_estanquero )
{
   while (true)
   {
      int i = producir_ingrediente();
    
      sem_wait(mostr_vacio[i]);
        sem_wait(exclusion_mutua);
        cout << "Estanquero " << num_estanquero << " pone en mostrado ingrediente: " << i << endl;
        sem_signal(exclusion_mutua);
      sem_signal(ingr_disp[i]);
      
   }
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar
    sem_wait(exclusion_mutua);
    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
    sem_signal(exclusion_mutua);
   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar
    sem_wait(exclusion_mutua);
    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
    sem_signal(exclusion_mutua);
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
   while( true )
   {
      sem_wait(ingr_disp[num_fumador]);
        sem_wait(exclusion_mutua);
        cout << "Fumador: " << num_fumador << " retira ingrediente: " << num_fumador << endl;
        sem_signal(exclusion_mutua);
      sem_signal(mostr_vacio[num_fumador]);
      fumar(num_fumador);

   }
}

//----------------------------------------------------------------------

int main()
{
   // declarar hebras y ponerlas en marcha
   cout << "____________________________________________________________________" << endl;
   cout << endl;
   cout << "			PROBLEMA DE LOS FUMADORES CON MÚLTIPLES ESTANQUEROS                      " << endl;
   cout << "____________________________________________________________________" << endl << flush;
   
   // declarar hebras y ponerlas en marcha
   
   thread hebras_fumadoras[num_fumadores];
   thread hebras_estanquero[num_estanqueros];
   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k] = thread (funcion_hebra_fumador,k);

   for (int k = 0; k < num_estanqueros; k++)
   	hebras_estanquero[k] = thread (funcion_hebra_estanquero,k);
   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k].join();

   for (int k = 0; k < num_estanqueros; k++)
   	hebras_estanquero[k].join();
   
}
