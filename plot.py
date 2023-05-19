import matplotlib
import math
import matplotlib.pyplot as plt


path="./logdata.txt"
f=open(path)
input_data=f.read()
gruoping=3

splited_data=input_data.split()
#print(splited_data)

j=0
for i in splited_data:

    theta=((2*math.pi*(270/360))/(1080/gruoping))*j+(math.pi/2)
    x=float(i)*math.cos(theta)
    y=float(i)*math.sin(theta)
    #print(f"x,y={x,y}")
    plt.plot([0,x],[0,y])
    j+=1
plt.show()
