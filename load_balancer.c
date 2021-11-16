/* Copyright 2021 Diaconu Tudor-Gabriel */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "load_balancer.h"
#include "Hashtable.h"

// structura de load_balancer
struct load_balancer {
	// hashring-ul in care stocam toate replicile
	int *hash_ring;

	// vector de adrese de server prin care accesam hashtable-urile
	server_memory **servers;

	// numarul de replici din hashring, fiind util pentru eficienta
	int num_replicas;
};

// functiile de hash pentru servere, respectiv chei
unsigned int hash_function_servers(void *a) {
	unsigned int uint_a = *((unsigned int *)a);

	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = (uint_a >> 16u) ^ uint_a;
	return uint_a;
}

unsigned int hash_function_key(void *a) {
	unsigned char *puchar_a = (unsigned char *) a;
	unsigned int hash = 5381;
	int c;

	while ((c = *puchar_a++))
		hash = ((hash << 5u) + hash) + c;

	return hash;
}

// initializarea load_balancer-ului
load_balancer* init_load_balancer() {
	load_balancer *load_bal;
	// alocarea load-balancer-ului efectiv
	load_bal = malloc(sizeof(*load_bal));

	// alocarea hashring-ului cu numarul maxim de replici posibil
	load_bal->hash_ring = malloc(100000 * 3 * sizeof(unsigned int));

	// alocarea vectorului de adrese cu numarul maxim de servere posibil
	load_bal->servers = calloc(100000, sizeof(server_memory*));
	load_bal->num_replicas = 0;

	return load_bal;
}

// functia de stocare a unei valori
void loader_store(load_balancer* main, char* key, char* value, int* server_id) {
	int nr = main->num_replicas;

	// punem in variabila hash-ul cheii pe care dorim sa o stocam
	// si hash-ul ultimei replici de pe hashring
	unsigned int key_hash = hash_function_key(key);
	unsigned int last_serv_hash = hash_function_servers(&main->hash_ring[nr - 1]);

	// cazul in care vom stoca in "interiorul" hashring-ului
	if (key_hash < last_serv_hash) {
		// parcurgem hashring-ul
		for(int i = 0; i < main->num_replicas; i++) {
			// tinem intr-o variabila hash-ul replicii pe care ne aflam
			unsigned int serv_hash = hash_function_servers(&main->hash_ring[i]);

			// verificam daca hash-ul cheii este mai mic decat hash-ul replicii
			// pentru a vedea daca aceasta este replica pe care vrem sa stocam
			if(key_hash <= serv_hash) {
				// daca da, punem in variabila *server_id, care reprezinta id-ul
				// server-ului care sta la baza replicii
				*server_id = main->hash_ring[i] % 100000;

				// stocam efectiva perechea cheie valoare pe server-ul gasit
				server_store(main->servers[*server_id], key, value);

				break;
			}
		}
	} else {
		// cazul in care stocam la finalul hashring-ului

		// server-ul pe care vom stoca va fi de fapt server-ul de la
		// baza primei replici a hashring-ului
		*server_id = main->hash_ring[0] % 100000;
		server_store(main->servers[*server_id], key, value);
	}
}

// functia de obtinere a unei valori dupa cheie
char* loader_retrieve(load_balancer* main, char* key, int* server_id) {
	int nr = main->num_replicas;

	// punem in variabila hash-ul cheii pe care dorim sa o stocam
	// si hash-ul ultimei replici de pe hashring
	unsigned int key_hash = hash_function_key(key);
	unsigned int last_serv_hash = hash_function_servers(&main->hash_ring[nr - 1]);

	if (key_hash < last_serv_hash) {
		// cazul in care returnam din "interiorul" hashring-ului
		for(int i = 0; i < main->num_replicas; i++) {
			// parcurgem hashring-ul
			unsigned int serv_hash = hash_function_servers(&main->hash_ring[i]);
			// tinem intr-o variabila hash-ul replicii pe care ne aflam

			// verificam daca hash-ul cheii este mai mic decat hash-ul replicii
			// pentru a vedea daca aceasta este replica de pe care vrem sa returnam
			if(key_hash <= serv_hash) {
				// daca da, punem in variabila *server_id, care reprezinta id-ul
				// server-ului care sta la baza replicii
				*server_id = main->hash_ring[i] % 100000;

				// returnam valoarea dorita
				return server_retrieve(main->servers[*server_id], key);
			}
		}
	}

	// daca se ajunge aici, inseamna ca hash-ul cheii este mai mare
	// decat hash-ul tuturor replicilor de pe hashring

	// server-ul de pe care vom returna va fi de fapt server-ul de la
	// baza primei replici a hashring-ului
	*server_id = main->hash_ring[0] % 100000;
	return server_retrieve(main->servers[*server_id], key);
}

// functie de adaugare in "interiorul" hashring-ului
void normal_add_hashring(load_balancer* main, int label, int *pos, int *i) {
	// facand aceasta functie intr-un for, daca conditia este indeplinita,
	// vom adauga pe hashring pe pozitia i
	*pos = *i;

	// shiftam la dreapta elementele de la i + 1 la numarul de replici
	for (int j = main->num_replicas; j > *i; j--) {
		main->hash_ring[j] = main->hash_ring[j - 1];
	}

	// adaugam in hashring eticheta replicii pe pozitia i
	main->hash_ring[*i] = label;

	// crestem contorul si numarul de replici cu 1
	*i = *i + 1;
	main->num_replicas = main->num_replicas + 1;
}

// functia de adaugare in hashring dupa id, in cazul in care hash-urile
// replicilor sunt egale, iar id-ul replicii curente este mai mare decat
// cel al urmatoarei replici
void by_id_add_hashring(load_balancer* main, int label, int *pos, int *i) {
	// crestem contorul, deoarece vom adauga noua replica pe urmatoarea pozitie
	// fata de cea pe care ne aflam
	*i = *i + 1;
	*pos = *i;

	// shiftam elementele la dreapta
	for (int j = main->num_replicas; j > *i; j--) {
		main->hash_ring[j] = main->hash_ring[j - 1];
	}

	// adaugam in hashring eticheta replicii
	main->hash_ring[*i] = label;

	// cresten contorul si numarul de replici
	*i = *i + 1;
	main->num_replicas = main->num_replicas + 1;
}

// functia generica de adaugare in hashring
void add_to_hashring(load_balancer* main, int label, int *pos) {
	// punerea in variabila a hash-ului etichetei
	unsigned int id_hash = hash_function_servers(&label);

	if (main->num_replicas == 0) {
		// cazul in care adaugam primul element al hashring-ului
		main->hash_ring[0] = label;
		main->num_replicas = 1;

		*pos = 0;
	} else {
		// cazul in care hashring-ul deja are elemente
		int nr = main->num_replicas;
		unsigned int last_hash = hash_function_servers(&main->hash_ring[nr - 1]);
		// punerea in variabila a hash-ului ultimei replici de pe hashring

		if(id_hash < last_hash) {
			// cazul in care adaugam in "interiorul" hashring-ului
			for(int i = 0; i < main->num_replicas; i++) {
				// parcurgerea hashring-ului
				unsigned int serv_hash = hash_function_servers(&main->hash_ring[i]);
				// punerea in variabila a hash-ului replicii pe care ne aflam

				int next_id = main->hash_ring[i + 1];
				// punerea in variabila a etichetei replicii urmatoare fata de cea
				// pe care ne aflam

				if(id_hash < serv_hash) {
					// gasirea replicii cu hash mai mare fata de replica pe care o
					// adaugam noi si adaugarea in hashring
					normal_add_hashring(main, label, pos, &i);

					break;
				} else if (id_hash == serv_hash) {
					// cazul in care hash-urile sunt egale, iar sortarea se face
					// dupa id
						if(label < next_id) {
							normal_add_hashring(main, label, pos, &i);

							break;
						} else {
							by_id_add_hashring(main, label, pos, &i);
						}
				}
			}
		} else {
				// cazul in care adaugam replica la finalul hashring-ului
				main->hash_ring[nr] = label;
				main->num_replicas = main->num_replicas + 1;
				*pos = nr;
			}
		}
}

// redistribuirea clasica, cand adaugam in interiorul hashring-ului
void add_move(load_balancer* main, int label, int next_id, int pos) {
	// calculul server-ului reprezentat de replica
	int curr_id = label % 100000;
	int prev = pos - 1;

	// calculul hash-urilor al pozitiei pe care am adaugat si al
	// pozitiei precedente
	unsigned int prev_hash = hash_function_servers(&main->hash_ring[prev]);
	unsigned int curr_hash = hash_function_servers(&label);

	for (unsigned int i = 0; i < main->servers[next_id]->ht->hmax; i++) {
		// parcurgerea tuturor bucket-urilor din hashtable-ul urmatorului server
		linked_list_t *curr_bucket = main->servers[next_id]->ht->buckets[i];
		ll_node_t *curr_node = curr_bucket->head;

		while(curr_node) {
			ll_node_t *aux = curr_node->next;
			info *pair = (info*) curr_node->data;
			unsigned key_hash = hash_function_key(pair->key);

			// verificarea conditiei ca hash-ul obiectului care trebuie mutat
			// sa fie mai mic ca hash-ul replicii pe care am adaugat-o si
			// mai mare ca hash-ul replicii precedente
			if(key_hash <= curr_hash && key_hash > prev_hash) {
				// intai stocam perechea pe serverul curent, apoi stergem
				// de pe serverul urmator de pe care am mutat
				server_store(main->servers[curr_id], pair->key, pair->value);
				server_remove(main->servers[next_id], pair->key);
			}

			curr_node = aux;
		}
	}
}

// redistribuirea cand adaugam replica pe prima pozitie a hashring-ului
void add_first_move(load_balancer* main, int label, int curr_id) {
	// urmatoarea replica va fi a doua replica a hashring-ului in cazul nostru
	int next_id = main->hash_ring[1] % 100000;

	if (curr_id != next_id) {
		// redistribuirea are loc doar daca replicile consecutive reprezinta
		// servere diferite
		int last = main->num_replicas - 1;

		// calculul hash-urilor replicii curente si a replicii precedente, aceasta
		// fiind ultimul element al hashring-ului
		unsigned int prev_hash = hash_function_servers(&main->hash_ring[last]);
		unsigned int id_hash = hash_function_servers(&label);

		for (unsigned int i = 0; i < main->servers[next_id]->ht->hmax; i++) {
			// parcurgerea bucket-urilor de pe cea de-a doua replica a hashring-ului
			linked_list_t *curr_bucket = main->servers[next_id]->ht->buckets[i];
			ll_node_t *curr_node = curr_bucket->head;

			while(curr_node) {
				ll_node_t *aux = curr_node->next;
				info *pair = (info*) curr_node->data;
				unsigned key_hash = hash_function_key(pair->key);

				if(key_hash <= id_hash || key_hash > prev_hash) {
					// daca hash-ul cheii obiectului este mai mare decat hash-ul ultimei
					// replici din hashring si mai mic decat hash-ul replicii adaugate
					// vom face redistribuirea
					server_store(main->servers[curr_id], pair->key, pair->value);
					server_remove(main->servers[next_id], pair->key);
				}

				curr_node = aux;
			}
		}
	}
}

// functia de redistribuire a obiectelor pe noua replica adaugata
void add_move_in_server(load_balancer* main, int label, int position) {
	// functia primeste ca argument positia pe care a fost adaugata replica
	// in hashring
	int next_id;

	int curr_id = label % 100000;
	// calcularea server-ului pe care-l reprezinta replica

	if(main->num_replicas > 3) {
		// conditie pusa, deoarece daca adaugam primul server, nu avem de ce
		// sa redistribuim
		if (position != 0) {
			// cazul in care nu adaugam in la inceputul hashring-ului
			if (position < main->num_replicas - 1) {
				// cazul in care adaugam in interior
				// calculam id-ul server-ului replicii urmatoare
				next_id = main->hash_ring[position + 1] % 100000;
			} else if (position == main->num_replicas - 1) {
				// cazul in care adaugam pe ultima pozitie a hashring-ului
				// next_id va fi id-ul server-ului primei replici
					next_id = main->hash_ring[0] % 100000;
				}

			if (curr_id != next_id) {
				// mutam elementele doar daca replicile consecutive
				// au la baza servere diferite
				add_move(main, label, next_id, position);
			}
		} else {
				// cazul in care replica a fost adaugata pe prima pozitie
				add_first_move(main, label, curr_id);
			}
	}
}

// functia de adaugare a server-ului in load_balancer
void loader_add_server(load_balancer* main, int server_id) {
	// initializam server-ul pe care il adaugam
	main->servers[server_id] = init_server_memory();

	int position;

	// calcularea id-urilor celorlalte 2 replici
	int replica1_id = server_id + 100000;
	int replica2_id = server_id + 200000;

	// adaugarea in hashring si redistribuirea elementelor
	// pentru toate cele 3 replici ale server-ului
	add_to_hashring(main, server_id, &position);
	add_move_in_server(main, server_id, position);

	add_to_hashring(main, replica1_id, &position);
	add_move_in_server(main, replica1_id, position);

	add_to_hashring(main, replica2_id, &position);
	add_move_in_server(main, replica2_id, position);
}

// functia de obtinere a pozitiei pe care se afla replica cautata
// in hashring, utila pentru remove
void redeem_position(load_balancer* main, int label, int *pos) {
	for(int i = 0; i < main->num_replicas; i++) {
		if(main->hash_ring[i] == label) {
			*pos = i;
			break;
		}
	}
}

// functia clasica de mutare a elementelor, utila pentru remove
void remove_move(load_balancer* main, int label, int next_id, int pos) {
	// id-ul server-ului replicii pe care o stergem
	int curr_id = label % 100000;
	int prev_pos = pos - 1;

	// calcularea hash-ului pentru replica pe care o stergem si pentru replica
	// precedenta
	unsigned int prev_hash = hash_function_servers(&main->hash_ring[prev_pos]);
	unsigned int id_hash = hash_function_servers(&label);

	for (unsigned int i = 0; i < main->servers[curr_id]->ht->hmax; i++) {
		// parcurgerea bucket-urilor server-ului replicii pe care o stergem
		linked_list_t *curr_bucket = main->servers[curr_id]->ht->buckets[i];
		ll_node_t *curr_node = curr_bucket->head;

		while(curr_node) {
			ll_node_t *aux = curr_node->next;
			info *pair = (info *) curr_node->data;
			unsigned key_hash = hash_function_key(pair->key);

			// conditia de mutare, aceasta fiind ca hash-ul obiectului sa se afle
			// in intervalul dintre hash-ul replicii precedente si hash-ul replicii
			// curente; daca aceasta conditie este indeplinita, stocam pe server-ul
			// replicii urmatoare(aceasta fiind data ca argument al functiei)
			// si stergem de pe server-ul replicii sterse
			if(key_hash > prev_hash && key_hash <= id_hash) {
				server_store(main->servers[next_id], pair->key, pair->value);
				server_remove(main->servers[curr_id], pair->key);
			}

			curr_node = aux;
		}
	}
}

// functia de mutare a elementelor cand stergem prima replica
void remove_first_move(load_balancer* main, int label, int curr_id) {
	int next_id = main->hash_ring[1] % 100000;
	// id-ul server-ului urmatoarei replici, dupa cea stearsa, in acest caz fiind
	// id-ul server-ului celei de-a doua replici din hashring

	int nr = main->num_replicas;

	// calculul hash-ului replicii pe care dorim sa o stergem si al replicii
	// precedente, aceasta fiind, in acest caz, ultima replica din hashring
	unsigned int prev_hash = hash_function_servers(&main->hash_ring[nr - 1]);
	unsigned int id_hash = hash_function_servers(&label);

	for(unsigned int i = 0; i < main->servers[curr_id]->ht->hmax; i++) {
		// parcurgerea bucket-urilor server-ului replicii pe care o stergem
		linked_list_t *curr_bucket = main->servers[curr_id]->ht->buckets[i];
		ll_node_t *curr_node = curr_bucket->head;

		while(curr_node) {
			ll_node_t *aux = curr_node->next;
			info *pair = (info *) curr_node->data;
			unsigned int key_hash = hash_function_key(pair->key);

			// conditia de mutare, aceasta fiind ca hash-ul obiectului sa fie mai
			// mare decat hash-ul ultimei replici din hashring sau sa fie mai mic
			// decat hash-ul replicii pe care o stergem; daca aceasta conditie este
			// indeplinita, vom stoca obiectul pe server-ul urmator si il vom sterge
			// de pe serverul pe care il stergem
			if(key_hash > prev_hash || key_hash <= id_hash) {
				server_store(main->servers[next_id], pair->key, pair->value);
				server_remove(main->servers[curr_id], pair->key);
			}

			curr_node = aux;
		}
	}
}

// functia de redistribuire a elementelor din replica stearsa
void remove_move_in_server(load_balancer* main, int label, int* position) {
	int next_server_id;

	// calculul id-ului server-ului reprezentat de replica din argument
	int curr_id = label % 100000;

	// aflarea pozitiei replicii pe care
	redeem_position(main, label, position);

	if (*position != 0) {
		// cazul in care nu stergem prima replica din hashring
		if(*position < main->num_replicas - 1) {
			// calcularea id-ului server-ului reprezentat de replica urmatoare,
			// daca nu stergem replica de pe ultima pozitie din hashring
			next_server_id = main->hash_ring[*position + 1] % 100000;
		} else if (*position == main->num_replicas - 1) {
			// daca stergem replica de pe ultima pozitie, urmatorul id,
			// va fi id-ul server-ului primei replici din hashring
			next_server_id = main->hash_ring[0] % 100000;
		}

		// mutarea efectiva a elementelor pentru acest caz
		remove_move(main, label, next_server_id, *position);

	} else {
		remove_first_move(main, label, curr_id);
	}
}

// functia de stergere a replicii din hashring
void remove_from_hashring(load_balancer* main, int label, int position) {
	// gasirea pozitiei replicii din hashring pe care o stergem
	redeem_position(main, label, &position);

	// mutarea la stanga a elementelor din hashring pornind de la
	// pozitia gasita mai sus; astfel, va fi scos din hashring si
	// elementul pe care dorim sa-l stergem
	for(int i = position; i < main->num_replicas - 1; i++) {
		main->hash_ring[i] = main->hash_ring[i + 1];
	}

	// scaderea numarului de replici
	main->num_replicas = main->num_replicas - 1;
}

// functia de eliminare a unui server din load_balancer
void loader_remove_server(load_balancer* main, int server_id) {
	int position;

	// calcularea id-urilor si celorlalte 2 replici
	int replica1_id = server_id + 100000;
	int replica2_id = server_id + 200000;

	// redistribuirea elementelor si stergerea replicii pentru toate
	// cele 3 replici care constituie serverul
	remove_move_in_server(main, server_id, &position);
	remove_from_hashring(main, server_id, position);

	remove_move_in_server(main, replica1_id, &position);
	remove_from_hashring(main, replica1_id, position);

	remove_move_in_server(main, replica2_id, &position);
	remove_from_hashring(main, replica2_id, position);

	// elibararea memoriei alocate pentru serverul eliminat
	free_server_memory(main->servers[server_id]);
	main->servers[server_id] = NULL;
}

// functia de eliminare a load balancer-ului
void free_load_balancer(load_balancer* main) {
	// eliberarea vectorului hashring
	free(main->hash_ring);

	// eliberarea fiecarui server din vectorul de adrese de servere
	for(int i = 0; i < 100000; i++) {
		if(main->servers[i] != NULL) {
			free_server_memory(main->servers[i]);
		}
	}

	// eliberarea vectorului de adrese de servere
	free(main->servers);

	// eliberarea load_balancer-ului
	free(main);
}
