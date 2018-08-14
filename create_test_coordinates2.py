import numpy as np
import time

"""
this is often the most time consuming part of creating the fake data, 
writing it all in ASCII to a text file to be read in afterward

the process may be sped up quite significantly, but DEWESoft should be able
to create the data for us once it is up and running; no need to optimize
"""

start_time = time.time()

"""the name of the file to be filled with coordinates"""
make_coords = open('coords2.txt','w')

"""the number of frames/images that will be created, the number of rows
and columns in each frame (keep square and must be a factor of 4; 
I have no idea if it will break if not square)"""
num_frame  = 10
num_row    = 1024
num_column = 1024

"""to inform you of how big your end file will be, in bytes, so you may abort
if it is more than you bargained for"""
print('Predicted bytes of end file: {:,}'.format(
num_column*num_row*num_frame*31))
print('Expected wait time for program to complete file: {} minutes {} seconds'.format(int((48.2717*num_frame)//60), int((48.2717*num_frame)%60)))

value = 0
for i in range(0,num_frame):
    for j in range(0,num_row):
        for k in range(0,num_column):
            """the frame number in 3 hex digits"""
            make_coords.write(format(i, '03x')) #frame
            make_coords.write(" ")
            """the row number in 3 hex digits"""
            make_coords.write(format(j, '03x')) #row
            make_coords.write(" ") 
            """create a random x-ray event"""
            if (abs(np.random.normal(0,1))>4):
                make_coords.write(format(int(np.random.normal(900,10)), '03x'))
            else:
                """or just create Gaussian background, no negatives"""
                value = int(np.random.normal(100,2))
                if (value<=0):
                    """data of each pixel value in 3 digits of hex"""
                    make_coords.write(format(0, '03x'))
                else:
                    """data of each pixel value in 3 digits of hex"""
                    make_coords.write(format(int(value), '03x')) #column/data
            make_coords.write(" ") 
            """local time counter (caps at 4095)"""
            make_coords.write(format(int(time.time()-start_time), '03x'))
            make_coords.write(" ")
            """detector temperatures (~-80C)"""
            make_coords.write(format(int(np.random.normal(193,1)), '03x'))
            make_coords.write(" ")
            make_coords.write(format(int(np.random.normal(193,1)), '03x'))
            make_coords.write(" ")
            make_coords.write(format(int(np.random.normal(193,1)), '03x'))
            #structure of packet allows us to know the column
            make_coords.write("\n\n")
            
"""close the file so you don't break the computer"""
make_coords.close()
"""tells you how long it took in case you left"""
final = (time.time() - start_time)
print('Time spent running program: {} minutes {} seconds'.format(int(final//60), int(final%60)))