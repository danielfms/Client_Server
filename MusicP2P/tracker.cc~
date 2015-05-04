#include <iostream>
#include <czmq.h>
#include <thread> 
#include <unordered_map>
#include <set>
#include <vector>
#include <utility>


using namespace std;
/*En nodoSS, guarda las canciones y sus respectivas partes que tiene un nodo.
	Ademas guardo la direccion donde el nodo recibe las solicitudes y su estado.	
	nodo:canciones. idnodo,cancion,vector<partes>

	En songP, registro todas las canciones que hay en el sistema y sus respectivas partes.

	handleNodeMessage, maneja el mensaje que llega desde un nodo.
	Tres tipos: register,query y update.	
*/
typedef pair < unordered_map<string,vector<string>>, pair<string,bool > > tipoNodo;
unordered_map<zframe_t*,tipoNodo > nodoSS; 
unordered_map<string,vector<string>> songP;
unordered_map<zframe_t*,string> nodoAddress;

void handleNodeMessage(zmsg_t* msg,zmsg_t* outmsg,void* nodos){
	zframe_t* idN=zmsg_pop(msg);
	zframe_t* copy=zframe_dup(idN);
	string code(zmsg_popstr(msg)); 

	if(code.compare("register")==0){
		string address(zmsg_popstr(msg));
		int nsong=atoi(zmsg_popstr(msg));
		for (int i = 0; i < nsong;i++){ //Itero por cada cancion del nodo
			string song(zmsg_popstr(msg));
			int npsong=atoi(zmsg_popstr(msg));
			vector<string> s; //partes
			for (int j = 0; j < npsong;j++){  //Saco las partes de una cancion
				string p(zmsg_popstr(msg));
				s.push_back(p);
			}
			zframe_t* copy=zframe_dup(idN);
			songP[song]=s;
			unordered_map<string,vector<string>> partsAndSong;
			partsAndSong[song]=s;
			pair<string,bool > dirEstado=make_pair(address,true);
			tipoNodo newNodo=make_pair(partsAndSong,dirEstado);
			nodoSS[copy]=newNodo;
			//cout<<"Tamaño de nodoSS:"<<nodoSS.size()<<endl;
		}					
	}else if(code.compare("query")==0){
		string song(zmsg_popstr(msg));
		zmsg_addstr(outmsg,song.c_str()); //song
		zmsg_addstr(outmsg,to_string(songP[song].size()).c_str()); //npart totales de una cancion
		for ( auto it = nodoSS.begin(); it != nodoSS.end(); ++it ){ //Recorro tabla hash para buscar la cancion
			if(it == nodoSS.end())break;
    	zframe_t* idNN= it->first;
    	//zframe_print(a,"Id nodo, deberian ser solo 2");

    	tipoNodo sp=it->second; //songs/parts - address/state
    	if(sp.first.count(song)>0){
    		zmsg_addstr(outmsg,sp.second.first.c_str()); //dirNodo
    		int lp=sp.first[song].size(); // Cantidad de partes de la cancion que tiene este nodo
    		zmsg_addstr(outmsg,to_string(lp).c_str()); //nparts
    		for (int i = 0; i <lp;i++){
      		zmsg_addstr(outmsg,sp.first[song][i].c_str()); //nparts
     		}
    	}      	   	
 		}
    cout<<"Salida del handleNodeMessage"<<endl;
    zmsg_addstr(outmsg,"end");
    zframe_t* dup=zframe_dup(idN);
    zmsg_prepend(outmsg,&dup);
    zmsg_print(outmsg);
    zmsg_send(&outmsg,nodos);
	}else if(code.compare("update")==0){
		string song(zmsg_popstr(msg));
		string part(zmsg_popstr(msg));
		string dir(zmsg_popstr(msg));
		zframe_t* copy=zframe_dup(idN);
		if(nodoSS.count(copy)>0){  //Si el id esta registrado en la tabla hash
			if(nodoSS[copy].first.count(song)>0) //si el nodo tiene un registro de la cancion
				nodoSS[copy].first[song].push_back(part);
			else{
				vector<string> s;
				s.push_back(part);
				unordered_map<string,vector<string>> partsAndSong=nodoSS[copy].first;
				partsAndSong[song]=s;
				pair<string,bool > dirEstado=make_pair(dir,true);
				tipoNodo newNodo=make_pair(partsAndSong,dirEstado);
				nodoSS[copy]=newNodo;
			}
		}else{
			vector<string> s;
			s.push_back(part);
			unordered_map<string,vector<string>> partsAndSong;
			partsAndSong[song]=s;
			pair<string,bool > dirEstado=make_pair(dir,true);
			tipoNodo newNodo=make_pair(partsAndSong,dirEstado);
			nodoSS[copy]=newNodo;
		}
	}
}

int main(int argc, char** argv){


	// ./tracker puertoNodo
	zctx_t *context = zctx_new ();

	void* nodos = zsocket_new(context,ZMQ_ROUTER);
	int nodoPort=zsocket_bind(nodos, "tcp://*:%s",argv[1]);
	cout << "Listen to Nodes: "<< "localhost:" << nodoPort << endl;

	zmq_pollitem_t items[] = {{nodos, 0, ZMQ_POLLIN, 0}};
	while (true) {
    zmq_poll(items, 1, 10 * ZMQ_POLL_MSEC);
    if (items[0].revents & ZMQ_POLLIN) {
      	cerr << "From nodo\n";
      	zmsg_t* msg=zmsg_recv(nodos);
  			zmsg_print(msg);
      	zmsg_t* outmsg = zmsg_new();
      	handleNodeMessage(msg,outmsg,nodos);
    }     
	}

	return 0;
}
