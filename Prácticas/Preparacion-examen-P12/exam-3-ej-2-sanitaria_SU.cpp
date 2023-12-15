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

unsigned cont_fumadas[num_fumadores] = {0};

Semaphore exclusion_mutua(1);

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Monitor Hospital
//----------------------------------------------------------------------
//----------------------------------------------------------------------

class Hospital : public HoareMonitor
{

   private:

      // Declaramos variables permanentes
      int num_fumador;		                 

      CondVar                                // colas condición
         sanitaria,                          // cola donde espera la hebra sanitaria hasta que un fumador ha fumado 5 veces.
         fumadores[num_fumadores];           // cola donde esperan los fumadores a los que la hebra sanitaria está llamando viciosos.

      // Procedimientos
      public:

         Hospital();
         void insultar();
         void despertarSanitaria(unsigned vicioso);

};

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Implementación procedimientos del monitor
//----------------------------------------------------------------------
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------
Hospital::Hospital()
{
    sanitaria = newCondVar();
    for (int i = 0; i < num_fumadores; i++)
        fumadores[i] = newCondVar();
}

//----------------------------------------------------------------------
// Insultar al vicioso del fumador
//----------------------------------------------------------------------
void Hospital::insultar()
{
    sanitaria.wait();
    cout << "FUMAR MATA: ya los sabes fumador " << num_fumador << endl;
    fumadores[num_fumador].wait();
}

//----------------------------------------------------------------------
// Despertar a la hebra sanitaria (intentarlo)
//----------------------------------------------------------------------
void Hospital::despertarSanitaria(unsigned vicioso)
{
    if (cont_fumadas[vicioso] == 5)
    {
        num_fumador = vicioso;
        sanitaria.signal();
        fumadores[num_fumador].signal();
        cout << "Soy el fumador " << num_fumador << " y me han llamado vicioso." << endl;
        cont_fumadas[num_fumador] = 0;
    }
}


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
//----------------------------------------------------------------------
// Monitor Estanco
//----------------------------------------------------------------------
//----------------------------------------------------------------------

class Estanco : public HoareMonitor
{

   private:

      // Declaramos variables permanentes
      int num_ingrediente;		     // (-1 cuando no hay ingrediente en el mostrador, 0,1,2 en caso contrario)

      CondVar                                // colas condición
         mostrador,  			     // cola en la que espera el estanquero a que el mostrador se vacíe.                         
         ingr_disp[num_fumadores];	     // colas en las que cada fumador espera a que su ingrediente esté disponible en el mostrador.

      // Procedimientos
      public:

         Estanco();
         void ponerIngrediente(unsigned i);
         void esperarRecogidaIncrediente();
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
   num_ingrediente = -1;
   mostrador       = newCondVar();
   for (unsigned i = 0; i < num_fumadores; i++)
      ingr_disp[i] = newCondVar();

}

//----------------------------------------------------------------------
// Obtener ingrediente i del mostrador
//----------------------------------------------------------------------

void Estanco::obtenerIngrediente(unsigned i)
{
   assert( i < num_fumadores);
   
   // Si el ingrediente no es del fumador, espera
   if (num_ingrediente != i) ingr_disp[i].wait();

   cout << "Se ha retirado del mostrador el ingrediente nº: "<< i << endl;
   // Lo retira del mostrador
   num_ingrediente = -1;
   mostrador.signal();

}

//----------------------------------------------------------------------
// Poner ingrediente en el mostrador
//----------------------------------------------------------------------

void Estanco::ponerIngrediente(unsigned i)
{
   
   // El número de ingrediete pasa a ser i
   num_ingrediente = i;
   cout << "Ingediente nº "<< i << " puesto en el mostrador." << endl;
   ingr_disp[i].signal();
   
}

//----------------------------------------------------------------------
// Esperar recogida del ingrediente
//----------------------------------------------------------------------

void Estanco::esperarRecogidaIncrediente()
{
   
   // Si hay un ingrediente encima del mostrador, el estanquero espera a que se recoja
   if(num_ingrediente != -1){
      cout << "Estanquero espera a que se recoja el ingrediente " << num_ingrediente << endl;
      mostrador.wait();
   }
   
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero( MRef<Estanco> monitor )
{
   while (true)
   {
      unsigned i = producir_ingrediente();
      monitor->ponerIngrediente(i);
      monitor->esperarRecogidaIncrediente();
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

    cont_fumadas[num_fumador]++;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( MRef<Estanco> monitor,MRef<Hospital> monitor1, unsigned i )
{
   while( true )
   {
      monitor->obtenerIngrediente(i);
      fumar(i);
      monitor1->despertarSanitaria(i);


   }
}

//----------------------------------------------------------------------
// función que ejecuta la hebra sanitaria
void  funcion_hebra_sanitaria( MRef<Hospital> monitor)
{
   while( true )
   {
      monitor->insultar();

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
   MRef<Hospital> monitor1 = Create<Hospital>() ;
   // declarar hebras y ponerlas en marcha
   
   thread hebras_fumadoras[num_fumadores];
   thread hebra_estanquero(funcion_hebra_estanquero, monitor);
   thread hebra_sanitaria(funcion_hebra_sanitaria,monitor1);
   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k] = thread (funcion_hebra_fumador,monitor,monitor1, k);
   	
   hebra_estanquero.join();
   hebra_sanitaria.join();
   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k].join();
   
}