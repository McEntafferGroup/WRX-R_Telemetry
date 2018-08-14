import numpy as np
import time
import os
import time

"""
frames and rows are 24 bits, column data is only 12
"""

start_time = time.time()

"""number of frames, columns, and rows identical to the create_test_coordinates values so it doesn't break or run forever"""
num_row    = 1024
num_column = 1024
npd = 3             #nybbles per data packet-chunk

"""file to write the bytes to in order to be read after, I have several on hand"""
r_bytes = open('C:\\Users\\PSU_Telemetry\\Documents\\WORKPLACE(CHRIS)\\Python\\r_bytes3.tad', 'bw')

"""literals and whatnot needed for packaging in Tyler's format"""
s12 = 140        # start row literal, 8C
s34 = 141        #second row literal, 8D
                 #beginning of new row data stream

LTC0 = 15        #local time literal, F
LTC1 = 14        #local time literal, E

r1 = 13          #row literal, D

t01 = 12         #temp sensor literal, C
t02 = 11         #temp sensor literal, B
t11 = 10         #temp sensor literal, A
t12 = 9          #temp sensor literal, 9

p  = [4, 5, 6, 7]  #pixel counters

e12 = 142        #end of row literal, 8E
e34 = 143        #end of row literal, 8F

"""
start
time
row 
volt/curr
    all columns
end
"""

for frame in range(20):
    for row in range(1024):
        """start literals"""
        r_bytes.write(bytes([s12]))
        r_bytes.write(bytes([s34]))
        
        ##time section
        """get the time since start as a hex string"""
        time0 = format(int((time.time()-start_time)*1000),'06x')
        
        r_bytes.write(bytes([(LTC0<<4)+int(time0[:1],16)]))
        r_bytes.write(bytes([int(time0[1:3],16)]))
        
        r_bytes.write(bytes([(LTC1<<4)+int(time0[3:4],16)]))
        r_bytes.write(bytes([int(time0[4:],16)]))
        
        """write the row"""
        r_bytes.write(bytes([(r1<<4)+int(format(row,'03x')[:1],16)]))
        r_bytes.write(bytes([int(format(row,'03x')[1:],16)]))
        
        ##temperature section
        if not (row%2):
            """even portion (current)"""
            curr1 = format(np.random.randint(0,2000), '06x')
            
            curr2 = format(np.random.randint(2000,4000), '06x')
            
            """first half of first half"""
            r_bytes.write(bytes([(t01<<4)+int(curr1[:1],16)]))
            """second half of first half"""
            r_bytes.write(bytes([int(curr1[1:3],16)]))
            """first half of second half"""
            r_bytes.write(bytes([(t02<<4)+int(curr1[3:4],16)]))
            """second half of second half"""
            r_bytes.write(bytes([int(curr1[4:],16)]))
            
            """repeat pattern above"""
            r_bytes.write(bytes([(t11<<4)+int(curr2[:1],16)]))
            r_bytes.write(bytes([int(curr2[1:3],16)]))
            r_bytes.write(bytes([(t12<<4)+int(curr2[3:4],16)]))
            r_bytes.write(bytes([int(curr2[4:],16)]))
            
        else:
            """odd portion (voltage)"""
            volt1 = format(np.random.randint(4000,6000), '06x')
            
            volt2 = format(np.random.randint(6000,8000), '06x')
            
            r_bytes.write(bytes([(t01<<4)+int(volt1[:1],16)]))
            r_bytes.write(bytes([int(volt1[1:3],16)]))
            r_bytes.write(bytes([(t02<<4)+int(volt1[3:4],16)]))
            r_bytes.write(bytes([int(volt1[4:],16)]))
            r_bytes.write(bytes([(t11<<4)+int(volt2[:1],16)]))
            r_bytes.write(bytes([int(volt2[1:3],16)]))
            r_bytes.write(bytes([(t12<<4)+int(volt2[3:4],16)]))
            r_bytes.write(bytes([int(volt2[4:],16)]))
        
        """column section"""
        for col in range(1024):
            """all the bits"""
            r_bytes.write(bytes([(p[col%4]<<4)+
                            int(format(col,'03x')[:1],16)]))
                            
            r_bytes.write(bytes([int(format(col,'03x')[1:],16)]))
        
        
        """end literals"""
        r_bytes.write(bytes([e12]))
        r_bytes.write(bytes([e34]))


"""close the file so the the file can be altered and opened"""
use_coords.close()

#raw_data.close()
r_bytes.close()

"""in case you left, again"""
final = (time.time() - start_time)
print('Time spent running program: {} minutes {} seconds'.format(int(final//60), int(final%60)))