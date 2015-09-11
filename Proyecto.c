//Este Codigo esta basado en la experiencia de el ensayo, codigo casero, ejemplos vistos, 
// y mucho pero mucho error y explosiones de sistema.

#include <unistd.h> 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

//Esta funcion se encarga de la lextura de comandos. recive y retorna un puntero de char
char *leer_cmd()
{
  char *cmd = NULL;  //Se declara un puntero de char para almacenar el comando.
  size_t tam = 0;
  getline(&cmd, &tam, stdin); // se utiliza getline para leer la entrada ya que guarda un espacio de memoria
  return cmd;                 // permitiendo liberarla mas tarde, ademas de tener mas posibilidades de input
}                             // finalmente se retorna el comando 

//Esta funcion se encarga de dividir el comando en palabras para su interpretacion, resive la linea de comandos obtenita
char **parse(char *cmd)         //en procesos anteriores
{
  int posicion = 0;              // se inicializa un entero que apunta en la posicion 0 de doble puntero de char
//se inicializa el doble puntero de char que almacenara las palabras separadas se utuliza malloc para reservar memoria y evitar
//al infinito con un puntero nulo ademas de ser util para conservar variaciones a travez de las llamadas a funciones.
  char **varg = malloc(100 * sizeof(char*)); 
// se crea un puntero de char para almacenar las palabras de forma individual.
  char *word;                           
//strtok corta la palabra de el arreglo cmd que se encuentra entre las limitaciones " \t\r\n\a", esto es muy eficiente y util                     
  word = strtok(cmd, " \t\r\n\a");     
//se inicialisa un loop para conseguir todas las palabras que se detiene cuando no hay mas, en caso de no ingresar un comando vacio
// simplemente no entra en el loop.
  while (word != NULL) {
    varg[posicion] = word;
    posicion++;
    word = strtok(NULL, " \t\r\n\a");
  }
// la ultima posicion de el arreglo de palabras se coloca como NULL para delimitar su termino
  varg[posicion] = NULL;
  return varg;
}

// Esta funcion retorna el nombre del archivo proximo a un < y > para poder utilizarlo en la redireccion en I/O.
char *search_file( char *s){   

     return strtok(s, " ><\t\r\n\a");             
}

//Se encarga de buscar la posicion del < mas sercano y entregar todo el texto que este a su derecha para buscar el archivo a redireccionar
char *inr(char *cmd){
  char *s;
  char *file;
//strstr busca en el arreglo la palabra que calza con la entregada en este caso "<" , retorna un puntero de char con el texto que sigue a
// "<" y nulo cuando no encuentra nada
  s = strstr(cmd, "<");
  if(s == NULL){
    return '\0';
  }
  else{
    file=search_file(s);
    return file;
  }
}
//Hace lo mismo que "<" solo que para ">" 
char *outr(char *cmd){
  char *s;
  char *file;
  s = strstr(cmd, ">");
  if(s == NULL){
    return '\0';
  }
  else{
    file=search_file(s);
    return file;
  }
}
//redireccion es la funcion que se encarga de redireccionar el I/O de un proceso o archivo dados los descriptores de archivo 
// in y out, ademas se entrega el comando a ejecutar y modificar.
int redireccion (int in, int out, char *cmd){

//se crea un hijo
  pid_t childPID;
//iniciamos un estado para el padre
  int status;
// soy el padre?
  if ((childPID = vfork()) > 0) {  
//espero a mi hijo         
    wait(&status);               
  } 
//soy el hijo?
  else if (childPID == 0)
  {
    //si el descriptor de archivo in fue enviado
      if (in != 0)
        {
          //se utiliza dup2 para cambiar el descriptor de archivo estandar del hijo por el entregado por el pipe
          dup2 (in, 0);
          //se cierra el descriptor ya que tenemos una copia
          close (in);
        }
    //si el descriptor de archivo out fue enviado se efectua lo mismo que en in
      if (out != 1)
        {
          dup2 (out, 1);
          close (out);
        }
      //Ahora revisamos la existencia de I/O por parte de archivos < >
      //llamamos a la funcion outr vista anteriormente
       char *fout=outr(cmd);
      //si no se encuentra un > no se hace nada, si lo encuentra pregunta si ya tengo un output de la pipe
      // notese que en esta funcion solo se encuentran procesos entre pipes incluyendo el primer proceso de la linea de comando
      // es por esto que esta algoritmo se utiliza mas para detectar errores de doble referecia de input y output.
       //en el caso de el output como todos estos procesos estan entre pipes cualquier archivo > es invalido
    if(fout!='\0'&&out==1){
      //se busca > en la linea de comando
        char *s;
        s = strstr(cmd, ">");
      //se reserva memoria
        char *buffer = malloc(sizeof(char) * 1024);
      //se copia todo el comando anterior a > sin el archivo ni el >
        strncpy ( buffer, cmd, s - cmd);
      // esto para cortar el comando y no resulte en error el doble output
        cmd=buffer;
      }
       // por el contrario el primer proceso de la linea de comandos puede tener su input de un archivo <
      // es por esto que en caso de encontrarlo debemos redireccionar su input.
      char *fin=inr(cmd);
     if(fin != '\0'&&in==0){
      char *s;
        s = strstr(cmd, "<");
        char *buffer = malloc(sizeof(char) * 1024); 
        strncpy ( buffer, cmd, s - cmd);
        cmd=buffer;
       //Todos los procesos anteriores son equivalentes al > pero para este caso
      // abrimos el archivo fin para solo lectura con fopen
        FILE *ptr=fopen(fin, "r");
      //si es nulo, tira error
        if(ptr==NULL){
          perror("Shellsita(error)->");
          exit(EXIT_FAILURE);
        }
        //de lo contrario del puntero FILE obtenemos un descriptor de archivo con fileno
        int fdin =fileno(ptr);
        //en el caso de ser exitoso utilizamos dup2 para redireccionar el input
        if(fdin >= 0){
          //se utiliza STDIN_FILENO y STDOUT_FILENO debido a naturaleza de los I/O en vez de 0 y 1.
          int duperr=dup2(fdin, STDIN_FILENO);
          //errores
           if (duperr < 0){
            perror("Shellsita(error)->");
            exit(EXIT_FAILURE);
           }
           //se cierra el descriptor ya tengo una copia
          close(fdin);
       }
        else{
          perror("Shellsita(error)->");
          exit(EXIT_FAILURE);
        }
      }
      //se parsea el comando resultante de todo el proceso
      char **varg = parse(cmd);
      //se ejecuta el nuevo proceso y se analizan errores
     if(execvp(*varg, varg) < 0) {
        perror("Shellsita(error)->");
        exit(EXIT_FAILURE);
     }
 }

  return 1;
}
//funcion encargada de detectar las pipes , dividir el comando, mantener la recursividad y pasar los descriptores correspondientes
// resive el comando
int pipes (char *cmd)
{
  //un entero que mantiene el input para el primer proceso de la linea de comandos en caso de no tener input de otro lugar
  int in;
  //un arreglo de 2 enteros que almacenara los descriptores de archivos de la pipe, el espacio 0 es para el input el 1 para el output
  int fd [2];
  in = 0;
  //este output hace referencia a la lectura del ultimo proceso en el comando, si no posee otra referencia
  // su output debe hacer referencia a la misma direccion del proceso padre
  int out=STDOUT_FILENO;
  //al igual que con < y > utilizamos pecla y strstr para ir cortando la linea de comandos.
  char *pecla;
  pecla = strstr(cmd, "|");
  //recursividad que permite analizar todas las pipes de el comando
  //se detiene cuando no encuentra mas pipes
  while (pecla != NULL)
  {
    //Oh gran dios todo poderoso ¡¡¡¡SALVE MALLOC!!!
     char *buffer = malloc(sizeof(char) * 1024); 
     strncpy ( buffer, cmd, pecla - cmd);
     //se optienen los descriptores de la siguiente pipe
     pipe (fd);
     //se corta el comando para mantener la recursividad
     cmd=pecla+1;
     //se llama a redireccion para ejecutar el processo, se le pasa el descriptor de input y de output
     // ademas de elpedazo del comando correspondiente a ese proceso (el de la izquierda de la pipe)
     redireccion (in, fd [1] , buffer);
     //se cierra el mismo directorio pasado al proceso, ya que el proceso de la derecha no lo necesita
     close (fd [1]);
     //se guarda el input para el proceso de la derecha de la pipe lo puede obtener
     in = fd [0];
     //se busca la siguente pecla
     pecla = strstr(cmd, "|");
   }
//en caso de no haber pipes en el comando, o ser el ultimo proceso a ejecutar
//se crea un hijo 
//al igual que en el caso anterior donde el primer proceso era un caso especial
// el ultimo  y los procesos unicos tambien lo son es por esto que se genero esta instancia en
// donde se efectuan los mismos pasos que redireccionar()
    pid_t childPID;
    int status; 
    if ((childPID = vfork()) > 0) {       
    wait(&status);     
   } 
  else if (childPID == 0){
//se crea un hijo al ser el ultimo processo el input puede provenir de una pipe como tambien no
    if (in != 0)
    dup2 (in, STDIN_FILENO);
  //como es proceso unico se revisan tanto sus input como su output
    char *fout=outr(cmd);
    if(fout!='\0'){
        char *s;
        s = strstr(cmd, ">");
        char *buffer = malloc(sizeof(char) * 1024); 
        strncpy ( buffer, cmd, s - cmd);
        cmd=buffer;
        FILE *ptr=fopen(fout, "ab");
        if(ptr==NULL){
          perror("Shellsita(error)->");
          exit(EXIT_FAILURE);
        }
        int fdout =fileno(ptr);
        if(fdout >= 0){
          int duperr=dup2(fdout, STDOUT_FILENO);
           if (duperr < 1){
            perror("Shellsita(error)->");
            exit(EXIT_FAILURE);
           }
          close(fdout);
       }
        else{
          perror("Shellsita(error)->");
          exit(EXIT_FAILURE);
        }
      }
//en caso de no tener output asignado, se le da el directorio del padre para que omprima en pantalla 
      else{
        int duperr=dup2(out, STDOUT_FILENO);
         if (duperr < 0){
            perror("Shellsita(error)->");
            exit(EXIT_FAILURE);
           }
      }
//se buscan input por medio de redireccion de ficheros
      char *fin=inr(cmd);
      if(fin != '\0'&&in==0){
        char *s;
        s = strstr(cmd, "<");
        char *buffer = malloc(sizeof(char) * 1024); 
        strncpy ( buffer, cmd, s - cmd);
        cmd=buffer;
        FILE *ptr=fopen(fin, "r");

        if(ptr==NULL){
          perror("Shellsita(error)->");
          exit(EXIT_FAILURE);
        }
        int fdin =fileno(ptr);
        if(fdin >= 0){
          int duperr=dup2(fdin, STDIN_FILENO);
           if (duperr < 0){
            perror("Shellsita(error)->");
            exit(EXIT_FAILURE);
           }
          close(fdin);
       }
        else{
          perror("Shellsita(error)->");
          exit(EXIT_FAILURE);
        }
      }
//se parsea el comando
    char **varg = parse(cmd);
//en el caso de ser proceso unico y estar vacio 
    if(varg==NULL){
    exit(0);
    }
    else if (execvp(*varg, varg) < 0 ) {
        perror("Shellsita(error)->");
        exit(EXIT_FAILURE);
     }
  }        
  else
    printf("Shellsita(error)->Error process failed \n"); 
  
  return 1;

}
//encadenamiento se encarga de separar los comandos entre &&
//es la primera funcion en ejecutarse al llegar nueva entrada
int encadenamiento(char *cmd){
//i el comando es vacio retorna
  if(*cmd =='\0')
    return 1;
//se efectuan los mismos pasos que para < > y asi obtener  los comandos independientes
  char *s;

  s = strstr(cmd, "&&");  

  if(s == NULL){
    if (*cmd =='\0')
      return 1;
    else {
      //en caso de ser comando unico sin &&
      //se inicia el proceso en busqueda de pipes
      return pipes(cmd); 
    }
  }
  else {
    //si existen &&
    char *buffer=malloc(sizeof(char) * 1024); 
    strncpy ( buffer, cmd, s - cmd);
    //se corta el comando y se pasa el lado izquierdo restante a la busqueda de pipes
    //si todo esta en orden y se ejecuta bien, se pasa a el siguiente seccion del comando
    if(pipes(buffer) == 1){
      char *cmd2=s+2;
      //el comando resultante derecho se devuelve a la misma funcion en busqueda de mas &&
      return encadenamiento (cmd2);
    }
    else{
      return 1;
    }
  }
}
//se funcion principal main
int main(int argc, char *argv[]){
  char *cmd;
  char **varg;
  //estado del loop
  int estado=1;
  while(estado){ 
    printf("Shellsita $ "); 
    //se lee el input
    cmd=leer_cmd();
    //por alguna razon desconocida la funcion encargada del exit dejo de funcionar al diseñar las pipes
    // aunque ambas no tienen referencia en el nivel de capa donde se encuentran
    // asi que se recurrio a una forma mas clasica.
    char *s = strstr(cmd, "exit");
    if(s!=NULL){
      exit(0);
    }
    else
    //envia el comando virgen a su primera inspeccion sobre el &&
    estado=encadenamiento(cmd);
  //se libera la memoria del comando para no tapar la memoria
    free(cmd);
  }
  //fin del programa
  return 0;

}