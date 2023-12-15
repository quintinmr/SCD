#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <vector>
#include "scd.h"

using namespace std ;
using namespace scd ;

const int num_fumadores = 4 ,    // numero de fumadores 
          num_recolectoras = 2;  // número de hebras recolectores

unsigned cont_colillas[num_recolectoras] = {0}; //contador de colillas por papelera. Inicialmente vacías.

Semaphore mostr_vacio  = 1,                // Semáforo para control del mostrador (1 si vación, 0 si ocupado)
          ingr_disp[num_fumadores] = {0,0,0,0},          // Semáforo que controla la disponibilidad del ingrediente i
                                           // (1 si el ingred. i está en el mostrador, 0 si no)
          //recolectoras[num_recolectoras],  // Array de semáforos para control de las hebras recolectoras
          fumador_bloq_vaciando_pape = 1,  // Semáforo en el que espera un fumador a que se vacíe su papelera
          exclusion_mutua            = 1;  // Semáforo que resuelve la exclusión mutua en la salida de pantalla

vector<Semaphore> recolectoras;
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
// Función que simula la acción de tirar una colilla

void tirar_colilla( int num_fumador )
{

    unsigned num_papelera = num_fumador % num_recolectoras;
    sem_wait(exclusion_mutua);
    cout << "Fumador " << num_fumador << " tira la colilla en la papelera " << num_papelera << endl;
    // incrementar el número de colillas de su papelera
    cont_colillas[num_papelera]++;
    sem_signal(exclusion_mutua);

    //si ese número ya ha llegado a 4, despierta a la recolectora para que 
    // vacíe la papelera y el fumador espera a que se vacíe.
    if (cont_colillas[num_papelera] == 4){
         sem_signal(recolectoras[num_papelera]);
         sem_wait(fumador_bloq_vaciando_pape);
    }
  
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
      fumar(num_fumador);
      tirar_colilla(num_fumador);

   }
}

//----------------------------------------------------------------------
// función que ejecuta la hebra recolectora
void  funcion_hebra_recolectora(unsigned num_recolectora)
{
   while( true )
   {
       
       sem_wait(recolectoras[num_recolectora]);
       sem_wait(exclusion_mutua);
       cout << "Recolectora " << num_recolectora << " vaciando papelera..." << endl;
       cont_colillas[num_recolectora] = 0;
       sem_signal(exclusion_mutua);
       sem_signal(fumador_bloq_vaciando_pape);

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
   
   
   for (unsigned i = 0; i < num_recolectoras; i++ ) recolectoras.push_back(Semaphore(0));

   // declarar hebras y ponerlas en marcha
   
   thread hebras_fumadoras[num_fumadores];
   thread hebras_recolectoras[num_recolectoras];
   thread hebra_estanquero(funcion_hebra_estanquero);

   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k] = thread (funcion_hebra_fumador,k);

   for (int k = 0; k < num_recolectoras; k++)
   	hebras_recolectoras[k] = thread (funcion_hebra_recolectora,k);
   	
   hebra_estanquero.join();
   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k].join();

   for (int k = 0; k < num_recolectoras; k++)
   	hebras_recolectoras[k].join();
   
}
