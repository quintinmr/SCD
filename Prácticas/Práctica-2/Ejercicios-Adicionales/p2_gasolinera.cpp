#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "scd.h"

using namespace std ;
using namespace scd ;

const unsigned num_hebras_gasoil   = 4,     //número de hebras coche gasoil
               num_hebras_gasolia  = 6,     //número de hebras coche gasolina
               num_surt_gasoil     = 2,     //número de surtidores de gasoil
               num_surt_gasolina   = 3;     //número de surtidores de gasolina

mutex salida;

enum tipo {GASOIL, GASOLINA};

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
//  Monitor
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
class Gasolinera : public HoareMonitor
{
    private:
        // Declaración de variables permanentes del monitor
        unsigned num_surt_uso_gasoil,           //número de surtidores de gasoil en uso
                 num_surt_uso_gasolina;         //número de surtidores de gasolina en uso
        CondVar 
                cola_gasoil,    //cola en la que esperan los coches que desea gasoil
                cola_gasolina;   //cola en la que esperan los coches que desean gasolina

    public:
        // Declaración de los procedimientos del monitor
        Gasolinera();
        void entra_coche_gasoil(unsigned num_coche);
        void sale_coche_gasoil(unsigned num_coche);
        void entra_coche_gasolina(unsigned num_coche);
        void sale_coche_gasolina(unsigned num_coche);
};

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
//                         Implementación de los métodos del monitor
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------
Gasolinera::Gasolinera()
{
    num_surt_uso_gasoil   = 0;
    num_surt_uso_gasolina = 0;
    cola_gasoil   = newCondVar();
    cola_gasolina = newCondVar();
}
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
// Función que simula la entrada al surtidor de gasoil
//----------------------------------------------------------------------------------------
void Gasolinera::entra_coche_gasoil(unsigned num_coche)
{
    //Debe esperar a que quede un surtidor de gasoil libre
    if (num_surt_uso_gasoil == num_surt_gasoil)
        cola_gasoil.wait();
        
    assert(num_surt_uso_gasoil <= num_surt_gasoil);
    
    cout << "Coche "<< num_coche << " de Gasoil entra en gasolinera. Núm. surtidores de gasoil libres: " << num_surt_gasoil - num_surt_uso_gasoil << endl; 
    num_surt_uso_gasoil++;
}
//----------------------------------------------------------------------------------------
// Función que simula la entrada al surtidor de gasolina
//----------------------------------------------------------------------------------------
void Gasolinera::entra_coche_gasolina(unsigned num_coche)
{
    //Debe esperar a que quede un surtidor de gasolina libre
    if (num_surt_uso_gasolina == num_surt_gasolina)
        cola_gasolina.wait();
    
    assert(num_surt_uso_gasolina <= num_surt_gasolina);
    cout << "Coche "<< num_coche << " de Gasolina entra en gasolinera. Núm. surtidores de gasolina libres: " << num_surt_gasolina - num_surt_uso_gasolina << endl; 
    num_surt_uso_gasolina++;
}
//----------------------------------------------------------------------------------------
// Función que simula la salida del surtidor de gasoil
//----------------------------------------------------------------------------------------
void Gasolinera::sale_coche_gasoil(unsigned num_coche)
{
    num_surt_uso_gasoil--;
    assert(num_surt_uso_gasoil >= 0);
    cout << "Coche número " << num_coche << " de Gasoil sale de la gasolinera." << endl;
    cola_gasoil.signal();

}
//----------------------------------------------------------------------------------------
// Función que simula la salida del surtidor de gasolina
//----------------------------------------------------------------------------------------
void Gasolinera::sale_coche_gasolina(unsigned num_coche)
{
    num_surt_uso_gasolina--;
    assert(num_surt_uso_gasoil >= 0);
    cout << "Coche número " << num_coche << " de Gasolina sale de la gasolinera." << endl;
    cola_gasolina.signal();
}
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
// Función repostar(), común a todas las hebras
void repostar(int coche, tipo tipo)
{

   // calcular milisegundos aleatorios de duración de la acción de repostar)
   chrono::milliseconds duracion_repo( aleatorio<10,100>() );
   salida.lock();
   // informa de que comienza a repostar
   cout << "Comienza a repostar coche número " << coche <<  (tipo == tipo::GASOIL ? " gasoil: ": " gasolina: ") << duracion_repo.count() << " milisegundos" << endl;
   salida.unlock();
   // espera bloqueada un tiempo igual a ''duracion_repo' milisegundos
   this_thread::sleep_for( duracion_repo );
   salida.lock();
   // informa de que ha terminado de repostar
   cout << "Termina de repostar coche número " << coche <<  (tipo == tipo::GASOIL ? " gasoil. ": " gasolina.")<< endl;
   salida.unlock();

}
//----------------------------------------------------------------------------------------
// Función salir(), común a todas las hebras
void salir(int coche, tipo tipo)
{

   // calcular milisegundos aleatorios de duración de la acción de salir)
   chrono::milliseconds duracion_salir( aleatorio<10,100>() );
   salida.lock();
   // informa de que comienza a salir
   cout << "Comienza a salir coche número " << coche <<  (tipo == tipo::GASOIL ? " gasoil: ": " gasolina: ") << duracion_salir.count() << " milisegundos" << endl;
   salida.unlock();
   // espera bloqueada un tiempo igual a ''duracion_repo' milisegundos
   this_thread::sleep_for( duracion_salir );
    salida.lock();
   // informa de que ha terminado de salir
   cout << "Termina de salir coche número " << coche <<  (tipo == tipo::GASOIL ? " gasoil. ": " gasolina.")<< endl;
   salida.unlock();
}
//----------------------------------------------------------------------

// Función que ejecuta cada hebra gasoil
void funcion_hebra_gasoil (MRef<Gasolinera> monitor, unsigned num_coche)
{
    while (true)
    {
        monitor->entra_coche_gasoil(num_coche);
        repostar(num_coche, tipo::GASOIL);
        monitor->sale_coche_gasoil(num_coche);
        salir(num_coche, tipo::GASOIL);
    }
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
// Función que ejecuta cada hebra gasolina
void funcion_hebra_gasolina (MRef<Gasolinera> monitor, unsigned num_coche)
{
    while (true)
    {
        monitor->entra_coche_gasolina(num_coche);
        repostar(num_coche, tipo::GASOLINA);
        monitor->sale_coche_gasolina(num_coche);
        salir(num_coche, tipo::GASOLINA);
    }
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
//  Programa principal
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
int main ()
{
   cout << "--------------------------------------------------------------------" << endl
        << "Problema de la gasolinera con monitor SU. " << endl
        << "--------------------------------------------------------------------" << endl
        << flush ;

   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<Gasolinera> monitor = Create<Gasolinera>() ;

   // crear y lanzar las hebras
   thread hebras_gasoil[num_hebras_gasoil], hebras_gasolina[num_hebras_gasolia];
   
   for (int i = 0; i < num_hebras_gasoil; i++)
   	hebras_gasoil[i] = thread ( funcion_hebra_gasoil, monitor, i );
   for (int i = 0; i < num_hebras_gasolia; i++)
   	hebras_gasolina[i] = thread ( funcion_hebra_gasolina, monitor,i);

   // esperar a que terminen las hebras
   for (int i = 0; i < num_hebras_gasoil; i++)
   	hebras_gasoil[i].join() ;
   for (int i = 0; i < num_hebras_gasolia; i++)
   	hebras_gasolina[i].join() ;

}
