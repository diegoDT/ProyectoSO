#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <mysql.h>
#include <pthread.h>
#include <netinet/tcp.h>

//Estructuras
typedef struct{
	//Nombre del ataque 
	char nombre[20];
	//Da�o del ataque 
	int damage;
	//Informacion del ataque 
	char info[200];
	//Tipo del ataque1
	char tipo[20];
}Tataque;
typedef struct{
	//Nombre del pokemon 
	char nombre[20];
	//Un pokemon solo puede tener 4 ataques
	Tataque ataques[4];
	//Tipo del pokemon 
	char tipo[20];
}Tpokemon;
//Jugadores conectados
typedef struct{
	//Apodo del jugador conectado
	char apodo[10];
	//Nivel del jugador 
	char nivel[20];
	//Socket asignado al jugador
	int socket;
	//Equipo formado por 2 pokemon del jugador
	Tpokemon equipo[2];
}Tjugador;
typedef struct{
	//Lista de jugadores conectados
	Tjugador conectados[100];
	//Numero de jugadores conectados 
	int numeroconectados;
}Tlistaconectados;
//En la primera version solo 1 combate a la vez
typedef struct{
	//En el combate participan solo 2 jugadores
	Tjugador jugador1;
	Tjugador jugador2;
	//Ganador del combate
	char ganador[10];
	//En el combate participaran 2 pokemon de cada jugador
	Tpokemon pokemons[2];
	//Puntuacion jugador 1
	int puntuacion1;
	//Puntuacion jugador 2
	int puntuacion2;
	//Identificador del combate
	int id;
}Combate;

//Variables globales 
	MYSQL *conn;
	Tlistaconectados lista;
	//Estructura para el acceso excluyente
	pthread_mutex_t accesoexcluyente;
	

//Consulta mayor puntuacion de un jugador 
int Mayor_puntuacion_jugador(char apodo[10]){

	MYSQL_RES *resultado;
	MYSQL_ROW row;
	int err;
	char consulta[200];
	sprintf(consulta,"SELECT puntuacion.puntuacion FROM puntuacion,jugador,combate WHERE jugador.apodo='%s' AND puntuacion.jugador='%s' AND puntuacion.combate = combate.id ORDER BY puntuacion.puntuacion DESC",apodo,apodo);
	err=mysql_query (conn,consulta);
	if (err!=0) {
		printf ("Error al consultar datos de la base %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	resultado = mysql_store_result (conn);
	row = mysql_fetch_row (resultado);
	
	if (row == NULL)
		return -1;
	else{
		
		int puntuacion = atoi(row[0]);
		return puntuacion;
	}	
	
}
//Datos de un combate
int Datos_combate(int id, char ganador[20], char jugador1[20],char jugador2[20],int *puntuacion1,int *puntuacion2){
		
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	int err;
	char consulta[300];
	
	sprintf(consulta,"SELECT combate.ganador,puntuacion.jugador,puntuacion.puntuacion FROM combate,puntuacion WHERE combate.id = %d AND puntuacion.combate = %d",id,id);
	err = mysql_query(conn,consulta);
	if(err!=0){
		printf("Error al realizar la consulta %u %s\n", mysql_errno(conn),mysql_error(conn));
		exit(1);
	}
	resultado = mysql_store_result(conn);
	row = mysql_fetch_row(resultado);
	if(row==NULL){
		printf("No se han recogido datos de la consulta\n");
		return -1;
	}
	else{
		strcpy(ganador,row[0]);
		strcpy(jugador1,row[1]);
		*puntuacion1 = atoi(row[2]);
		row = mysql_fetch_row(resultado);
		strcpy(jugador2,row[1]);
		*puntuacion2 = atoi(row[2]);
		return 1;
	}
}
//Numero de partidas ganadas por un jugador
int Numero_partidas_ganadas(char apodo[20]){
	
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	int err;
	char consulta[200];
	
	//Miramos si existe el jugador
	sprintf(consulta,"SELECT apodo FROM jugador WHERE apodo ='%s'",apodo);
	err = mysql_query(conn,consulta);
	if(err!=0){
		printf("Error al realizar la consulta %u %s\n", mysql_errno(conn),mysql_error(conn));
		exit(1);
	}
	resultado = mysql_store_result(conn);
	row = mysql_fetch_row(resultado);
	if(row == NULL){
		printf("No se han recogido datos de la consulta");
		return -1;
	}
	//Miramos el numero de partidas ganadas
	else{
		sprintf(consulta,"SELECT ganador FROM combate WHERE ganador='%s'",apodo);
		err=mysql_query (conn, consulta);
		if(err!=0){
			printf("Error al realizar la consulta %u %s\n", mysql_errno(conn),mysql_error(conn));
			exit(1);
		}
		//Recogida del resultado de la consulta
		int cont=0;
		resultado=mysql_store_result(conn);
		row=mysql_fetch_row(resultado);
		if(row==NULL){
			//No ha ganado ninguna partida
			return 0;
		}
		else{
			while(row!=NULL){
				cont++;
				row=mysql_fetch_row(resultado);
			}
			return cont;
		}
	}
}
//Registrar jugador
int Registrar_jugador(char nombre[10],char pass[10],char apodo[20]){
	
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	int err;
	char consulta[200];
	
	sprintf(consulta,"SELECT nombrecuenta,pass,apodo FROM jugador WHERE nombrecuenta='%s' AND pass='%s' AND apodo='%s'",nombre,pass,apodo);
	err= mysql_query(conn,consulta);
	if(err!=0){
		printf("Error al realizar la consulta %u %s\n", mysql_errno(conn),mysql_error(conn));
		exit(1);
	}
	resultado = mysql_store_result(conn);
	row = mysql_fetch_row(resultado);
	if(row == NULL){//No existe el jugador
		//No existe el jugador
		return 1;
	}
	else{
		//Existe el jugador
		return -1;
	}
}
//Logear jugador
int Logear_jugador(char nombre[10],char pass[10], char apodo[20]){
	
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	int err;
	char consulta[200];
	
	sprintf(consulta,"SELECT nombrecuenta,pass,apodo FROM jugador WHERE nombrecuenta='%s' AND pass='%s'",nombre,pass);
	err= mysql_query(conn,consulta);
	if(err!=0){
		printf("Error al realizar la consulta %u %s\n", mysql_errno(conn),mysql_error(conn));
		exit(1);
	}
	resultado = mysql_store_result(conn);
	row = mysql_fetch_row(resultado);
	if(row == NULL){
		//No existe el jugador
		return -1;
	}
	else{
		strcpy(apodo,row[2]);
		//Existe el jugador
		return 1;
	}
}
//A�adir jugador a la lista de conectados
int Add_jugador(char nombre[20],int socket,Tlistaconectados *lista){
		
	int resultado = 1;
	int i = 0;
		
	//Busqueda
	while(i<lista->numeroconectados && resultado == 1){
		if(strcmp(nombre,lista->conectados[i].apodo)==0){
			//Implica que el jugador ya esta conectado
			resultado = 0;
		}
		i++;
	}
	if(resultado == 1){
		strcpy(lista->conectados[lista->numeroconectados].apodo,nombre);
		lista->conectados[lista->numeroconectados].socket = socket;
		lista->numeroconectados++;
		//Jugador a�adido, devuelve 1
		return resultado;
	}
	else{
		//Jugador no a�adido porque ya esta conectado , devuelve 0
		return resultado;
	}
	
}	
//Eliminar jugador de la lista de conectados
int Eliminar_jugador(char nombre[20],Tlistaconectados *lista){
			
	int resultado = 0;
	int i = 0;
			
	//Busqueda
	while(i<lista->numeroconectados && resultado == 0){
		if(strcmp(nombre,lista->conectados[i].apodo)==0){
		//Implica que el jugador esta conectado
		resultado = 1;
		}
		else{
			i++;
		}
	}
	if(resultado == 1){
		for(i;i<lista->numeroconectados-1;i++){
			strcpy(lista->conectados[i].apodo,lista->conectados[i+1].apodo);
			lista->conectados[i].socket = lista->conectados[i+1].socket;
		}
		lista->numeroconectados--;
		//Jugador eliminado, devuelve 1
		return resultado;
	}
	else if (resultado==0){
		//Jugador no conectado o que no existe, por tanto no se puede eliminar, devuelve 0
		return resultado;
	}
}
//Devuelve el socket de un jugador conectado
int Socket_jugador(char nombre[20],Tlistaconectados *lista){
	
	int resultado = 0;
	int i = 0;
			
	//Busqueda
	while(i<lista->numeroconectados && resultado == 0){
		if(strcmp(nombre,lista->conectados[i].apodo)==0){
		//Implica que el jugador esta conectado
		    resultado = 1;
		}
		else{
			i++;
		}
	}
	if(resultado == 1){
		//Devuelve el socket del jugador conectado
		return lista->conectados[i].socket;
	}
	else{
		//Jugador no conectado o no existente , devuelve 0
		return resultado;
	}
}
//Funcion que crea el codigo que enviara el sevidor con el numero de jugadores conectados y su nombre  
void Crear_codigo(char codigo[1000],Tlistaconectados *lista){
				
	int i = 0;
				
	//A�ade al principio del codigo el numero de jugadores conectados
	sprintf(codigo,"6/%d",lista->numeroconectados);
	//A�ade los jugadores conectados al codigo , siempre que haya conectados
	if(lista->numeroconectados > 0){
		for(i = 0; i<lista->numeroconectados;i++){
			//Los nombres separados por /
			sprintf(codigo,"%s/%s",codigo,lista->conectados[i].apodo);
		}
	}
}
void Crear_codigo_sockets(char nombrecodigo[1000], char socketcodigo[1000], Tlistaconectados *lista){
								
	char *p;
					
    p = strtok(nombrecodigo,"/");
    sprintf(socketcodigo,"%d",Socket_jugador(p,lista));
    p = strtok(NULL,"/");
	while(p != NULL){
		sprintf(socketcodigo,"%s/%d",socketcodigo,Socket_jugador(p,lista));
		p = strtok(NULL,"/");
	}
}
//Procedimiento para notificar a todos los usuarios conectados los cambios de la lista de conectados	
void Notificacion_ListaConectados(int s, Tlistaconectados *lista){
	char notificacion [1000];
	
	Crear_codigo(notificacion, lista);
	printf("%s\n",notificacion);
	
	if(lista->numeroconectados>0){
		for(int j=0;j<lista->numeroconectados;j++){
			//if(lista->conectados[j].socket!=s)
			write(lista->conectados[j].socket,notificacion,strlen(notificacion));
	}
	}
}
void *Atender_Cliente(void *socket){
		
	int sock_conn;
	int *s;
	s = (int *) socket;
	sock_conn = *s;
	int ret;
	char peticion[1000];
	char respuesta[1000];
	int terminar = 0;
	int resultado;
	
	while(terminar == 0){
		//Servicio
		//read hace 2 cosas , guarda lo que llega en el socket en el buffer peticion
		//y devuelve el numero de Bytes del mensaje , es decir su tamaño
		//el tama�o de un char es de 1 Byte , o lo que es lo mismo 8 bits
		ret=read(sock_conn,peticion, sizeof(peticion));
		printf("Recibido\n");
		//A�adimos el caracter de fin de string para que no escriba lo que hay despues en el buffer
		peticion[ret] = '\0';
		//Vamos a ver que quieren(analizar la peticion)
		char *p = strtok(peticion,"/");
		int codigo = atoi(p);
		switch(codigo){
			//Login
			case 0:
				p = strtok(NULL,"/");
				char apodo[20];
				char nombre[10];
				char pass[10];
				strcpy(nombre,p);
				p = strtok(NULL,"/");
				strcpy(pass,p);
				int resultado = Logear_jugador(nombre,pass,apodo);
				if(resultado == 1){
					sprintf(respuesta,"0/%d/%s\0",resultado,apodo);
					pthread_mutex_lock (&accesoexcluyente);//Indicamos que no se interrumpa
					resultado = Add_jugador(apodo,sock_conn,&lista);
					pthread_mutex_unlock (&accesoexcluyente);//Indicamos que ya se puede interrumpir
					
					if(resultado == 1){
						printf("Se ha conectado %s\n",apodo);
					}
					else{
						printf("Error al a�adir un jugador a la lista de conectados");
					}
					Notificacion_ListaConectados(sock_conn, &lista);
				}
				else if (resultado==-1){
					sprintf(respuesta,"0/%d\0",resultado);
					terminar=1;
					write(sock_conn,respuesta,strlen(respuesta));
					printf("%s\n", respuesta);
				}
				//Desactivamos el algoritmo de Nagle.
				//int flag = 1;
				//int result = setsockopt(sock_conn,IPPROTO_TCP,TCP_NODELAY,(char *) &flag,sizeof(int));
				//if(result==-1)
					//printf("Error al desactivar el algoritmo de Nagle.\n");
				//write(sock_conn, respuesta, strlen(respuesta));
				//printf("%s\n", respuesta);
				//Volvemos a activar el algoritmo de Nagle.
				//flag=0;
				//int result = setsockopt(sock_conn,IPPROTO_TCP,TCP_NODELAY,(char *) &flag,sizeof(int));
				//if(result==-1)
					//printf("Error al activar el algoritmo de Nagle.\n");
				//Notificacion_ListaConectados(sock_conn, &lista);
				
				
				
				
				break;
			//Consulta Cristian
			case 1:
				p = strtok(NULL,"/");
				apodo[20];
				strcpy(apodo,p);
				printf("%s\n", apodo);
				sprintf(respuesta,"1/%d\0",Mayor_puntuacion_jugador(apodo));
				write(sock_conn,respuesta,strlen(respuesta));
				break;
			//Consulta Diego	
			case 2:
				p = strtok(NULL,"/");
				apodo[20];
				strcpy(apodo,p);
				sprintf(respuesta,"3/%d\0",Numero_partidas_ganadas(apodo));
				write(sock_conn,respuesta,strlen(respuesta));
				break;
			//Consulta Joel	
			case 3:
				p = strtok(NULL,"/");
				int id = atoi(p);
				char ganador[20],jugador1[20],jugador2[20];
				int puntuacion1,puntuacion2;
				resultado = Datos_combate(id,ganador,jugador1,jugador2,&puntuacion1,&puntuacion2);
				if(resultado == -1){
					sprintf(respuesta,"2/%d\0",resultado);
					write(sock_conn,respuesta,strlen(respuesta));
				}
				else{
					sprintf(respuesta,"2/%d/%s/%s/%d/%s/%d\0",resultado,ganador,jugador1,puntuacion1,jugador2,puntuacion2);
					write(sock_conn,respuesta,strlen(respuesta));
				}
				break;
			//Registrarse
			case 4:
				p = strtok(NULL,"/");
				apodo[20];
				nombre[10];
				pass[10];
				strcpy(nombre,p);
				p = strtok(NULL,"/");
				strcpy(pass,p);
				p = strtok(NULL,"/");
				strcpy(apodo,p);
				resultado = Registrar_jugador(nombre,pass,apodo);
				if(resultado == 1){
					printf("Se ha registrado el jugador %s correctamente\n",apodo);
					Add_jugador(apodo,sock_conn,&lista);
				}
				else{
					printf("Error al registrar al jugador %s",apodo);
				}
				sprintf(respuesta,"4/%d\0",resultado);
				write(sock_conn,respuesta,strlen(respuesta));
				break;
			//Desconectar
			case 5:
				p = strtok(NULL,"/");
				strcpy(apodo,p);
				pthread_mutex_lock (&accesoexcluyente);//Indicamos que no se interrumpa
				resultado = Eliminar_jugador(apodo,&lista);
				pthread_mutex_unlock (&accesoexcluyente);//Indicamos que ya se puede interrumpir
				if(resultado == 1){
					printf("Se acabo el servicio para el jugador %s\n",apodo);
					terminar = 1;
				}
				break;
			default:
				printf("Error en el codigo\n");
				break;
		}
		
	}
	//Se acabo el servicio para el cliente
	close(sock_conn); 
}
int main(int argc, char *argv[])
{
	int sock_listen,sock_conn;
	struct sockaddr_in serv_adr;
	int sockets[100];
	int i;
	pthread_t thread[100];
	lista.numeroconectados = 0;
	pthread_mutex_init (&accesoexcluyente, NULL);
	
	conn = mysql_init(NULL);
	if (conn==NULL) {
		printf ("Error al crear la conexion: %u %s\n", 
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	//inicializar la conexion, indicando nuestras claves de acceso
	// al servidor de bases de datos (user,pass)
	conn = mysql_real_connect (conn, "localhost","root", "mysql", "juego", 0, NULL, 0);
	if (conn==NULL)
	{
		printf ("Error al inicializar la conexion: %u %s\n", 
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	// INICIALIZACIONES
	//Abrimos el socket
	if ((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("Error creando socket\n");
	// Hacemos el bind en el port
	memset(&serv_adr, 0, sizeof(serv_adr));// inicializa a cero serv_addr
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY); /* Lo mete en IP local */
	serv_adr.sin_port = htons(9240);
	if (bind(sock_listen, (struct sockaddr *) &serv_adr, sizeof(serv_adr)) < 0)
		printf("Error en el bind\n");
	// Limitamos el numero de conexiones pendientes
	if (listen(sock_listen, 10) < 0)
		printf("Error en el listen\n");
	for(i=0;;i++){
		printf("Escuchando\n");
		//sock_conn es el socket que utilizaremos para el cliente
		sock_conn = accept(sock_listen, NULL, NULL);
		printf("He recibido conexion\n");
		//Desactivamos el algoritmo de Nagle.
		int flag = 1;
		int result = setsockopt(sock_conn,IPPROTO_TCP,TCP_NODELAY,(char *) &flag,sizeof(int));
		if(result==-1)
			printf("Error al desactivar el algoritmo de Nagle.\n");
		//Almacenamos en el vector de sockets , el socket con el que nos comunicaremos con el usuario recien conectado
		sockets[i] = sock_conn;
		//Crear thread y decirle lo que tiene que hacer
		//&thread[i] es un parametro de salida
		//&sockets[i] es un parametro de entrada que le pasa a la funcion Atender_Cliente
		pthread_create(&thread[i],NULL,Atender_Cliente,&sockets[i]);
	}
	//Cerramos la conexion con la base de datos
	mysql_close(conn);
	return 0;

}
