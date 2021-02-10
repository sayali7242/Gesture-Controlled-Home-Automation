import cv2
import pickle
import numpy as np
import tensorflow as tf
mnist = tf.keras.datasets.mnist

clf = tf.keras.models.load_model('mnist_model', compile=False)
img2 = cv2.imread("pred_0.jpg")
img2= cv2.resize(img2,dsize=(28,28), interpolation = cv2.INTER_CUBIC)
img2=cv2.cvtColor(img2, cv2.COLOR_BGR2GRAY)

np_image_data = np.asarray(img2)
np_image_data=np_image_data/255.0

print(np_image_data)

np_image_data=np_image_data.flatten()
np_final = np.expand_dims(np_image_data,axis=0)

preds=clf.predict(np_final)[0]
print(preds)
print(np.argmax(preds))
