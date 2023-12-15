/*
   -Nombre: Quintín
   -Apellidos: Mesa Romero
   -DNI: 78006011Q
*/
#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const unsigned 
               num_hebras_gasolia    = 6,     //número de hebras coche gasolina
               num_surt_gasolina     = 3,     //número de surtidores de gasolina
               num_procesos_esperado = num_hebras_gasolia+1,  // número de procesos esperado
               id_gasolinera         = num_hebras_gasolia,     // id del proceso gasolinera
               etiq_comienza         = 0,     // etiqueta que indica comienzo de repostaje
               etiq_finaliza         = 1;     // etiqueta que indica fin de repostaje

//----------------------------------------------------------------------
/// @brief plantilla de función para generar un entero aleatorio uniformemente
/// distribuido entre dos valores enteros, ambos incluidos
/// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
/// @tparam min - valor mínimo (int)
/// @tparam max - valor máximo (int)
/// @return número 'int' aleatorio uniformemente distribuido entew 'min' y 'max', ambos incluidos
///
template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
// Función repostar(), común a todas las hebras
void repostar()
{

   // calcular milisegundos aleatorios de duración de la acción de repostar)
   chrono::milliseconds duracion_repo( aleatorio<10,100>() );
   // espera bloqueada un tiempo igual a ''duracion_repo' milisegundos
   this_thread::sleep_for( duracion_repo );
  

}
// ---------------------------------------------------------------------
// Función que ejecuta el proceso coche
// ---------------------------------------------------------------------
void funcion_coche(const unsigned id)
{
    unsigned valor;

    while(true)
    {

        // solicitar a gasolinera comenzar a repostar
        cout << "Coche número "<< id << " solicita repostar. " << endl;
        MPI_Ssend(&valor, 1, MPI_INT, id_gasolinera,etiq_comienza, MPI_COMM_WORLD);

        // repostar
        repostar();

        // indicar a gasolinera que se ha terminado de repostar
        MPI_Ssend(&valor, 1, MPI_INT, id_gasolinera,etiq_finaliza, MPI_COMM_WORLD);
    
    }
    
}

// ---------------------------------------------------------------------
// Función que ejecuta el proceso gasolinera
// ---------------------------------------------------------------------
void funcion_gasolinera()
{
    unsigned petición = 0,                  // petición recibida
             surt_lib = num_surt_gasolina,  // número de surtidores libres en este momento
             etiq_aux = 0;                  // etiqueta aceptable

    MPI_Status estado ;                     // metadatos de las dos recepciones

    while(true)
    {
        // determinar la(s) etiqueta(s) aceptables
        if (surt_lib > 0)              
            etiq_aux = MPI_ANY_TAG;  
        
        else etiq_aux = etiq_finaliza;

        // recibir petición de cualquier coche con esa etiqueta
        MPI_Recv(&petición, 1, MPI_INT, MPI_ANY_SOURCE, etiq_aux, MPI_COMM_WORLD, &estado);
        
        // procesamiento del mensaje que se ha recibido
        switch(estado.MPI_TAG)
        {
            case etiq_comienza:
                cout << "Coche " << estado.MPI_SOURCE << " ha comenzado a repostar." << endl;
                surt_lib--;
                break;

            case etiq_finaliza:
                cout << "Coche " << estado.MPI_SOURCE << " ha terminado de repostar." << endl;
                surt_lib++;
                break;
        }
    }
    

}

// ---------------------------------------------------------------------


int main( int argc, char *argv[] )
{
   int id_propio, num_procesos_actual;

   // inicializar MPI, leer identif. de proceso y número de procesos
   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

   if ( num_procesos_esperado == num_procesos_actual )
   {
      // ejecutar la operación apropiada a 'id_propio'
      if ( id_propio == id_gasolinera )
         funcion_gasolinera();
        
      else 
         funcion_coche(id_propio);
      
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos_esperado << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   // al terminar el proceso, finalizar MPI
   MPI_Finalize( );
   return 0;
}
