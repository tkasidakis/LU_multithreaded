/*Kasidakis Theodoros
LU decomposition implementation using threads and binary semaphores for synchronization*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "semaphores.h"

struct T {
	pthread_t id; /* id for each thread */
	int out; /* flag which informs us if a thread is "active" for LU decomposition */
	int myline; /* variable that matches which line is in each thread */
	int iret; /* variable which stores the value from pthread_create() */
	int my_q; /* id of the "personal" semaphore .Each thread has a personal semaphore */
};

/*Shared memory*/
struct T *threads_info; /* A "dynamic" table that keeps the information that every thread needs */
int k; /* counter which is used to the whole LU process */
int mtx; /* id of the mtx semaphore which is used for mutual exclusion between threads */
int main_q; /* id of the semaphore which is used only by main.Main waits until threads finish LU decomposition */
double **a; /* matrix where LU is applied */
double **l; /* matrix L that occurs after LU */
int rows; /* number of rows of the A,L and U matrix */
int columns; /* number of columns of the A,L and U matrix */
int done_my_row; /* counter which increases when a thread finishes the LU */
int available_for_lu; /* flag which is used for waking up main.When all threads have out=1 main is up.*/

/*Signatures of functions*/
void *foo(void *args);

int main(int argc,char *argv[]) {

	int thread_counter;
	int i;
	int j;

	printf("Number of rows:");
	scanf("%d",&rows);
	printf("Number of columns:");
	scanf("%d",&columns);

	a=(double**)malloc(rows*sizeof(double*));
	l=(double**)malloc(columns*sizeof(double*));
	if(a==NULL||l==NULL) {
		fprintf(stderr,"Problem with memory allocation.\n");
		exit(EXIT_FAILURE);
	}
	for(i=0;i<rows;i++){
		a[i]=(double*)malloc((columns)*sizeof(double));
		l[i]=(double*)malloc((columns)*sizeof(double));
		if(a[i]==NULL||l[i]==NULL){
			fprintf(stderr,"Problem with memory allocation.\n");
			exit(EXIT_FAILURE);
		}
	}
	for(i=0;i<rows;i++) {
		for(j=0;j<columns;j++) {
			printf("Enter value for a[%d][%d]:",i,j);
			scanf("%lf",&a[i][j]);
		}
	}

	for(i=0;i<rows;i++) {/*Initialization of the L matrix */
		for(j=0;j<columns;j++) {
			if(j>i) {
				l[i][j]=0;
			}
			if(i==j) {
				l[i][j]=1;
			}
		}
	}

	printf("\n===BEFORE THE LU DECOMPOSITION===\n");
	for(i=0;i<rows;i++) {/*Printing the matrix*/
		printf("\n");
		for(j=0;j<columns;j++) {
			printf("%.1lf ",a[i][j]);
		}
	}
	printf("\n");

	threads_info=(struct T*)malloc(sizeof(struct T)*(rows-1));
	if(threads_info==NULL) {
		fprintf(stderr,"Problem with memory allocation\n");
		exit(EXIT_FAILURE);
	}
	mtx=init(mtx,1); /* Creating semaphore for mutual exlusion */
	main_q=init(main_q,0); /* Creating semaphore for main */
	k=0; /*k is initialized to zero so that it is ready for LU decomposition*/
	available_for_lu=0;
	done_my_row=0;
	for(thread_counter=0;thread_counter<rows-1;thread_counter++) { /*Creating threads for LU decomposition*/
		threads_info[thread_counter].myline=thread_counter+1; /* We need SIZE-1 threads because the first line of the matrix does not perform LU decomposition */
		threads_info[thread_counter].out=0;
		threads_info[thread_counter].my_q=init(threads_info[thread_counter].my_q,1);
		threads_info[thread_counter].iret=pthread_create(&threads_info[thread_counter].id,NULL,foo,(void *)(&threads_info[thread_counter]));
		if(threads_info[thread_counter].iret){
			fprintf(stderr,"Error - pthread_create() return code: %d\n",threads_info[thread_counter].iret);
			exit(EXIT_FAILURE);
		}
	}

	down(main_q);/*main waits all the threads to finish LU decomposition*/

	printf("\n===AFTER THE LU DECOMPOSITION===\n");
	printf("\n=== U MATRIX ===\n");
	for(i=0;i<rows;i++) {/*Printing the U matrix after the LU decomposition*/
		printf("\n");
		for(j=0;j<columns;j++) {
			printf("%.1lf ",a[i][j]);
		}
	}
	printf("\n");
	printf("\n=== L MATRIX ===\n");
	for(i=0;i<rows;i++) {/*Printing the L matrix after the LU decomposition*/
		printf("\n");
		for(j=0;j<columns;j++) {
			printf("%.1lf ",l[i][j]);
		}
	}
	printf("\n");
	for(i=0;i<rows-1;i++) { /* Destroy semaphores and free the dynamically allocated memory */
		destroy(threads_info[i].my_q);
	}
	destroy(mtx);
	destroy(main_q);
	free(threads_info);
	for(j=0;j<columns;j++) {
		free(a[j]);
		free(l[j]);
	}
	free(a);
	free(l);
	return(0);
}

void *foo(void *args) {
	struct T *thr_args=(struct T *)args;
	while(1) {
		down(thr_args->my_q); /*Each thread waits to its own personal semaphore.Each thread has a line and executes LU decomposition without mutual exclusion(concurrently). */
		int j;
		double neg=a[thr_args->myline][k]; /*neg means numerator_of_the_element_guide*/
		l[thr_args->myline][k]=neg/a[k][k];
		if(k==0) {
			for(j=0;j<columns;j++) {
				a[thr_args->myline][j]=a[thr_args->myline][j]-(neg/a[k][k])*a[k][j];
			}
		}
		else {
			for(j=k;j<columns;j++) {
				a[thr_args->myline][j]=a[thr_args->myline][j]-(neg/a[k][k])*a[k][j];
			}
		}
		down(mtx);
		done_my_row++;
		if(thr_args->myline==k+1) { /* As the LU progresses, k + 1 line is no longer needed. That is why the thread corresponding to that line is terminated. */
			if(done_my_row==rows-1-k) { /* As the LU progresses, each time they work SIZE-1-k threads-lines. */
				thr_args->out=1; /* This thread does not need anymore. */
				done_my_row=0;
				k++;
				int c;
				for(c=0;c<rows-1;c++) {
					if(threads_info[c].out==0) { /* The last thread should update the rest(up(everyone)) to move the process 'deeper' into the table. */
						available_for_lu++;
						up(threads_info[c].my_q);
					}
				}
				if(available_for_lu==0) { /* Ιf everyone is out , last thread must wake up main.*/
					up(mtx);
					up(main_q);
					break;
				}
				else { /* Otherwise,thread terminates.*/
					available_for_lu=0;
					up(mtx);
					break;
				}
			}
			else { /* I am the thread that has to leave but not the last one, so I simply leave without waking up someone. */
				thr_args->out=1;
				up(mtx);
				break;
			}
		}
		else { /*I'm not the thread to finish. I'm going back to my personal semaphore.
		Before I do that I have to check if i am the last thread in the process. In that case,i must wake up all the other threads.*/
			if(done_my_row==rows-1-k) {
				k++;
				done_my_row=0;
				int c;
				for(c=0;c<rows-1;c++) {
					if(threads_info[c].out==0) {
						up(threads_info[c].my_q);
					}
				}
				up(mtx);
			}
			else{
				up(mtx);
			}
		}
	}
	return(NULL);
}
