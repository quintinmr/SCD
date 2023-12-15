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

const int num_fumadores   = 3 ,
          num_estanqueros = 4;

Semaphore mtx(1);

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   sem_wait(mtx);
   cout << "Estanquero empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;
   sem_signal(mtx);
   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   sem_wait(mtx);
   cout << "Estanquero termina de producir ingrediente " << num_ingrediente << endl;
   sem_signal(mtx);

   return num_ingrediente ;
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Monitor Estanco
//----------------------------------------------------------------------
//----------------------------------------------------------------------

class Estanco : public HoareMonitor
{

   private:

      // Declaramos variables permanentes
      unsigned contador[num_fumadores];  // contador para el núm. de items por ingrediente que hay en el mostrador.
      CondVar                            // colas condición
         mostrador[num_fumadores],  	  // cola en la que esperan los estanquero a que el espacio de mostrador que quieren se vacíe.                         
         ingr_disp[num_fumadores];	     // colas en las que cada fumador espera a que su ingrediente esté disponible en el mostrador.

      // Procedimientos
      public:

         Estanco();
         void ponerIngrediente(unsigned i, unsigned j);
        // void esperarRecogidaIncrediente();
         void obtenerIngrediente(unsigned i);

};

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Implementación procedimientos del monitor
//----------------------------------------------------------------------
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------

Estanco::Estanco()
{
   
   for (unsigned i = 0; i < num_fumadores; i++){
      ingr_disp[i] = newCondVar();
      mostrador[i] = newCondVar();
      contador[i]  = 0;
   }

}

//----------------------------------------------------------------------
// Obtener ingrediente i del mostrador
//----------------------------------------------------------------------

void Estanco::obtenerIngrediente(unsigned i)
{
   assert( i < num_fumadores);
   
   if (contador[i] == 0) ingr_disp[i].wait();

   contador[i]--;
   cout << "Se ha retirado del mostrador el ingrediente nº: "<< i << endl;
   mostrador[i].signal();

}

//----------------------------------------------------------------------
// Poner ingrediente en el mostrador
//----------------------------------------------------------------------

void Estanco::ponerIngrediente(unsigned i, unsigned j)
{
   
   if (contador[i] == 4)
      mostrador[i].wait();
   contador[i]++;
   cout << "Estanquero " << j << " ha puesto ingediente nº "<< i << " en el mostrador." << endl;
   ingr_disp[i].signal();
   
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero( MRef<Estanco> monitor, unsigned num_estanquero )
{
   while (true)
   {
      unsigned i = producir_ingrediente();
      monitor->ponerIngrediente(i,num_estanquero);
   }
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar
    sem_wait(mtx);
    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
    sem_signal(mtx);

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar
    sem_wait(mtx);
    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
    sem_signal(mtx);
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( MRef<Estanco> monitor, unsigned i )
{
   while( true )
   {
      monitor->obtenerIngrediente(i);
      fumar(i);

   }
}

//----------------------------------------------------------------------

int main()
{
   // declarar hebras y ponerlas en marcha
   cout << "____________________________________________________________________" << endl;
   cout << endl;
   cout << "	  PROBLEMA DE LOS FUMADORES CON MONITORES                       " << endl;
   cout << "____________________________________________________________________" << endl << flush;
   
   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<Estanco> monitor = Create<Estanco>() ;
   
   // declarar hebras y ponerlas en marcha
   
   thread hebras_fumadoras[num_fumadores];
   thread hebras_estanqueros[num_estanqueros];
   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k] = thread (funcion_hebra_fumador,monitor, k);

   for (int k = 0; k < num_estanqueros; k++)
   	hebras_estanqueros[k] = thread (funcion_hebra_estanquero,monitor, k);
   	
   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k].join();

   for (int k = 0; k < num_estanqueros; k++)
   	hebras_estanqueros[k].join();
   
}
