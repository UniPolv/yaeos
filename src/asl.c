/* * * * * * * * * * * * * * * * * * * * * * * * *
 * YAEOS' phase 1 implementation proposed by     *
 * - Francesco Fornari 							 *
 * - Gabriele Fulgaro							 *
 * - Mattia Polverini							 *
 * 												 *
 * Operating System course						 *
 * A.A. 2017/2018 								 *
 * Alma Mater Studiorum - University of Bologna  *
 * * * * * * * * * * * * * * * * * * * * * * * * */
 
#include "asl.h"

semd_t semd_table[MAXSEMD];
semd_t *semdFree_h = &semd_table[MAXSEMD];
semd_t *semdhash[ASHDSIZE];


int insertBlocked(int *key, pcb_t *p){
	if (p == NULL) return -1;
	else {
/*
Complete information at point [5] in design_choices.txt
*/
		int hash = (((int)key)/2)%ASHDSIZE;	// get hash index (PROJECT CHOICES)
		if (semdhash[hash] == NULL){	// if index is empty or s_next (recursion) is NULL then alloc semaphore
			if (semdFree_h == NULL) return -1;
			else {
				semdhash[hash] = semdFree_h;	// put head of free list in hash index
				semdFree_h = semdFree_h->s_next;	// head of free list is next of free list
				semdhash[hash]->s_next = NULL;
				semdhash[hash]->s_procQ = NULL;
				semdhash[hash]->s_key = key;
				p->p_semKey = key;
				insertProcQ(&semdhash[hash]->s_procQ,p);
				return 0;
			}
		} else if (semdhash[hash]->s_key == key){		// if key==s_key then insert pcb to same semaphore
			p->p_semKey = key;
			insertProcQ(&semdhash[hash]->s_procQ,p);
			return 0;
		} else {		// if key!=s_key then recursive check s_next
			semd_t * saved = semdhash[hash];
			semdhash[hash] = semdhash[hash]->s_next;	// go to next semaphore (node) of this hash index
			int ret = insertBlocked(key,p);
			if (saved->s_next != semdhash[hash]) saved->s_next = semdhash[hash]; // link the node inserted (in recursion)
			semdhash[hash] = saved;		//restore hash node
			return ret;
		}
	}
}

pcb_t *headBlocked(int *key){
	int hash = (((int)key)/2)%ASHDSIZE;
	if (semdhash[hash] == NULL) return NULL;
	if (semdhash[hash]->s_key == key) return headProcQ(semdhash[hash]->s_procQ);
	else {
		semd_t * saved = semdhash[hash];
		semdhash[hash] = semdhash[hash]->s_next;
		pcb_t *ret = headBlocked(key);
		semdhash[hash] = saved;
		return ret;
	}
}

pcb_t* removeBlocked(int *key){
	int hash = (((int)key)/2)%ASHDSIZE;
	pcb_t * ret = NULL;
	semd_t * saved = NULL;
	if (semdhash[hash] == NULL) ret = NULL;
	else if (semdhash[hash]->s_key == key) {	// if node has that key
		ret = removeProcQ(&semdhash[hash]->s_procQ);
		ret->p_semKey = NULL;
		if (semdhash[hash]->s_procQ == NULL){	// free the semaphore
			semdhash[hash]->s_key = NULL;
			saved = semdhash[hash]->s_next;	// save next node
			semdhash[hash]->s_next = semdFree_h;		//link to head of free list
			semdFree_h = semdhash[hash];				// free node is new head
			semdhash[hash] = saved;
		}
	} else {		// if node has't that key
		saved = semdhash[hash];
		semdhash[hash] = semdhash[hash]->s_next;	//move to next node
		ret = removeBlocked(key);
		// if next node is changed (in recursion) then link new node
		if (saved->s_next != semdhash[hash]) saved->s_next = semdhash[hash];
		semdhash[hash] = saved;	//restore hash node
	}
	return ret;
}

void forallBlocked(int *key, void (*fun)(pcb_t *pcb, void *), void *arg){
	int hash = (((int)key)/2)%ASHDSIZE;
	if (semdhash[hash] != NULL) {
		if (semdhash[hash]->s_key == key) forallProcQ(semdhash[hash]->s_procQ, fun, arg);
		else {
			semd_t * saved = semdhash[hash];
			semdhash[hash] = semdhash[hash]->s_next;	// go to next node of this hash and...
			forallBlocked(key, fun, arg);				// ...check if it has this key
			semdhash[hash] = saved;
		}
	}
}

void outChildBlocked(pcb_t *p){
	if ((p != NULL) && (p->p_semKey != NULL)){
		semd_t * saved;
		int hash = (((int)(p->p_semKey))/2)%ASHDSIZE;
		if (semdhash[hash] != NULL) {
			if (semdhash[hash]->s_key == p->p_semKey) {
				outProcQ(&semdhash[hash]->s_procQ, p);
				p->p_semKey = NULL;
				if (semdhash[hash]->s_procQ == NULL){	// free the semaphore
					semdhash[hash]->s_key = NULL;
					saved = semdhash[hash]->s_next;	// save next node
					semdhash[hash]->s_next = semdFree_h;		//link to head of free list
					semdFree_h = semdhash[hash];				// free node is new head
					semdhash[hash] = saved;
				}
			}
			else {
				saved = semdhash[hash];
				semdhash[hash] = semdhash[hash]->s_next;	// go to next node of this hash and...
				outChildBlocked(p);							// ...check for pcb key
				if (saved->s_next != semdhash[hash]) saved->s_next = semdhash[hash];
				semdhash[hash] = saved;
			}
		}
	}
}

void initASL(){
	semdFree_h = semdFree_h-1;	//index of array
	// if pcbfree_h point to last then follower is NULL otherwise is next
	semdFree_h->s_next = (semdFree_h < &semd_table[MAXSEMD-1]) ? semdFree_h+1 : NULL;
	if (semdFree_h > semd_table) initASL();
}
