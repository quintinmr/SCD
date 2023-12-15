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

// numero de fumadores 

const int num_fumadores = 3 ;

unsigned buzon[3],                   // array que almacena los cigarrillos introducidos por los fumadores
         primera_libre    = 0,        // variable para gestión de casillas libres del buffer
         primera_ocupada  = 0;        // variable para gestión de casillas ocupadas del buffer

unsigned 
         contador_cigarros_fum[num_fumadores] = {0}; //contador del número de cigarros enviados de cada fumador

Semaphore mostr_vacio  = 1,       // Semáforo para control del mostrador (1 si vación, 0 si ocupado)
          ingr_disp[3] = {0,0,0}, // Semáforo que controla la disponibilidad del ingrediente i
                                  // (1 si el ingred. i está en el mostrador, 0 si no)
          contrabandista = 0,     // Semáforo que controla la hebra contrabandista
          envio_sobres   = 3,     // Semáforo para enviar sobre (hasta 3 sobres se pueden enviar)
          exclusion_mutua= 1;
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

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
   while( true )
   {
      sem_wait(ingr_disp[num_fumador]);
         cout << "Fumador: " << num_fumador << " retira ingrediente: " << num_fumador << endl;
      sem_signal(mostr_vacio);

      // inserción de cigarro en buzón
      sem_wait(envio_sobres);
         buzon[primera_libre] = num_fumador;
         primera_libre = (primera_libre+1)%num_fumadores;
         cout << "Fumador "<< num_fumador << " envía cigarrillo al buzón" << endl;
      sem_signal(contrabandista);

   }
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del contrabandista
void  funcion_hebra_contrabandista(  )
{
   
   unsigned contador_extracciones = 0;

   while( true )
   {
      //duerme durante un tiempo aleatorio entre 20 y 150ms
      chrono::milliseconds duracion_fumar( aleatorio<20,150>() );
      this_thread::sleep_for( duracion_fumar);

      sem_wait(exclusion_mutua);
         cout << "Contrabandista intenta sacar cigarrillo del buzón." << endl;
      sem_signal(exclusion_mutua);
      
      //intenta sacar un cigarrillo del buzón. Si está vacío, espera a que haya al menos uno
      sem_wait(contrabandista);

      // extracción del sobre y contabilización del número de cigarrillos enviados
      // por el fumador
      unsigned num_cig = buzon[primera_ocupada];
      primera_ocupada = (primera_ocupada+1)%num_fumadores;
      cout << "Contrabandista saca cigarrillo del buzón." << endl;
      contador_extracciones++;
      contador_cigarros_fum[num_cig]++;
      
      if (contador_extracciones % 4 == 0)
      {
         for (unsigned i = 0; i < num_fumadores; i++)
         {
            sem_wait(exclusion_mutua);
            cout << "XXXXXXXX FUMADOR " << i << " HA ENVIADO " << contador_cigarros_fum[i] << " CIGARRILLOS DE CONTRABANDO XXXXXXXX" << endl;
            contador_cigarros_fum[i] = 0;
            sem_signal(exclusion_mutua);
         }
      }
      
      sem_signal(envio_sobres);
   }
}

//----------------------------------------------------------------------

int main()
{
   // declarar hebras y ponerlas en marcha
   cout << "____________________________________________________________________" << endl;
   cout << endl;
   cout << "			PROBLEMA DE LOS FUMADORES-CONTRABANDISTA                      " << endl;
   cout << "____________________________________________________________________" << endl << flush;
   

   // declarar hebras y ponerlas en marcha
   
   thread hebras_fumadoras[num_fumadores];
   thread hebra_estanquero(funcion_hebra_estanquero);
   thread hebra_contrabandista(funcion_hebra_contrabandista);
   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k] = thread (funcion_hebra_fumador,k);
   	
   hebra_estanquero.join();

   hebra_contrabandista.join();
   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k].join();
   
}
