  # encoding: utf-8
import os
import sys
sys.path.insert(0, '../..')
import cv2
import time
import numpy as np
# from common.HKcamera import HKIPcamera_1 as HKIPcamera
from common.HKcamera import HKIPcamera

class HKCamera(object):
    def __init__(self, ip, name, pw):
        self._ip = ip
        self._name = name
        self._pw = pw
        HKIPcamera.init(self._ip, self._name, self._pw)

    def getFrame(self):
        frame = HKIPcamera.getframe()
        return frame

ip = '192.168.0.' + '13'
name = str('admin')
pw = str('a1234567')
camera = HKCamera(ip, name, pw) 

frame = np.array(camera.getFrame()) # 720 1280 3
cv2.imwrite('1.jpg', frame)
print('ok')
# exit()

count = 0
i = 0
time_pre = time.time()
while(1):
	count = count + 1
	# time.sleep(0.04)
	# print(count)

	time_1 = time.time()
	frame = np.array(camera.getFrame()) 
	time_2 = time.time()

	# cv2.imwrite('1/' + str(count) + '.jpg', frame)
	# print('{}, {:.3f}'.format(count, time_2 - time_1))
	
	if time_2 - time_1 > 0.10:
		time_now = time.time()
		print('{:.3f}, {}, {:.3f}, {}'.format(time_2 - time_1, i, time_now - time_pre, count))
		time_pre = time_now
		i = 0
	else:
		# print('{}, {:.3f}'.format(count, time_2 - time_1))
		i = i+1
		
	# cv2.imshow('image', frame)
	# if cv2.waitKey(1) & 0xFF == ord('q'):
	# 	break

# cv2.destroyAllWindows()








