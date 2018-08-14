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

int main (int argc, char *argv[])
{	 
	/*The size of the detector*/
	int num_row    = 1024;
	int num_column = 1024;
	
	/*Initialize a Python environment*/
	Py_Initialize();
	
	/*Run a command as if typing into a Python shell, a simple way of importing my other program*/
	if (!(PyRun_SimpleString("import just_plot")))
	{
		/*If it does not import, throw an error*/
		PyErr_Print();
	}
	
	/*Python variables used to store the pixel values before being added to the DATA list*/
	PyObject* temp_dat1 = 0;
	PyObject* temp_dat2 = 0;
	PyObject* temp_dat3 = 0;
	PyObject* temp_dat4 = 0;
	
	/*The next section is a load of Python variables used within the plotting function*/
	//=================================================================================================//
	// PREVIOUS        - the last frame's data/pixel values                                            //
	// DATA            - the current frame's data/pixel values                                         //
	// TOTAL           - the accumulated pixel data from successive subtracted images                  //
	// FRAME_NUMBER    - the number of the current frame we are on                                     //
	// first_or_second - a flag to inform the plot whether or not it must reset the plotting surface   //
	// lt              - the local time pulled from the stream                                         //
	// ts1, ts2, ts3   - the temperature sensors' readings embedded in the stream                      // 
	// data_location   - the position in the DATA list into which the next pixel value will be placed  //
	//=================================================================================================//
	PyObject* PREVIOUS = Py_BuildValue("i", 0);
	PyObject* DATA;
	PyObject* TOTAL = Py_BuildValue("i", 0);
	PyObject* FRAME_NUMBER;
	PyObject* first_or_second = Py_BuildValue("i", 0);
	PyObject* lt;
	PyObject* ts1;
	PyObject* ts2;
	PyObject* ts3;
	PyObject* data_location;
	
	/*the number of frames between resets of the plot*/
	int reset_number = 10; 
	
	/*The following section is for the Python module and the functions from within it that will be used*/
	//=================================================================================================//
	// plot_this - originally used for debugging                                                       //
	// SHOW      - used to test whether Python variables were being properly created                   //
	// PLACE     - inserts a given value into a specified point in a list                              //
	// sub_plot  - the main plotting function used to display the detector images                      //
	// make_list - creates the empty Python list of the correct size for DATA storage                  //
	//=================================================================================================//
	PyObject* pymodule    = PyImport_ImportModule("just_plot");
	PyObject* plot_this   = PyObject_GetAttrString(pymodule, "plot_this");
	PyObject* SHOW        = PyObject_GetAttrString(pymodule, "SHOW");
	PyObject* PLACE       = PyObject_GetAttrString(pymodule, "PLACE");
	PyObject* sub_plot    = PyObject_GetAttrString(pymodule, "sub_plot");
	PyObject* make_list   = PyObject_GetAttrString(pymodule, "LIST");
	
	/*Literals placed into the stream for proper navigation*/
	uint8_t s12 = 140;	//start row literal, 8C
	uint8_t s34 = 141; 	//start row literal, 8D
	int   s1234 = 35981;//start row literal, 8C8D
	                	//beginning of new row data stream
	                    
	uint8_t f1 = 15; 	//frame literal, F
	uint8_t r1 = 14;	//row literal, E
	                    
	uint8_t ltc1 = 13; 	//local time literal, D
	
	uint8_t t1 = 12;	//temp sensor literal 1, C
	uint8_t t2 = 11;	//temp sensor literal 2, B
	uint8_t t3 = 10;	//temp sensor literal 3, A
	                
	uint16_t p[4]  = {4, 5, 6, 7};	
						//pixel counter
	                 
	uint8_t e12 = 142;	//end of row literal, 8E
	uint8_t e34 = 143;	//end of row literal, 8F
	int   e1234 = 36495;//end of row literal, 8E8F
	
	/*used to determine when the plots will be flushed*/
	int pack_counter = 0;
	
	/*Finite State Machine enumerated type*/
	enum state {S_PACKET_START, S_FRAME, S_ROW, S_LTC, S_TEMP_1, S_TEMP_2, S_TEMP_3, S_DATA_1, S_DATA_2, S_DATA_3, S_DATA_4, S_PACKET_END};
	/*the state the machine is currently in*/
	state STATE = S_PACKET_START;
	
	uint16_t word = 0;	    //current word
	uint16_t next_word = 0; //the next word
	int     check = 0;	    //checker for literals at the start of word
	int       loc = 0;	    //location in memblock word is at 
	
	/*whether or not it is time to graph*/	
	bool graphing = false;
	/*the number of whole frames recieved*/
	int frame_counter  = 0;
	/*the position within a Tyler Packet the current pixel value is at*/
	int column_counter = 0;
	
	/*the current frame number*/
	int frame = 0;
	/*an array used to compare the current packet's frame with the previous packet's frame; a change in frame is useful information*/
	int pos_frame[2] = {-1,-1};
	
	/*the current row the data belongs in*/
	int row = 0;
	
	/*the local time and temperature sensor variables*/						
	int local_time = 0;
	int temp1, temp2, temp3 = 0;
	
	//create variable for reading in .TAD file
    std::ifstream read_in;
    //open it for read in, in binary, at the end
    //read_in.open("C:\\Users\\LabCad2\\Desktop\\r_bytes2.tad", std::ios::in|std::ios::binary|std::ios::ate);
    read_in.open("D:/WRX-R TEST DATA (PACKET 2)/SomeMVWarm_Card0.tad", std::ios::in|std::ios::binary|std::ios::ate);
    //read_in.open("C:/Users/PSU_Telemetry/Documents/WORKPLACE(CHRIS)/Python/r_bytes2.tad", std::ios::in|std::ios::binary|std::ios::ate);
	//size of chunk to read in at a time (currently 4 so I can word swap)
    int size = 4;//2109440; //read_in.tellg();
    					//size of 1 frame in bytes 
						//maybe not the best idea for in flight, but fine now
	
    //set the position to the beginning of the file
    read_in.seekg(0, std::ios_base::beg);
    
    //char array for reading in a bunch at a time
    char* memblock;
    memblock = new char [size];
    
    //used to fix bit reversal
    char* backwards;
    backwards = new char [size];
    char* forward;
    forward = new char [size];
    
    /*the size of the cabbage minor frame header*/
    int cabbage = 12;
    
    /*and the char array used to hold it*/
    char* garbage;
    garbage = new char [cabbage];
    
    /*a string used to hold the binary of the bytes for reversal*/
    std::string b_string = "";
    /*a char needed to help in reversing*/
    char hold = '0';
    
    //make a huge list in python so values can be placed out of order
    PyObject* det_size = Py_BuildValue("i", num_row*num_column); 
    
    DATA = PyObject_CallFunctionObjArgs(make_list, det_size, NULL);
    
    /*if the file cannot be read, inform the end user*/
    if (read_in.fail())
    {
		std::cout << "Uh-oh.\n";
	}
	else
	{
		/* TODO (#1#): THE FINITE STATE MACHINE 
		could possibly read in by 1 byte until 
		the row start is found, then read in by 8 million
		*/
		/*while there is more to be read*/
		while (!read_in.eof())
		{
			/*the location within memblock starts at 0*/
			loc = 0;
			/*TODO(#1#): POINT OF FAILURE WHEN READING IN, SIZE MAY BE OUT OF SYNC WITH STREAM AND INCREMENTING loc WILL CAUSE INDEXING ERROR*/
			//read_in.read( (memblock), size);
			/*read into the backwards char array*/
			
			
			//change between backwards and memblock depending on how awful the endianness and such is
			read_in.read( (backwards), size);
			//read_in.read( (memblock), size);
			
			//THE BLOCK TO UNCOMMENT DEPENDING ON BAD ENDIANNESS; DO NOT FORGET THE LINE ABOVE
			
			
			//reverse the bits of each byte
			for (int fix1 = 0; fix1 < size; fix1++)
			{
				//bytes->decimal->binary string->reverse it->number
				b_string = (std::bitset<8>(backwards[fix1]&0xFF).to_string());
				
				//does the reversing
				for (int f = 0, b = 8; f!=b; f++, b--)
				{
					hold = b_string[b-1];
					b_string[b-1] = b_string[f];
					b_string[f] = hold;
				}
				//place the bytes in the proper order into forward
				forward[fix1] = std::stoi(b_string, nullptr, 2);
			}
			
			//fix the endianness
			memblock [0] = forward [2];
			memblock [1] = forward [3];
			memblock [2] = forward [0];
			memblock [3] = forward [1];
			
			//END OF BIG COMMENT BLOCK
			
			/*while the location is not at the end of the array*/
			while (loc<size)
			{	
				/*word is the first 2 bytes*/
				word = (uint16_t)(memblock[loc]<<8) + (uint8_t)(memblock[loc+1]);
				
				/*if reading the next word will not cause errors*/
				if (loc+3<=size)
				{
					/*the word following the current one*/
					next_word = (uint16_t)(memblock[loc+2]<<8)+(uint8_t)(memblock[loc+3]);
					
					
					//HEADER STUFF TO BE REMOVED EVENTUALLY
					//[0040, 2c98] the cabbage header
					if ((word == 64)&&(next_word == 11416))
					{
						/*change location so it knows to to read more*/
						loc = size;
						/*read the whole header into garbage, where it belongs*/
						read_in.read( (garbage), cabbage);
						/*go back to the top of the loop*/
						continue;
					}
				}
				
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
							STATE = S_FRAME;
						}
						else
						{
							/*we keep looking until we find what we are looking for*/
							continue;
						}
					} break;
					//frame
					case S_FRAME: 
					{
						/*if the upper nibble is the frame literal*/
						if (check == f1)
						{
							/*the frame number is the lower 3 nibbles*/
							frame = (word&0x0FFF);
							/*advance the state*/
							STATE = S_ROW;
							
							/*swap the second with the first possible frame*/
							pos_frame[1] = pos_frame[0];
							/*the first possible frame is now the current frame*/
							pos_frame[0] = frame;
							/*if this is a new frame*/
							if (pos_frame[0] != pos_frame[1])
							{
								/*alter the Python variable holding the frame number*/
								FRAME_NUMBER = Py_BuildValue("i", pos_frame[0]);
								/*increment the number of frames received*/
								frame_counter += 1;
								
								//maybe a problem?
								/*when to flush the plots is determined by the number of frames since the first one*/
								first_or_second = Py_BuildValue("i", (frame_counter%reset_number));
								
								/*if this is not the first frame*/
								if (frame_counter > 0)
								{
									/*we should send the data to Python*/
									graphing = true;
								}
							}
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
							/*advance the state*/
							STATE = S_LTC;
							/*reset the column counter for a new row*/
							column_counter = 0;
						}
						else
						{
							/*we keep looking until we find what we are looking for*/
							continue;
						}
					} break;
					//local time counter
					case S_LTC: 
					{
						/*if the upper nibble is the time literal*/
						if (check==ltc1)
						{
							/*save the lower 3 nibbles to a Python variable*/
							lt  = Py_BuildValue ("i", (word&0x0FFF));
							/*advance the state*/
							STATE = S_TEMP_1;
						}
						else
						{
							/*we keep looking until we find what we are looking for*/
							continue;
						}
					} break;
					//temperature sensor 1
					case S_TEMP_1:
					{
						/*this template will be identical for all 3 temperature sensors*/
						/*if the upper nibble is the temperature sensor literal*/
						if (check==t1)
						{
							/*save the lower 3 nibbles to a Python variable*/
							ts1 = Py_BuildValue ("i", (word&0x0FFF));
							/*advance the state*/
							STATE = S_TEMP_2;
						}
						else
						{
							/*we keep looking until we find what we are looking for*/
							continue;
						}
					} break;
					//temperature sensor 2
					case S_TEMP_2:
					{
						if (check==t2)
						{
							ts2 = Py_BuildValue ("i", (word&0x0FFF));
							STATE = S_TEMP_3;
						}
						else
						{
							continue;
						}
					} break;
					//temperature sensor 3
					case S_TEMP_3:
					{
						if (check==t3)
						{
							ts3 = Py_BuildValue ("i", (word&0x0FFF));
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
							if ((graphing) && (row == num_row-1))
							{
								/*change the boolean so it does not plot something with incomplete data*/
    							graphing = false;
								
								/*THIS IS THE BIG CALL*/
								/*this is the function call to Python to plot the data given*/
								//===================================================================================================//
								// def sub_plot(prev, dat, total, frame_number, first_or_second, lt, ts1, ts2, ts3)                  //
								// where:                                                                                            //
								//     prev            - the previous frame's data                                                   //
								//     dat             - the current frame's data                                                    //
								//     total           - the accumulated image from the subtracted frames produced by dat and prev   //
								//     frame_number    - the current frame's number (for display purposes)                           //
								//     first_or_second - whether or not it is time to flush the plots for new data                   //
								//     lt              - the local time                                                              //
								//     ts1, ts2, ts3   - the temperature sensor readings                                             //
								//                                                                                                   //
								//     return value    - the sum of the previous subtracted frames and the new subtracted frame      //
								//===================================================================================================//
								TOTAL = PyObject_CallFunctionObjArgs(sub_plot, PREVIOUS, DATA, TOTAL, FRAME_NUMBER, first_or_second, lt, ts1, ts2, ts3, NULL);
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
	
	/*dereference all of the Python varaibles (to stop memory leaks)*/
	//I am definitely missing some
	Py_DECREF(lt);
	Py_DECREF(ts1);
	Py_DECREF(ts2);
	Py_DECREF(ts3);
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
    
    //close the file
    read_in.close();
        
    //return success!
    return 0;
}
