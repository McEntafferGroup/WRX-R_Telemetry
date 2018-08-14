#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdint.h>
#include <string>
#include <vector>
#include <sstream>
#include "Python.h"
#include <stdlib.h>
#include <bitset>

/* 
** Currently very slow version that will be altered when the data comes out properly again so it can be used through DeweSoft on launch night
*/

/*The size of the detector*/
const int num_row    = 1024;
const int num_column = 1024;

/*Initialize a Python environment*/
Py_Initialize();

/*Run a command as if typing into a Python shell, a simple way of importing my other program*/
int imported = PyRun_SimpleString("import just_plot3");

/*The following section is for the Python module and the functions from within it that will be used*/
//=================================================================================================//
// plot_this - originally used for debugging                                                       //
// SHOW      - used to test whether Python variables were being properly created                   //
// PLACE     - inserts a given value into a specified point in a list                              //
// sub_plot  - the main plotting function used to display the detector images                      //
// make_list - creates the empty Python list of the correct size for DATA storage                  //
//=================================================================================================//
PyObject* pymodule    = PyImport_ImportModule("just_plot3");
PyObject* plot_this   = PyObject_GetAttrString(pymodule, "plot_this");
PyObject* SHOW        = PyObject_GetAttrString(pymodule, "SHOW");
PyObject* PLACE       = PyObject_GetAttrString(pymodule, "PLACE");
PyObject* sub_plot    = PyObject_GetAttrString(pymodule, "sub_plot");
PyObject* make_list   = PyObject_GetAttrString(pymodule, "LIST");

/*Python variables used to store the pixel values before being added to the DATA list*/
PyObject* temp_dat1 = 0;
PyObject* temp_dat2 = 0;
PyObject* temp_dat3 = 0;
PyObject* temp_dat4 = 0;

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
PyObject* PREVIOUS = Py_BuildValue("i", 0);
PyObject* DATA = PyObject_CallFunctionObjArgs(make_list, det_size, NULL);
PyObject* TOTAL = Py_BuildValue("i", 0);
PyObject* FRAME_NUMBER = Py_BuildValue("i", 0);
PyObject* first_or_second = Py_BuildValue("i", 0);
PyObject* lt;
PyObject* ts0;
PyObject* ts1;
PyObject* data_location;

/*the number of frames between resets of the plot*/
const int reset_number = 10; 

/*used to determine when the plots will be flushed*/
int pack_counter = 0;

/*Finite State Machine enumerated type*/
enum state {S_PACKET_START, S_LTC_1, S_LTC_2, S_ROW, S_TEMP_01, S_TEMP_02, S_TEMP_11, S_TEMP_12, S_DATA_1, S_DATA_2, S_DATA_3, S_DATA_4, S_PACKET_END};
/*the state the machine is currently in*/
state STATE = S_PACKET_START;

/*the number of whole frames recieved*/
int frame_counter  = 0;
/*the position within a Tyler Packet the current pixel value is at*/
int column_counter = 0;
	
/*the current row the data belongs in*/
int row = 0;

/*the local time and temperature sensor variables*/						
int local_time = 0;
int temp1, temp2 = 0;



void Process_Data (unsigned char* dataArray, int dataArraySize)
{
	/*Literals placed into the stream for proper navigation*/
	uint8_t s12 = 140;	//start row literal, 8C
	uint8_t s34 = 141; 	//start row literal, 8D
	int   s1234 = 35981;//start row literal, 8C8D
	                	//beginning of new row data stream
	                    
	uint16_t ltc0 = 15; //time literal, F
	uint16_t ltc1 = 14;	//time literal, E
	                    
	uint16_t r1  = 13; 	//row literal, D
	
	uint16_t t01 = 12;	//temp sensor literal 1, C
	uint16_t t02 = 11;	//temp sensor literal 2, B
	uint16_t t11 = 10;	//temp sensor literal 3, A
	uint16_t t12 =  9;  //temp sensor literal 4, 9
	                
	uint16_t p[4]  = {4, 5, 6, 7};	
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
	
	/*the location within dataArray starts at 0*/
	loc = 0;
	
	while (loc < dataArraySize)
	{
		/*word is the first 2 bytes*/
		word = (uint16_t)(dataArray[loc]<<8) + (uint8_t)(dataArray[loc+1]);
						
		/*check is the upper nibble of word*/
		check = (int)(word>>12);
		/*increase the location to where it will read in from next*/
		loc+=2;
		
		/*if the number of pixel values we received is larger than the number of columns, change the state*/
		if (column_counter>=num_column)
        {				
		    /*then we must find the end of the current packet*/
            STATE = S_PACKET_END;
		}
		
		switch(STATE)
		{
			case S_DATA_1: 
			{
				if (check == p[0])
				{
					/*everytime a pixel data is found, increment the counter*/
                    column_counter += 1;
                    /*a Python variable is used to store the current data point*/
                    temp_dat1 = Py_BuildValue("i", (word&0x0FFF));
                    /*the location within the 1-D array is also saved to a Python variable*/
                    data_location = Py_BuildValue("i", (row*(num_row)+column_counter-1));
                    /*because only Python variables may be used when callinga Python function*/
                    DATA = PyObject_CallFunctionObjArgs(PLACE, DATA, temp_dat1, data_location, NULL);
                    
                    /*update the state to look for more data (since the column size of 1024 is divisible by 4, only the fourth data check will ever see the end of the data section*/
                    STATE = S_DATA_2;
            	}
                else
                {
                	/*we keep looking until we find what we are looking for*/
                    continue;
				}
			} break;
			
			case S_DATA_2: 
			{
				/*same jive as above*/
				if (check == p[1])
				{
                    column_counter += 1;
                    temp_dat2 = Py_BuildValue("i", (word&0x0FFF));
                    data_location = Py_BuildValue("i", (row*(num_row)+column_counter-1));
                    DATA = PyObject_CallFunctionObjArgs(PLACE, DATA, temp_dat2, data_location, NULL);
                    
                    STATE = S_DATA_3;
            	}
                else
                {
                    /*we keep looking until we find what we are looking for*/
                    continue;
				}
				
			} break;
			
			case S_DATA_3: 
			{
				/*still the same*/
				if (check == p[2])
				{
                    column_counter += 1;
                    temp_dat3 = Py_BuildValue("i", (word&0x0FFF));
                    data_location = Py_BuildValue("i", (row*(num_row)+column_counter-1));
                    DATA = PyObject_CallFunctionObjArgs(PLACE, DATA, temp_dat3, data_location, NULL);
                    
                    STATE = S_DATA_4;
            	}
                else
                {
                    /*we keep looking until we find what we are looking for*/
                    continue;
				}
			
			} break;
				
			case S_DATA_4: 
			{
				/*woah now, this one is different*/
				if (check == p[3])
				{
                    column_counter += 1;
                    temp_dat4 = Py_BuildValue("i", (word&0x0FFF));
                    data_location = Py_BuildValue("i", (row*(num_row)+column_counter-1));
                    DATA = PyObject_CallFunctionObjArgs(PLACE, DATA, temp_dat4, data_location, NULL);
                    
                    /*if this data would be the 1024th pixel*/
                    if (column_counter>=num_column)
            		{	
            		    /*reset the pixel counter*/
			    		column_counter=0;
			    		/*change the state and start looking for the next piece of the packet*/
                		STATE = S_PACKET_END;
					}
					else
					{
						/*or send it back to look for more data*/
                        STATE = S_DATA_1;
                	}
                }
                else
                {
                    /*we keep looking until we find what we are looking for*/
                    continue;
				}
				
			} break;
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
				if (check==ltc0)
				{
					/*save the lower 3 nibbles to a C++ variable*/
					local_time = (word&0x0FFF);
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
				if (check==ltc1)
				{
					/*save the total value as a Python variable*/
					lt = Py_BuildValue("i", (local_time<<12)+(word&0x0FFF));
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
					row = (word&0x0FFF);
					
					if (row == 0)
					{
						/*increment the number of frames received*/
						frame_counter += 1;
						/*alter the Python variable holding the frame number*/
						FRAME_NUMBER = Py_BuildValue("i", frame_counter);
						
						//maybe a problem?
						/*when to flush the plots is determined by the number of frames since the first one*/
						first_or_second = Py_BuildValue("i", (frame_counter%reset_number));
						
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
				if (check==t01)
				{
					/*save the lower 3 nibbles to a C++ variable*/
					temp1 = (word&0x0FFF);
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
				if (check==t02)
				{
					/*store complete value as Python variable*/
					ts0 = Py_BuildValue ("i", (temp1)+(word&0x0FFF));
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
				if (check==t11)
				{
					temp2 = (word&0x0FFF);
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
				if (check==t12)
				{
					ts1 = Py_BuildValue ("i", (temp2)+(word&0x0FFF));
					STATE = S_DATA_1;
				}
				else
				{
					continue;
				}
			} break;
			//end
			case S_PACKET_END: 
			{
				/*if the word is the end packet literal*/
				if(word == e1234)
				{
					/*advance the state*/
					STATE = S_PACKET_START;
					/*increment the packet counter*/
					pack_counter += 1;
					
					/*if the data should be graphed and the frame has been completely read in*/
					if ((row == num_row-1))
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
						TOTAL = PyObject_CallFunctionObjArgs(sub_plot, PREVIOUS, DATA, TOTAL, FRAME_NUMBER, first_or_second, lt, ts0, ts1, NULL);
						/*the previous frame's data is then given to the PREVIOUS variable for proper use in the next frame*/
						PREVIOUS = DATA;
					}
				}
				else
				{
					/*we keep looking until we find what we are looking for*/
					continue;
				}
			} break;
			//it's broken
			default: std::cout << "I fucked up somewhere." << std::endl; break;
		}
	}
	
	
	
}

void deref_py()
{
	/*dereference all of the Python varaibles (to stop memory leaks)*/
	//I am definitely missing some
	Py_DECREF(lt);
	Py_DECREF(ts0);
	Py_DECREF(ts1);
	Py_DECREF(data_location);
	Py_DECREF(temp_dat1);
	Py_DECREF(temp_dat2);
	Py_DECREF(temp_dat3);
	Py_DECREF(temp_dat4);
	Py_DECREF(PREVIOUS);
	Py_DECREF(DATA);
	Py_DECREF(TOTAL);
	Py_DECREF(FRAME_NUMBER);
	Py_DECREF(first_or_second);
	Py_DECREF(pymodule);
	Py_DECREF(plot_this);
	Py_DECREF(SHOW);
	Py_DECREF(PLACE);
	Py_DECREF(sub_plot);
	/*close the Python instance*/
    Py_Finalize();
}


int main (int argc, char *argv[])
{	 	        
	    
}
