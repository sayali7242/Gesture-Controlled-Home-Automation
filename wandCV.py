# For camera module
from picamera import PiCamera
from picamera.array import PiRGBArray

# For image processing
import numpy as np
import cv2
import os
import pickle
import time
import subprocess
import tensorflow as tf

# For AWS IoT
from awscrt import io, mqtt, auth, http
from awsiot import mqtt_connection_builder
import time as t
import json


ENDPOINT="ENDPOINT HERE"
CLIENT_ID="Raspi"
PATH_TO_KEY = "../certs/e8b71904cf-private.pem.key"
PATH_TO_CERT = "../certs/e8b71904cf-certificate.pem.crt"
PATH_TO_ROOT = "../certs/G2-RootCA1.pem"
TOPIC = "pi/outgoing-IR"

event_loop_group = io.EventLoopGroup(1)
host_resolver = io.DefaultHostResolver(event_loop_group)
client_bootstrap = io.ClientBootstrap(event_loop_group, host_resolver)
mqtt_connection = mqtt_connection_builder.mtls_from_path(
			endpoint=ENDPOINT,
			cert_filepath=PATH_TO_CERT,
			pri_key_filepath=PATH_TO_KEY,
			client_bootstrap=client_bootstrap,
			ca_filepath=PATH_TO_ROOT,
			client_id=CLIENT_ID,
			clean_session=False,
			keep_alive_secs=6
			)
print("Connecting to {} with client ID '{}'...".format(
		ENDPOINT, CLIENT_ID))

connect_future = mqtt_connection.connect()

connect_future.result()
print("Connected!")


os.system('rm pred_*.png')

# initializing Picamera
camera = PiCamera()
camera.framerate = 15  # 15fps refresh rate is optimal for our hardware, with (832,624) resolution.
camera.resolution = (832, 624)
rawCapture = PiRGBArray(camera, size = (832, 624))

clf = tf.keras.models.load_model('mnist_model', compile=False)   # Load saved MNIST model
# Define parameters for the required blob
params = cv2.SimpleBlobDetector_Params()

# setting the thresholds
params.minThreshold = 50
params.maxThreshold = 255

# filter by color
params.filterByColor = 0
params.blobColor = 255

# filter by circularity
params.filterByCircularity = 1
params.minCircularity = 0.5

# filter by area
params.filterByArea = 0
params.minArea = 30
# params.maxArea = 1500

# creating object for SimpleBlobDetector
detector = cv2.SimpleBlobDetector_create(params)


flag = 0
points = []
lower_blue = np.array([255, 255, 0])
upper_blue = np.array([255, 255, 0])

imnum=0
# Function for Pre-processing
def last_frame(img):
	global imnum
	global points
	retval, img = cv2.threshold(img, 80, 255, cv2.THRESH_BINARY)
	imnum+=1
	minx=99999
	miny=99999
	maxx=0
	maxy=0

	# Segmenting image to traced path

	height, width = img.shape
	for i in range(height):
		for j in range(width):
			if img[i,j]>0:
				if i<minx:
					minx=i
				if j<miny:
					miny=j
				if i>maxx:
					maxx=i
				if j>maxy:
					maxy=j
	
	minx-=10 if minx>10 else 0
	miny-=10 if miny>10 else 0
	maxx+=10 if maxx<height-10 else height
	maxy+=10 if maxy<width-10 else width
	img=img[minx:maxx, miny:maxy]

	cv2.imwrite('pred_'+str(imnum)+'.png', img)  # Dump image to disk for manual verification, debug.

	#Preprocess for classification. MNIST digits are 28x28.


	img = cv2.resize(img, (28, 28), interpolation=cv2.INTER_CUBIC)
	np_image_data = np.asarray(img)
	np_image_data=np_image_data/255.0
	np_image_data=np_image_data.flatten()
	np_final = np.expand_dims(np_image_data,axis=0)
	
	preds=clf.predict(np_final)[0]		# Call neural network for image prediction (inference only)

	print(np.argmax(preds))
	
	# Below, publish detected command to AWS IoT Topic

	data = "IR_"+str(np.argmax(preds))
	message = {"data" : data}
	mqtt_connection.publish(topic=TOPIC, payload=json.dumps(message), qos=mqtt.QoS.AT_LEAST_ONCE)
	time.sleep(1.0)

time.sleep(0.1)

framecount=0
idle_fr=0

for image in camera.capture_continuous(rawCapture, format='bgr', use_video_port=True):
	frame = image.array
	frame = cv2.flip(cv2.rotate(frame, cv2.ROTATE_180),1)  # Rotate 180 to account for how our camera was mounted.
	#cv2.imshow("Original", frame)
	frame = cv2.resize(frame, (frame.shape[1]//4, frame.shape[0]//4))
	frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)    

	#detecting keypoints
	keypoints = detector.detect(frame)
	frame_with_keypoints = cv2.drawKeypoints(frame, keypoints, np.array([]), (0, 0, 255), cv2.DRAW_MATCHES_FLAGS_DRAW_RICH_KEYPOINTS)

	# USE THE FOLLOWING COMMENTED BLOCK ONLY FOR DEBUGGING
	#starting and ending circle
	#frame_with_keypoints = cv2.circle(frame_with_keypoints, (140, 70), 10, (0, 255, 0), 2)
	#frame_with_keypoints = cv2.circle(frame_with_keypoints, (170, 130), 10, (0, 0, 255), 2)
	# frame_with_keypoints = cv2.rectangle(frame_with_keypoints, (0,0), (320,75), (0,0,255), 1)
	# frame_with_keypoints = cv2.rectangle(frame_with_keypoints, (0,165), (320,240), (0,0,255), 1)
	# frame_with_keypoints = cv2.rectangle(frame_with_keypoints, (0,75), (320,165), (0,255,0), 1)

	#points_array = cv2.KeyPoint_convert(keypoints)
	points_array = cv2.KeyPoint_convert(keypoints)
	#print(points_array)
	if flag == 1 and len(points_array)!=0:
		# Get coordinates of the wand tip and append them to points list.
		idle_fr=0
		points.append(points_array[0])
		framecount+=1        
		#print(framecount)

	if flag == 1 and len(points_array)==0:
		idle_fr+=1
		print(idle_fr)

		# Interpolate between points in list.
	for i in range(1, len(points)):
		cv2.line(frame_with_keypoints, tuple(points[i-1]), tuple(points[i]), (255, 255, 0), 8)

	if flag == 1:
		# Next line DEBUG only - end region.
		#if int(points_array[0][1]) in range(0,75) or int(points_array[0][1]) in range(165,240):
		if framecount>=50 or idle_fr>=7:
			print("Tracing Done!!")
			frame_with_keypoints = cv2.inRange(frame_with_keypoints, lower_blue, upper_blue)
			try:
				last_frame(frame_with_keypoints)
			except:
				pass
			points=[]
			flag=0
			empty=0
			framecount=0
			idle_fr=0

	if flag == 0:
		# Next line DEBUG only - start region.
		#if int(points_array[0][1]) in range(76,164):
		if len(points_array)>0:
			time.sleep(0.5)
			print("Start Tracing!!")
			flag = 1

				
	cv2.imshow("video",frame_with_keypoints)
	#cv2.imshow("video 2",frame)
	rawCapture.truncate(0)
	key = cv2.waitKey(1) & 0xFF
	if key == ord('q'):
		break


cv2.destroyAllWindows()
disconnect_future = mqtt_connection.disconnect()
disconnect_future.result()

