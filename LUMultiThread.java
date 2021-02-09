/*Kasidakis Theodoros*/

import java.util.Scanner;

public class LUMultiThread {
	/*Shared Memory between the Threads*/
	private static int k; /* counter which is used to the whole LU process */
	private static MySemaphore Mtx; /* id of the mtx semaphore which is used for mutual exclusion between threads */
	private static MySemaphore Main_q; /* id of the semaphore which is used only by main.Main waits until threads finish LU decomposition */
	private static double [][] A;
	private static double [][] L;
	private static double [][] U;
	private static double [] temp;
	private static int rows; /* number of rows of the A,L and U matrix */
	private static int columns; /* number of columns of the A,L and U matrix */
	private static int done_my_row; /* counter which increases when a thread finishes the LU */
	private static int available_for_lu; /* flag which is used for waking up main.When all threads have out=1 main is up.*/
	private static MyLUThread [] Threads_of_LU;
	private static Thread [] t;
	/*End of Shared Memory*/

	private static class MyLUThread implements Runnable {
		private int out; /* flag which informs us if a thread is "active" for LU decomposition */
		private int myline; /* variable that matches which line is in each thread */
		private MySemaphore My_Q; /* id of the "personal" semaphore .Each thread has a personal semaphore */

		public MyLUThread(int nmyline) {
			myline=nmyline;
			out=0;
			My_Q=new MySemaphore(1);
		}
		public void run() {
			while(true) {
				My_Q.down();
				int j;
				double neg=A[myline][k]; /*neg means numerator_of_the_element_guide*/
				L[myline][k]=neg/A[k][k];
				if(k==0) {
					for(j=0;j<columns;j++) {
						A[myline][j]=A[myline][j]-(neg/A[k][k])*A[k][j];
					}
				}
				else {
					for(j=k;j<columns;j++) {
						A[myline][j]=A[myline][j]-(neg/A[k][k])*A[k][j];
					}
				}
				Mtx.down();
				done_my_row++;
				if(myline==k+1) { /* As the LU progresses, k + 1 line is no longer needed. That is why the thread corresponding to that line is terminated. */
					if(done_my_row==rows-1-k) { /* As the LU progresses, each time they work SIZE-1-k threads-lines. */
						out=1; /* This thread does not need anymore. */
						done_my_row=0;
						k++;
						int c;
						for(c=0;c<rows-1;c++) {
							if(Threads_of_LU[c].out==0) { /* The last thread should update the rest(up(everyone)) to move the process 'deeper' into the table. */
								available_for_lu++;
								Threads_of_LU[c].My_Q.up();
							}
						}
						if(available_for_lu==0) { /* Ιf everyone is out , last thread must wake up main.*/
							Mtx.up();
							Main_q.up();
							break;
						}
						else { /* Otherwise,thread terminates.*/
							available_for_lu=0;
							Mtx.up();
							break;
						}
					}
					else { /* I am the thread that has to leave but not the last one, so I simply leave without waking up someone. */
						out=1;
						Mtx.up();
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
							if(Threads_of_LU[c].out==0) {
								Threads_of_LU[c].My_Q.up();
							}
						}
						Mtx.up();
					}
					else{
						Mtx.up();
					}

				}
			}
		}
	}

	public static void main(String [] args) {

		int thread_counter;
		int i;
		int j;
		double max=-1;
		int max_line_for_change=-1;
		Scanner sc = new Scanner(System.in);



		System.out.print("Enter number of rows:");
		rows=sc.nextInt();
		System.out.print("Enter number of columns:");
		columns=sc.nextInt();

		A = new double[rows][columns];
		L = new double[rows][columns];
		U = new double[rows][columns];
 		temp = new double[columns];
		Threads_of_LU = new MyLUThread[rows-1];
		t = new Thread[rows-1];
		Mtx = new MySemaphore(1);
		Main_q = new MySemaphore(0);
		k=0;
		done_my_row=0;
		available_for_lu=0;

		for(i=0;i<A.length;i++){
            for(j=0;j<A[i].length;j++) {
                System.out.print("A["+i+"]"+"["+j+"]:");
                A[i][j]=sc.nextDouble();
            }
        }

		for(i=0;i<rows;i++) {/*Initialization of the L matrix */
			for(j=0;j<columns;j++) {
				if(j>i) {
					L[i][j]=0;
				}
				if(i==j) {
					L[i][j]=1;
				}
			}
		}

		if(A[0][0]==0) {
			for(i=0;i<rows;i++) {
				if(A[i][0]>max) {
					max_line_for_change=i;
					max=A[i][0];
				}
			}
		}

		if(max_line_for_change!=-1) {
			for(j=0;j<columns;j++) {
				temp[j]=A[0][j];
				A[0][j]=A[max_line_for_change][j];
				A[max_line_for_change][j]=temp[j];
			}
		}

		System.out.println();

		System.out.println("===BEFORE THE LU DECOMPOSITION===");
			for(i=0;i<rows;i++) {/*Printing the matrix*/
			System.out.println();//printf("\n");
			for(j=0;j<columns;j++) {
				System.out.print(A[i][j]+" ");
			}
		}

		System.out.println();

		for(thread_counter=0;thread_counter<rows-1;thread_counter++) { /*Creating threads for LU decomposition*/
			Threads_of_LU[thread_counter] = new MyLUThread(thread_counter+1);
			t[thread_counter] = new Thread(Threads_of_LU[thread_counter]);
		}
		for(thread_counter=0;thread_counter<rows-1;thread_counter++) { /*Creating threads for LU decomposition*/
			t[thread_counter].start();
		}

		Main_q.down();

		System.out.println();
		System.out.println();

		System.out.println("===AFTER THE LU DECOMPOSITION===");

		System.out.println();

		System.out.println("=== U MATRIX ===");
			for(i=0;i<rows;i++) {/*Printing the U matrix after the LU decomposition*/
				System.out.println();
				for(j=0;j<columns;j++) {
					System.out.print(A[i][j]+" ");
				}
			}

		System.out.println();
		System.out.println();

		System.out.println("=== L MATRIX ===");
		for(i=0;i<rows;i++) {/*Printing the U matrix after the LU decomposition*/
			System.out.println();
			for(j=0;j<columns;j++) {
				System.out.print(L[i][j]+" ");
			}
		}

		System.out.println();
	}
}
