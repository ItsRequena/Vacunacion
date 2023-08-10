#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define MUTEX

//Buffer de datos al que acceden los hilos
#define TAMANO_BUFFER 10000
#define TANDAS 10

FILE *f_entrada,*f_salida;
pthread_mutex_t mutex,mutex2;
pthread_cond_t espera[TANDAS];
int habitantes,vacunas_iniciales,min_vacunas_tanda,max_vacunas_tanda,min_fabric,max_fabric,min_reparto,max_reparto,min_reaccion,max_reaccion,min_despl,max_despl;
int centros=5;
int fabricas=3;
int tandas=10;
int max_vacunas_fabrica=400;

int contadorVacunas=0;//contador de todas las vacunas que se han hecho el total tiene que ser al final igual a los habitantes

typedef struct{
    int vacunas;//vacunas que se almacenan antes de entregarlas al centro de vacunacion
    int vacunasTotalesGeneradas;
}inf_farmaceuticas;

inf_farmaceuticas farmaceuticas[3];//tenemos 3 centros de vacunacion

typedef struct{//informacion del centro
    int vacunasDisponibles;
}inf_centro;

inf_centro centro[5];

typedef struct{
    int tanda; // a que tanda pertence
    int vacunado; // si esta vacunado o no ( 0 no vacunado, 1 vacunado)
    int id; // nº del habitante
} inf_habitante;

inf_habitante *habitante = NULL; // futuro array de habitantes

void *vacunacion (void *parametro);
void *fabrica_vacunas();
void reparto(int parametro);
void *estado (int i,int id,int centro);
int comprobacion_tanda_vacunada(int tanda,inf_habitante habitante[habitantes]);
void *bucle();

int main(int argc, char* argv[]){
    
	pthread_t th,th2;
    habitante = calloc(habitantes, sizeof(int));

    if (argc>3){
        printf("nº de parametros incorrecto");
        return 1;
    }
    else if(argc==3){
        f_entrada = fopen(argv[1],"r");
        f_salida = fopen(argv[2],"w");

    }
    else if(argc==2){
        f_entrada = fopen(argv[1],"r");
        f_salida = fopen("nombre_fichero_salida.txt","w");
    }
    else if(argc==1){
        f_entrada = fopen("nombre_fichero_entrada.txt","r");
        f_salida = fopen("nombre_fichero_salida.txt","w");
    }

    fscanf(f_entrada,"%i",&habitantes);
    fscanf(f_entrada,"%i",&vacunas_iniciales);
    fscanf(f_entrada,"%i",&min_vacunas_tanda);
    fscanf(f_entrada,"%i",&max_vacunas_tanda);
    fscanf(f_entrada,"%i",&min_fabric);
    fscanf(f_entrada,"%i",&max_fabric);
    fscanf(f_entrada,"%i",&min_reparto);
    fscanf(f_entrada,"%i",&max_reparto);
    fscanf(f_entrada,"%i",&min_reaccion);
    fscanf(f_entrada,"%i",&max_reaccion);
    fscanf(f_entrada,"%i",&min_despl);
    fscanf(f_entrada,"%i",&max_despl);

    fprintf(f_salida,"VACUNACIÓN EN PANDEMIA: CONFIGURACIÓN INICIAL\n");
    fprintf(f_salida,"Habitantes: %i\n",habitantes);
    fprintf(f_salida,"Centro de vacunación: %i\n",centros);
    fprintf(f_salida,"Fábricas: %i\n",fabricas);
    fprintf(f_salida,"Vacunados por tanda: %i\n",habitantes/tandas);
    fprintf(f_salida,"Vacunas iniciales en cada centro: %i\n",vacunas_iniciales);
    fprintf(f_salida,"Vacunas totales por fábrica: %i\n",max_vacunas_fabrica);
    fprintf(f_salida,"Mínimo número de vacunas fabricadas en cada tanda: %i\n",min_vacunas_tanda);
    fprintf(f_salida,"Máximo número de vacunas fabricadas en cada tanda: %i\n",max_vacunas_tanda);
    fprintf(f_salida,"Tiempo mínimo de fabricación de una tanda de vacunas: %i\n",min_fabric);
    fprintf(f_salida,"Tiempo máximo de fabricación de una tanda de vacunas: %i\n",max_fabric);
    fprintf(f_salida,"Tiempo máximo de reparto de vacunas a los centros: %i\n",max_reparto);
    fprintf(f_salida,"Tiempo máximo que un habitante tarda en ver que está citado para vacunarse: %i\n",max_reaccion);
    fprintf(f_salida,"Tiempo máximo de desplazamiento del habitante al centro de vacunación: %i\n",max_despl);
    printf("VACUNACIÓN EN PANDEMIA: CONFIGURACIÓN INICIAL\n");
    printf("Habitantes: %i\n",habitantes);
    printf("Centro de vacunación: %i\n",centros);
    printf("Fábricas: %i\n",fabricas);
    printf("Vacunados por tanda: %i\n",habitantes/tandas);
    printf("Vacunas iniciales en cada centro: %i\n",vacunas_iniciales);
    printf("Vacunas totales por fábrica: %i\n",max_vacunas_fabrica);
    printf("Mínimo número de vacunas fabricadas en cada tanda: %i\n",min_vacunas_tanda);
    printf("Máximo número de vacunas fabricadas en cada tanda: %i\n",max_vacunas_tanda);
    printf("Tiempo mínimo de fabricación de una tanda de vacunas: %i\n",min_fabric);
    printf("Tiempo máximo de fabricación de una tanda de vacunas: %i\n",max_fabric);
    printf("Tiempo máximo de reparto de vacunas a los centros: %i\n",max_reparto);
    printf("Tiempo máximo que un habitante tarda en ver que está citado para vacunarse: %i\n",max_reaccion);
    printf("Tiempo máximo de desplazamiento del habitante al centro de vacunación: %i\n",max_despl);

    contadorVacunas=vacunas_iniciales*5;//se empieza con el numero de vacunas iniciales de los 5 centros de vacunacion

	pthread_mutex_init (&mutex, NULL);
    for(int k=0;k<centros; k++){
        pthread_cond_init(&espera[k], NULL); // condicion de espera para cada centro cuando se queden sin vacunas
        centro[k].vacunasDisponibles=vacunas_iniciales; // asignamos a cada centro las vacunas iniciales
    }
    pthread_create(&th2,NULL,bucle,NULL); // creamos un thread que nos mantenga la fabricacion de vacunas y la reparticion siempre activas
    for(int i=0; i<TANDAS; i++){
        for(int j=0; j<habitantes/TANDAS; j++){
            habitante[j].tanda=i;
            habitante[j].vacunado=0;
            habitante[j].id=(120*i)+j;
            pthread_create(&th,NULL,vacunacion,(void*)&habitante[j]);
        }
        while(comprobacion_tanda_vacunada(i,habitante)==0){} // hasta que no este toda la tanda vacunada no pasa
    }
    printf("Vacunacion completa!\n");
    fprintf(f_salida,"Vacunacion completa!\n");
    fclose(f_salida);
}

/* Funcion que se ejecuta en el threaad */
void *vacunacion(void *parametro){
    inf_habitante persona = *(inf_habitante *) parametro;

    int cent = ( rand() % 5) + 1;

    // habitante es llamado para vacunarse

    sleep( (rand() % max_reaccion) + 1); // tiempo hasta que el habitante se da cuenta de que ha sido seleccionado
    estado(0,persona.id,cent);
    sleep( (rand() % max_despl) + 1); // tiempo hasta que el habitante llega al centro

    // habitante llega al centro
    // comprobar si hay vacuna
    pthread_mutex_lock(&mutex); // bloqueo el mutex
    if (centro[cent].vacunasDisponibles==0)
        estado(1,persona.id,cent);
    while(centro[cent].vacunasDisponibles==0)
        pthread_cond_wait(&espera[cent], &mutex);
    //habitante es vacunado
    estado(2,persona.id,cent);
    habitante[persona.id].vacunado=1;
    centro[cent].vacunasDisponibles-= 1;
    pthread_mutex_unlock(&mutex); // desbloqueo el mutex

}
void *fabrica_vacunas(){
    for (int j=0; j<3; j++){
        int vacunasgeneradasFarm1 = ( rand() % (max_vacunas_tanda-min_vacunas_tanda+1)) + min_vacunas_tanda;
        int tiempoGenFarm1 = ( rand() % (max_fabric-min_fabric+1)) + min_fabric;
        int i;
        if (farmaceuticas[j].vacunasTotalesGeneradas<400){
            if(farmaceuticas[j].vacunasTotalesGeneradas+vacunasgeneradasFarm1>400)
                vacunasgeneradasFarm1=400-farmaceuticas[j].vacunasTotalesGeneradas; //en caso de que superen las 400 , fabricamos las que falten
            for (int i=0;i<vacunasgeneradasFarm1;i++){
                farmaceuticas[j].vacunas++;//se incrementa el numero de vacunas que tiene para repartir la farmaceutica
                contadorVacunas++;//aumenta el numero total de vacunas que se han creado
            }
        //se suman las vacunas generadas al numero total de vacunas creadas por esa farmaceutica
        farmaceuticas[j].vacunasTotalesGeneradas+=vacunasgeneradasFarm1;
        sleep(tiempoGenFarm1);//por cada tanda de vacunas tardan x tiempo en hacerlas
        reparto(j);
        }
    }
}

void reparto(int parametro){
    int NumeroFarmaceutica= parametro;
    int tiemporeparto = ( rand() % (max_reparto)) + 1;
    sleep(tiemporeparto);//por cada vacuna tardan x tiempo en repartirlas
    for(int i=0;i<5;i++){
        int agregar=farmaceuticas[NumeroFarmaceutica].vacunas*(0.2*(i+1)); //repartimos el 20% a cada centro
        centro[i].vacunasDisponibles+=agregar;
        //se quita el numero de vacunas agregadas de la reserva de la farmaceutica
        farmaceuticas[NumeroFarmaceutica].vacunas-=agregar;
        printf("Se han repartido %i vacunas al centro %i\n",agregar,i+1);
        fprintf(f_salida,"Se han repartido %i vacunas al centro %i\n",agregar,i+1);
        pthread_cond_signal(&espera[i]); //activamos la condicion de espera en caso de que haya algun centro sin vacunas

    }
}

void *estado(int i,int id,int centro){
    if (i==0){
        printf("Habitante %i elige el centro %i para vacunarse\n",id,centro);
        fprintf(f_salida,"Habitante %i elige el centro %i para vacunarse\n",id,centro);
    }
    else if(i==2){
        printf("Habitante %i vacunando el centro %i\n",id,centro);
        fprintf(f_salida,"Habitante %i vacunando el centro %i\n",id,centro);
    }
    else if(i==1){
        printf("Habitante %i esperando ser vacunado en el centro %i...\n",id,centro);
        fprintf(f_salida,"Habitante %i esperando ser vacunado en el centro %i...\n",id,centro);
    }
}

int comprobacion_tanda_vacunada(int tanda,inf_habitante habitante[habitantes]){ //comprueba si una tanda esta completamente vacunada
    for(int i=0;i<habitantes/TANDAS; i++){
        if(habitante[tanda*120+i].vacunado==0)
            return 0;
    }
    return 1;
}

void *bucle(){
    while(contadorVacunas<habitantes){
        fabrica_vacunas();
    }
}