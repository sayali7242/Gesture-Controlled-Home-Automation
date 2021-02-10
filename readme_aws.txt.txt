1. Adding Arduino and RasPi as AWS IoT things:
	To create an AWS IoT thing for these two devices, we followed the steps mentioned the document below:
	https://docs.aws.amazon.com/iot/latest/developerguide/iot-moisture-create-thing.html

2. Creating AWS Lambda functions:
	For both the implementations, we created two different lambda functions by following the steps given the below document:
	https://docs.aws.amazon.com/lambda/latest/dg/getting-started-create-function.html

3. Creating AWS Rules for Arduino and RPi:
	i.	We created two different rules for Arduino and RPi implementation following the steps given in: https://docs.aws.amazon.com/iot/latest/developerguide/iot-create-rule.html
	ii.	While creating the rules, the rule query statement for RPi implementation was set to 'SELECT * FROM 'pi/outgoing-IR'' and for Arduino it was set to 'SELECT * FROM 'arduino/outgoing-gyroscope''.
	iii.	The action for both the rules was set to 'Send a message to a lambda function', where we provided the name of corresponding lambda functions.
4. Attach webhooks with AWS Lambda:
	i.	Open an account on IFTTT.com
	ii.	Go to https://ifttt.com/maker_webhooks
	iii.	Click on 'Settings'
	iv.	https://maker.ifttt.com/use/{your_key} copy the key to your webhooks service.
	v.	On the AWS side, write a lambda function code to send data to webhooks URL.
	vi.	Give the trigger path as: path: '/trigger/' + event.data + '/with/key/{your_key}'
	vii.	By replacing this key in the attached JS code for lambda functions, you should be able to connect your lambda function with your webhooks service.

5. Creating Applets on IFTTT:
	i.	On IFTTT.com go to 'My Applets'
	ii.	Click on 'Create'
	iii.	On the 'Create your own' page, click on the 'add' button inside 'if this' bloc.
	iv.	It will open 'choose a service' page. Search for 'webhooks' and click on that.
	v.	Add the event name as the data string sent by lambda function.
	vi.	Click on 'Then that' and select 'smartLife' from the services.
	vii.	On the next page, choose the actions to be taken on the selected device.
	viii.	Add IFFTTT filter for more degrees of control.
	ix.	Save and activate the Applet.



