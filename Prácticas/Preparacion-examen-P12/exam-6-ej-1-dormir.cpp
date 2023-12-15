/*
// FUMADOR QUE FUMA Y A LA 3ª VEZ, SE ECHA A DORMIR
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

const int num_fumadores = 3 ;

unsigned cont_fumadas[num_fumadores] = {0},      // contador del número de veces que el fumador i-ésimo fuma 
         cont_ingred[num_fumadores]  = {0};      // contador del número de ingredientes de cada tipo que se han puesto

Semaphore mostr_vacio  = 1,       // Semáforo para control del mostrador (1 si vación, 0 si ocupado)
          ingr_disp[3] = {0,0,0}, // Semáforo que controla la disponibilidad del ingrediente i
                                  // (1 si el ingred. i está en el mostrador, 0 si no)
          exclusion_mutua = 1,    // semáforo para el control de la salida por pantalla
          cama[3] = {0,0,0};// semáforo donde espera el fumador que se duerme
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

      if (cont_ingred[i] == 3){
        sem_signal(cama[i]);
      }
        

      sem_wait(mostr_vacio);
        cout << "Estanquero pone en mostrado ingrediente: " << i << endl;
        cont_ingred[i]++;
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
    cont_fumadas[num_fumador]++;
    sem_signal(exclusion_mutua);
    
}

//-------------------------------------------------------------------------
// Función que simula la acción de dormir, como un retardo aleatoria de la hebra

void dormir( int num_fumador )
{

   // informa de que comienza a dormir
    sem_wait(exclusion_mutua);
    cout << "zzzzzz Fumador " << num_fumador << "  :" << " empieza a dormir zzzzzz" << endl;
    sem_signal(exclusion_mutua);

    // el fumador que duerme espera en su cama hasta que el estanquero lo despierte
    sem_wait(cama[num_fumador]);

   // informa de que ha terminado de dormir
    sem_wait(exclusion_mutua);
    cout << " oooo Fumador " << num_fumador << "  : termina de dormir, comienza espera de ingrediente. oooo" << endl;
    sem_signal(exclusion_mutua);
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
   while( true )
   {
      sem_wait(ingr_disp[num_fumador]);
        cout << "Fumador: " << num_fumador << " retira ingrediente: " << num_fumador << endl;
      sem_signal(mostr_vacio);

      if (cont_fumadas[num_fumador] < 3 || cont_ingred[num_fumador] > 3)
        fumar(num_fumador);
      else
        dormir(num_fumador);
    
   }
}

//----------------------------------------------------------------------

int main()
{
   // declarar hebras y ponerlas en marcha
   cout << "____________________________________________________________________" << endl;
   cout << endl;
   cout << "			PROBLEMA DE LOS FUMADORES                        " << endl;
   cout << "____________________________________________________________________" << endl << flush;
   
   // declarar hebras y ponerlas en marcha
   
   thread hebras_fumadoras[num_fumadores];
   thread hebra_estanquero(funcion_hebra_estanquero);
   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k] = thread (funcion_hebra_fumador,k);
   	
   hebra_estanquero.join();
   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k].join();
   
}