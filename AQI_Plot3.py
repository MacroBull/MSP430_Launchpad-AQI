#! /usr/bin/python

import serial
import time

from macrobull.misc import serialChecker
from macrobull.dynamicPlot import dynamicFigure,average

#pa=dict(alpha=0.3, linewidth=1.2, marker='o', markersize=2.4)
procFunc=[lambda x,y:average(300,x,y), lambda x,y:average(0,x,y) ]
ser = serial.Serial(serialChecker(), 9600, timeout=0.5)


def volt2Density(mv):
	#0:0.6, 0.4:3 0.468:3.36
	if mv<3500:
		return (mv-600)*0.4/(3.0-0.6)
	else:
		return 500

def density2AQI(d,CType="C"):
	CC=(0, 35, 75, 115, 150, 250, 350, 500)
	CA=(0, 15.4, 40.4, 65.4, 150.4, 250.4, 350.4, 500.4)
	I=(0, 50, 100, 150, 200, 300, 400, 500)

	CList={"C":CC,"A":CA}
	C=CList[CType]

	if d>C[-1]: return 500
	for i in range(len(I)):
		if d<C[i]: return I[i-1]+(I[i]-I[i-1])*(d-C[i-1])/(C[i]-C[i-1])


def getData(fs):
	ad,aaa,aac=[],[],[]
	w=ser.inWaiting()
	#print(w)
	if w>0:
		data=ser.read(w)
		print data
		#for t in data.split('\n'):
		for t in data.split(' '):
			try:
				mv=int(t)
				#mv=int(t) * 4.88
			except ValueError,e:
				pass
			else:
				if 600<mv<3500:
					d=volt2Density(mv)
					aa=density2AQI(d,"A")
					ac=density2AQI(d,"C")
					prtln = "raw= {:4} mV, PM2.5 density= {:5.2f} ug/m^3, AQI(C)= {:6.2f} , AQI(A)= {:6.2f} \n".format(mv,d,ac,aa)
					
					print(prtln)
					f.write(prtln)
					f.flush()
					
					ad.append(d)
					aaa.append(aa)
					aac.append(ac)

	'''
	ad = [ sum(ad)/len(ad) ]
	aaa = [ sum(aaa)/len(aaa) ]
	aac = [ sum(aac)/len(aac) ]
	'''

	df.appendData(ad,'Density',211, procFunc=procFunc,plotArgs=dict(alpha=0.4, linewidth=1.2, marker='o', markersize=2.4), flotArgs=dict(alpha=0.6, linewidth=2.4))
	df.appendData(aaa,'AQI(A)',212)
	df.appendData(aac,'AQI(C)',212, procFunc=procFunc,plotArgs=dict(alpha=0.3, linewidth=1.2, marker='o', markersize=2.4), flotArgs=dict(alpha=0.6, linewidth=2.4))

	'''
	df.subplots[211].set_ylim(10,40)
	df.subplots[212].set_ylim(10,50)
	'''

f=open("AQI_data_" + time.ctime() , "w")

#df=dynamicFigure(updateInterval=500,screenLen=300)
df=dynamicFigure(updateInterval=8000, screenLen=10800)
#df.newYLim = df.__null__

df.newData=getData

df.run()

f.close()
ser.close()
print("closed")
