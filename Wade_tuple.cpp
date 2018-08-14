// TestPython.cpp : Defines the entry point for the console application.
//

#include <direct.h>
#include <string>
#include <stdio.h>
#include <python.h>
#include <windows.h>
#include <iostream>


int main()
{
	PyObject *pName, *pModule, *pFunc, *pFunc2;
	PyObject *pArgs, *pValue;

	printf("Start\n");

	Py_Initialize();

	//set the path to the release folder
	//wchar_t * path = L"C:/Users/Wade/Documents/Visual Studio 2015/Projects/TestPython/Release";
	//PySys_SetPath(path);

	pModule = PyImport_ImportModule("wade");

	if (pModule != NULL)
	{

	//https://github.com/Frogee/PythonCAPI_testing
	pFunc = PyObject_GetAttrString(pModule, "SumTuple");
	pFunc2 = PyObject_GetAttrString(pModule, "TuplePrint");

	//define size for the image and tuple size
	int size =  1024 * 1024;

	//use the bool even to know to write the tuple1 or tuple2 - do this instead of copying Data to Previous
	bool even = true;

	if (pFunc && PyCallable_Check(pFunc))
	{
		//define the two tuples
		PyObject *mytuple1 = PyTuple_New(size);
		PyObject *mytuple2 = PyTuple_New(size);

		//run multiple iterations of building a tuple
		for (int j = 0; j < 1000; j++)
		{
			//initialize tuples to 0
			for (int k = 0; k < size; k++)
			{
				PyTuple_SetItem(mytuple1, k, PyLong_FromLong(k));
				PyTuple_SetItem(mytuple2, k, PyLong_FromLong(k));
			}
			//fill the tuple
			for (int i = 0; i < size; i++)
			{
				//only fill one tuple, even first then odd
				if (even)
				{
					PyTuple_SetItem(mytuple1, i, PyLong_FromLong(i));
				}
				else
				{
					PyTuple_SetItem(mytuple2, i, PyLong_FromLong(i + 1));
				}
			}

			//call function - swap order of tuple1 and tuple2 based on bool
			if (even)
			{
				pValue = PyObject_CallFunctionObjArgs(pFunc, mytuple1, mytuple2, NULL);
			}
			else
			{
				pValue = PyObject_CallFunctionObjArgs(pFunc, mytuple2, mytuple1, NULL);
			}

			//error check pValue
			if (pValue == NULL)
			{
				PyErr_Print();
				break;
			}
			PyObject_CallFunctionObjArgs(pFunc2, PyTuple_GET_ITEM(mytuple1, 45), NULL);
			//invert even odd flag
			even = !even;
		}

		//free the tuples and the return value
		Py_DECREF(mytuple1);
		Py_DECREF(mytuple2);
		Py_DECREF(pValue);
	}
	else
	{
		printf("Method not found.\n");
	}

	//free the function and module
	Py_XDECREF(pFunc);
	Py_DECREF(pModule);

	}
	else
	{
		PyErr_Print();
		printf("script not found or script error\n");
	}

	Py_Finalize();
	
	printf("\nAny key to continue\n");
	getchar();
	return 0;
}
