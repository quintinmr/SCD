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
               
               num_surt              = 3,
               surtidores[num_surt]  = {4,5,3},

               num_hebras            = 15,     //número de hebras coche gasolina
               num_procesos_esperado = num_hebras+1,  // número de procesos esperado
               id_gasolinera         = num_hebras,     // id del proceso gasolinera
               // etiqueta que indica comienzo de repostaje va desde 0 a (num--surt-1)
               etiq_finaliza         = num_surt;     // etiqueta que indica fin de repostaje

unsigned 
   num_surtidores_libres[ num_surt ] = { surtidores[0], surtidores[1], surtidores[2] };

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

    const unsigned tipo_combustible = aleatorio<0,num_surt-1>();

    while(true)
    {

        // solicitar a gasolinera comenzar a repostar
        cout << "Coche número "<< id << " solicita repostar. " << endl;
        MPI_Ssend(&tipo_combustible, 1, MPI_INT, id_gasolinera,tipo_combustible, MPI_COMM_WORLD);

        // repostar
        repostar();

        // indicar a gasolinera que se ha terminado de repostar
        MPI_Ssend(&tipo_combustible, 1, MPI_INT, id_gasolinera,etiq_finaliza, MPI_COMM_WORLD);
    }
    
}

// ---------------------------------------------------------------------
// Función que ejecuta el proceso gasolinera
// ---------------------------------------------------------------------
void funcion_gasolinera()
{
    unsigned peticion = 0,                  // petición recibida
             etiq_aux = 0;                  // etiqueta aceptable

    MPI_Status estado ;                     // metadatos de las dos recepciones

    while(true)
    {
        // determinar si hay mensajes de terminar
        int mensajes = 0;
        MPI_Iprobe(MPI_ANY_SOURCE, etiq_finaliza, MPI_COMM_WORLD, &mensajes, &estado);

        if (mensajes > 0)
        {
            // recibir petición de cualquier coche con esa etiqueta
            MPI_Recv(&peticion, 1, MPI_INT, estado.MPI_SOURCE, etiq_finaliza, MPI_COMM_WORLD, &estado);
            num_surtidores_libres[peticion]++;
            cout << "Gasolinera procesando petición para fin de repostaje de coche " << estado.MPI_SOURCE << " en surtidor tipo " << peticion << endl;
        }

        // comprobar y recibir mensajes de comienzo de repostaje, por orden de tipo de combustible
        for (unsigned i = 0; i < num_surt; i++)
            if (num_surtidores_libres[i] > 0)
            {
                int mensaje_tipo = 0;
                MPI_Iprobe(MPI_ANY_SOURCE, i, MPI_COMM_WORLD, &mensaje_tipo, &estado);

                if (mensaje_tipo > 0)
                {
                    MPI_Recv(&peticion, 1, MPI_INT, estado.MPI_SOURCE, i, MPI_COMM_WORLD, &estado);
                    num_surtidores_libres[i]--;
                    mensajes = 1;
                    cout << "Gasolinera procesando petición de inicio de repostaje de coche " << estado.MPI_SOURCE << " en surtidor "<< peticion << endl;

                }
            }
	
        if (mensajes == 0)
           this_thread::sleep_for( chrono::milliseconds( 20 )); 
        
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
