var express = require('express');
var router = express.Router();

/* GET home page. */
router.get('/', function(req, res, next) {
  res.render('index', { 
  	title: 'Express',
  	ip_server: '151.66.34.180' });
});


router.get('/sensors', function(req, res, next) {
  res.render('sensors', { title: 'Sensors' });
});

router.get('/settings', function(req, res, next) {
  res.render('settings', { title: 'Settings' });
});

module.exports = router;
