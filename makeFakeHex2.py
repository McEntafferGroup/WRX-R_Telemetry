import numpy as np
import time
import mmap
import os
import time

"""
frames and rows are 24 bits, column data is only 12
"""

start_time = time.time()

"""number of frames, columns, and rows identical to the create_test_coordinates values so it doesn't break or run forever"""
num_frame  = 10
num_row    = 1024
num_column = 1024
npd = 3             #nybbles per data packet-chunk

"""file to read in coords from"""
use_coords = open('coords2.txt', 'r')

"""file to write the bytes to in order to be read after, I have several on hand"""
r_bytes = open('r_bytes2.tad', 'bw')

"""literals and whatnot needed for packaging in Tyler's format"""
s12 = 140     # start row literal, 8C
s34 = 141     #second row literal, 8D
                    #beginning of new row data stream

f1 = 15          #frame literal, F

r1 = 14          #row literal, E

LTC1 = 13           #local time literal, D

t1 = 12             #temp sensor literal 1, C
t2 = 11             #temp sensor literal 2, B
t3 = 10             #temp sensor literal 3, A

p  = [4, 5, 6, 7]  #pixel counters

e12 = 142     #end of row literal, 8E
e34 = 143     #end of row literal, 8F

"""counters and lists used for holding numbers before writing to file"""
g = 0
n = 0
frames  = ['']*npd
rows    = ['']*npd
columns = []
column  = []
times   = ['']*npd
temps1  = ['']*npd
temps2  = ['']*npd
temps3  = ['']*npd

"""to keep the lists generalized, initialization is done in nested loops"""
for a in range(0, num_column):
    column = []
    for b in range(0, npd):
        column.append([''])
    columns.append(column)

word = ''
line_counter = 1
total = 0

print('Expected wait time for program to complete file: {} minutes {} seconds'.format(int((29*num_frame)//60), int((29*num_frame)%60)))

for line in use_coords:
    n = 0
    g = 0 
    if ((line_counter%num_column==1) and (line_counter != 1)): 
        #every Nth line where N = num of columns
        #and this is not the first line
        total = (total + line_counter)      #total line count
        line_counter = 1                    #reset line counter
        """begin writing the bytes file"""
        """s12"""
        r_bytes.write(bytes([s12]))
        """s34"""
        r_bytes.write(bytes([s34]))
        """f1 and first nybble of frameID"""
        r_bytes.write(bytes([(f1<<4)+int(frames[0],16)]))
        """next 2 nybbles of frameID"""
        r_bytes.write(bytes([(int(frames[1],16)<<4)+int(frames[2],16)]))
        """r1 and first nybble of rowID"""
        r_bytes.write(bytes([(r1<<4)+int(rows[0],16)]))
        """next 2 nybles of rowID"""
        r_bytes.write(bytes([(int(rows[1],16)<<4)+int(rows[2],16)]))
        """LTC1 and first nybble of time"""
        r_bytes.write(bytes([(LTC1<<4)+int(times[0],16)]))
        """next 2 nybbles of time"""
        r_bytes.write(bytes([(int(times[1],16)<<4)+int(times[2],16)]))
        """now the temperatures"""
        r_bytes.write(bytes([(t1<<4)+int(temps1[0],16)]))
        r_bytes.write(bytes([(int(temps1[1],16)<<4)+int(temps1[2],16)]))
        r_bytes.write(bytes([(t2<<4)+int(temps2[0],16)]))
        r_bytes.write(bytes([(int(temps2[1],16)<<4)+int(temps2[2],16)]))
        r_bytes.write(bytes([(t3<<4)+int(temps3[0],16)]))
        r_bytes.write(bytes([(int(temps3[1],16)<<4)+int(temps3[2],16)]))
        """end bytes section for ID values"""
        
        i = 0
        for i in range(0, num_column):
            """pixel counter + first nybble of data for each if-statement"""
            if (i%4==0):
                r_bytes.write(bytes([((p[0]<<4)+int(columns[i][0],16))]))
            elif (i%4==1):
                r_bytes.write(bytes([((p[1]<<4)+int(columns[i][0],16))]))
            elif (i%4==2):    
                r_bytes.write(bytes([((p[2]<<4)+int(columns[i][0],16))]))
            elif (i%4==3):
                r_bytes.write(bytes([((p[3]<<4)+int(columns[i][0],16))]))
            
            """write the last 2 nybbles of data regardless of counter"""
            r_bytes.write(bytes([((int(columns[i][1],16)<<4)+int(columns[i][2],16))]))
        
        """e12"""
        r_bytes.write(bytes([e12]))
        """e34"""
        r_bytes.write(bytes([e34]))
        
    for char in line:
        if (char != ' ') and (char != '\n'): #if the char is not a space
            word = word + str(char)
            n = n + 1
            if (n<=3):
                """place the word into a variable"""
                frames[n-1] = word
                #print(word)
                """reset the word"""
                word = ''
            elif (n>3) and (n<=6):
                rows[n-4] = word
                #print(word)
                word = ''
            elif (n>6) and (n<=9):
                columns[line_counter-1][n-7] = word
                #print(word)
                word = ''
            elif (n>9) and (n<=12):
                times[n-10] = word
                #print(word)
                word = ''
            elif (n>12) and (n<=15):
                temps1[n-13] = word
                #print(word)
                word = ''
            elif (n>15) and (n<=18):
                temps2[n-16] = word
                #print(word)
                word = ''
            elif (n>18) and (n<=21):
                temps3[n-19] = word
                #print(word)
                word = ''
            
            
    if (line != ' ') and (line != '\n'):
        line_counter = line_counter + 1


"""close the file so the the file can be altered and opened"""
use_coords.close()

#raw_data.close()
r_bytes.close()

"""in case you left, again"""
final = (time.time() - start_time)
print('Time spent running program: {} minutes {} seconds'.format(int(final//60), int(final%60)))