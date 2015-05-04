#include <czmq.h>
#include <iostream>
#include <thread> 
#include <unordered_map>
#include <dirent.h>
#include <vector>
#include <set>
#include <cstdlib>
#include <list>
#include <queue>

using namespace std;

vector<string> canciones; //Las canciones que tiene el nodo, me sirve para una iteracion.
set<string> songSearch; //Me sirve para saber si tengo que descargar la cancion.
unordered_map<string,int> songN; //Cual es el numero de partes de una cancion.
typedef unordered_map<string,vector<string>> DIC;
/*
partes: canciones que tengo y sus respectivas partes.
partNodo: una parte que nodos la tienen.
nodoParts: un nodo que partes tiene

*/
DIC partes,partNodo,nodoParts;
list<pair< string,int > > listP;
queue<zmsg_t*> msgSolicitudes;

//Me crea las partes y las carpetas de cada archivo. 
void inicializarMusic(){
  cout<<"Lista de archivos"<<endl;
  DIR *dir;
  struct dirent *ent;
  dir = opendir ("./music/");
  if (dir == NULL) 
   cout<<"No puedo abrir el directorio"<<endl;
  while ((ent = readdir (dir)) != NULL) {
      if ( (strcmp(ent->d_name, ".")!=0) && (strcmp(ent->d_name, "..")!=0) ){
        char* nombre=ent->d_name;
        canciones.push_back(nombre);
        songSearch.insert(nombre);
        mkdir(nombre,0777);
        string comando="split -b 512k -d ./music/"+string(nombre)+" ./"+string(nombre)+"/s.";
        system(comando.c_str());
      }
  }
  closedir (dir);
}

//Creo un mapa con las canciones que yo tengo y sus respectivas partes
//DEBO REGISTRAR AQUI CUANDO DESCARGO UNA CANCION?
void crearmapa(){
	int l= canciones.size();
	for (int i = 0;i<l; ++i){
		DIR *dir;
		struct dirent *ent;
		string carpeta="./"+canciones[i];
		vector<string> parts;
		dir = opendir (carpeta.c_str());
		if (dir == NULL) 
	   cout<<"No puedo abrir el directorio"<<endl;
		while ((ent = readdir (dir)) != NULL) {
		    if ( (strcmp(ent->d_name, ".")!=0) && (strcmp(ent->d_name, "..")!=0) ){
		        string part(ent->d_name);
		       	parts.push_back(part);
		    }
		}
		partes[canciones[i]]=parts;
  	closedir (dir);
	}
}


//Para registrar las canciones y partes del nodo en el tracker.
//La direccion debe ser la de donde hacer solicitudes de descarga
void registermsg(zmsg_t* msg,string dir){
	crearmapa(); //creo tabla hash con todoas las partes de cada cancion
	zmsg_addstr(msg,"register"); //code
	zmsg_addstr(msg,dir.c_str()); //direccion
	int l= partes.size();
	zmsg_addstr(msg,to_string(l).c_str()); //nsong

	for ( auto it = partes.begin(); it != partes.end(); ++it ){
      string song= it->first;
      zmsg_addstr(msg,song.c_str()); //song
      vector<string> spartes=it->second;
      int pl=spartes.size();
      zmsg_addstr(msg,to_string(pl).c_str()); //npartes
      for (int i = 0; i <pl;i++){
        zmsg_addstr(msg,spartes[i].c_str()); //part
      }
    }
}

//Funcion para ordenar la lista, por la parte mas rara. La que esta en menos nodos.
bool compare (pair< string ,int > first, pair< string ,int > second){
  return first.second < second.second;
}

/*
Crea un mensaje por parte, y selecciona al azar el nodo al cual se le va a hacer la solicitud.
*/
void msgSolicitud(zctx_t* context,string addressrecibir,string song){

   void* enviarSolicitudes= zsocket_new(context,ZMQ_DEALER);
  //Selecciono desde la parte mas rara y comienzo a enviar el mensaje de transferencia de la parte
  for ( auto it = listP.begin(); it != listP.end(); ++it ){
    zmsg_t* msgSolicitar=zmsg_new();
    string parte=it->first;
    vector<string> nodes=partNodo[parte];
    int random=rand() % nodes.size();
    string selectedNode=nodes[random]; //saco una direccion en la cual tengo que hacer la solicitud
    cout<<"Nodo seleccionado: "<<selectedNode<<endl;
    zmsg_addstr(msgSolicitar,song.c_str()); //song
    zmsg_addstr(msgSolicitar,parte.c_str()); //parte
    zmsg_addstr(msgSolicitar,addressrecibir.c_str()); //recibir
   
    zsocket_connect(enviarSolicitudes,selectedNode.c_str()); //Socket

    cout<<"Mensaje:"<<endl;
    zmsg_print(msgSolicitar);
    zmsg_send(&msgSolicitar,enviarSolicitudes);
    //zsocket_disconnect (enviarSolicitudes,selectedNode.c_str());
  }
   //zsocket_destroy (context, enviarSolicitudes);
}

void handleTrackerMessage(zmsg_t* msg,zctx_t* context,string addressrecibir){
  cout<<"From tracker\n";
  zmsg_print(msg);
	//NO ESTOY SEGURO 	QUE ESTO ESTE BIEN. LIMPIAR SIEMPRE LA LISTA
  listP.clear(); //Elimino lo que pueda contener esta lista, me va a servir para la parte mas rara.
  string song(zmsg_popstr(msg)); //song
  int npartes=atoi(zmsg_popstr(msg));//npartes
  songN[song]=npartes;
  //creo una tablahash parte:nodos que la tienen

  //limpio partNodo
  partNodo.clear();
  while(true){
    string nodo(zmsg_popstr(msg)); //direccion recibirsolicitud
    if(nodo.compare("end")==0)
      break;
    int np=atoi(zmsg_popstr(msg));
    for (int i = 0; i < np; i++){
     string parte(zmsg_popstr(msg));
     if(partNodo.count(parte)>0)
        partNodo[parte].push_back(nodo);
      else{
        vector<string> ns;
        ns.push_back(nodo);
        partNodo[parte]=ns;
      }
    }
  }
  //Guardo en la lista, las partes y la cantidad de nodos que la tienen.
  for ( auto it = partNodo.begin(); it != partNodo.end(); ++it ){
    string part=it->first;
    vector<string> nodes=it->second;
    listP.push_back(make_pair(part,nodes.size()));
  }
  listP.sort(compare); //Ordeno
  cout<<"Parte mas rara\n";
  for (auto it=listP.begin(); it!=listP.end(); ++it){
    string pparte=it->first;
    int v=it->second;
    cout<<"Parte:"<<pparte<<" n:"<<v<<endl;
  }
  msgSolicitud(context,addressrecibir,song);
}

//Manejo de las solicitudes de descargar/enviar a otro nodo una parte que yo tengo.
void handleSolicitudMessage(zmsg_t* msg,zctx_t* context){
  zframe_t* idN=zmsg_pop(msg);  //idNodo
  string song(zmsg_popstr(msg)); //song
  string fname(zmsg_popstr(msg)); //parte
  string addressrecibir(zmsg_popstr(msg));
  void* enviar = zsocket_new(context,ZMQ_DEALER);
  zsocket_connect(enviar,addressrecibir.c_str());
  string directotioMusica="./"+song+"/";
  zfile_t *file = zfile_new(directotioMusica.c_str(),fname.c_str());
  zfile_close(file);
  string path=directotioMusica+fname;
  zchunk_t *chunk = zchunk_slurp(path.c_str(),0);
  zmsg_t* transferir=zmsg_new();
  if(!chunk) {
    cout << "Cannot read file!" << endl;
    zmsg_addstr(transferir,"ERROR");
    zmsg_send(&transferir,enviar);
  }else{
    zmsg_addstr(transferir,song.c_str());   
    zmsg_addstr(transferir,fname.c_str());
    zframe_t *frame = zframe_new(zchunk_data(chunk), zchunk_size(chunk));
    zmsg_append(transferir, &frame);
    zframe_t* copy=zframe_dup(idN);
    sleep(4);
    zmsg_send(&transferir,enviar);
    }
}

void enviarInformeTracker(void* tracker,string song,string parte,string dirrecibirSolicitud){
  zmsg_t* msg=zmsg_new();
  zmsg_addstr(msg,"update");
  zmsg_addstr(msg,song.c_str());
  zmsg_addstr(msg,parte.c_str());
  zmsg_addstr(msg,dirrecibirSolicitud.c_str());
  zmsg_send(&msg,tracker);
}

void handleRecibirMessage(zmsg_t* msg,void* tracker,string dirrecibirSolicitud){
  zframe_t* idN=zmsg_pop(msg);
  string song(zmsg_popstr(msg));
  string fname(zmsg_popstr(msg)); //parte
  string directotioMusica="./"+song+"/";
  zfile_t *download = zfile_new(directotioMusica.c_str(),fname.c_str());
  zfile_output(download);
  zframe_t *filePart =zmsg_pop(msg);
  zchunk_t *chunk = zchunk_new(zframe_data(filePart), zframe_size(filePart)); 
  zfile_write(download, chunk, 0);
  zfile_close(download);

  enviarInformeTracker(tracker,song,fname,dirrecibirSolicitud);
  if(partes.count(song)>0){ //Si ya he descargado alguna parte de la cancion
     partes[song].push_back(fname);
     //verifico si ya estan todas las partes, si estan creo el archivo y digo donde lo guardo
     cout<<"Parte:"<<partes[song].size()<<" "<<songN[song]<<endl;
  }else{
    cout<<"Parte:1"<<" "<<songN[song]<<endl;
    vector<string > p;
    p.push_back(fname);
    partes[song]=p;
  }
	if(partes[song].size()==songN[song]){
      cout<<"Uno las partes:"<<song<<endl;
      string comando="cat "+directotioMusica+"s.* > ./music/"+song;
      system(comando.c_str());
      canciones.push_back(song);
      songSearch.insert(song);
  }
}

//Funcion que me sirver para crear un hilo que siempre escuche cuando hay solicitudes
void recvSolicitud_enviar(void* recibirSolicitudes,zctx_t *context){
  zmq_pollitem_t items[] = {{recibirSolicitudes, 0, ZMQ_POLLIN, 0}};
  cout << "Listening! Hilo:recvSolicitud_enviar" << endl;
  while (true) {
    if(!msgSolicitudes.empty()){
      zmsg_t* m=zmsg_dup(msgSolicitudes.front());
      handleSolicitudMessage(m,context);
      msgSolicitudes.pop();
    }
    zmq_poll(items, 1, 10 * ZMQ_POLL_MSEC);
    if (items[0].revents & ZMQ_POLLIN) {
      cerr << "From nodo[recibirSolicitud]\n";
      zmsg_t* msg = zmsg_recv(recibirSolicitudes);
      zmsg_print(msg);
      msgSolicitudes.push(msg);
      //handleSolicitudMessage(msg,context);
    }      
  }
}

//Funcion para crear hilo que siempre este escuchando si me mandaron una parte y envio el informe al tracker
void recibir_enviarInforme(void* recibir,void* tracker,string dirrecibirSolicitud){
  zmq_pollitem_t items[] = {{recibir, 0, ZMQ_POLLIN, 0}};
  cout << "Listening! Hilo:recibir_enviarInforme" << endl;
  while (true) {
    zmq_poll(items, 1, 10 * ZMQ_POLL_MSEC);
    if (items[0].revents & ZMQ_POLLIN) {
      cerr << "From nodo[recibir]\n";
      zmsg_t* msg = zmsg_recv(recibir);
      zmsg_print(msg);
      handleRecibirMessage(msg,tracker,dirrecibirSolicitud);
    }      
  }
}

int main(int argc, char** argv){
  //./nodo iptracker puertotracker miIP puertoRecibir puertoRecibirSolicitudes

	zctx_t *context = zctx_new ();
	//Sockets Fijos
	void* tracker = zsocket_new(context,ZMQ_DEALER);
	//zsocket_connect(tracker, "tcp://localhost:5555");
  zsocket_connect(tracker, "tcp://%s:%s",argv[1],argv[2]);
  string cadtracker="tcp://"+string(argv[1])+":"+string(argv[2]);

	void* recibir = zsocket_new(context,ZMQ_ROUTER); //files
	//int recibirPort=zsocket_bind(recibir, "tcp://*:5556");
  int recibirPort=zsocket_bind(recibir, "tcp://*:%s",argv[4]);
	cout << "Listen to Nodes: "<< "localhost:" << recibirPort << endl;
	//string addressRecibir="tcp://localhost:5556";
  string addressRecibir="tcp://"+string(argv[3])+":"+string(argv[4]);

	void* recibirSolicitudes = zsocket_new(context,ZMQ_ROUTER);
	//int recibirSolicitudesPort=zsocket_bind(recibirSolicitudes, "tcp://*:5557");
  int recibirSolicitudesPort=zsocket_bind(recibirSolicitudes, "tcp://*:%s",argv[5]);
	cout << "Listen to Nodes [Request]: "<< "localhost:" << recibirSolicitudesPort << endl;
  string addressSolicitudes="tcp://"+string(argv[3])+":"+string(argv[5]);

	inicializarMusic(); //partir canciones y guardalas en dos estructuras de datos

	zmsg_t* msg=zmsg_new();
	registermsg(msg,addressSolicitudes);
	zmsg_send(&msg,tracker);

  thread t1(recvSolicitud_enviar,recibirSolicitudes,context); //hilo recibirSolicitudes y enviar partes
  thread t2(recibir_enviarInforme,recibir,tracker,addressSolicitudes); //hilo recibir partes y enviar informe al tracker
  
  while(true){
    sleep(2);
    int op;
    string song;
    cout<<"::::::::::::::::::Bittorrent:::::::::::::::::::"<<endl;
    cout<<":::::::::::::::::::::::::::::::::::::::::::::::"<<endl;

    cout<<"1.Reproducir"<<endl; //Buscar si la cancion esta en el arbol sino sugerir que la descargue
    cout<<"2.Descargar"<<endl;

    cin>>op;
    switch(op){
      case 1:
      {
        cout<<"Por favor ingrese el nombre de la cancion a reproducir: ";  
        cin>>song;
        if(songSearch.count(song)>0){
          string comando="mpg123 ./music/"+song+" &";
          system(comando.c_str());
        }
        else{
          cout<<"La cancion no se encuentra en este nodo, por favor descargela(opcion 2)"<<endl;
        }
        break;
      }       
      case 2:
      {
	      cout<<"Por favor ingrese el nombre de la cancion a descargar: ";  
	      cin>>song;
	      if(songSearch.count(song)>0){
	        cout<<"La cancion ya se encuentra en este nodo"<<endl;
	      }else{
	        zmsg_t* msgquery=zmsg_new();
	        zmsg_addstr(msgquery,"query");
	        zmsg_addstr(msgquery,song.c_str());
	        zmsg_send(&msgquery,tracker);
	        zmsg_t* msg = zmsg_recv(tracker);
	        zmsg_print(msg);
	        handleTrackerMessage(msg,context,addressRecibir);
	      }
        break;
      }       
      default:
        cout<<"Opcion no valida"<<endl;
    }
  }

	return 0;
}



