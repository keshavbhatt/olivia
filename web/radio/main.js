var baseUrl = "http://ktechpit.com/USS/Olivia/radio/"

$(document).bind("mobileinit", function(){
        $.mobile.defaultPageTransition = 'slidefade';
        $.mobile.ajaxEnabled = true;
        $.mobile.hideUrlBar = false;
});

//persist navbar
$(function() {
    $( "[data-role='navbar']" ).navbar();
    $( "[data-role='header']" ).toolbar();
 });

var countries = [
            "Algeria",
            "Argentina",
            "Australia",
            "Austria",
            "Bangladesh",
            "Belarus",
            "Belgium",
            "Bosnia and Herzegovina",
            "Brasil",
            "Brazil",
            "Bulgaria",
            "Canada",
            "Chile",
            "China",
            "Colombia",
            "Costa Rica",
            "Croatia",
            "Cyprus",
            "Czech Republic",
            "Denmark",
            "Deutschland",
            "Dominican Republic",
            "Ecuador",
            "Egypt",
            "España",
            "Estonia",
            "Finland",
            "France",
            "Germany",
            "Greece",
            "Honduras",
            "Hungary",
            "Iceland",
            "India",
            "Indonesia",
            "Iran",
            "Iraq",
            "Ireland",
            "Israel",
            "Italy",
            "Jamaica",
            "Japan",
            "Kazakhstan",
            "Latvia",
            "Lebanon",
            "Lithuania",
            "Luxembourg",
            "Malta",
            "Mexico",
            "Moldova",
            "Morocco",
            "Netherlands",
            "Netherlands Antilles",
            "New Zealand",
            "Nigeria",
            "Norway",
            "Pakistan",
            "Peru",
            "Philippines",
            "playing",
            "Poland",
            "Portugal",
            "Puerto Rico",
            "Romania",
            "Russia",
            "Russian Federation",
            "Serbia",
            "Singapore",
            "Slovakia",
            "Slovenia",
            "South Africa",
            "South Korea",
            "Spain",
            "Sri Lanka",
            "Sweden",
            "Switzerland",
            "Syria",
            "Taiwan",
            "Thailand",
            "Trinidad and Tobago",
            "Tunisia",
            "Turkey",
            "UK",
            "Ukraine",
            "United Arab Emirates",
            "United Kingdom",
            "United States",
            "United States of America",
            "Uruguay",
            "USA",
            "Venezuela",
            "Vietnam"
        ]


// Update the contents of the toolbars
$( document ).on( "pageshow", "[data-role='page']", function() {
    // Each of the four pages in this demo has a data-title attribute
    // which value is equal to the text of the nav button
    // For example, on first page: <div data-role="page" data-title="Info">
    var current = $( this ).jqmData( "title" );
    // Change the heading
    $( "[data-role='header'] h1" ).text( current );
    // Remove active class from nav buttons
    $( "[data-role='navbar'] a.ui-btn-active" ).removeClass( "ui-btn-active" );
    // Add active class to current nav button
    $( "[data-role='navbar'] a" ).each(function() {
        if ( $( this ).text() === current ) {
            $( this ).addClass( "ui-btn-active" );
        }
    });
});


function showLoading() {
     $.mobile.loading("show", {
        text: "Loading..",
        textVisible: false,
        theme: "b",
        overlay: true
    });
}




var title,artist,album,coverUrl,songId,albumId,artistId,millis;
var base64; //returns base64 versio of album art to c++
var colorThief ; // colorThief object init
var dominantColor; //global
var html_data; //global html_data for youtube search
var stations_loaded = false;
var track_loaded = false;
var artist_loaded = false;


$(document).ready(function($) {

    $(function () {
        $("[data-role=popup]").popup().enhanceWithin();
    });

    $(function() {
           $.mobile.defaultHomeScroll = 0;
    });

    $("#coverImage").load(function(){
          dominantColor = colorThief.getColor(document.querySelector("#coverImage"));
    });
});


//  core functions -------------

function loadCountries(){
    $.ajax({
        url: baseUrl+"list/countries.php",
               type:"GET",
        success: function(html) {
            $("#countries_result").append(html).listview("refresh");
        }
    });
}

function loadLanguages(){
    $.ajax({
        url: baseUrl+"list/languages.php",
               type:"GET",
        success: function(html) {
            $("#languages_result").append(html).listview("refresh");
        }
    });
}

function showStations(query,queryType){
    showLoading();
    $.ajax({
       url: baseUrl+"list/stations.php",
              type:"GET",
               data:{
                   "query":escape(query),
                   "type":queryType
               },
       success: function(html) {
           $.mobile.loading("hide");
           $.mobile.changePage($('#stations_page'));
           $("#stations_result").html(html).listview("refresh");
           $('#stations_page .ui-content').trigger('create');
           $('#stations_page .ui-content').fadeIn('slow');
       }
   });
}

function loadTopStations(type){
    if(paginator.isOffline("radio","loadTopStations",type)){
        if(type==="most-played"){
             var html_ = paginator.load("radio","loadTopStations",type);
             $("#most-played").html(html_).listview("refresh");
             $("#most-played-tab-anchor").attr("style","background-color: rgba(26, 152, 199, 0.67) !important;border-color:transparent !important;");
             $("#most-voted-tab-anchor").attr("style","");
        }
        if(type==="most-voted"){
             var html = paginator.load("radio","loadTopStations",type);
             $("#most-voted").html(html).listview("refresh");
             $("#most-voted-tab-anchor").attr("style","background-color: rgba(26, 152, 199, 0.67) !important;border-color:transparent !important;");
             $("#most-played-tab-anchor").attr("style","");
        }
    }else{
    //topStations
        $.ajax({
           url: baseUrl+"list/top-stations.php",
                  type:"GET",
                   data:{
                    "type":type
                   },
           success: function(html) {
               if(type==="most-played"){
                    paginator.save("radio","loadTopStations",type,html);
                    $("#most-played").html(html).listview("refresh");
                    $("#most-played-tab-anchor").attr("style","background-color: rgba(26, 152, 199, 0.67) !important;border-color:transparent !important;")
                    $("#most-voted-tab-anchor").attr("style","");
               }
               if(type==="most-voted"){
                    paginator.save("radio","loadTopStations",type,html);
                    $("#most-voted").html(html).listview("refresh");
                    $("#most-voted-tab-anchor").attr("style","background-color: rgba(26, 152, 199, 0.67) !important;border-color:transparent !important;");
                    $("#most-played-tab-anchor").attr("style","");
               }
           }
       });
    }
}

function playStation(streamDetail){
    mainwindow.playRadioFromWeb(streamDetail);
}

$(document).on("pagebeforeshow","#radio_page",function(){
    $('#radio_search_input').unbind();

    $('#radio_search_input').keypress(function(event){
        var keycode = (event.keyCode ? event.keyCode : event.which);
        if(keycode == '13'){
            station_search($(this).val())
        }
    });
});

function station_search(query){
    showLoading();
    if(paginator.isOffline("radio","station_search",query)){
        $.mobile.loading("hide");
        $.mobile.changePage($('#stations_page'));

        var html = paginator.load("radio","station_search",query);

        $("#stations_result").html(html).listview("refresh");
        $('#stations_page .ui-content').trigger('create');
        $('#stations_page .ui-content').fadeIn('slow');
    }else{
        $.ajax({
           url: baseUrl+"list/search.php",
                  type:"GET",
                   data:{
                    "query":query
                   },
           success: function(html) {
               $.mobile.loading("hide");
               $.mobile.changePage($('#stations_page'));

               paginator.save("radio","station_search",query,html);

               $("#stations_result").html(html).listview("refresh");
               $('#stations_page .ui-content').trigger('create');
               $('#stations_page .ui-content').fadeIn('slow');
           }
       });
    }
}






