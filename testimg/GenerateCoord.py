import cv2
#bgr select 0,1,2
forground_bgr=1
background_bgr=2
f=open("coord.txt","w")
img=cv2.imread('.\\1280x720\\U.bmp')
outimg=img.copy()
img=cv2.cvtColor(img,cv2.COLOR_RGB2GRAY)

height,width=img.shape
height,width,dim=outimg.shape
print(height,width)
for i in range(height):
    step=0
    for j in range(width):
        k=img[i,j]
        if(k>128):
            outimg[i,j,0]=0;
            outimg[i, j, 1] = 0;
            outimg[i, j, 2] = 0;
            outimg[i,j,background_bgr]=255
        if(k<128):
            step=step+1
            if(step==1):
                line = " x:" + str(j) + "y:" + str(i) + "\n"
                f.writelines(line)
            outimg[i, j, forground_bgr] = 255
        if(k>128 and step>1):
            line = " x:" + str(j) + "y:" + str(i) + "\n"
            f.writelines(line)
            step=-1



f.close()
cv2.imshow("disp",outimg)
cv2.waitKey(0)
