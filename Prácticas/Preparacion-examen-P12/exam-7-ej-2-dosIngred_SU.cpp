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
mutex mtx;

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

vector<int> producir_ingredientes()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   cout << "Estanquero : empieza a producir ingredientes (" << duracion_produ.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   int a = 0, b = 0;
   vector<int> ingreds;
   while (a == b)
   {
        a = aleatorio<0,num_fumadores-1>();
        b = aleatorio<0,num_fumadores-1>();
   }
   
   ingreds.push_back(a);
   ingreds.push_back(b);

   // informa de que ha terminado de producir
   cout << "Estanquero : termina de producir ingredientes " << ingreds[0] << " y " << ingreds[1] << endl;

   return ingreds ;
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
      int ingred_a, ingred_b;        // (-1 cuando no hay ingrediente en el mostrador, 0,1,2 en caso contrario)

      CondVar                        // colas condición
         mostrador,                  // cola en la que espera el estanquero a que el fumador i recoja su ingrediente del mostrador                        
         ingr_disp[num_fumadores];	 // colas en las que cada fumador espera a que su ingrediente esté disponible en el mostrador.
        
      // Procedimientos
      public:

         Estanco();
         void ponerIngrediente(vector<int> ingreds);
         void esperarRecogidaIngrediente();
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
   
   ingred_a = -1;
   ingred_b = -1;

   mostrador = newCondVar();

   for (unsigned i = 0; i < num_fumadores; i++)
   {
      ingr_disp[i] = newCondVar();
   }

}

//----------------------------------------------------------------------
// Obtener ingrediente i del mostrador
//----------------------------------------------------------------------

void Estanco::obtenerIngrediente(unsigned i)
{
   assert( i < num_fumadores);
   
   // Si el ingrediente no es del fumador, espera
   if (i != ingred_a && i != ingred_b) ingr_disp[i].wait();

   cout << "Se ha retirado del mostrador el ingrediente nº: "<< i << endl;
   // Lo retira del mostrador
   if (i == ingred_a) ingred_a = -1;
   else if (i == ingred_b) ingred_b = -1;

   if (ingred_a == -1 && ingred_b == -1)
      mostrador.signal();
   
}

//----------------------------------------------------------------------
// Poner ingrediente en el mostrador
//----------------------------------------------------------------------

void Estanco::ponerIngrediente(vector<int> ingreds)
{
   
   //assert(ingreds.size() == 2);

   ingred_a = ingreds[0];
   ingred_b = ingreds[1];


   cout << "Ingedientes nº "<< ingred_a << " y " << ingred_b << " puestos en el mostrador." << endl;
   ingr_disp[ingred_a].signal();
   ingr_disp[ingred_b].signal();

   
}

void Estanco::esperarRecogidaIngrediente()
{
   if (ingred_a != -1 || ingred_b != -1 )
      mostrador.wait();
   
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero( MRef<Estanco> monitor )
{
   while (true)
   {
      vector<int> ingredientes = producir_ingredientes();
      monitor->ponerIngrediente(ingredientes);
      monitor->esperarRecogidaIngrediente();
   }
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar
    mtx.lock();
    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
    mtx.unlock();
   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar
    mtx.lock();
    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
    mtx.unlock();
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
   thread hebra_estanquero(funcion_hebra_estanquero, monitor);
   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k] = thread (funcion_hebra_fumador,monitor, k);
   	
   hebra_estanquero.join();
   
   for (int k = 0; k < num_fumadores; k++)
   	hebras_fumadoras[k].join();
   
}