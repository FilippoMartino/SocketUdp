  	
  	#include "Address.hpp"
	#include <errno.h>


    #ifndef __SOCKETUDP_HPP
    #define __SOCKETUDP_HPP
	#define DEFAULT_PORT 8080
	#define MAX_BUFFER_SIZE 4096
	
	/*

		già definiti nella classe Address:

   		#define IP_LO "127.0.0.1"
        #define IP_DHCP "0.0.0.0"
        #define PORT_ZERO 0		

	*/

	class Socket_udp{

	private:

		int sock_id;

		char* address;
		int port;

	public:
		
		//costruttore di default
		Socket_udp();

		//costruttore con solo loopback
		Socket_udp(bool);

		//costruttore con sola porta
		Socket_udp(int);

		//costruttore con sia porta che loopback
		Socket_udp(int, bool);
		
		~Socket_udp();

		void set_sockId();
		int do_bind();
		void set_broadcast(bool);

		bool invia_raw(Address , void*, int);
		bool invia_file(Address, char*);
		bool invia_stringa(Address, char*);

		bool ricevi_raw(Address*, void*, int);
		bool ricevi_file(Address*);
		char* ricevi_stringa(Address*);
		
		long int calculate_file_size(char*);
		char* adding_copy_to_filename(char*);
		

	};

	//in ogni caso nei costruttori verrà fatta la bind()

	Socket_udp::Socket_udp(){

		//settiamo porta e indirizzo di default
		//this->port = DEFAULT_PORT;
		//this->address = strdup(IP_LO);

		//settiamo il soketID
		set_sockId();

		//if (do_bind())
		//	printf("Error doing bind(): %s\n", strerror(errno));

	}

	//porta di default, test sull'indirizzo loopback
	Socket_udp::Socket_udp(bool loopback){

		//settiamo il soketID
		set_sockId();

		if (do_bind())
			printf("Error doing bind(): %s\n", strerror(errno));

		if (loopback)
			this->address = strdup(IP_LO);
		else
			this->address = strdup(IP_DHCP);

		this->port = DEFAULT_PORT;

	}

	//porta passata dall'utente, indirizzo di default: IP_LO
	Socket_udp::Socket_udp(int port){

		//settiamo il soketID
		set_sockId();

		if (do_bind())
			printf("Error doing bind(): %s\n", strerror(errno));

		this->port = port;
		this->address = strdup(IP_LO);

	}

	//l'utilizzatore decide a sua discrezione cosa utilizzare
	Socket_udp::Socket_udp(int port, bool loopback){

		//settiamo il soketID
		set_sockId();

		

		if (loopback)
			this->address = strdup(IP_LO);
		else
			this->address = strdup(IP_DHCP);dd

		this->port = port;

		if (do_bind())
			printf("Error doing bind(): %s\n", strerror(errno));

		
	}

	Socket_udp::~Socket_udp(){
                    
    	free(this->address); 
                
    }

    /*

	La invia_raw permette l'invio di qualsiasi tipo di dato
	ad uno specifico destinatario, effettua il controllo e 
	l'intercettazione di eventuali errori durante la sendto().
	Unica funzione della libreria con la chiamata alla api 
	sendto().
	Riceve:
		
	- Address destinatario:		L'indirizzo del destinatario nel
								formato Address, libreria fatta in classe 
								(char* indirizzo ip, int port)
	
	- void* something_to_send:	Il tipo di dato che si desidera spedire, con
								il formato void* questa funzione accetta tutti
								tipi possibili, ma prestando attenzione a passare 
								il dato PER REFERENZA, accettando la funzione il 
								puntatore del dato che si andrà a spedire
	
	- int size:					Indica la grandezza del file da spedire, dato che 
								essendo 'something_to_send' un puntatore, per fare la 
								'sendto()' necessitiamo della grandezza del dato, e non
								del suo puntatore.

	Restituisce:

	Valore booleano di risposta alla sendto(), [errore gestito tramite errno]

	false in caso di successo nell'invio del file
	true in caso di fallimento nell'invio del file

	*/

	bool Socket_udp::invia_raw(Address dest, void* something_to_send, int size){
		
		//struttura dati in cui migrare l'indirizzo
		struct sockaddr_in destinatario;
		//migrazione indirizzo
		destinatario = dest.getBinary();		

		//salviamo la dimensione di un sockaddr
		int sockaddr_size = sizeof(sockaddr);

		//prova a spedire il file
		if (!sendto(this->sock_id, something_to_send, size, 0, (struct sockaddr*) &destinatario, sockaddr_size)){
			printf("Error doing sendto(): %s\n", strerror(errno));
			//se esito negativo restituisce true
			return true;
		}

		//se esisto positivo restituisce false
		return false;

	}

	/*

		Riceve l'indirizzo del destinatario a cui mandare il messaggio
		che verrà convertito in una struttura socket, successivamente si 
		manda il messaggio con la struttura come messaggio quello passato
		alla funzione.
		Restiutisce al chiamate l'esito della funzione.

	*/

	bool Socket_udp::invia_stringa(Address dest, char* message){

		//spediamo il destinatario, la stringa e la dimensione di quest'ultima 
		if (invia_raw(dest, message, strlen(message))){
			printf("Error doing sendto(): %s\n", strerror(errno));
			return true;
		}else
			return false;
	}

	/*

		Questo metodo invia il file contenuto nel percorso 'path'

		opera mediante l'utlizzo della invia_raw:

		Calcola anzitutto la grandezza (in byte) del file passatoci,
		successivamente lo invia al destinatario (specificato come argomento
		del metodo), successivamente invia (sempre al destinatario) il
		percorso del file.
		Legge tutta l'immagine in modalita 'rb', cioè read-binary, che salva
		in un array di char (i byte sono salvati come char); questo array
		verrà utilizzato per riempire i vari pacchetti.

		Successivamente divide la grandezza del file per la grandezza
		massima di un pacchetto (per defualt MAX_BUFFER_SIZE) e ottiene il numero
		di pacchetti 'pieni' da spedire, crea anche un variabile
		con il modulo della grandezza del file per la grandezza massima del
		buffer, in modo da calcolare la grandezza dell'ultimo pacchetto, che verrà
		riempito con i byte finali del file.
		Ogni volta che un pacchetto viene riempito il pacchetto ottenuto viene inviato 
		al destinatario, che procederà a ricomporlo.
		Così facendo suddivide i file in pacchetti che verranno letti con l'altro metodo
		(ricevi_file(...)).

		Ogni volta che si richiama una API di sistema vengono gestiti gli eventuali errori

		Return value:

		Restituisce 'true' nel caso di errori 
		Restituisce 'false' in caso di successo


	*/

	bool Socket_udp::invia_file(Address dest, char* path){

		//apriamo il file passato dall'utente in modalità lettura
		//molto importante il rd, read binary
		FILE* file = fopen(path,"rb");

		//calcolo la dimensione del file mediante la funzione 'calculate_file_size'
		long int file_size =  calculate_file_size(path);

		//invio al destinatario la dimensione del file
		if(invia_raw(dest, &file_size, sizeof(long int))){

			printf("Error sending file size: %s\n", strerror(errno));
			return true;

		}

		//invio al server il nome dell'immagine
		if(invia_raw(dest, path, strlen(path))){

			printf("Error sending file lenght: %s\n", strerror(errno));
			return true;

		}

		//buffer in cui verrà salvato l'intero file
		char buffer[file_size];
		
		//leggo tutto il file e lo metto nel buffer
		fread(buffer, file_size, sizeof(char), file);

		//chiudo il file
		fclose(file);
		
		//pacchetto udp standard
		//NOTARE IL TIPO DEI VETTORI CHE UTILIZZERÒ PER SPEDIRE
		//DATI, SONO CHAR, PERCHÈ I BYTE SONO SALVATI COSÌ

		char pacchetto[MAX_BUFFER_SIZE];

		//calcolo il numer di pacchetti pieni 
		int numero_pacchetti_pieni = file_size / MAX_BUFFER_SIZE;

		//mediante il modulo calcolo il resto della dimensione del file
		//che sarà di conseguenza la dimesione dell'ultimo

		int dimensione_ultimo_pacchetto = file_size % MAX_BUFFER_SIZE;
		
		//creo l'ultimo pacchetto che verrà inviato con il giusto nuero di byte
		char ultimo_pacchetto[dimensione_ultimo_pacchetto];

		//contatore sui pacchetti inviati
		int pacchetti_inviati = 0;

		//contatore sull buffer contenente il file
		int i = 0;
		
		//ciclo fino ad aver'inviato tutti i pacchetti pieni
		while (pacchetti_inviati < numero_pacchetti_pieni){

			for (int j = 0; j < MAX_BUFFER_SIZE; j++){

				//per ogni pacchetto (utilizzo la stessa variabile sovrascrivendone i valori 
				//mediante il for) copio i byte che compongono la variabile
				//il valore incrementale del buffer è slegato rispetto ai cicli, per far si
				//di avere sempre il valore successivo a quello salvato in un pacchetto
				pacchetto[j] = buffer[i];
				i++;

			}

			//invio il pacchetto appena composto
			if(invia_raw(dest, pacchetto, MAX_BUFFER_SIZE)){

				printf("Error sending packet: %s\n", strerror(errno));
				return true;

			}
			pacchetti_inviati ++;
		}

		//se (nel remotissimo caso) il file fosse stato divisibile
		//per la grandezza massima del buffer (MAX_BUFFER_SIZE) non procedo
		//a riempire l'ultimo buffer
		if (!dimensione_ultimo_pacchetto){

			//in questo ciclo proceso a riempire un pacchetto di dimensioni
			//ridotte con quanto resta del file
			for (int j = 0; j < dimensione_ultimo_pacchetto; j++){
					
					ultimo_pacchetto[j] = buffer[i];
					i++;

			}
			
			//invia l'ultimo pacchetto al destinatario	
			if(invia_raw(dest, ultimo_pacchetto, dimensione_ultimo_pacchetto)){

				printf("Error sending packet: %s\n", strerror(errno));
					return true;	

			}

		}

		return false;
		
	}
 	
	/*

		Questo metodo contiene l'unica chiamata alla API di sistema 'recvfrom(...)'
		
		Si prepone di ricevere un qualsiasi tipo di valore da un mittente:

		Address* mitt: 			Il mittente che sta inviando un tipo di dato:
								Viene passato per referenza in modo da poter, in 
								un momento successivo, permettere il riconoscimento
								del socket del mittente da parte di chi ha invocato
								questo metodo, questa variabile viene infatti modificata
								tramite la 'setBinary()' della classe Address() che salva
								i valori di porta ed indirizzo in questo tipo di calasse
								i valori vengno presi dalla struttura sockaddr che serve per 
								la funzione recvfrom()
		
		void* buffer 			Il buffer contenente il valore che ci verrà inviato, deve
								necessariamente essere passato per referenza, in modo che 
								(qualsiasi valore ci venga spedito) questa funzione possa
								salvarvici un valore

		int buffer_size 		La grandezza del tipo di dato che ci verra spedito:
								int -> sizeof(int)
								char -> sizeof(char)
								char*||int* array -> strlen(array)

		return value:

		Ritona true nel caso ci siano stati dei problemi durante la chiamata alla recvfrom(...)
		La revfrom() restituisce, in caso di successo, il numero di byte ricevuti.
		
		Ritrona false nel caso si successo

	*/

	bool Socket_udp::ricevi_raw(Address* mitt, void* buffer, int buffer_size){
		
		//creiamo una nuva struttura che sarà quella dell'indirizzo del mittente
		struct sockaddr_in mittente;

		//devo salvarmi la grandezza in una variabile perchè non posto passare per referenza una macro
		int struct_lenght = sizeof(sockaddr);

		//mettiamo il server in ascolto,in caso di successo, restituirà il numero di byte lettra
		//e gestiamo l'errore nel caso ci sia..
		int ret = recvfrom(this->sock_id, buffer, buffer_size, 0, (struct sockaddr*) &mittente, (socklen_t*) &struct_lenght);

		if(ret <= 0){

			printf("Error doing recvfrom(): %s\n", strerror(errno));
			return true;
		
		}
		
		//imposto i dati del mittente nell'indirizzo ricevuto con la recvfrom (ricordarsi il -> perchè stiamo lavorando su un puntatore)
		mitt->setBinary(mittente);

		//se è andato tutto bene
		return false;

	}

	/*

		Riceve una semplice stringa sfruttando il metodo
		ricevi_raw()

	*/

	char* Socket_udp::ricevi_stringa(Address* mitt){

		char buffer[MAX_BUFFER_SIZE + 1];
		
		if(ricevi_raw(mitt, buffer, 13))
			return NULL;

		buffer[MAX_BUFFER_SIZE + 1] = '\0';

		return strdup(buffer);	
	
	} 

	/*

		Questo metodo funzione esclusivamente se l'invio del file 
		è stato fatto con l'altro metodo contenuto in questa libreria: invia_file(...)

		Riceve il nome e la grandezza del file, che servono, rispettivamente, per il nome
		del file che si andrà a creare e per il calcolo dei numeri di pacchetti da ricevere
		(completi, MAX_BUFFER_SIZE) e per il calcolo della dimensione dell'ultimo pacchetto.

		Successivamente si riceve un pacchetto alla volta e lo si 'svuota' nel buffer
		che andrà a contenere il file completo
		Dopodichè si riceve l'ultimo pacchetto di grandezza inferiore agli altri.

		Finita con successo la ricezione del file si procede alla stampa di quest ultimo nella
		stessa directory in cui è salvato il programma chiamante (nel caso in cui non si specifichi)
		un diverso path

		return value:

		Restituisce true nel caso si sia incorso in errori durante la chiamata ad API di sistema

		Restituisce false in caso di successo


	*/


	bool Socket_udp::ricevi_file(Address* mitt){

		//riceviamo innanzitutto la grandezza del file
		long int file_size;
		ricevi_raw(mitt, &file_size, sizeof(long int));

		//ricevo il nome del file
		char file_name[MAX_BUFFER_SIZE];
		ricevi_raw(mitt, file_name, MAX_BUFFER_SIZE);

		//creo il buffer che conterrà l'intera immagine
		char buffer[file_size];
		
		//calcolo il numero di pacchetti pieni e la dimensione dell'utlimo pacchetto
		int numero_pacchetti_pieni = file_size / MAX_BUFFER_SIZE;
		int dimensione_ultimo_pacchetto = file_size % MAX_BUFFER_SIZE;

		//inizializzo il pacchetto standard
		char pacchetto[MAX_BUFFER_SIZE];
		//inzializzo il pacchetto che andrà a contenere il resto del modulo
		//ciè l'ultimo pacchetto spedito
		char ultimo_pacchetto[dimensione_ultimo_pacchetto];	
		
		//contatore sui pacchetti ricevuti, serve per i while
		int pacchetti_ricevuti = 0;
		//contatore sul buffer, serve per rendere l'inserimento di dati
		//indipendente dai cicli
		int i = 0;

		//ricevo in questo ciclo la maggiorparte della dimensione
		//del file, quella divisible per al grandezza del pacchetto
		while (pacchetti_ricevuti < numero_pacchetti_pieni) {

			//ricevo il pacchetto
			if(ricevi_raw(mitt, pacchetto, MAX_BUFFER_SIZE)){

				printf("Error receving packet: %s\n", strerror(errno));
				return true;

			}
			
			//carico il pacchetto nel buffer, successivamente la variabile
			//verrà riutilizzata
			for (int j = 0; j < MAX_BUFFER_SIZE; j++){
				
				buffer[i] = pacchetto[j];
				//printf("buffer[%d]: %d\n", i, buffer[i]);
				i++;
						
			}
			
			//incremento il numero di pacchetti ricevuti
			pacchetti_ricevuti ++;

		}

		//ricevo l'ultimo pacchetto (attenzione ai parametri)
		if(ricevi_raw(mitt, ultimo_pacchetto, dimensione_ultimo_pacchetto)){

			printf("Error receving last packet: %s\n", strerror(errno));
			return true;

		}
		
		//carico nel buffer il restante del file
		for (int j = 0; j < dimensione_ultimo_pacchetto; j++){
			
			buffer[i] = pacchetto[j];
			i++;

		}

		//apro il file con il nome passatoci dal mittente
		FILE* my_file = fopen(file_name, "w");

		//trascrivo l'intero buffer sul nuovo file
		fwrite(buffer, sizeof(buffer), sizeof(char), my_file);	
		//chiudo il file
		fclose(my_file);
		
		return false;

	}

	

	

	/*
		
		Setta semplicemente il socket id con i valori che
		servono per una trasmissione TCP:
		AF_INET: la famiglia
		SOCK_DGRAM: il diagramma
		0: i flag

	*/

	void Socket_udp::set_sockId(){

		this->sock_id = socket(AF_INET, SOCK_DGRAM, 0);

		if (this->sock_id == -1)
			printf("Errore opening socket: %s\n", strerror(errno));

	}

	/*
		
		In questa funzione faccio la bind, funzione
		che associa l'indirizzo della nostra macchina
		al socket: è necessario fare la bind quando 
		vogliamo ricevere qualcosa.
		Restituisce l'esito della bind.
	
	*/

	int Socket_udp::do_bind(){

		//creo un oggetto address con l'indirizzo attuale della macchina
		Address myself (strdup(this->address), this->port);

		//creo una nuova struttura 
		struct sockaddr_in myself_sock;

		//assegno l'inidirizzo alla struttura
		myself_sock = myself.getBinary();

		//salvo la grandezza in una variabile
		int sockaddr_size = sizeof(sockaddr);

		//faccio la bind
		int ret = bind(this->sock_id, (struct sockaddr*) &myself_sock,  sockaddr_size);

		return ret;

	}

	long int Socket_udp::calculate_file_size(char* file_path){

		//apro il file in modalità lettura
    	FILE* file = fopen(file_path, "r"); 
  
 	   	//controlliamo che il file passato dall'utente esista effettivamente 
    	if (file == NULL) { 

        printf("File Not Found!\n"); 
        return -1; 
    	
    	} 
  		
    	/*

			per utilizzo di questa fuzione vedre es calcolare_dimesione_file
			nella cartella esercizi

    	*/

    	fseek(file, 0L, SEEK_END); 
  
   		// calcoliamo la grandezza del file passato mediante ftell()
    	long int file_size = ftell(file); 
  
 	    // chiudo il file 
    	fclose(file); 
  
    	return file_size;

	}

	/*

		Aggiunge semplicemente una dicitura che permetta la
		diversificazione del file nel caso si trovi un duplicato

	*/
	
	void Socket_udp::set_broadcast(bool activate_broadcast){
	
		int value = activate_broadcast;
	
		if (value){
			
			setsockopt(this->sock_id, SOL_SOCKET, SO_BROADCAST, &value, sizeof(bool));	
			
		}

	}


	char* Socket_udp::adding_copy_to_filename(char* file_name){

		//divide il file dall'estensione
		char* token = strtok(file_name, ".");

		//crea la varabile in cui riscrivere il nome del file
		char* copy_name = strdup("");

		//aggiunge il nome
		copy_name = strcat(copy_name, token);

		//aggiunge il classico simbolino
		copy_name = strcat(copy_name, strdup("(1)"));

		//prende l'estensione
		token = strtok(NULL, ".");

		//concateno la stringa finale con l'estensione
		copy_name = strcat(copy_name, strdup("."));
		copy_name = strcat(copy_name, token);

		//restituisce il valore 
		return strdup(copy_name);

	}

    #endif // __SOCKETUDP_HPP
