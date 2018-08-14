#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdint.h>
#include <string>
#include <vector>
#include <sstream>
#include "C:/Progra~2/python36-32/include/Python.h"
#include <stdlib.h>
#include <bitset>

/* 
** Currently very slow version that will be altered when the data comes out properly again so it can be used through DeweSoft on launch night
*/

int main (int argc, char *argv[])
{	 
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
	const int num_row    = 1024;
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
	PyRun_SimpleString("import just_plot3");

	PyObject* pymodule  = PyImport_ImportModule("just_plot3");
	PyObject* plot_this = PyObject_GetAttrString(pymodule, "plot_this");
	PyObject* sub_plot  = PyObject_GetAttrString(pymodule, "sub_plot");
	/*Python variable used to store the pixel values before being added to the DATA list*/
	PyObject* temp_dat = Py_BuildValue("i", 0);

	//make a huge list in python so values can be placed out of order
	PyObject* det_size = Py_BuildValue("i", num_row*num_column);

	/*The next section is a load of Python variables used within the plotting function*/
	//=================================================================================================//
	// DATA1           - frame of data, alternating with DATA2                                         //
	// DATA2           - frame of data, alternating with DATA1                                         //
	// FRAME_NUMBER    - the number of the current frame we are on                                     //
	// first_or_second - a flag to inform the plot whether or not it must reset the plotting surface   //
	// lt              - the local time pulled from the stream                                         //
	// ts0, ts1        - the temperature sensors' readings embedded in the stream                      // 
	//=================================================================================================//
	PyObject* DATA1 = PyTuple_New(num_row*num_column);
	PyObject* DATA2 = PyTuple_New(num_row*num_column);
	PyObject* FRAME_NUMBER = PyLong_FromLong(0);
	PyObject* first_or_second = PyLong_FromLong(0);
	PyObject* lt  = PyLong_FromLong(0);
	PyObject* ts0 = PyLong_FromLong(0);
	PyObject* ts1 = PyLong_FromLong(0);
	
	// initialize it to 0
	for (int h = 0; h < num_row*num_column; h++)
	{
		PyTuple_SetItem(DATA1, h, PyLong_FromLong(0));
		PyTuple_SetItem(DATA2, h, PyLong_FromLong(0));
	}
	
	/*the number of frames between resets of the plot*/
	const int reset_number = 75-1; //subtract 1 so the next value vanishes

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
	
	uint8_t part0 = 0;  //to fix word order
	uint8_t part1 = 0;
	uint8_t part2 = 0;
	uint8_t part3 = 0;
	
	int half_word[4] = { 3, 2, 1, 0 };
	
	bool graphing = false;
	unsigned long plot_counter = 0;
	uint16_t WORDS[2] = { 0, 0 }; //16-bit int
	int data_location = 0;
					
	/*all variables set; can start doing things*/

	//
	//
	/*AND THERE WAS VARIABLES*/
	//
	//

	//
	//
	//BEGIN
	//
	//		
	
	//std::ofstream bin_test;
	//bin_test.open("C:\\Users\\Public\\after_Dev.tad", std::ios::binary);
	
    std::ifstream read_in;
    read_in.open("C:\\Users\\Public\\testdata_bdd_edits_write_words.tad", std::ios::in|std::ios::binary|std::ios::ate);
	//read_in.open("C:\\Users\\PSU_Telemetry\\Desktop\\testdata_rows.bin", std::ios::in|std::ios::binary|std::ios::ate);
	//size of chunk to read in at a time (currently 4 so I can word swap)
	
    //set the position to the beginning of the file
    read_in.seekg(0, std::ios_base::beg);
    
    //char array for reading in a bunch at a time
    char* d;
    d = new char [size];
	
	/*the location within dataArray starts at 0*/
	loc = 0;
			
	//maybe s is only 1 byte at a time, then have to read twice before going to word and must
	//use half_word[] as a table for proper organization of bits
	
	//location is less than the size of current piece
	//may need to use pop again if the pieces are only single char's
	while (!read_in.eof())
	{
		read_in.read( d, size);
		
		loc = 0;
		
		uint16_t WORDS[2] = { 0, 0 }; //16-bit int
				
		while (loc < size)
		{			
			//the 16 bit ints to be read
			WORDS[0] = ((uint16_t)d[loc + 3] << 8) + (uint8_t)d[loc + 2];
			WORDS[1] = ((uint16_t)d[loc + 1] << 8) + (uint8_t)d[loc + 0];

			/*increase the location to where it will read in from next*/
			loc += 4;

			for (int half = 0; half<2; half++)
			{
				word = WORDS[half];

				//std::cout<<word<<std::endl;
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
							//lt = Py_BuildValue("i", (local_time << 12) | (word & 0x0FFF));
							lt = PyLong_FromLong((local_time << 12) | (word & 0x0FFF));
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
							if (!((word & 0x0FFF) < num_row))
							{
								continue;
							}
	
							/*the current row is the lower 3 nibbles*/
							row = (word & 0x0FFF);
	
							if (row == 0)
							{
								/*increment the number of frames received*/
								frame_counter += 1;
								/*alter the Python variable holding the frame number*/
								//FRAME_NUMBER = Py_BuildValue("i", frame_counter);
								FRAME_NUMBER = PyLong_FromLong(frame_counter);
	
								//maybe a problem?
	
	
								//may be obsolete, removed to test
								graphing = true;
	
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
							//ts0 = Py_BuildValue("i", (temp1 << 12) | (word & 0x0FFF));
							ts0 = PyLong_FromLong((temp1 << 12) | (word & 0x0FFF));
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
							//ts1 = Py_BuildValue("i", (temp2 << 12) | (word & 0x0FFF));
							ts1 = PyLong_FromLong((temp2 << 12) | (word & 0x0FFF));
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
							//bin_test << word << std::endl;
							/*everytime a pixel data is found, increment the counter*/
							column_counter += 1;
							/*the location within the 1-D array is also saved to a Python variable*/
							data_location = (row*(num_row)+column_counter - 1);
							/*because only Python variables may be used when calling a Python function*/
							//DATA1 = PyObject_CallFunctionObjArgs(PLACE, DATA1, temp_dat, data_location, NULL);
	
							//alternate which DATA is current and previous
							if (frame_counter%2)
							{
								PyTuple_SetItem(DATA1, data_location, PyLong_FromLong(long(word & 0xFFFF)));
							}
							else
							{
								PyTuple_SetItem(DATA2, data_location, PyLong_FromLong(long(word & 0xFFFF)));
							}
							
							//Py_DECREF(temp_dat);
							//Py_DECREF(data_location);
							//PyRun_SimpleString("gc.collect()");
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
								if ((row == num_row - 1) && (graphing))
								{
									//const char mystr[] = "Endword";
									//myFile.write(mystr, strlen(mystr));
	
									//std::cout<<"passed condition"<<std::endl;
									/*change the boolean so it does not plot something with incomplete data*/
									graphing = false;
	
									//when to flush the plots is determined by the number of frames since the first one
									//to be sure that both first frames are created
									plot_counter += 1;
									//first_or_second = Py_BuildValue("i", (plot_counter%reset_number));
									first_or_second = PyLong_FromLong((plot_counter%reset_number));
	
									/*THIS IS THE BIG CALL*/
									/*this is the function call to Python to plot the data given*/
									//===================================================================================================//
									// def sub_plot(prev, dat, total, frame_number, first_or_second, lt, ts0, ts1)                       //
									// where:                                                                                            //
									//     prev            - the previous frame's data                                                   //
									//     dat             - the current frame's data                                                    //
									//     frame_number    - the current frame's number (for display purposes)                           //
									//     first_or_second - whether or not it is time to flush the plots for new data                   //
									//     lt              - the local time                                                              //
									//     ts0, ts1        - the temperature sensor readings                                             //
									//===================================================================================================//
									
									if (frame_counter%2)
									{
										PyObject_CallFunctionObjArgs(sub_plot, DATA2, DATA1, FRAME_NUMBER, first_or_second, lt, ts0, ts1, NULL);
									}
									else
									{
										PyObject_CallFunctionObjArgs(sub_plot, DATA1, DATA2, FRAME_NUMBER, first_or_second, lt, ts0, ts1, NULL);
									}
								}
							}
							else
							{
								//keep looking
								continue;
							}
						}
					} break;
				} // end of SWITCH
			} // end of FOR
		} // end of WHILE loc < size
	}
	/*just to inform the end user of the success*/
	std::cout << std::endl << pack_counter  << " packets read." ;
    std::cout << std::endl << frame_counter << " frames read." << "\n\n";
    
	//so we can look at the final frame
    std::cout << "Type EXIT to terminate the program." << std::endl;
    std::string CLOSE = " ";
    while (CLOSE != "EXIT")
    {
    	std::cin >> CLOSE;
    }
	
	/*close the Python instance*/
    Py_Finalize();
    
    //close the file
    read_in.close();
    //bin_test.close();
        
    //return success!
    return 0;
}
