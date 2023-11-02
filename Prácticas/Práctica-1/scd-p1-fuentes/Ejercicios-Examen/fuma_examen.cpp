//------------------------------------------------------------------------------------
// Nombre: Quintín
// Apellidos: Mesa Romero
// SCD (grupo viernes)
// Correo: quintinmr@correo.ugr.es
//------------------------------------------------------------------------------------
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

unsigned num_veces_fuma[num_fumadores] = {0},  // Array en el que se contabiliza el número de veces que el fumador X ha fumado
         fumador_desbloq_x_sanitaria   = 0;    // Número de fumador que acaba de desbloquear la hebra sanitaria

Semaphore mostr_vacio  = 1,       // Semáforo para control del mostrador (1 si vación, 0 si ocupado)
          ingr_disp[3] = {0,0,0}, // Semáforo que controla la disponibilidad del ingrediente i
                                  // (1 si el ingred. i está en el mostrador, 0 si no)
          sanitaria_free       = 0,  // Semáforo que me controla cuándo la hebra sanitaria está libre
          fumador_desbloqueado = 0;  // Semáforo que me controla cuándo un fumador está bloqueado por la hebra sanitaria
         
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

    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;

   // El fumador ha fumado, así que se contabiliza esta acción de fumar (B)
   num_veces_fuma[num_fumador]++;

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
   while( true )
   {
      sem_wait(ingr_disp[num_fumador]); // (B): el fumador ha sido desbloqueado por el estanquero
                                        //    Si en este punto ha fumado ya 5 veces, esta será la 6ª vez que el estanquero lo desbloquea
         
        if (num_veces_fuma[num_fumador] == 5)  // ha fumado ya 5 veces
        {
            sem_signal(sanitaria_free);        // desbloquea a la hebra sanitaria  (B1)
            sem_wait(fumador_desbloqueado);       // se bloquea esperando a que la sanitaia lo libere (B2)
            cout << "Soy el fumador " << num_fumador << " y me han llamado vicioso." << endl; // (B3)
            fumador_desbloq_x_sanitaria = num_fumador;
        }
        
        // (B4)
        // continua con el problema original: retira el ingrediente, desbloquea estanquero, fuma y vuelve al inicio
        cout << "Fumador: " << num_fumador << " retira ingrediente: " << num_fumador << endl;
    
        sem_signal(mostr_vacio);
        fumar(num_fumador);

   }
}

//----------------------------------------------------------------------
// función que ejecuta la hebra sanitaria
void  funcion_hebra_sanitaria ()
{
    while (true)
    {
        sem_wait(sanitaria_free);  // se bloquea la hebra sanitaria (A)
            cout << "FUMAR MATA: ya lo sabes, fumador " << fumador_desbloq_x_sanitaria << endl;  // (C1)
        sem_signal(fumador_desbloqueado); // se desbloquea al fumador // (C2)

    }
}

//----------------------------------------------------------------------


int main()
{
   // declarar hebras y ponerlas en marcha
   cout << "____________________________________________________________________" << endl;
   cout << endl;
   cout << "			                FUMA_EXAMEN                             " << endl;
   cout << "____________________________________________________________________" << endl << flush;
   
   // declarar hebras y ponerlas en marcha
   
   thread hebras_fumadoras[num_fumadores];
   thread hebra_estanquero(funcion_hebra_estanquero);
   thread hebra_sanitaria(funcion_hebra_sanitaria);


    for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k] = thread (funcion_hebra_fumador,k);
   	
    hebra_estanquero.join();
   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k].join();

   hebra_sanitaria.join();
   
}
