"""

dePLOT.py is designed to decode packets from the WRX-R Telemetry Stream

all variables used are declared before the main loop and catagorized

Finite State Machine will run until the stream/file ends 

"""
import matplotlib
matplotlib.use('TkAgg')
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.cm
import struct
from struct import pack
from struct import unpack
import gc
import time
import operator

start_time = time.time()

"""variables for navigating the stream"""
s1234 = b'\x8c\x8d' #start row literal, 8C8D
                    #beginning of new row data stream

f1 = 15             #frame literal, F

r1 = 14             #row literal, E

ltc1 = 13           #local time literal, D

t1 = 12             #temp sensor literal 1, C
t2 = 11             #temp sensor literal 2, B
t3 = 10             #temp sensor literal 3, A

p  = [4, 5, 6, 7]   #pixel counter

e1234 = b'\x8e\x8f' #end of row literal, 8E8F

"""files"""
"""the filepath to the .tad written from the previous program"""
read_bytes = open('D:/WRX-R TEST DATA (PACKET 2)/MVE_TestBox_PSU_16BitRingCount_Try2_2017_07_19_193406_Card0.tad', 'rb')
# read_bytes = open('/Users/LabCad2/Desktop/RingCounterFakeData.tad', 'rb')
# read_bytes = open('/Users/LabCad2/Desktop/r_bytes2.tad', 'rb')
# read_bytes = open('r_bytes2048x2048x10.tad', 'rb')
# read_bytes = open('r_bytes_HUGE.tad', 'rb')

## FRC Numbers
"""variables for controlling stream"""
"""keep these the same as the previous 2 programs unless using one of my alternate files, in which case:
2048x2048x10 is 10 frames, 2048 rows and columns
HUGE is 100 frames with 2048 rows and columns"""
num_frame  = 100
num_row    = 1024
num_column = 1024
pack_counter = 0
col_looper = False
STATE = 'S_PACKET_START'
word   = b''

"""variables for plotting"""
graphing = False
frame_counter  = 0
row_counter    = 0
column_counter = 0 

frame = 0
pos_frame = ['','']
frames = [0 for a in range(num_frame)]

row = 0
pos_row = ['','']
rows = [0 for a in range(num_row)]

local_time = 0
temp1 = 0
temp2 = 0
temp3 = 0

data = list()
total  = [[0 for a in range(num_column)] for b in range(num_row)]
maxed, maxed1 = 0, 0
tot_norm = [[0 for a in range(num_column)] for b in range(num_row)]

"""functions"""
def Abin(data):
    binned = [[0 for a in range(len(data[0])>>2)] for b in range(len(data)>>2)]
    for i in range(len(data)):
        j=0
        k=0
        while j<len(data[i]):
            binned[i>>2][j>>2]=binned[i>>2][j>>2]+data[i][j]+data[i][j+1]
            j=j+2
            
    return binned
sized=0
"""toss the TAD file header BS so the bytes line up"""
JOKER = read_bytes.read(20780580) #340 for header
while (True):
    try:
        """run until we hit the end of the stream"""
        """this script reads 2 bytes at a time, 
        C++ reads by 1, this may be altered"""
        word = read_bytes.read(4)
        print(word)
        """add this line to fix the endianness and this whole fucking loop"""
        word2 = word[-1::-1]
        print(word2)
        
        word = unpack('<2H',word2)
        
        
        print(word)
        
        if(word2!=b'\xfek(@'):
            data.append((unpack('>2H', word)[0]<<8)+unpack('>2H', word)[1])
            
            pack_counter += 1
            
        STATE = 'S_PACKET_START'
        
        ## FRAME PLOT
        if (pack_counter%(num_row*num_column)==0) and (pack_counter!=0):
            frame_counter+=1
            plot_time = time.time()
            
            """bin the data"""
            d_bin = np.reshape(np.array(data),(num_row, num_column))
            
            for a in range(len(d_bin)):
                """
                add all values from d_bin to total
                """
                total[a] = np.add(total[a], d_bin[a])
            
            maxed = max(list(map(max, total)))
            
            tot_norm = list(map(
            (lambda a: [b/maxed for b in a]), total))
            
            """np array used for the graph"""
            td = np.array(tot_norm)


            
            """np array used for the graph"""
            d = np.array(d_bin)
            
            if ((frame_counter-1)==0):
                """
                for the first frame
                
                in the first plot (the left one)
                
                input d as the data, cm.winter as the color, 
                square plots, origin in the lower left corner, 
                normalized from 0.5 to 1.0 intensity
                """
                fig = plt.figure(1,figsize=(10,5))
                ax1 = plt.subplot(121)
                im1 = ax1.imshow(d,cmap=matplotlib.cm.winter,
                aspect='equal',origin='lower')#, vmin=0.5,vmax=1.0)
                
                """label the axes; these will never change"""
                ax1.xaxis.set_label_text(str('columns [')+'0'+
                str('-->')+str(len(d_bin)-1)+str(']'))
                ax1.yaxis.set_label_text(str('rows [')+'0'+
                str('-->')+str(len(d_bin[0])-1)+str(']'))
                
            else:
                """clear and fully redraw the graph"""
                #ax1.clear()
                #im1 = ax1.imshow(d,cmap=matplotlib.cm.winter,
                #aspect='equal',origin='lower')
                
                """or replace the data and redraw in the same window"""
                im1.set_data(d)    
                ax1.redraw_in_frame()
                
            """update the title with every new frame"""
            ax1.set_title("FRAME NUMBER: "+str(
            frames[frame_counter-1]))
            
            """
            draw the figure, can pause here for distinction
            between the new frame and cumulative frame
            """
            #fig.canvas.draw()
            #fig.canvas.flush_events()
            
            ## SUMMED GRAPH SECTION
            """np array used for the graph"""
            td = np.array(tot_norm)
            
            if ((frame_counter-1)==0):
                """
                for the first frame
                
                in the second plot (the right one)
                
                input td as the data, cm.winter as the color, 
                square plots, origin in the lower left corner, 
                normalized from 0.3 to 1.0 intensity
                """
                ax2 = plt.subplot(122)
                im2 = ax2.imshow(td, cmap=matplotlib.cm.winter,
                aspect='equal',origin='lower', vmin=0.3, vmax=1.0)
                
                """label the axes; these will never change"""
                ax2.xaxis.set_label_text(str('columns [')+'0'+
                str('-->')+str(len(tot_norm)-1)+str(']'))
                ax2.yaxis.set_label_text(str('rows [')+'0'+
                str('-->')+str(len(tot_norm[0])-1)+str(']'))
                
            else:
                #ax2.clear()
                #im2 = ax2.imshow(td, cmap=matplotlib.cm.winter,
                #aspect='equal',origin='lower')
                """
                replace the data in the graph
                redraw  the data in the graph
                """
                im2.set_data(td)
                ax2.redraw_in_frame()
                
            """update the title with every new frame"""
            ax2.set_title("FRAMES "+str(frames[0])+"-->"+str(
            frames[frame_counter-1]))
            
            """show the changes by updating the plots"""
            fig.canvas.draw()
            fig.canvas.flush_events()
            
            plt.show(block=False)
            
            """show the time taken"""
            print("Time taken to plot this frame: {}".format(
            time.time()-plot_time))
            
            """
            Clean-up after plotting so the data is not contaminated
            """
            
            maxed1 = 0
            maxed  = 0        
            data = list()    
            
            gc.collect()
            
    ## After the stream/file ends
    except IndexError:
        print("Total number of packets:", pack_counter)
        
        print('Time spent running program: {}'.format(
        time.time() - start_time))
        
        print('\nTo obtain the value of a pixel in the left graph, ')
        print('the proper syntax is \'d_bin[row][column]\'.')
        print('\nThe second graph\'s syntax is \'tot_norm[row][column]\'.')
        
        read_bytes.close()
        break