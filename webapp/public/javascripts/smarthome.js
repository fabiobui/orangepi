//Funzioni comuni
function Display_Load()   
{ 
    $("#loading").fadeIn(900,0);
    $("#loading").html('<img src="/images/ajax-loader-hp.gif" />');
};

//Hide Loading Image
function Hide_Load()
{
  $("#loading").fadeOut('slow');
};

function getArduino_OLD() {
  // gestisce i dettagli istanze
  Display_Load();
  var ip = $("#ip_num").val();
  $.ajax({
    url: "http://"+ip+"/arduino?callback=?",
    type: "GET",
    async: false,
    cache: false,
    timeout: 30000,
    jsonpCallback: 'jsonCallback',
    contentType: "application/json",
    dataType: 'jsonp',
    success: function(data) {
            Hide_Load();
    },
    error: function (xhr, ajaxOptions, thrownError) {
        Hide_Load();
        console.log(xhr.status);
        console.log(thrownError);
        $('#system_info').append("<p><small>Error calling server!</small></p>");
    }
  });
  return true;      
}


function getArduino() {
  // gestisce i dettagli istanze
  var ip = $("#ip_num").val();

  Display_Load();
  $.getJSON("http://"+ip+":8080/node/71")
    .fail(function() {
       Hide_Load();
       console.log( "error calling server" );
       $('#system_info').append("<p><small>Error calling server on node 71!</small></p>");
    })
   .done(function(data) {
      Hide_Load();
      if (data) {
        console.log(data);
        var temp_out = parseFloat(data.temperature);
        $('#temp_out').empty();
        $('#temp_out').append(temp_out.toFixed(1)+'°');
        var lux_out = parseInt(data.Lux);
        $('#lux_out').empty();
        $('#lux_out').append(lux_out);         
        $('#system_info').append("<p><small>Node 71: "+JSON.stringify(data)+"</small></p>");
      }
  });


  Display_Load();
  $.getJSON("http://"+ip+":8080/node/45")
    .fail(function() {
       Hide_Load();
       console.log( "error calling server" );
       $('#system_info').append("<p><small>Error calling server on node 45!</small></p>");
    })
   .done(function(data) {
      Hide_Load();
      if (data) {
        console.log(data);
        var temp_in = parseFloat(data.temperature);
        $('#temp_in').empty();
        $('#temp_in').append(temp_in.toFixed(1)+'°');
        var lux_in = parseInt(data.Lux);
        $('#lux_in').empty();
        $('#lux_in').append(lux_in);         
        $('#system_info').append("<p><small>Node 45: "+JSON.stringify(data)+"</small></p>");
      }
  });



  return true;      
}


function jsonCallback_OLD(data) {
            if (data) {
              console.log(data);
              var temp_in = parseFloat(data.T);
              $('#temp_in').empty();
              $('#temp_in').append(temp_in.toFixed(1)+'°');
              var temp_out = parseFloat(data.ExtT);
              $('#temp_out').empty();
              $('#temp_out').append(temp_out.toFixed(1)+'°');
              var lux_out = parseInt(data.ExtLux);
              $('#lux_out').empty();
              $('#lux_out').append(lux_out);  
              var hum_out = parseInt(data.ExtHumi);
              $('#hum_out').empty();
              $('#hum_out').append(hum_out);     
              var lux_in = parseInt(data.IntLux);
              $('#lux_in').empty();
              $('#lux_in').append(lux_in);  
              var mbar_in = parseFloat(data.Mbar);
              $('#mbar_in').empty();
              $('#mbar_in').append(mbar_in.toFixed(1));    
              $('#system_info').append("<p><small>"+data.msg+"</small></p>");
              $('input[name="sw-main"]').bootstrapSwitch('state', (data.MS=='ON'), true);
              $('input[name="sw-fan"]').bootstrapSwitch('state', (data.FAN=='ON'), true);
              $('input[name="sw-season"]').bootstrapSwitch('state', (data.S=='summer'), true);
              $('input[name="sw-curtain"]').bootstrapSwitch('state', (data.AutoCurt=='ON'), true);
              $('input[name="t_min"]').val(data.Tmin);
              $('input[name="t_max"]').val(data.Tmax);
              $('input[name="lux"]').val(data.Lux);
            }
}


function postArduino(sensor, state, tmin, tmax) {
  // gestisce i dettagli istanze
//  var data = [{"sensor":"test", "state":"true"}];
  Display_Load();
  var ip = $("#ip_num").val();
  $.ajax({
    url: "http://"+ip+"/postino?callback=?",
    type: "GET",
    async: false,
    cache: false,
    timeout: 30000,
    contentType: "application/json",
    dataType: 'jsonp',
    jsonpCallback: 'jsonCallback',
    data : {"sensor": sensor, "state": state, "tmin": tmin, "tmax" : tmax},
    success: function(data) {
            getArduino();
            Hide_Load();
    },
    error: function (xhr, ajaxOptions, thrownError) {
        Hide_Load();
        console.log(xhr.status);
        console.log(thrownError);
        $('#system_info').append("<p><small>Error calling server!</small></p>");
    }
  });
  return true;      
}


$(document).ready(function() {	

  $( '#loadArduino' ).click( function() {
  	getArduino();
  });

  $('input[name="sw-main"]').on('switchChange.bootstrapSwitch', function(event, state) {
    var stato = (state == false) ? 0 : 1;
    postArduino('M', stato, 0, 0);
  });

  $('input[name="sw-fan"]').on('switchChange.bootstrapSwitch', function(event, state) {
    var stato = (state == false) ? 2 : 1;
    postArduino('F', stato, 0, 0);
  });

  $('input[name="sw-curtain"]').on('switchChange.bootstrapSwitch', function(event, state) {
    var stato = (state == false) ? 0 : 1;
    postArduino('C', stato, 0, 0);
  });


  $( '#setTemp' ).click( function() {
    var tmin = $('input[name="t_min"]').val();
    var tmax = $('input[name="t_max"]').val();
    postArduino('T', false, tmin, tmax);
  });

  $( '#curtUp' ).click( function() {
    postArduino('C', '2', 0, 0);
  });

  $( '#curtStop' ).click( function() {
    postArduino('C', '3', 0, 0);
  });

  $( '#curtDown' ).click( function() {
    postArduino('C', '4', 0, 0);
  });

  $( '#setCurtLux' ).click( function() {
    var lux = $('input[name="lux"]').val();
    postArduino('C', lux, 0, 0);
  });

// Initials
  // initialize all the inputs
  $('input[name="sw-main"]').bootstrapSwitch();
  $('input[name="sw-fan"]').bootstrapSwitch();
  $('input[name="sw-season"]').bootstrapSwitch();
  $('input[name="sw-curtain"]').bootstrapSwitch();
});
