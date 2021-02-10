First Register your Raspberry Pi as a thing on Amazon AWS IoT. Follow the instructions at:
https://docs.aws.amazon.com/iot/latest/developerguide/connecting-to-existing-device.html

Install the AWS IoT SDK, following the above instructions.

Next, place the mnist_model and wandCV.py files in the SAME directory, anywhere on your Pi. We suggest placing these in the /home/ directory, as it often makes it easier to launch scripts automatically at startup.


Install Python 3.7.3 and TensorFlow, cv2, picamera and numpy modules, preferably using pip.

Edit the AWS endpoint in the wandCV.py file (line 21), and update the paths to key, cert and root in the subsequent lines based on your installation of the AWS IoT SDK.

Build an illuminator circuit, and connect the picamera to the PiCamera connector.
Follow instructions in the readme_aws.txt to set up interfaces to your smart devices.

Launch the wandCV.py file, and use a reflective tip stuck to the end of anything to trace gestures.