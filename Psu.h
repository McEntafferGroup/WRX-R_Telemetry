// PennStateDll.cpp : Defines the entry point for the console application.
//
// 2017-07-31
// K. Wade Nye
// Ulyssix Technologies, Inc
//
// Example code for using a method for external software to add data to a queue
// and an asyncronous thread to wait for when there is data 

#if !defined(PENNSTATE_H)
#define PENNSTATE_H

#ifndef LINUX
#include <windows.h>	// Include windows.h for data types.
#else
#include <windrvr.h>
#endif

#ifdef __cplusplus
extern "C"{
#endif

//Defines export of API
#define PENNSTATE_API __declspec(dllexport)



//********************************************************
// Start of user code

//*** Define Enum and Structs
	// None


//*** Declare function prototypes for export

	//Writes data into the queue
	PENNSTATE_API int WriteDataToQueue(unsigned char* dataArray, int dataArraySize);

	//inits handles and starts data process thread
	PENNSTATE_API void DataProcessStart();

	//stops data process thread and cleans up handles
	PENNSTATE_API void DataProcessStop();

	//stops data process thread and cleans up handles
	PENNSTATE_API int IsProcessRunning();
	
// End of user code
//********************************************************



#ifdef __cplusplus
}
#endif


#endif
