'use strict';
console.log('Loading function');

var https = require('https');

exports.handler = function(event, context) {

    var body='';
    var jsonObject = JSON.stringify(event);

    // the post options
    var optionspost = {
        host: 'maker.ifttt.com',
        path: '/trigger/' + event.data + '/with/key/mIsQrat9cX_CRx5BaBRo4',
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        }
    };
    
        var reqPost = https.request(optionspost, function(res) {
        console.log("statusCode: ", res.statusCode);
        res.on('data', function (chunk) {
            body += chunk;
        });
    });

    reqPost.write(jsonObject);
    reqPost.end();
    
    var optionspost = {
        host: 'maker.ifttt.com',
        path: '/trigger/' + event.data + '/with/key/bKNq_k--vBdwhNOr7Y7dGh',
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        }
    };
    
    var reqPost = https.request(optionspost, function(res) {
        console.log("statusCode: ", res.statusCode);
        res.on('data', function (chunk) {
            body += chunk;
        });
    });

    reqPost.write(jsonObject);
    reqPost.end();
};