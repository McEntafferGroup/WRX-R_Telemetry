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
FIRST_FRAME = True

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
read_bytes = open('D:/WRX-R TEST DATA (PACKET 2)/MVE_TestBox_PSU_FakeData_Try3_2017_07_19_193859_Card0.tad', 'rb')
# read_bytes = open('/Users/LabCad2/Desktop/RingCounterFakeData.tad', 'rb')
# read_bytes = open('/Users/LabCad2/Desktop/r_bytes2.tad', 'rb')
# read_bytes = open('r_bytes2048x2048x10.tad', 'rb')
# read_bytes = open('r_bytes_HUGE.tad', 'rb')
"""see what the MV encoder really says"""
# transcribe = open('/Users/LabCad2/Desktop/MV_TRANSCRIPTION.txt', 'w')

## FRC Numbers
"""variables for controlling stream"""
"""keep these the same as the previous 2 programs unless using one of my alternate files, in which case:
2048x2048x10 is 10 frames, 2048 rows and columns
HUGE is 100 frames with 2048 rows and columns"""
num_frame  = 500
num_row    = 1024
num_column = 1024
pack_counter = 0
col_looper = False
STATE = 'S_PACKET_START'
word   = b' '

"""variables for plotting"""
graphing = False
frame_counter  = 0
row_counter    = 0
column_counter = 0 

frame = 0
pos_frame = ['','']
frames = [-1, -1]

row = 0
pos_row = ['','']
rows = [0 for a in range(num_row)]

local_time = 0
temp1 = 0
temp2 = 0
temp3 = 0

data = [[0 for a in range(num_column)] for b in range(num_row)]
total  = [[0 for a in range(num_column)] for b in range(num_row)]
maxed, maxed1 = 0, 0
tot_norm = [[0 for a in range(num_column)] for b in range(num_row)]


sized=0
"""toss the TAD file header BS so the bytes line up"""
# JOKER = read_bytes.read(340)#20780580) #340 for header
while (word!=b''):
    try:
        """run until we hit the end of the stream"""
        """this script reads 2 bytes at a time, 
        C++ reads by 1, this may be altered"""
        word = read_bytes.read(4)
        
        if (word==b''):
            break
        
        # print(word)
        """add this line to fix the endianness and this whole fucking loop"""
        word2 = word[-1::-1]
        
        words = ['','']
        
        for k in range(2):
            words[1] = format(int(format(unpack(
            '<2H', word)[0],'016b')[-1::-1],2),'04x')
            
            words[0] = format(int(format(unpack(
            '<2H', word)[1],'016b')[-1::-1],2),'04x')
            
            check = (int(words[k],16))>>12
            
            """need to figure out how 0040 2C98 looks and skip next shit"""
            if (words==['0040', '2c98']):
                """removes all the rest of the header"""
                garbage = read_bytes.read(12)
                
                # jums = unpack('>6H', garbage)
                # for i in jums:
                #     print(format(i,'02x'))
                # input('pause')
                break
            
            word2 = pack('>H', int(words[k],16))
            
            # transcribe.write('sim_mv_encoder: received {}\n '.format(
            # words[k].upper()))
            
            ## FINITE STATE MACHINE
            if ((STATE is 'S_DATA_1') or col_looper):
                """keep looping until all columns have been read"""
                col_looper = True
                
                if (column_counter < (num_column)):
                    """
                    column counter is for the number of columns received properly
                    while i is for the number of columns that SHOULD HAVE been
                    processed, making i the useable counter
                    """
                    
                    i = i + 1
                    
                    if (STATE is 'S_DATA_1'):
                        if (check == p[0]):
                            """downshift word to ONLY check its upper nibble"""
                            column_counter = column_counter + 1
                            data[row][column_counter-1] = (
                            (word2[0]<<8)+word2[1]-(p[0]<<12))
                            
                            STATE = 'S_DATA_2'
                            
                        else:
                            #col_looper = False
                            continue
                            
                    elif (STATE is 'S_DATA_2'):
                        if (check == p[1]):
                            column_counter = column_counter + 1
                            data[row][column_counter-1] = (
                            (word2[0]<<8)+word2[1]-(p[1]<<12))
                            
                            STATE = 'S_DATA_3'
                            
                        else:
                            #col_looper = False
                            continue
                        
                    elif (STATE is 'S_DATA_3'):
                        if (check == p[2]):
                            column_counter = column_counter + 1
                            data[row][column_counter-1] = (
                            (word2[0]<<8)+word2[1]-(p[2]<<12))
                            
                            STATE = 'S_DATA_4'
                            
                        else:
                            #col_looper = False
                            continue
                    
                    #this is where it gets tricky
                    elif (STATE is 'S_DATA_4'):
                        if (check == p[3]):
                            column_counter = column_counter + 1
                            data[row][column_counter-1] = (
                            (word2[0]<<8)+word2[1]-(p[3]<<12))
                            
                            if (column_counter >= (num_column)):
                                col_looper = False
                                STATE = 'S_PACKET_END'
                                continue
                            else:
                                STATE = 'S_DATA_1'   
                                
                        else:
                            #col_looper = False
                            continue
                            
                else:
                    col_looper = False
                    STATE = 'S_PACKET_END'
    
    
    
            elif (STATE is 'S_PACKET_START'):
                if (word2 == s1234):
                    STATE = 'S_FRAME'
                    
                else:
                    continue
                    
            #first 3 nybbles of frameID in hex
            elif (STATE is 'S_FRAME'):
                if (check == f1):
                    
                    frame = ((word2[0]<<8) + word2[1])-(f1<<12)
                    
                    pos_frame[1] = pos_frame[0]
                    pos_frame[0] = frame
                    if (pos_frame[0] != pos_frame[1]):  #a new frame
                        """start timer for reading/plotting"""
                        plot_time = time.time()
                        
                        if (frames[0]==(-1)):
                            frames[0] = pos_frame[0]
                            
                        else:
                            frames[1] = pos_frame[0]
                        
                        frame_counter = frame_counter + 1
                        if (frame_counter>0):
                            graphing = True
                    
                    column_counter = 0
                    
                    STATE = 'S_ROW'
                    
                else:
                    continue
                    
            #first 3 nybbles of rowID in hex
            elif (STATE is 'S_ROW'):
                if (check == r1):
                    row = ((word2[0]<<8) + word2[1])-(r1<<12)
                    
                    i = 0
                    column_counter = 0
                    
                    STATE = 'S_LTC'
                    
                else:
                    continue
                    
            elif (STATE == 'S_LTC'):
                if (check == ltc1):
                    local_time = ((word2[0]<<8) + word2[1])-(ltc1<<12)
                    
                    STATE = 'S_TEMP_1'
                    
                else:
                    continue
                    
            elif (STATE == 'S_TEMP_1'):
                if (check == t1):
                    temp1 = ((word2[0]<<8) + word2[1])-(t1<<12)
                    
                    STATE = 'S_TEMP_2'
                    
                else:
                    continue
                    
            elif (STATE == 'S_TEMP_2'):
                if (check == t2):
                    temp2 = ((word2[0]<<8) + word2[1])-(t2<<12)
                    
                    STATE = 'S_TEMP_3'
                    
                else:
                    continue
                    
            elif (STATE == 'S_TEMP_3'):
                if (check == t3):
                    temp3 = ((word2[0]<<8) + word2[1])-(t3<<12)
                    
                    STATE = 'S_DATA_1'
                    
                else:
                    continue
                    
            elif (STATE is 'S_PACKET_END'):
                
                if (word2 == e1234):
                    
                    pack_counter += 1
                    STATE = 'S_PACKET_START'
                    
                    ## FRAME PLOT
                    if (graphing) and (pack_counter%num_row==0):
                        """
                        if this is a new frame and the correct number of 
                        packets passed through
                        
                        reset the boolean for whether or not to graph
                        """
                        graphing = False
                        
                        
                        """identify the reading in as the slow process"""
                        #print('IT HAS TAKEN {} SECONDS TO READ IN 1 FRAME'.format(time.time()-start_time))
                        
                        """bin the data"""
                        d_bin = np.array(data)
                        
                        """acquire the maximum value for normalization"""
                        maxed1 = d_bin.max()
                        
                        ## SUMMED MATHS SECTION
                        total = total + d_bin
                        
                        """acquire the maximum value for normalization"""
                        maxed = total.max()
                        
                        """normalize every data point in total for tot_norm"""
                        tot_norm = total/maxed
                        
                        """np array used for the graph"""
                        td = np.array(tot_norm)
                        ## END SUMMED MATHS
                        
                        """normalize the values"""
                        d_bin = d_bin/maxed1
                        
                        """np array used for the graph"""
                        d = np.array(d_bin)
                        
                        if (FIRST_FRAME):
                            """
                            for the first frame
                            
                            in the first plot (the left one)
                            
                            input d as the data, cm.winter as the color, 
                            square plots, origin in the lower left corner, 
                            normalized from 0.5 to 1.0 intensity
                            """
                            
                            fig = plt.figure(1,figsize=(15,5))
                            ax1 = plt.subplot(131)
                            im1 = ax1.imshow(d,cmap=matplotlib.cm.hot,
                            aspect='equal',origin='lower', vmin=0.5,vmax=1.0)
                            
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
                        frames[1]))
                        
                        """
                        draw the figure, can pause here for distinction
                        between the new frame and cumulative frame
                        """
                        #fig.canvas.draw()
                        #fig.canvas.flush_events()
                        
                        ## SUMMED GRAPH SECTION
                        if (FIRST_FRAME):
                            """
                            for the first frame
                            
                            in the second plot (the right one)
                            
                            input td as the data, cm.winter as the color, 
                            square plots, origin in the lower left corner, 
                            normalized from 0.3 to 1.0 intensity
                            """
                            ax2 = plt.subplot(132)
                            im2 = ax2.imshow(td, cmap=matplotlib.cm.hot,
                            aspect='equal',origin='lower', vmin=0.3, vmax=1.0)
                            
                            """label the axes; these will never change"""
                            ax2.xaxis.set_label_text(str('columns [')+'0'+
                            str('-->')+str(len(tot_norm)-1)+str(']'))
                            ax2.yaxis.set_label_text(str('rows [')+'0'+
                            str('-->')+str(len(tot_norm[0])-1)+str(']'))
                            
                            ax2.text(
                            1200, 400, 'LOCAL TIME: {}'.format(local_time))
                            ax2.text(
                            1200, 300, 'TEMP SENSOR 1: {}'.format(temp1))
                            ax2.text(
                            1200, 200, 'TEMP SENSOR 2: {}'.format(temp2))
                            ax2.text(
                            1200, 100, 'TEMP SENSOR 3: {}'.format(temp3))
                            
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
                            
                            ax2.texts.clear()
                            
                            ax2.text(
                            1200, 400, 'LOCAL TIME: {}'.format(local_time))
                            ax2.text(
                            1200, 300, 'TEMP SENSOR 1: {}'.format(temp1))
                            ax2.text(
                            1200, 200, 'TEMP SENSOR 2: {}'.format(temp2))
                            ax2.text(
                            1200, 100, 'TEMP SENSOR 3: {}'.format(temp3))
                            
                        """update the title with every new frame"""
                        ax2.set_title("FRAMES "+str(frames[0])+"-->"+str(
                        frames[1]))
                        
                        
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
                        FIRST_FRAME = False  
                        
                        gc.collect()
                        
                        
                else:
                    continue
                    
    ## After the stream/file ends
    except struct.error:#IndexError:
        print('Tadaa')
        break

print("Total number of packets:", pack_counter)

print('Time spent running program: {}'.format(
time.time() - start_time))

print('\nTo obtain the value of a pixel in the left graph, ')
print('the proper syntax is \'d_bin[row][column]\'.')
print('\nThe second graph\'s syntax is \'tot_norm[row][column]\'.')

read_bytes.close()
# transcribe.close()