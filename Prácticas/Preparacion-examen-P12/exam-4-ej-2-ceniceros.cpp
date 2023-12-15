#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <vector>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "scd.h"

using namespace std ;
using namespace scd ;


const unsigned num_fumadores    = 10 ,     // número de fumadore
          num_limpiadoras  = 5;       // número de hebras limpiadoras

unsigned uso_cenicero[num_limpiadoras] = {0};  // contador de usos de los ceniceros

Semaphore mostr_vacio  = 1,                         // Semáforo para control del mostrador (1 si vación, 0 si ocupado)
          ingr_disp[num_fumadores] = {0,0,0,0,0,0,0,0,0,0},    // Semáforo que controla la disponibilidad del ingrediente i
                                                    // (1 si el ingred. i está en el mostrador, 0 si no)
          fum_bloq_vac_cenicero = 0, // semáforo que bloquea al fumador que invoca a la limpiadora de su cenicero
          exclusion_mutua       = 1, // semáforo para control de la exclusión mutua en la salida por pantalla
          limpiadoras[num_limpiadoras] = {0,0,0,0,0}; // semáforo para el control de las hebras limpiadoras
   
                                                   // a la hora de vaciar los ceniceros asociados.

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   sem_wait(exclusion_mutua);
   cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;
   sem_signal(exclusion_mutua);
   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   sem_wait(exclusion_mutua);
   cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;
   sem_signal(exclusion_mutua);

   return num_ingrediente ;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
   while (true)
   {
      int i = producir_ingrediente();
      sem_wait(mostr_vacio);
         cout << "Estanquero pone en mostrado ingrediente: " << i << endl;
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

//-------------------------------------------------------------------------
// Función actualizar cenicero

void actualizar_cenicero( unsigned num_fumador )
{

   unsigned num_cenicero = num_fumador % num_limpiadoras;
    uso_cenicero[num_cenicero]++;
   sem_wait(exclusion_mutua);
   cout << "...fumador " << num_fumador << " usa el cenicero "<< num_cenicero << endl;
   sem_signal(exclusion_mutua);

   if (uso_cenicero[num_cenicero] == 7){
        sem_signal(limpiadoras[num_cenicero]);
        sem_wait(fum_bloq_vac_cenicero);
   }

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( unsigned num_fumador )
{
   while( true )
   {
      sem_wait(ingr_disp[num_fumador]);
         cout << "Fumador: " << num_fumador << " retira ingrediente: " << num_fumador << endl;
      sem_signal(mostr_vacio);
      fumar(num_fumador);
      actualizar_cenicero(num_fumador);

   }
}

//----------------------------------------------------------------------
// función que ejecuta la hebra de limpieza
void  funcion_hebra_limpiadora( unsigned num_limpiadora )
{
   while( true )
   {
      sem_wait(limpiadoras[num_limpiadora]);
      sem_wait(exclusion_mutua);
        cout << "...limpiadora " << num_limpiadora << " limpiando cenicero..." << endl;
        uso_cenicero[num_limpiadora] = 0;
        sem_signal(fum_bloq_vac_cenicero);
      sem_signal(exclusion_mutua);
      

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
   
   //for (unsigned i = 0; i < num_limpiadoras; i++ ) limpiadoras.push_back(Semaphore(0));

   
   // declarar hebras y ponerlas en marcha
   
   thread hebras_fumadoras[num_fumadores];
   thread hebras_limpiadoras[num_limpiadoras];
   thread hebra_estanquero(funcion_hebra_estanquero);

   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k] = thread (funcion_hebra_fumador,k);

   for (int k = 0; k < num_limpiadoras; k++)
   	hebras_limpiadoras[k] = thread (funcion_hebra_limpiadora,k);
   	
   hebra_estanquero.join();
   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k].join();

   for (int k = 0; k < num_limpiadoras; k++)
   	hebras_limpiadoras[k].join();
   
}