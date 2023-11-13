#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "scd.h"

using namespace std ;
using namespace scd ;

const int 
    numero_lectores = 5,    // número de hebras lectoras
    numero_escritores = 4;  // número de hebras escritoras

mutex controlador;   // controla la escritura por pantalla

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Monitor Lec_Esc
//----------------------------------------------------------------------
//----------------------------------------------------------------------

class Lec_Esc : public HoareMonitor
{

    // Declaración de variables permanentes
    unsigned num_lect_leyendo;               // número de lectores leyendo
    bool escribiendo;                        // true si hay algún escritor escribiendo

    CondVar                                  // colas condición
      lectura,                               // cola en la que esperan los lectores
      escritura;                             // cola en la que esperan los escritores

    // Procedimientos del monitor
    public:
      
      Lec_Esc();

      void ini_lectura();
      void fin_lectura();

      void ini_escritura();
      void fin_escritura();
      
};

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Implementación procedimientos del monitor
//----------------------------------------------------------------------
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------

Lec_Esc::Lec_Esc()
{
    num_lect_leyendo = 0;
    escribiendo      = false;
    lectura          = newCondVar();
    escritura        = newCondVar();
}

//----------------------------------------------------------------------
// Inicio de lectura
//----------------------------------------------------------------------

void Lec_Esc::ini_lectura()
{
    // si hay algún escritor, esperar
    if (escribiendo) lectura.wait();

    // si no hay escritores escribiendo, se registra un lector más
    num_lect_leyendo++;

    // desbloqueo de lectores
    lectura.signal();

}

//----------------------------------------------------------------------
// Fin lectura
//----------------------------------------------------------------------

void Lec_Esc::fin_lectura()
{
    // registrar un lector menos
    num_lect_leyendo--;
    // si este es el último lector que había, entonces, se desbloquea escritor
    if (num_lect_leyendo == 0) escritura.signal();
}

//----------------------------------------------------------------------
// Inicio escritura
//----------------------------------------------------------------------
void Lec_Esc::ini_escritura()
{
    // si hay otros (lectores o escritores), esperar
    if (num_lect_leyendo > 0 or escribiendo) escritura.wait();

    escribiendo = true;
}

//----------------------------------------------------------------------
// Fin escritura
//----------------------------------------------------------------------

void Lec_Esc::fin_escritura()
{
    // registrar que ya no hay escritor
    escribiendo = false;

    //  SE LE DA PRIORIDAD A LOS LECTORES
    // SE DESPIERTAN TODOS LOS LECTORES
    if (!lectura.empty()) lectura.signal();

    // si no, desperar un escritor
    escritura.signal();
}

//-------------------------------------------------------------------------
// Función que simula la acción de leer por parte de la hebra i

void leer(int i)
{
	chrono::milliseconds duracion_lectura( aleatorio<10,100>() );
	// informa de la espera
	controlador.lock();
	cout << "Lector " << i << " leyendo (" << duracion_lectura.count() << " milisegundos)" << endl;
	controlador.unlock();
	this_thread::sleep_for(duracion_lectura);
}

// Función que simula la acción de escribir por parte de la hebra i

void escribir(int i)
{
	chrono::milliseconds duracion_escritura( aleatorio<10,100>() );
	// informa de la espera
	controlador.lock();
	cout << "Escritor " << i << " escribiendo (" << duracion_escritura.count() << " milisegundos)" << endl;
	controlador.unlock();
	this_thread::sleep_for(duracion_escritura);
}

//-------------------------------------------------------------------------
// Función que simula la acción de esperar, como un retardo aleatoria de la hebra
void esperar( )
{

   // calcular milisegundos aleatorios de duración de la acción de esperar)
   chrono::milliseconds duracion_esperar( aleatorio<20,200>() );

   // espera bloqueada un tiempo igual a ''duracion_esperar' milisegundos
   this_thread::sleep_for( duracion_esperar);

   
}

//----------------------------------------------------------------------
// Función que ejecutan las hebras lectoras
//----------------------------------------------------------------------

void funcion_hebra_lectora (MRef<Lec_Esc> monitor, unsigned num_hebra)
{
    while (true)
    {
        monitor->ini_lectura();
        leer(num_hebra);
        monitor->fin_lectura();
        esperar();

    }
}

//----------------------------------------------------------------------
// Función que ejecutan las hebras escritoras
//----------------------------------------------------------------------

void funcion_hebra_escritora (MRef<Lec_Esc> monitor, unsigned num_hebra)
{
    while (true)
    {
        monitor->ini_escritura();
        escribir(num_hebra);
        monitor->fin_escritura();
        esperar();

    }
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

int main()
{

	cout << "____________________________________________________________________" << endl;
	cout << endl;
	cout << "			PROBLEMA DE LOS LECTORES Y ESCRITORES                        " << endl;
	cout << "____________________________________________________________________" << endl << flush;
   
   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<Lec_Esc> monitor = Create<Lec_Esc>() ;
   
   // declarar hebras y ponerlas en marcha
   thread hebras_lectoras[numero_lectores],
   	  hebras_escritoras[numero_escritores];
   
   for (int i = 0; i < numero_lectores; i++)
   	hebras_lectoras[i] = thread(funcion_hebra_lectora, monitor, i);
   for (int i = 0; i < numero_escritores; i++)
   	hebras_escritoras[i] = thread(funcion_hebra_escritora, monitor, i);
   for (int i = 0; i < numero_lectores; i++)
   	hebras_lectoras[i].join();
   for (int i = 0; i < numero_escritores; i++)
   	hebras_escritoras[i].join();
   	
}




