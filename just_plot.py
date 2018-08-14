"""

just_plot.py is designed to be called from C++ with the proper data extracted from the WRX-R telemetry stream in order to display the data in a graphical format 

"""
import sys
sys.path.append("C:/Users\\PSU_Telemetry\\Miniconda3\\Lib\\site-packages")
import matplotlib
#matplotlib.use('TkAgg')
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.cm
import struct
from struct import pack
from struct import unpack
import gc
import time
import operator
import fileinput
import datetime

## FRC Numbers
"""variables for controlling stream"""
num_row    = 1024
num_column = 1024

"""variables for plotting"""
# graphing = False
# pack_counter = 0

# data = [[0 for a in range(num_column)] for b in range(num_row)]
# maxed, maxed1 = 1, 1
# tot_norm = [[0 for a in range(num_column)] for b in range(num_row)]

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
"""

##C++
if (graphing) and (pack_counter%num_row==0)
{
    graphing = false;
    plot_this (data, frame_counter, frames, total);
}
"""
def LIST(size):
    return [0 for i in range(size)]
    

def SHOW(something):
    print(something)

def PLACE(a_list, anything, position):
    a_list[position] = anything
    return a_list

##GLOBALS NEEDED FOR SPEED

fig = plt.figure(1,figsize=(20,10))
fig.set_facecolor((0.753,0.753,0.753))
#fig.set_facecolor((0.831,0.686,0.216))

ax1 = plt.subplot(231)
ax4 = plt.subplot(234)
ax2 = plt.subplot(232)
ax5 = plt.subplot(235)
ax3 = plt.subplot(233)

# z = np.array([[0,0],[0,0]])
# 
# im1 = ax1.imshow(z,cmap=matplotlib.cm.winter,
# aspect='equal',origin='lower', vmin=50, vmax=1500)
# 
# im2 = ax2.imshow(z,cmap=matplotlib.cm.winter,
# aspect='equal',origin='lower', vmin=-19, vmax=46)
# 
# im3 = ax3.imshow(z,cmap=matplotlib.cm.hot,
# aspect='equal',origin='lower', vmin=12, vmax=43)

ax4.set_xlabel("DN")
ax4.set_ylabel("COUNTS")

ax5.set_xlabel("DN")
ax5.set_ylabel("COUNTS")

ax3.set_title("CUMULATIVE")

def sub_plot(prev, dat, total, frame_number, first_or_second, lt, ts1, ts2, ts3):
    start_time = time.time()
    
    if (first_or_second==1):
        """clear it all out with each reset frame"""
        ax1.clear()
        ax4.clear()
        ax2.clear()
        ax5.clear()
        ax3.clear()
        return 0

    else:
        """properly shape the arrays"""
        data = np.reshape(dat, (num_row, num_column))
        previous = np.reshape(prev, (num_row, num_column))
        """subtract out the previous frame"""
        new_data = data-previous
        
        if (first_or_second==2):
            """set the proper total"""
            total = new_data
            """create global variables for faster plotting"""
            global im1
            global im2
            global im3
            """define the first generation of the graph"""
            im1 = ax1.imshow(data,cmap=matplotlib.cm.hot,
            aspect='equal',origin='lower', vmin=50, vmax=1500)
            
            im2 = ax2.imshow(new_data,cmap=matplotlib.cm.hot,
            aspect='equal',origin='lower', vmin=-19, vmax=46)
            
            im3 = ax3.imshow(total,cmap=matplotlib.cm.hot,
            aspect='equal',origin='lower', vmin=12, vmax=43)
        
        else:
            """accumulate values in total"""
            total = total+new_data
            """set each data without redrawing everything"""
            im1.set_data(data)
            ax1.redraw_in_frame()
            
            im2.set_data(new_data)
            ax2.redraw_in_frame()
            
            im3.set_data(total)
            ax3.redraw_in_frame()
        
        ##RANGES ALTERED IN ORDER TO KEEP UP WITH TYLER'S FORAMATTING
        """set titles and create histograms"""
        ax1.set_title("FRAME: {}".format(frame_number))
        
        ax4.hist(np.reshape(data,-1), bins=100, histtype='stepfilled', range=(0,937))

        ax2.set_title("FRAME: {}".format(frame_number))
        
        ax5.hist(np.reshape(new_data,-1), bins=100, histtype='stepfilled', range=(-19,46))
        
        """remove the previous time and temperature sensor datas"""
        ax3.texts.clear()
        """reset the time and temperature sensor datas"""
        ax3.text(0, -350, 'LOCAL TIME: {}'.format(lt), fontsize=20, fontweight='bold')
        ax3.text(0, -600, 'TEMP SENSOR 1: {}'.format(ts1), fontsize=20, fontweight='bold')
        ax3.text(0, -850, 'TEMP SENSOR 2: {}'.format(ts2), fontsize=20, fontweight='bold')
        ax3.text(0, -1100, 'TEMP SENSOR 3: {}'.format(ts3), fontsize=20, fontweight='bold')
        """make sure everything is visible"""
        fig.canvas.draw()
        fig.canvas.flush_events()
        plt.show(block=False)
        
    """so I know how much better it needs to be"""
    print('Time spent plotting frame {}: {}'.format(frame_number, time.time()-start_time))
    
    """save every frame for later viewing"""
    plt.savefig('WRX-R/PSU_WRX-R_DATA_{}'.format(
        datetime.datetime.now().strftime ("%Y_%h_%d_%H%M%S")))
    
    """return the accumulated result"""
    return total 


def plot_this (data, frame_number, total):
    """
    FRAME COUNTER MUST BE >0
    """
    
    start_time = time.time()
    
    d_bin = list()
    point = 0
    for a in range(num_row):
        d_bin.append(data[point:point+num_column])
        point = point + num_column
    
    """if total is 0, create a list for it"""
    if (total == 0):
        total  = [[0 for a in range(len(d_bin[0]))] for b in range(len(d_bin))]
    
    ## FRAME PLOT
    
    """acquire the maximum value for normalization"""
    #maxed1 = max(list(map(max, d_bin)))
    
    ## SUMMED MATHS SECTION
    for a in range(len(d_bin)):
        """add all values from d_bin to total"""
        total[a] = np.add(total[a], d_bin[a])
    
    """acquire the maximum value for normalization"""
    #maxed = max(list(map(max, total)))
    
    """normalize every data point in total for tot_norm"""
    #tot_norm = list(map((lambda a: [b/maxed for b in a]), total))
    
    """np array used for the graph"""
    td = np.array(total)
    ## END SUMMED MATHS
    
    """normalize the values"""
    #d_bin = list(map((lambda a: [b/maxed1 for b in a]), d_bin))
    
    """np array used for the graph"""
    d = np.array(d_bin)
    
    fig = plt.figure(1,figsize=(10,5))
    ax1 = plt.subplot(121)
    im1 = ax1.imshow(d,cmap=matplotlib.cm.winter,
    aspect='equal',origin='lower', vmin=700,vmax=1000)
    
    """label the axes; these will never change"""
    ax1.xaxis.set_label_text(str('columns [')+'0'+
    str('-->')+str(len(d_bin)-1)+str(']'))
    ax1.yaxis.set_label_text(str('rows [')+'0'+
    str('-->')+str(len(d_bin[0])-1)+str(']'))
    

    """update the title with every new frame"""
    ax1.set_title("FRAME NUMBER: "+str(frame_number))
    
    ## SUMMED GRAPH SECTION

    ax2 = plt.subplot(122)
    im2 = ax2.imshow(td, cmap=matplotlib.cm.winter,
    aspect='equal',origin='lower', vmin=700, vmax=1000)
    
    """label the axes; these will never change"""
    ax2.xaxis.set_label_text(str('columns [')+'0'+
    str('-->')+str(len(tot_norm)-1)+str(']'))
    ax2.yaxis.set_label_text(str('rows [')+'0'+
    str('-->')+str(len(tot_norm[0])-1)+str(']'))

    """update the title with every new frame"""
    ax2.set_title("FRAMES "+str(0)+"-->"+str(frame_number))
    
    """show the changes by updating the plots"""
    fig.canvas.draw()
    fig.canvas.flush_events()
    
    plt.show(block=False)
    
    gc.collect()
    
    ## After the stream/file ends
    
    print('Time spent running program: {}'.format(
    time.time() - start_time))
    
    # if (frame_number==7):
    #     time.sleep(15)
    
    fig.clear()
    
    return total