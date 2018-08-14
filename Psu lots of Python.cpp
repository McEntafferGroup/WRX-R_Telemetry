// PennStateDll.cpp : Defines the entry point for the console application.
//
// 2017-07-31
// K. Wade Nye
// Ulyssix Technologies, Inc
//
// Example code for using a method for external software to add data to a queue
// and an asyncronous thread to wait for when there is data 

#include "stdafx.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>		//bdd edit
#include <string.h>		//bdd edit
#include <queue>		//added for data queue
#include <process.h>	//added for threading
#include <iostream>		//added to write to console
#include <fstream>		//used to write TAD to file
#include <string>

#include "Psu.h"
#include "C:/Progra~2/python36-32/include/Python.h"
#include "stdint.h"

using namespace std;

//***********************************************************************************
// Globals
CRITICAL_SECTION lockGlobals;					//used to allow threads to access globals
CRITICAL_SECTION lockQueues;					//used to allow only one thread to add/remove from the queue
volatile bool gThreadRun = false;				//used to stop thread
bool Running = false;							//used to report to the API if the process is running
HANDLE processThread;							//handle for thread
HANDLE waitForData;								//object for the wait
queue<vector<unsigned char>> queueData;			//queue for vector of unsigned char
queue<int> queueDataSize;						//queue for the size of the data array

												//***********************************************************************************
												// Internal Methods

												//Thread to read data from queue and then process it
unsigned int __stdcall mythread(void*)
{
	//local var for keeping the thread running
	bool lThreadRun = true;

	//open the file
	ofstream myFile ("C:\\Users\\Public\\testdata_bdd_edits_write_words.tad", ios::out | ios::binary);

	//open the Python
	Py_Initialize();

	//
	//
	/*AND ON THE THIRD DAY, HE SAID, "LET THERE BE VARIABLES"*/
	//
	//

	/*
	** Currently very slow version that will be altered when the data comes out properly again so it can be used through DeweSoft on launch night
	*/

	/*The size of the detector*/
	const int num_row = 1024;
	const int num_column = 1024;

	/*Run a command as if typing into a Python shell, a simple way of importing my other program*/
	//int imported = PyRun_SimpleString("import just_plot3");

	/*The following section is for the Python module and the functions from within it that will be used*/
	//=================================================================================================//
	// plot_this - originally used for debugging                                                       //
	// SHOW      - used to test whether Python variables were being properly created                   //
	// PLACE     - inserts a given value into a specified point in a list                              //
	// sub_plot  - the main plotting function used to display the detector images                      //
	// make_list - creates the empty Python list of the correct size for DATA storage                  //
	//=================================================================================================//
	//imports from the Python include site-packages directory
	//PyRun_SimpleString("import just_plot3");
	
	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.path.append(\"C:\\Program Files (x86)\\Python36-32\\Lib\\site-packages\")");
	PyRun_SimpleString("import matplotlib");
	PyRun_SimpleString("import numpy as np");
	PyRun_SimpleString("import matplotlib.pyplot as plt");
	PyRun_SimpleString("import matplotlib.cm");
	PyRun_SimpleString("import gc");
	PyRun_SimpleString("import time");
	PyRun_SimpleString("import operator");
	PyRun_SimpleString("import fileinput");
	PyRun_SimpleString("import datetime");
	
	PyRun_SimpleString("num_row    = 1024");
	PyRun_SimpleString("num_column = 1024");
	PyRun_SimpleString("dat = [0 for i in range(num_row*num_column)]");
	
	PyRun_SimpleString("fig = plt.figure(1,figsize=(20,10))");
	PyRun_SimpleString("ax1 = plt.subplot(231)");
	PyRun_SimpleString("ax4 = plt.subplot(234)");
	PyRun_SimpleString("ax2 = plt.subplot(232)");
	PyRun_SimpleString("ax5 = plt.subplot(235)");
	PyRun_SimpleString("ax3 = plt.subplot(233)");
	/*
	PyObject* pymodule = PyImport_ImportModule("just_plot3");
	PyObject* plot_this = PyObject_GetAttrString(pymodule, "plot_this");
	PyObject* SHOW = PyObject_GetAttrString(pymodule, "SHOW");
	PyObject* PLACE = PyObject_GetAttrString(pymodule, "PLACE");
	PyObject* sub_plot = PyObject_GetAttrString(pymodule, "sub_plot");
	PyObject* make_list = PyObject_GetAttrString(pymodule, "LIST");*/

	/*Python variable used to store the pixel values before being added to the DATA list*/
	PyObject* temp_dat = 0;

	//make a huge list in python so values can be placed out of order
	PyObject* det_size = Py_BuildValue("i", num_row*num_column);

	/*The next section is a load of Python variables used within the plotting function*/
	//=================================================================================================//
	// PREVIOUS        - the last frame's data/pixel values                                            //
	// DATA            - the current frame's data/pixel values                                         //
	// TOTAL           - the accumulated pixel data from successive subtracted images                  //
	// FRAME_NUMBER    - the number of the current frame we are on                                     //
	// first_or_second - a flag to inform the plot whether or not it must reset the plotting surface   //
	// lt              - the local time pulled from the stream                                         //
	// ts0, ts1        - the temperature sensors' readings embedded in the stream                      // 
	// data_location   - the position in the DATA list into which the next pixel value will be placed  //
	//=================================================================================================//
	/*PyObject* PREVIOUS = Py_BuildValue("i", 0);
	PyObject* DATA = PyObject_CallFunctionObjArgs(make_list, det_size, NULL);
	PyObject* TOTAL = Py_BuildValue("i", 0);*/
	//PyObject* FRAME_NUMBER = Py_BuildValue("i", 0);
	int FRAME_NUMBER = 0;
	//PyObject* first_or_second = Py_BuildValue("i", 0);
	int first_or_second = 0;
	//PyObject* lt = Py_BuildValue("i", 0);
	int lt = 0;
	//PyObject* ts0 = Py_BuildValue("i", 0);
	int ts0 = 0;
	//PyObject* ts1 = Py_BuildValue("i", 0);
	int ts1 = 0;
	//PyObject* data_location = Py_BuildValue("i", 0);
	int data_location = 0;

	/*the number of frames between resets of the plot*/
	const int reset_number = 75;

	/*used to determine when the plots will be flushed*/
	int pack_counter = 0;

	/*Finite State Machine enumerated type*/
	enum state { S_PACKET_START, S_LTC_1, S_LTC_2, S_ROW, S_TEMP_01, S_TEMP_02, S_TEMP_11, S_TEMP_12, S_DATA_1, S_DATA_2, S_DATA_3, S_DATA_4, S_PACKET_END };
	/*the state the machine is currently in*/
	state STATE = S_PACKET_START;

	/*the number of whole frames recieved*/
	int frame_counter = 0;
	/*the position within a Tyler Packet the current pixel value is at*/
	int column_counter = 0;

	/*the current row the data belongs in*/
	int row = 0;

	/*the local time and temperature sensor variables*/
	int local_time = 0;
	int temp1, temp2 = 0;

	/*Literals placed into the stream for proper navigation*/
	uint8_t s12 = 140;	//start row literal, 8C
	uint8_t s34 = 141; 	//start row literal, 8D
	int   s1234 = 35981;//start row literal, 8C8D
						//beginning of new row data stream

	uint16_t ltc0 = 15; //time literal, F
	uint16_t ltc1 = 14;	//time literal, E

	uint16_t r1 = 13; 	//row literal, D

	uint16_t t01 = 12;	//temp sensor literal 1, C
	uint16_t t02 = 11;	//temp sensor literal 2, B
	uint16_t t11 = 10;	//temp sensor literal 3, A
	uint16_t t12 = 9;   //temp sensor literal 4, 9

	uint16_t p[4] = { 4, 5, 6, 7 };
	//pixel counter

	uint8_t e12 = 142;	//end of row literal, 8E
	uint8_t e34 = 143;	//end of row literal, 8F
	int   e1234 = 36495;//end of row literal, 8E8F

	uint16_t word = 0;	    //current word
	uint16_t next_word = 0; //the next word
	int     check = 0;	    //checker for literals at the start of word
	int       loc = 0;	    //location in memblock word is at 

	int size = 4;       //size of stuff to read in at a time
	
						/*all variables set; can start doing things*/

						//
						//
						/*AND THERE WAS VARIABLES*/
						//
						//
	ofstream current_state ("C:\\Users\\Public\\debug_it.txt", ios::out );

	std::string py_line = "";

	while (lThreadRun)
	{
		//wait for new data using the waitForData event- once found, reset the event
		WaitForSingleObject(waitForData, INFINITE);
		ResetEvent(waitForData);

		//important to store Queue Size as a variable
		//the processes are asyncronous and size can change while the loop runs 
		int queueSize = queueData.size();

		//process data
		for (int i = 0; i < queueSize; i++)
		{
			EnterCriticalSection(&lockQueues);

			//read first element from queue, write to file, then pop queue to remove first element
			vector<unsigned char> d = queueData.front();	//no lock needed bc front is just "peeking" at data - no change to the queue
			int s = queueDataSize.front();

			//write data to file - requires pointer to start of the vector

			//myFile.write ((char*)&d[0], s);

			//replace this jazz with all of the processing done by me
			//
			//
			//BEGIN
			//
			//		
			
			/*the location within dataArray starts at 0*/
			loc = 0;

			uint16_t WORDS[2] = { 0, 0 }; //16-bit int


			for (int j = 0; j<(s / 4); j++)
			{
				WORDS[0] = ((uint16_t)d[j * 4 + 3] << 8) + (uint8_t)d[j * 4 + 2];
				WORDS[1] = ((uint16_t)d[j * 4 + 1] << 8) + (uint8_t)d[j * 4 + 0];
			
				//maybe s is only 1 byte at a time, then have to read twice before going to word and must
				//use half_word[] as a table for proper organization of bits
			
				//location is less than the size of current piece
				//may need to use pop again if the pieces are only single char's
				
				for (int half = 0; half<2; half++)
				{
					word = WORDS[half];
					//play here
					myFile.write((char*)&word,sizeof(word));

					current_state << STATE << endl << hex << word << endl;
	
					/*check is the upper nibble of word*/
					check = (int)(word >> 12);
	
					/*if the number of pixel values we received is larger than the number of columns, change the state*/
					if (column_counter >= num_column)
					{
						/*then we must find the end of the current packet*/
						STATE = S_PACKET_END;
					}
					
					switch (STATE)
					{
						//start
						case S_PACKET_START:
						{
							/*if word is the start packet literal*/
							if (word == s1234)
							{
								/*advance the state*/
								STATE = S_LTC_1;
							}
							else
							{
								/*we keep looking until we find what we are looking for*/
								continue;
							}
						} break;
						//local time counter part 1 
						case S_LTC_1:
						{
							/*if the upper nibble is the time literal*/
							if (check == ltc0)
							{
								/*save the lower 3 nibbles to a C++ variable*/
								local_time = (word & 0x0FFF);
								/*advance the state*/
								STATE = S_LTC_2;
							}
							else
							{
								/*we keep looking until we find what we are looking for*/
								continue;
							}
						} break;
						//local time counter part 2
						case S_LTC_2:
						{
							/*if the upper nibble is the time literal*/
							if (check == ltc1)
							{
								/*save the total value as a Python variable*/
								lt = (local_time << 12) + (word & 0x0FFF);
								/*advance the state*/
								STATE = S_ROW;
							}
							else
							{
								/*we keep looking until we find what we are looking for*/
								continue;
							}
						} break;
						//row
						case S_ROW:
						{
							/*if the upper nibble is the row literal*/
							if (check == r1)
							{
								/*the current row is the lower 3 nibbles*/
								row = (word & 0x0FFF);
		
								if (row == 0)
								{
									/*increment the number of frames received*/
									frame_counter += 1;
									/*alter the Python variable holding the frame number*/
									FRAME_NUMBER = frame_counter;
		
									//maybe a problem?
									/*when to flush the plots is determined by the number of frames since the first one*/
									first_or_second = (frame_counter%reset_number);
		
									//may be obsolete, removed to test
									/*
									if (frame_counter > 1)
									{
									graphing = true;
									}
									*/
								}
								/*advance the state*/
								STATE = S_TEMP_01;
								/*reset the column counter for a new row*/
								column_counter = 0;
							}
							else
							{
								/*we keep looking until we find what we are looking for*/
								continue;
							}
						} break;
						//temperature sensor 0 part 1
						case S_TEMP_01:
						{
							/*this template will be identical for both temperature sensors*/
							/*if the upper nibble is the temperature sensor literal*/
							if (check == t01)
							{
								/*save the lower 3 nibbles to a C++ variable*/
								temp1 = (word & 0x0FFF);
								/*advance the state*/
								STATE = S_TEMP_02;
							}
							else
							{
								/*we keep looking until we find what we are looking for*/
								continue;
							}
						} break;
						//temperature sensor 0 part 2
						case S_TEMP_02:
						{
							if (check == t02)
							{
								/*store complete value as Python variable*/
								ts0 = (temp1 << 12) + (word & 0x0FFF);
								/*advance the state*/
								STATE = S_TEMP_11;
							}
							else
							{
								continue;
							}
						} break;
						//temperature sensor 1 part 1
						case S_TEMP_11:
						{
							if (check == t11)
							{
								temp2 = (word & 0x0FFF);
								STATE = S_TEMP_12;
							}
							else
							{
								continue;
							}
						} break;
						//temperature sensor 1 part 2
						case S_TEMP_12:
						{
							if (check == t12)
							{
								ts1 = (temp2<<12) + (word & 0x0FFF);
								STATE = S_PACKET_END;
							}
							else
							{
								continue;
							}
						} break;
						//end
						case S_PACKET_END:
						{
							if (column_counter < num_column)
							{
								/*everytime a pixel data is found, increment the counter*/
								column_counter += 1;
								/*a Python variable is used to store the current data point*/
								//temp_dat = Py_BuildValue("i", (word & 0xFFFF));
								/*the location within the 1-D array is also saved to a Python variable*/
								//data_location = Py_BuildValue("i", (row*(num_row)+column_counter - 1));
								data_location = (row*(num_row)+column_counter - 1);
								/*because only Python variables may be used when calling a Python function*/
								//DATA = PyObject_CallFunctionObjArgs(PLACE, DATA, temp_dat, data_location, NULL);
								py_line = "dat[" + std::to_string(data_location) + "] = " + std::to_string(word & 0xFFFF);
								PyRun_SimpleString(py_line.c_str());
								py_line = "";
							}
							//if this is no longer pixel data
							else
							{
								/*if the word is the end packet literal*/
								if (word == e1234)
								{
									//the smoking gun
									column_counter = 0;
									/*advance the state*/
									STATE = S_PACKET_START;
									/*increment the packet counter*/
									pack_counter += 1;
									/*if the data should be graphed and the frame has been completely read in*/
									if ((row == num_row - 1))
									{
										/*change the boolean so it does not plot something with incomplete data*/
										//graphing = false;
		
										/*THIS IS THE BIG CALL*/
										/*this is the function call to Python to plot the data given*/
										//===================================================================================================//
										// def sub_plot(prev, dat, total, frame_number, first_or_second, lt, ts0, ts1)                       //
										// where:                                                                                            //
										//     prev            - the previous frame's data                                                   //
										//     dat             - the current frame's data                                                    //
										//     total           - the accumulated image from the subtracted frames produced by dat and prev   //
										//     frame_number    - the current frame's number (for display purposes)                           //
										//     first_or_second - whether or not it is time to flush the plots for new data                   //
										//     lt              - the local time                                                              //
										//     ts0, ts1        - the temperature sensor readings                                             //
										//                                                                                                   //
										//     return value    - the sum of the previous subtracted frames and the new subtracted frame      //
										//===================================================================================================//
										//TOTAL = PyObject_CallFunctionObjArgs(sub_plot, PREVIOUS, DATA, TOTAL, FRAME_NUMBER, first_or_second, lt, ts0, ts1, NULL);
										if (first_or_second == 1)
										{
											PyRun_SimpleString("ax1.clear()");
											PyRun_SimpleString("ax4.clear()");
											PyRun_SimpleString("ax2.clear()");
											PyRun_SimpleString("ax5.clear()");
											PyRun_SimpleString("ax3.clear()");

											PyRun_SimpleString("ax4.set_xlabel('DN')");
											PyRun_SimpleString("ax4.set_ylabel('COUNTS')");

											PyRun_SimpleString("ax5.set_xlabel('DN')");
											PyRun_SimpleString("ax5.set_ylabel('COUNTS')");

											PyRun_SimpleString("ax3.set_title('CUMULATIVE')");

											PyRun_SimpleString("total = 0");
										}
										else
										{
											//shape the arrays
											//DAT IS THE DATA ARRAY PIXEL VALUES
											PyRun_SimpleString("data = np.reshape(dat, (num_row, num_column))");
											PyRun_SimpleString("previous = np.reshape(prev, (num_row, num_column))");
											//subtract the previous frame
											PyRun_SimpleString("new_data = data-previous");

											if (first_or_second == 2)
											{
												//set proper total
												PyRun_SimpleString("total = new_data");
												//globals for faster plotting
												PyRun_SimpleString("global im1");
												PyRun_SimpleString("global im2");
												PyRun_SimpleString("global im3");
												//define first generation of the graph
												PyRun_SimpleString("im1 = ax1.imshow(data,cmap=matplotlib.cm.hot,aspect='equal',origin='lower', vmin=50, vmax=1500)");

												PyRun_SimpleString("im2 = ax2.imshow(new_data,cmap=matplotlib.cm.hot,aspect='equal',origin='lower', vmin=-19, vmax=46)");

												PyRun_SimpleString("im3 = ax3.imshow(total,cmap=matplotlib.cm.hot,aspect='equal',origin='lower', vmin=12, vmax=43)");
											}
											else
											{
												//accumulate values in total
												PyRun_SimpleString("total = total+new_data");
												//set data without redrawing everything
												PyRun_SimpleString("im1.set_data(data)");
												PyRun_SimpleString("ax1.redraw_in_frame()");

												PyRun_SimpleString("im2.set_data(new_data)");
												PyRun_SimpleString("ax2.redraw_in_frame()");

												PyRun_SimpleString("im3.set_data(total)");
												PyRun_SimpleString("ax3.redraw_in_frame()");
											}
											//set title and make histograms
											PyRun_SimpleString(("ax1.set_title('FRAME: {}'.format(" + std::to_string(FRAME_NUMBER) + "))").c_str());

											PyRun_SimpleString("ax4.hist(np.reshape(data,-1), bins=100, histtype='stepfilled', range=(0,937))");

											PyRun_SimpleString(("ax2.set_title('FRAME: {}'.format("+std::to_string(FRAME_NUMBER)+"))").c_str());

											PyRun_SimpleString("ax5.hist(np.reshape(new_data,-1), bins=100, histtype='stepfilled', range=(-19,46))");

											//remove previous non-graph data
											PyRun_SimpleString("ax3.texts.clear()");
											//replace the removed datas
											PyRun_SimpleString(("ax3.text(0, -350, 'LOCAL TIME: {}'.format(" + std::to_string(lt)+"), fontsize=20, fontweight='bold')").c_str());

											PyRun_SimpleString(("ax3.text(0, -600, 'TEMP SENSOR 0: {}'.format("+std::to_string(ts0)+"), fontsize=20, fontweight='bold')").c_str());

											PyRun_SimpleString(("ax3.text(0, -850, 'TEMP SENSOR 1: {}'.format("+std::to_string(ts1)+"), fontsize=20, fontweight='bold')").c_str());

											//make sure you can see everything
											PyRun_SimpleString("fig.canvas.draw()");
											PyRun_SimpleString("fig.canvas.flush_events()");
											PyRun_SimpleString("plt.show(block=False)");
										}
										//the save image line
										PyRun_SimpleString("");
										/*the previous frame's data is then given to the PREVIOUS variable for proper use in the next frame*/
										//PREVIOUS = DATA;
										PyRun_SimpleString("prev = dat");
									}
								}
								else
								{
									//keep looking
									continue;
								}
							}
						} break;
						//it's broken
						//default: std::cout << "I fucked up somewhere." << std::endl; break;
					} // end of SWITCH
				} // end of FOR
			} // end of WHILE loc < s


			//**
			//**
			//**END
			//**
			//**

			//lock/unlock to prevent other thread from accessing the queue - only needed on add, pop and push

			queueData.pop();
			queueDataSize.pop();
			LeaveCriticalSection(&lockQueues);
		}

		//use the critical section to get a local copy of the global thread run
		EnterCriticalSection(&lockGlobals);
		lThreadRun = gThreadRun;
		LeaveCriticalSection(&lockGlobals);
	}

	//close the file
	myFile.close();
	current_state.close();

	//close the Python
	//apparently i do not need to close the python
	/*dereference all of the Python variables (to stop memory leaks)*/
	//I am definitely missing some
	//some need to die, some need not
	//Py_DECREF(lt);
	//Py_DECREF(ts0);
	//Py_DECREF(ts1);
	//Py_DECREF(data_location);
	//Py_DECREF(temp_dat1);
	//Py_DECREF(temp_dat2);
	//Py_DECREF(temp_dat3);
	//Py_DECREF(temp_dat4);
	//Py_DECREF(PREVIOUS);
	//Py_DECREF(DATA);
	//Py_DECREF(TOTAL);
	//Py_DECREF(FRAME_NUMBER);
	//Py_DECREF(first_or_second);
	//Py_DECREF(pymodule);
	//Py_DECREF(plot_this);
	//Py_DECREF(SHOW);
	//Py_DECREF(PLACE);
	//Py_DECREF(sub_plot);
	
	/*close the Python instance*/
	Py_Finalize();

	//signal to the closing function that the thred is done
	EnterCriticalSection(&lockGlobals);
	gThreadRun = true;
	LeaveCriticalSection(&lockGlobals);

	//thread must return unsigned int
	return 0;
}



//***********************************************************************************
// Exported Methods

// Function : WriteDataToQueue
// Purpose  : Loads char arrays into the data queue 
// Inputs   : char* dataArray - pointer to char arrary
// Returns	: int - number of arrays in the queue
PENNSTATE_API int WriteDataToQueue(unsigned char* dataArray, int dataArraySize)
{
	//local copy of the thread run global
	bool lThreadRun = false;

	//use the critical section to get a local copy of the global thread run
	EnterCriticalSection(&lockGlobals);
	lThreadRun = gThreadRun;
	LeaveCriticalSection(&lockGlobals);

	//if the thread is running, then add data. Else return -1 as error code
	if (lThreadRun)
	{
		//copy the data to a vector array using the pointer and the length of the data - 1
		int tempLength = dataArraySize - 1;
		vector<unsigned char> temp(dataArray, dataArray + tempLength);

		//write data to queue - use mutex lock/unlock to prevent other thread from accessing the queue 
		EnterCriticalSection(&lockQueues);
		queueData.push(temp);
		queueDataSize.push(dataArraySize);
		LeaveCriticalSection(&lockQueues);

		//set the event to trigger the thread to process data
		SetEvent(waitForData);

		return queueData.size();
	}
	else
	{
		//return -1 as an error code
		return -1;
	}
}

// Function : DataProcessStart
// Purpose  : Setups up handles and starts thread
// Inputs   : n/a
// Returns	: n/a
PENNSTATE_API void DataProcessStart()
{
	//set global that process is running - used for reporting only
	Running = true;

	//init the critical section
	InitializeCriticalSectionAndSpinCount(&lockGlobals, 0x400);
	InitializeCriticalSectionAndSpinCount(&lockQueues, 0x400);

	//create event to wait for data
	waitForData = CreateEvent(NULL, TRUE, FALSE, TEXT("WaitForDataEvent"));

	//use critical section to change global and allow thread to run
	EnterCriticalSection(&lockGlobals);
	gThreadRun = true;
	LeaveCriticalSection(&lockGlobals);

	//start thread
	processThread = (HANDLE)_beginthreadex(0, 0, &mythread, (void*)0, 0, 0);
}

// Function : DataProcessStop
// Purpose  : Stops the thread and cleans up the handles
// Inputs   : n/a
// Returns	: n/a
PENNSTATE_API void DataProcessStop()
{
	//use critical section to change global and kill the thread
	EnterCriticalSection(&lockGlobals);
	gThreadRun = false;
	LeaveCriticalSection(&lockGlobals);

	//wait for thread to end - signal is that gThread goes back high
	bool temp = false;
	while (!temp)
	{
		//set the event to ensure that the thread is not stuck waiting
		SetEvent(waitForData);

		//enter critical section and wait get Thread Run
		EnterCriticalSection(&lockGlobals);
		temp = gThreadRun;
		LeaveCriticalSection(&lockGlobals);
	}

	//close the handle and clean up the critical section
	CloseHandle(processThread);
	CloseHandle(waitForData);
	DeleteCriticalSection(&lockGlobals);
	DeleteCriticalSection(&lockQueues);

	//set global that process is running - used for reporting only
	Running = false;
}

// Function : DataProcessStop
// Purpose  : Stops the thread and cleans up the handles
// Inputs   : n/a
// Returns	: 1 - proccess running 2 - process stopped
PENNSTATE_API int IsProcessRunning()
{
	if (Running)
		return 1; //for true
	else
		return 0; //for false
}
