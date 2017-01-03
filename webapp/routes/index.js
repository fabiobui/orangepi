var express = require('express');
var router = express.Router();

/* GET home page. */
router.get('/', function(req, res, next) {
  var newURL = req.get('host').replace (/\:[0-9]{1,4}.(.*)/, '$1');	
  res.render('index', { 
  	title: 'Express',
  	ip_server: newURL });
});


router.get('/sensors', function(req, res, next) {
  res.render('sensors', { title: 'Sensors' });
});

router.get('/settings', function(req, res, next) {
  res.render('settings', { title: 'Settings' });
});

module.exports = router;
