'use strict';
console.log('Loading function');

var https = require('https');

exports.handler = function(event, context) {

    var body='';
    var jsonObject = JSON.stringify(event);

    // the post options
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

    const words = event.data.split("_")
    if(words.length>1){
        var optionspost = {
            host: 'maker.ifttt.com',
            path: '/trigger/' + words[1] + '/with/key/bKNq_k--vBdwhNOr7Y7dGh',
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
        let  i = 1;
        let data = {"value1":words[1]};
        reqPost.write(JSON.stringify(data));
        reqPost.end();
        
    }
    else{
    }
};