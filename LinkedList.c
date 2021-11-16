/* Copyright 2021 Diaconu Tudor-Gabriel */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "LinkedList.h"
#include "utils.h"

linked_list_t*
ll_create(unsigned int data_size)
{
    linked_list_t *list = malloc(sizeof(linked_list_t));
    DIE(list == NULL, "malloc error");
    list->head = NULL;
    list->data_size = data_size;
    list->size = 0;
    return list;
}

/*
 * Pe baza datelor trimise prin pointerul new_data, se creeaza un nou nod care e
 * adaugat pe pozitia n a listei reprezentata de pointerul list. Pozitiile din
 * lista sunt indexate incepand cu 0 (i.e. primul nod din lista se afla pe
 * pozitia n=0). Daca n >= nr_noduri, noul nod se adauga la finalul listei. Daca
 * n < 0, eroare.
 */
void
ll_add_nth_node(linked_list_t* list, unsigned int n, const void* new_data)
{
    ll_node_t *new_node;
    if (list == NULL){
    	return;
    }
    new_node = (ll_node_t *)malloc(sizeof(ll_node_t));
    DIE(new_node == NULL, "malloc failed");
    new_node->data = malloc(list->data_size);
    DIE(new_node->data == NULL, "malloc failed");
    memcpy(new_node->data, new_data, list->data_size);
    new_node->next = NULL;
    if (list->size == 0) {
    	list->head = new_node;
    } else if (n == 0) {
    	new_node->next = list->head;
    	list->head = new_node;
    } else if (n >= list->size) {
				ll_node_t *p;
				p = list->head;
				if (p) {
					while(p->next != NULL) {
							p = p->next;
					}
					p->next = new_node;
				}
				new_node->next = NULL;
    } else {
				ll_node_t *p, *q;
				p = list->head;
				for(unsigned int i = 0; i < n - 1; i++) {
					p = p->next;
				}
				q = p->next;
				p->next = new_node;
				new_node->next = q;
			}

		if (list->head == NULL) {
			list->head = new_node;
		}
    list->size++;
}

/*
 * Elimina nodul de pe pozitia n din lista al carei pointer este trimis ca
 * parametru. Pozitiile din lista se indexeaza de la 0 (i.e. primul nod din
 * lista se afla pe pozitia n=0). Daca n >= nr_noduri - 1, se elimina nodul de
 * la finalul listei. Daca n < 0, eroare. Functia intoarce un pointer spre acest
 * nod proaspat eliminat din lista. Este responsabilitatea apelantului sa
 * elibereze memoria acestui nod.
 */
ll_node_t*
ll_remove_nth_node(linked_list_t* list, unsigned int n)
{
	if(list == NULL)
		return NULL;
	if(list->size == 0)
		return NULL;
	if(list->size == 1)
	{
		ll_node_t* c = list->head;
		list->head = NULL;
		return c;
	} else if (n == 0) {
			ll_node_t* r = list->head;
			list->head = list->head->next;
			list->size--;
			return r;
	} else if (n >= list->size - 1 && list->size > 1) {
		ll_node_t *p;
		p = list->head;
		for(unsigned int i = 0; i < list->size - 2; i++) {
			p = p->next;
		}
		ll_node_t *a = p->next;
		p->next = NULL;
		list->size--;
		return a;
	} else {
			ll_node_t *q;
			q = list->head;
			for(unsigned int i = 0; i < n - 1; i++) {
				q = q->next;
			}
			ll_node_t *del = q->next;
			ll_node_t *t = q->next->next;
			q->next = t;
			list->size--;
			return del;
		}
}

/*
 * Functia intoarce numarul de noduri din lista al carei pointer este trimis ca
 * parametru.
 */
unsigned int
ll_get_size(linked_list_t* list)
{
    return list->size;
}

/*
 * Procedura elibereaza memoria folosita de toate nodurile din lista, iar la
 * sfarsit, elibereaza memoria folosita de structura lista si actualizeaza la
 * NULL valoarea pointerului la care pointeaza argumentul (argumentul este un
 * pointer la un pointer).
 */
void
ll_free(linked_list_t** pp_list)
{
		if(*pp_list == NULL) {
			return;
		}
    ll_node_t *p = (*pp_list)->head;

		if (p != NULL) {
			while(p->next != NULL) {
				ll_node_t *aux = p->next;
				free(p->data);
				free(p);
				p = aux;
			}
    	free(p->data);
    	free(p);
		}

		free(*pp_list);
}

/*
 * Atentie! Aceasta functie poate fi apelata doar pe liste ale caror noduri STIM
 * ca stocheaza int-uri. Functia afiseaza toate valorile int stocate in nodurile
 * din lista inlantuita separate printr-un spatiu.
 */
void
ll_print_int(linked_list_t* list)
{
	if(list == NULL)
		return;
	ll_node_t *p = list->head;
  while (p != NULL) {
  	printf("%d ", *((int *)p->data));
  	p = p->next;
  }
  printf("\n");
}

/*
 * Atentie! Aceasta functie poate fi apelata doar pe liste ale caror noduri STIM
 * ca stocheaza string-uri. Functia afiseaza toate string-urile stocate in
 * nodurile din lista inlantuita, separate printr-un spatiu.
 */
void
ll_print_string(linked_list_t* list)
{
	if(list == NULL) {
		return;
	}
	ll_node_t *p = list->head;
	while (p != NULL) {
		printf("%s ", (char *)p->data);
		p = p->next;
	}
    printf("\n");
}
