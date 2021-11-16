/* Copyright 2021 Diaconu Tudor-Gabriel */
#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "LinkedList.h"
#include "Hashtable.h"

// functia de initializare a serverului
server_memory* init_server_memory() {
	server_memory *server;

	// alocarea memoriei pentru server
	server = malloc(sizeof(server_memory));

	// crearea hashtable-ului in care vor fi stocate informatiile de pe server
	server->ht = ht_create(15, hash_function_string, compare_function_strings);

	return server;
}

void server_store(server_memory* server, char* key, char* value) {
	// stocarea informatiei pe hashtable-ul din server
	ht_put(server->ht, key, strlen(key) + 1, value, strlen(value) + 1);
}

void server_remove(server_memory* server, char* key) {
	// stergerea intrarii de pe hashtable in functie de cheie
	ht_remove_entry(server->ht, key);
}

char* server_retrieve(server_memory* server, char* key) {
	// extragerea valorii dorite de pe hashtable in functie de cheie
	return ht_get(server->ht, key);
}

void free_server_memory(server_memory* server) {
	// eliberarea hashtable-ului din server
	ht_free(server->ht);

	// eliberarea server-ului efectiv
	free(server);
}
