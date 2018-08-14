"""

just_plot.py is designed to be called from C++ with the proper data extracted from the WRX-R telemetry stream in order to display the data in a graphical format 

must go t file location to alter because of weird permission BS

"""
# import sys
# sys.path.append("C:\Program Files (x86)\Python36-32\Lib\site-packages")
import matplotlib
from matplotlib import gridspec
#matplotlib.use('TkAgg')
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.cm
import gc
import time
import fileinput
import datetime
import sys

## FRC Numbers
"""variables for controlling stream"""
num_row    = 1024
num_column = 1024

##SAVEPATH
"""filepath to save the data"""
SAVEPATH = 'C:/Users/PSU_Telemetry/Documents/WORKPLACE(CHRIS)/'

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

"""functions that used to serve a purpose in C++"""
def LIST(size):
    return [0 for i in range(size)]
    

def SHOW(something):
    print(something)

def PLACE(a_list, anything, position):
    a_list[position] = anything
    return a_list
    
def CREATE(anything):
    new_thing = tuple([i for i in anything])
    return new_thing
    #return anything.deepcopy()

##GLOBALS NEEDED FOR SPEED

fig = plt.figure(1,figsize=(20,10))
#fig.set_facecolor((0.753,0.753,0.753))
#fig.set_facecolor((0.831,0.686,0.216))

gs = gridspec.GridSpec(2, 3,
                       width_ratios=[1,1,1],
                       height_ratios=[3,1]
                       )

ax1 = plt.subplot(gs[0])
ax2 = plt.subplot(gs[1])
ax5 = plt.subplot(gs[4])
ax3 = plt.subplot(gs[2])
ax6 = plt.subplot(gs[5])

plt.tight_layout()

thresh    = 50
up_thresh = 40000

#plt.show(block=False)

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

#saveData = open(SAVEPATH+'from_dll.tad', 'w')
#saveData.close()
    
def sub_plot (prev, dat, frame_number, first_or_second, lt, ts0, ts1):
    
    #saveData = open(SAVEPATH+'from_dll.tad', 'a')
    #saveData.write(str(dat))
    #saveData.close()
    
    try:
        
        if (first_or_second==1):
            """save a global to make the cumulative image make more sense"""
            global first_frame
            first_frame = frame_number
            
            """clear it all out with each reset frame"""
            ax1.clear()
            ax2.clear()
            ax5.clear()
            ax3.clear()
            ax6.clear()
            
        else:
            """properly shape the arrays"""
            data     = np.reshape(dat,  (num_row, num_column))
            previous = np.reshape(prev, (num_row, num_column))
            """subtract out the previous frame"""
            new_data = data-previous
            
            """new variable for ramp subtraction"""
            """column-wise mean"""
            #bg = np.mean(np.hsplit(new_data,2)[0],1)
            channels = np.hsplit(new_data,32)
            bg = np.array([i.mean() for i in channels])
            """new shape for easier maths"""
            bg = bg[np.newaxis,:]
            """reshape into column vector"""
            #bg = np.reshape(bg, (num_column,1))
            """average ramp"""
            ramp = bg.mean()

            """the average bg must be above 0"""
            if (ramp <= 0):
                """do not add anything to the total"""
                new_data = np.zeros(new_data.shape)
                bg = np.zeros(bg.shape)
            """use all bg's above zero"""
            #bg = np.clip(bg,0,0xFFFF)
            """only use bg if ALL are above 0"""
            # if np.any(bg<0):
            #     """do not add anything to the total"""
            #     new_data = np.zeros(new_data.shape)
            #     bg = 0
            
            if (first_or_second==2):
                """create global variables for faster plotting"""
                global im1, im2, im3, total
                
                """set the proper total"""
                """subtract by channel"""
                #try2 = np.array([channels[i]-bg[0][i] for i in range(bg.size)])
                #try2shaped = np.column_stack(try2)
                new_above_thresh = np.array([channels[i]-bg[0][i] for i in range(bg.size)])
                new_above_thresh = np.column_stack(new_above_thresh)
                """cut off values below the threshold"""
                new_above_thresh[new_above_thresh<thresh]=0
                new_above_thresh[new_above_thresh>up_thresh]=0
                """same as before"""
                total = new_above_thresh
                
                """define the first generation of the graph"""
                im1 = ax1.imshow(data,cmap=matplotlib.cm.hot,
                aspect='equal',origin='lower', vmin=50, vmax=50000)
                
                im2 = ax2.imshow(new_data,cmap=matplotlib.cm.hot,
                aspect='equal',origin='lower', vmin=-50, vmax=400)
                
                #maybe always accumulate
                im3 = ax3.imshow(total,cmap=matplotlib.cm.hot,
                aspect='equal',origin='lower', vmin=0, vmax=40000)
                #im3 = ax3.imshow((total>thresh)&(total<up_thresh),cmap=matplotlib.cm.hot,
                #aspect='equal',origin='lower', vmin=0, vmax=1)
                
            
            else:
                """accumulate values in total, subtract ramp"""
                #new_above_thresh = np.reshape(channels-bg,new_data.shape)
                new_above_thresh = np.array([channels[i]-bg[0][i] for i in range(bg.size)])
                new_above_thresh = np.column_stack(new_above_thresh)
                """cut off values below the threshold"""
                new_above_thresh[new_above_thresh<thresh]=0
                new_above_thresh[new_above_thresh>up_thresh]=0
                """same as before"""
                total = total+new_above_thresh
                
                """set each data without redrawing everything"""
                im1.set_data(data)
                ax1.redraw_in_frame()
                
                im2.set_data(new_data)
                ax2.redraw_in_frame()
                
                # im3.set_data((total>thresh)&(total<up_thresh))
                im3.set_data(total)
                ax3.redraw_in_frame()
            
            ##RANGES ALTERED IN ORDER TO KEEP UP WITH TYLER'S FORAMATTING
            """set titles and create histograms"""
            ax1.set_title("DETECTOR FRAME: {}".format(frame_number))
            """remove the ticks and labels"""
            ax1.set_xticks([])
            ax1.set_yticks([])
            
            """remove the previous ramp"""
            ax1.texts.clear()
            """reset the time and temperature sensor datas"""
            ax1.text(0,-350,'Ramp Value: %.3f' % ramp,fontsize=20, fontweight='bold')
            
            ax2.set_title("SUBTRACTED FRAME: {}".format(frame_number))
            ax2.set_xticks([])
            ax2.set_yticks([])
            
            ax5.clear()
            ax5.hist(np.reshape(new_data,-1), bins=100, histtype='stepfilled', range=(0,2500))
            ax5.set_xlabel("DN")
            ax5.set_ylabel("COUNTS")
            
            ax3.set_title("CUMULATIVE RAMP-SUBTRACTED FRAMES: {} to {}".format(first_frame,frame_number))
            ax3.set_xticks([])
            ax3.set_yticks([])
            
            ax6.clear()
            ax6.hist(np.reshape(total,-1), bins=100, histtype='stepfilled', range=(0,65535))
            ax6.set_ylim([0,200])
            ax6.set_xlabel("DN")
            ax6.set_ylabel("COUNTS")
            
            # """remove the previous time and temperature sensor datas"""
            # ax3.texts.clear()
            # """reset the time and temperature sensor datas"""
            # ax3.text(0, -350, 'Ramp Value: {}'.format(bg), fontsize=20, fontweight='bold')
            # ax3.text(0, -600, 'TEMP SENSOR 0: {}'.format(ts0), fontsize=20, fontweight='bold')
            # ax3.text(0, -850, 'TEMP SENSOR 1: {}'.format(ts1), fontsize=20, fontweight='bold')
            
            """make sure everything is visible"""
            fig.canvas.draw()
            fig.canvas.flush_events()
            plt.show(block=False)
        
    except:
        fig2 = plt.figure(0,figsize=(10,5))
        plt.text(0.5,0.5,'IT BROKE')
        plt.show(block=False)
    
    """save every frame for later viewing"""
    plt.savefig(SAVEPATH+'WRX-R/PSU_WRX-R_DATA_{}'.format(datetime.datetime.now().strftime ("%Y_%h_%d_%H%M%S")))
    